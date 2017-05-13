#include <exec.h>

#include <ints.h>
#include <memory.h>
#include <mm.h>
#include <paging.h>
#include <ramfs.h>
#include <stdint.h>
#include <string.h>
#include <threads.h>


#define USER_STACK_SIZE	(1024 * 1024)

#define ELF_NIDENT	16
#define ELF_CLASS	4
#define ELF_DATA	5
#define ELF_CLASS64	2
#define ELF_DATA2LSB	1
#define ELF_EXEC	2

#define PT_NULL	0
#define PT_LOAD	1
#define PF_X	(1 << 0)
#define PF_W	(1 << 1)
#define PF_R	(1 << 2)


struct elf_hdr {
	uint8_t e_ident[ELF_NIDENT];
	uint16_t e_type;
	uint16_t e_machine;
	uint32_t e_version;
	uint64_t e_entry;
	uint64_t e_phoff;
	uint64_t e_shoff;
	uint32_t e_flags;
	uint16_t e_ehsize;
	uint16_t e_phentsize;
	uint16_t e_phnum;
	uint16_t e_shentsize;
	uint16_t e_shnum;
	uint16_t e_shstrndx;
} __attribute__((packed));

struct elf_phdr {
	uint32_t p_type;
	uint32_t p_flags;
	uint64_t p_offset;
	uint64_t p_vaddr;
	uint64_t p_paddr;
	uint64_t p_filesz;
	uint64_t p_memsz;
	uint64_t p_align;
} __attribute__((packed));


struct exec_ctx {
	uintptr_t entry_point;
	uintptr_t stack_pointer;
	int argc;
	uintptr_t argv;
};


static int check_elf_hdr(const struct elf_hdr *hdr)
{
	const char magic[] = "\x7f""ELF";

	if (memcmp(hdr->e_ident, magic, sizeof(magic) - 1))
		return -1;

	if (hdr->e_ident[ELF_CLASS] != ELF_CLASS64)
		return -1;

	if (hdr->e_ident[ELF_DATA] != ELF_DATA2LSB)
		return -1;

	if (hdr->e_type != ELF_EXEC)
		return -1;

	return 0;
}

static int setup_stack(struct exec_ctx *ctx, struct mm *mm, size_t stack_size)
{
	const uintptr_t stack_end = USERSPACE_END;
	const uintptr_t stack_begin = USERSPACE_END - stack_size;

	if (mmap(mm, stack_begin, stack_end, VMA_PERM_READ | VMA_PERM_WRITE))
		return -1;

	ctx->stack_pointer = stack_end;
	return 0;
}

static int copy_args(struct exec_ctx *ctx, struct mm *mm,
			int argc, const char **argv)
{
	const size_t ptrsz = sizeof(void *);
	struct mm *me = thread_current()->mm;
	const void *nullptr = NULL;
	uintptr_t data, ptrs;
	size_t bytes = 0;


	for (int i = 0; i != argc; ++i)
		bytes += strlen(argv[i]) + 1;

	bytes = (bytes + ptrsz - 1) & ~(ptrsz - 1);
	data = ctx->stack_pointer - bytes;
	ptrs = data - ptrsz * (argc + 1);

	ctx->stack_pointer = ptrs;
	ctx->argc = argc;
	ctx->argv = ptrs;

	for (int i = 0; i != argc; ++i) {
		const size_t size = strlen(argv[i]) + 1;
		const void *begin = (void *)data;

		if (mcopy(mm, (uintptr_t)begin, me, (uintptr_t)argv[i], size))
			return -1;
		data += size;

		if (mcopy(mm, ptrs, me, (uintptr_t)&begin, ptrsz))
			return -1;
		ptrs += ptrsz;
	}

	if (mcopy(mm, ptrs, me, (uintptr_t)&nullptr, ptrsz))
		return -1;

	return 0;
}

static unsigned segment_flags(unsigned long flags)
{
	unsigned perm = VMA_PERM_READ;

	if (flags & PF_W)
		perm |= VMA_PERM_WRITE;
	if (flags & PF_X)
		perm |= VMA_PERM_EXECUTE;
	return perm;
}

