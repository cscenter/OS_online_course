#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/reg.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>


static unsigned long align_down(unsigned long x, unsigned long align)
{
	return x - (x % align);
}

static unsigned long align_up(unsigned long x, unsigned long align)
{
	return align_down(x + align - 1, align);
}

static void copy_data(pid_t pid, unsigned long addr,
			const char *data, size_t size)
{
	unsigned long word;
	const size_t word_size = sizeof(word);

	const unsigned long begin = align_down(addr, word_size);
	const unsigned long end = align_up(addr + size, word_size);

	/* Special case: copy a small amount of data, that fits one word */
	if (begin + word_size == end) {
		const size_t off = addr - begin;

		word = ptrace(PTRACE_PEEKDATA, pid, (void *)begin, NULL);
		memcpy((char *)&word + off, data, size);
		ptrace(PTRACE_POKEDATA, pid, (void *)begin, word);
		return;
	}

	/* Copy properly word aligned portion of the data at first */
	const unsigned long from = align_up(addr, word_size);
	const unsigned long to = align_down(addr + size, word_size);

	unsigned long off = from;
	size_t pos = from - addr;

	for (; off < to; off += word_size, pos += word_size) {
		memcpy(&word, &data[pos], word_size);
		ptrace(PTRACE_POKEDATA, pid, (void *)off, word);		
	}

	/* Then copy unaligned head and tail of the data */
	if (begin != addr) {
		const size_t head_off = addr - begin;
		const size_t head_size = word_size - head_off;

		word = ptrace(PTRACE_PEEKDATA, pid, (void *)begin, NULL);
		memcpy((char *)&word + head_off, data, head_size);
		ptrace(PTRACE_POKEDATA, pid, (void *)begin, word);
	}


	if (to != addr + size) {
		const size_t tail_size = (addr + size) - to;

		word = ptrace(PTRACE_PEEKDATA, pid, (void *)to, NULL);
		memcpy((char *)&word, data + (size - tail_size), tail_size);
		ptrace(PTRACE_POKEDATA, pid, (void *)to, word);
	}
}


int main(int argc, char **argv)
{
	if (argc < 2) {
		printf("command name expected\n");
		return -1;
	}

	const pid_t pid = fork();

	if (pid < 0) {
		printf("fork failed\n");
		return -1;
	}

	if (!pid) {
		char **args = calloc(argc, sizeof(char *));
		int i;

		for (int i = 0; i != argc - 1; ++i)
			args[i] = argv[i + 1];

		ptrace(PTRACE_TRACEME, 0, NULL, NULL);
		if (execvp(args[0], args) == -1) {
			printf("exec failed\n");
			return -1;
		}
		/* unreachable */
		return 0;
	}

	int status;

	waitpid(pid, &status, 0);

	while (!WIFEXITED(status)) {
		const char msg[] = "Hello, World!";
		const size_t size = sizeof(msg) - 1;

		unsigned long rax, rdi, rsi, rdx;

		ptrace(PTRACE_SYSCALL, pid, 0, 0);
		waitpid(pid, &status, 0);

		if (!WIFSTOPPED(status))
			continue;

		rax = ptrace(PTRACE_PEEKUSER, pid,
					8 * ORIG_RAX, NULL);
		rdi = ptrace(PTRACE_PEEKUSER, pid, 8 * RDI, NULL);
		rsi = ptrace(PTRACE_PEEKUSER, pid, 8 * RSI, NULL);
		rdx = ptrace(PTRACE_PEEKUSER, pid, 8 * RDX, NULL);

		if (rax == SYS_write && rdi == 1) {
			const size_t bytes = size < rdx
						? size : rdx;

			copy_data(pid, rsi, msg, bytes);
			ptrace(PTRACE_POKEUSER, pid,
					8 * RDX, (void *)bytes);
		}

		ptrace(PTRACE_SYSCALL, pid, 0, 0);
		waitpid(pid, &status, 0);
	}

	return 0;
}
