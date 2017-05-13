#include <multiboot.h>
#include <memory.h>

#define MB_HEADER_MAGIC	MULTIBOOT_HEADER_MAGIC
#define MB_HEADER_FLAGS	(MULTIBOOT_AOUT_KLUDGE | MULTIBOOT_MEMORY_INFO)
#define MB_HEADER_CKSUM	-(MB_HEADER_MAGIC + MB_HEADER_FLAGS)

#define PTE_PRESENT	(1 << 0)
#define PTE_WRITE	(1 << 1)
#define PTE_LARGE	(1 << 7)
#define CR4_PAE		(1 << 5)
#define CR0_PG		(1 << 31)
#define CR0_NE		(1 << 5)
#define EFER_MSR	0xC0000080
#define EFER_LM		(1 << 8)

	.section .bootstrap, "ax"
	.code32
	.global start32
	.extern main


	.align 16
/**
 * Multiboot Specification version 0.6.96 describes machine state at the moment
 * when this code gets control in the section 3.2 "Machine state". Short
 * summary:
 *   - CPU is in 32 bit protected mode with paging and interrupts disabled;
 *   - register EBX contains physical address of multiboot info structure
 *     which we need to get memory map, so we have to preserve that address;
 *   - if we are going to use stack we need to initialize it ourself.
 *
 * So we need to do the following things:
 *   - check that CPU support 64 bit mode (long mode), we skip this check for
 *     the sake of simplicity
 *   - setup and enable paging
 *   - switch to 64 bit long mode
 *   - call into C code since we don't want to write everything in Assembly.
 **/
start32:
	jmp 1f


	.align 16
/**
 * Multiboot header is required for multiboot compliant bootloader, structure
 * of the header described in the section 3.1.1 "The layout of Multiboot
 * header" of multiboot specification.
 **/
multiboot_header:
	.long MB_HEADER_MAGIC
	.long MB_HEADER_FLAGS
	.long MB_HEADER_CKSUM
	.long multiboot_header
	.long text_phys_begin
	.long data_phys_end
	.long bss_phys_end
	.long start32


	.align 16
/**
 * GDT (Global Descriptor Table). Segment registers in 32 bit protected mode
 * and in 64 bit long mode hold indexes that point to the entries in GDT.
 **/
gdt:
	/* first entry must be zero-filled according to Intel documentation */
	.quad 0x0000000000000000
	.quad 0x00209b0000000000 // 64 bit ring0 code segment

/**
 * First GDT pointer assumes identity mapping (logical address is equal to
 * physical address) and only used during initialization. After we switched to
 * the long mode we reload IDTR with the second pointer.
 **/
gdt_ptr:
	.word (gdt_ptr - gdt - 1)
	.long gdt

/**
 * We want OS kernel virtual address to be in the last 2 GB of long mode logical
 * address space (see kernel code model in the AMD64 ABI section 3.5.1
 * "Architectural Constraints"). So we setup mapping so that every logical
 * address in the higher 2 GB of logical address space maps to the lower 2 GB
 * of physical address space (and we assume that our image and so GDT is placed
 * in the lower 2 GB of physical memory).
 *
 * So for every physical address in the lower 2 GB we can get valid logical
 * address just by adding a fixed constant (VIRTUAL_BASE is defined in memory.h
 * header inside inc directory).
 **/
gdt_ptr64:
	.word (gdt_ptr - gdt - 1)
	.quad (gdt + VIRTUAL_BASE)

idt_ptr:
	.word 0
	.quad 0

	.align 16
/**
 * According to multiboot specification if we need stack, we have to setup it on
 * our own. This directive allocates a small amout of memory for stack right
 * inside our kernel image.
 **/
	.space 0x100
stack_top:

1:
	/* initialize stack pointer (register ESP) */
	movl $stack_top, %esp

	/* save multiboot info structure physical address on stack */
	pushl %ebx


	/**
	 * The following code enable 64 bit long mode. The whole precedure to
	 * enable long mode can be found in the Intel documentation and
	 * basically consists of a few steps:
	 *    - setup identity mapping
	 *    - set LM (Long Mode) bit in the special register (EFER)
	 *    - enable paging (PG bit in the CR0 register)
	 *    - initialize segment register
	 *    - jump into 64 bit code (to initialize CS segment register).
	 **/

	call setup_mapping

	movl $EFER_MSR, %ecx
	rdmsr
	orl $EFER_LM, %eax
	wrmsr
	
	movl %cr0, %eax
	orl $(CR0_PG | CR0_NE), %eax
	movl %eax, %cr0

	/* save physical address of the multiboot info structure in the EBX */
	popl %ebx

	/**
	 * We are going to load CS segment register, so we need to setup GDT.
	 * We also zeroing IDT along the way, but it's not mandatory.
	 **/
	lgdt gdt_ptr
	lidt idt_ptr

	/**
	 * To setup CS register we use long jump instruction. Selector 0x8
	 * points to the 64 bit ring0 code segment in the GDT. We use special
	 * "trampoline" label because CPU is in compatibility mode (32 bit) so
	 * we can't use 64 bit addresses yet thus destination address has to be
	 * close.
	 **/
	ljmpl $0x08, $trampoline


