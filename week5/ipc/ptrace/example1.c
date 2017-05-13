#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/reg.h>
#include <unistd.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


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
		unsigned long rax;

		ptrace(PTRACE_SYSCALL, pid, 0, 0);
		waitpid(pid, &status, 0);

		if (!WIFSTOPPED(status))
			continue;

		rax = ptrace(PTRACE_PEEKUSER, pid,
					8 * ORIG_RAX, NULL);
		printf("syscall %lu\n", rax);
		ptrace(PTRACE_SYSCALL, pid, 0, 0);
		waitpid(pid, &status, 0);
	}

	return 0;
}
