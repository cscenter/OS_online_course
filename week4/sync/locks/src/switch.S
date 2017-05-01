	.text
	.code64
	.global __switch_threads
	.global __thread_entry
	.extern thread_entry

__thread_entry:
	movq %r15, %rdi
	movq %r14, %rsi
	movq %r13, %rdx

	cld
	call thread_entry


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
