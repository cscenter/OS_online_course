#ifndef __INTS_H__
#define __INTS_H__

#include <stdint.h>

#define INTNO_DIVBYZERO	0
#define INTNO_SPURIOUS	255


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
typedef void (*interrupt_handler_t)(void);


/**
 * These two function will allow us temporarily disable interrupts.
 **/
static inline void local_int_disable(void)
{ __asm__ volatile ("cli" : : : "cc"); }

static inline void local_int_enable(void)
{ __asm__ volatile ("sti" : : : "cc"); }


static inline int local_int_state(void)
{
	const uint64_t RFLAGS_IF = 1ul << 9;
	uint64_t rflags;

	__asm__ ("pushf; popq %0" : "=r"(rflags) : : "memory");
	return rflags & RFLAGS_IF;
}

static inline int local_int_save(void)
{
	const int enabled = local_int_state();

	local_int_disable();
	return enabled;
}

static inline void local_int_restore(int enabled)
{
	if (enabled)
		local_int_enable();
}


/**
 * Exception handlers are usually setup only once at the begining of the kernel
 * initialization process. So this interface exists only to decouple the code
 * handling interrupts machinery from actual exception handlers.
 **/
void register_exception_handler(int exception, exception_handler_t handler);

/**
 * With hardware interrupts we should be able not only to register but also
 * unregister handlers (for example, if device is pluggable, like external HDD,
 * though we are not going to supports things like that).
 *
 * In real world operating system kernel things can be even more complicated,
 * just a few examples to give you an idea:
 *  - a few devices can share the same IRQ (signal the same interrupt), so we
 *    need to be able to register a few handlers for the same interrupt
 *  - mapping between devices and interrupts differs between architectures
 *    or even different configurations of the same architechture (x86: PIC vs
 *    APIC), so it's not easy to come up with an abstraction able to handle all
 *    different cases gracefully
 *  - there may be quite a few devices so you might need to decide which CPU
 *    handles what devices, moreover cost of handling an interrupt from the
 *    same device may be different on differnet CPUs.
 *
 * But all these complications are beyond scope of this example, so in our case
 * it will be simple interface:
 *  - allocate_interrupt - finds a free entry in the IDT, so we can use it for
 *    our purporses
 *  - register_interrupt_handler - registers a handler for previously allocated
 *    interrupt
 *
 * And who and how maps devices on interrupts is not concern of the interface
 * (though in real world it, probably, should be, since allocation of interrupt
 * and mapping might be very closely connected/dependent).
 **/
int allocate_interrupt(void);
void register_interrupt_handler(int intno, interrupt_handler_t handler);
 
void ints_setup(void);

#endif /*__INTS_H__*/
