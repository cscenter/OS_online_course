#ifndef __INTS_H__
#define __INTS_H__

#include <stdint.h>

#define INTNO_DIVBYZERO	0


struct frame {
	uint64_t rbp;
	uint64_t rbx;
	uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r12;
	uint64_t r11;
	uint64_t r10;
	uint64_t r9;
	uint64_t r8;
	uint64_t rax;
	uint64_t rcx;
	uint64_t rdx;
	uint64_t rsi;
	uint64_t rdi;
	uint64_t intno;
	uint64_t err;
	uint64_t rip;
	uint64_t cs;
	uint64_t rflags;
	uint64_t rsp;
	uint64_t ss;
} __attribute__((packed));

typedef void (*exception_handler_t)(void);


/**
 * Exception handlers are usually setup only once at the begining of the kernel
 * initialization process. So this interface exists only to decouple the code
 * handling interrupts machinery from actual exception handlers.
 **/
void register_exception_handler(int exception, exception_handler_t handler);

void ints_setup(void);

#endif /*__INTS_H__*/
