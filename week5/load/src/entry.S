	.text
	.code64
	.global __switch_threads
	.global __thread_entry
	.global __common_handler
	.global __thread_exit
	.extern thread_entry
	.extern isr_handler


__thread_exit:
	movq %rax, %rdi
	callq thread_exit


__thread_entry:
	movq %r15, %rdi
	movq %r14, %rsi
	movq %r13, %rdx

	cld
	call thread_entry
	jmp jump_back


__switch_threads:
	pushq %rbx
	pushq %rbp
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
	pushfq

	movq %rsp, (%rdi)
	movq %rsi, %rsp

	popfq
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %rbp
	popq %rbx

	retq


__common_handler:
	subq $120, %rsp
	movq %rbp, 0(%rsp)
	movq %rbx, 8(%rsp)
	movq %r15, 16(%rsp)
	movq %r14, 24(%rsp)
	movq %r13, 32(%rsp)
	movq %r12, 40(%rsp)
	movq %r11, 48(%rsp)
	movq %r10, 56(%rsp)
	movq %r9, 64(%rsp)
	movq %r8, 72(%rsp)
	movq %rax, 80(%rsp)
	movq %rcx, 88(%rsp)
	movq %rdx, 96(%rsp)
	movq %rsi, 104(%rsp)
	movq %rdi, 112(%rsp)
	cld
	movq %rsp, %rdi
	call isr_handler

jump_back:
	movq 0(%rsp), %rbp
	movq 8(%rsp), %rbx
	movq 16(%rsp), %r15
	movq 24(%rsp), %r14
	movq 32(%rsp), %r13
	movq 40(%rsp), %r12
	movq 48(%rsp), %r11
	movq 56(%rsp), %r10
	movq 64(%rsp), %r9
	movq 72(%rsp), %r8
	movq 80(%rsp), %rax
	movq 88(%rsp), %rcx
	movq 96(%rsp), %rdx
	movq 104(%rsp), %rsi
	movq 112(%rsp), %rdi
	addq $136, %rsp
	iretq