static int segment_map(struct mm *mm, struct elf_phdr *hdr, struct file *file)
{
	const unsigned perm = segment_flags(hdr->p_flags);
	const uintptr_t phys = buddy_alloc(0);

	const uintptr_t mask = ~((uintptr_t)PAGE_SIZE - 1);
	const uintptr_t from = hdr->p_vaddr & mask;
	const uintptr_t to = (hdr->p_vaddr + hdr->p_memsz + PAGE_SIZE - 1)
				& mask;

	struct mm *me = thread_current()->mm;
	void *buf = va(phys);
	uintptr_t addr = hdr->p_vaddr;
	size_t size = hdr->p_filesz;
	long offs = hdr->p_offset;

	if (!phys) {
		buddy_free(phys, 0);
		return -1;
	}

	if (mmap(mm, from, to, perm)) {
		buddy_free(phys, 0);
		return -1;
	}

	while (size) {
		const long toread = size < PAGE_SIZE ? size : PAGE_SIZE;
		const long read = ramfs_readat(file, buf, toread, offs);

		if (read != toread) {
			buddy_free(phys, 0);
			return -1;
		}

		if (mcopy(mm, addr, me, (uintptr_t)buf, toread)) {
			buddy_free(phys, 0);
			return -1;
		}

		size -= toread;
		addr += toread;
	}

	if (mset(mm, addr, 0, hdr->p_memsz - hdr->p_filesz)) {
		buddy_free(phys, 0);
		return -1;
	}

	buddy_free(phys, 0);
	return 0;
}

static int load_binary(struct exec_ctx *ctx, struct mm *mm,
			const struct elf_hdr *hdr, struct file *file)
{
	const long offset = (long)hdr->e_phoff;

	for (int i = 0; i != (int)hdr->e_phnum; ++i) {
		struct elf_phdr phdr;

		const long off = offset + (long)i * hdr->e_phentsize;
		const long size = ramfs_readat(file, &phdr, sizeof(phdr), off);

		if (size != sizeof(phdr))
			return -1;

		if (phdr.p_type != PT_LOAD)
			continue;

		if (segment_map(mm, &phdr, file))
			return -1;
	}
	ctx->entry_point = hdr->e_entry;
	return 0;
}

int exec(int argc, const char **argv)
{
	struct exec_ctx ctx;
	struct elf_hdr hdr;
	struct file *file;
	struct mm *new_mm;
	struct mm *old_mm;
	struct thread *me;

	if (argc <= 0)
		return -1;

	if (ramfs_open(argv[0], &file))
		return -1;

	if (ramfs_readat(file, &hdr, sizeof(hdr), 0) != sizeof(hdr)) {
		ramfs_close(file);
		return -1;
	}

	if (check_elf_hdr(&hdr)) {
		ramfs_close(file);
		return -1;
	}

	/**
	 * exec replaces current logical address space with a new one,
	 * so we create a new struct mm, and replace an old one with
	 * this new
	 **/
	new_mm = mm_create();
	if (!new_mm) {
		ramfs_close(file);
		return -1;
	}
	
	if (setup_stack(&ctx, new_mm, USER_STACK_SIZE)) {
		mm_release(new_mm);
		ramfs_close(file);
		return -1;
	}

	if (copy_args(&ctx, new_mm, argc, argv)) {
		mm_release(new_mm);
		ramfs_close(file);
		return -1;
	}

	if (load_binary(&ctx, new_mm, &hdr, file)) {
		mm_release(new_mm);
		ramfs_close(file);
		return -1;
	}

	me = thread_current();
	old_mm = me->mm;
	me->mm = new_mm;
	cr3_write(new_mm->cr3);
	mm_release(old_mm);
	ramfs_close(file);

	me->regs->rip = ctx.entry_point;
	me->regs->rsp = ctx.stack_pointer;
	me->regs->rdi = ctx.argc;
	me->regs->rsi = ctx.argv;
	me->regs->cs = USER_CS;
	me->regs->ss = USER_DS;
	me->regs->rflags = RFLAGS_IF;
	return 0;
}