/**
 * This function setup page table in the following way:
 *     - identity mapping for lower 4GB of physical memory, i. e. every logical
 *       address below 4GB mapped corresponds to the same physical address (
 *       according to the Intel documentation identity mapping required to
 *       enable paging)
 *     - higher 2 GB of logical address space mapped to the lower 2 GB of the
 *       physical address space (already mentioned above, it's due to ABI
 *       requirement); note that for the lower 2 GB of physical address we have
 *       alread two different mappings
 *     - middle mapping (it's not important so far, but will be usefull later).
 **/
setup_mapping:
	/**
	 * Pair ebx:eax holds 64 bit page table entry value. Note that we use
	 * 1 GB pages, since it's simpler. But some older QEMU versions don't
	 * support 1 GB pages, so if you have problems - update QEMU.
	 **/
	movl $(PTE_PRESENT | PTE_WRITE | PTE_LARGE), %eax
	movl $0, %ebx

	/* edi points to the page table */
	movl $(bootstrap_pml3 - VIRTUAL_BASE), %edi

	movl %eax, (%edi)
	movl %ebx, 4(%edi)

	movl %eax, 4080(%edi)
	movl %ebx, 4084(%edi)

	add $0x40000000, %eax
	adc $0, %ebx

	movl %eax, 8(%edi)
	movl %ebx, 12(%edi)

	movl %eax, 4088(%edi)
	movl %ebx, 4092(%edi)

	add $0x40000000, %eax
	adc $0, %ebx

	movl %eax, 16(%edi)
	movl %ebx, 20(%edi)

	add $0x40000000, %eax
	adc $0, %ebx

	movl %eax, 24(%edi)
	movl %ebx, 28(%edi)

	movl $(bootstrap_pml4 - VIRTUAL_BASE), %edi
	movl $(bootstrap_pml3 - VIRTUAL_BASE + PTE_PRESENT + PTE_WRITE), %eax
	movl %eax, (%edi)
	movl %eax, 2048(%edi)
	movl %eax, 4088(%edi)

	/**
	 * According to the Intel documentation we have to set PAE bit in the
	 * CR4 register before enabling 64-bit paging.
	 **/
	movl %cr4, %eax
	orl $CR4_PAE, %eax
	movl %eax, %cr4

	/* The CR3 register holds physical address of the root table. */
	movl $(bootstrap_pml4 - VIRTUAL_BASE), %eax
	movl %eax, %cr3
	ret


	.code64
trampoline:
	/**
	 * Now we are in the real 64 bit Long Mode and can use 64-bit pointers,
	 * so load 64-bit address of start64 (which is according to ABI
	 * compiled to work in the high 2GB of logical address space) and use
	 * absolute jump instruction.
	 **/
	movq $start64, %rax
	jmpq *%rax


	.text
start64:
	/**
	 * We are now in 64 bit long mode, and first of all we want to drop
	 * identity mapping (it's not required so far, but highly desirable).
	 * So we reload GDTR with new logical address and setup new stack.
	 **/
	lgdt gdt_ptr64
	movabsq $bootstrap_stack_top, %rax
	movq %rax, %rsp

	/**
	 * In long mode segment registers lost almost all of their meaning
	 * so it's OK to use zero selectors for all segment registers except
	 * CS in privileged code.
	 **/
	movw $0x0, %ax
	movw %ax, %ss
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs

	/* Disable identity mapping, so if someone by mistake uses identity
	 * mapping CPU will generate an exception. We also need to invaildate
	 * TLB, since it's not transparent, we do that reloading cr3. */
	movq $0, bootstrap_pml4
	movq %cr3, %rax
	movq %rax, %cr3

	/**
	 * EBX is 32-bit long, while we in 64 bit long mode we use use 64 bit
	 * address, make sure that higher 32 bits don't contain garbage and
	 * move EBX to RDI (register that used as a first argument of
	 * function).
	 **/
	xorq %rdi, %rdi
	movl %ebx, %edi

	/**
	 * Call into C code following all calling convetions. The first and only
	 * argument of the main function is physical address of the multiboot
	 * info structure. And from now on we will mostly code in C language
	 * which is much better than assembly.
	 **/
	cld
	movabsq $main, %rax
	call *%rax

	cli
1:
	hlt
	jmp 1b


	.bss

/* It's our long mode stack. */
	.align PAGE_SIZE
	.space PAGE_SIZE
bootstrap_stack_top:

/**
 * It's space allocated for our initial page table. All tables must be aligned
 * on 4 KB boundary.
 **/
	.align PAGE_SIZE
bootstrap_pml4:
	.space PAGE_SIZE
bootstrap_pml3:
	.space PAGE_SIZE
