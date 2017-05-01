#include <stdint.h>

#include <threads.h>
#include <time.h>
#include <apic.h>
#include <ints.h>


static const uint32_t TIMER_PERIODIC = (1 << 17);
static const uint32_t TIMER_ONESHOT = (0 << 17);
static const uint32_t TIMER_MASK = (1 << 16);


static const uint32_t TIMER_DIV1 = 11;
static const uint32_t TIMER_DIV2 = 0;
static const uint32_t TIMER_DIV4 = 1;
static const uint32_t TIMER_DIV8 = 2;
static const uint32_t TIMER_DIV16 = 3;
static const uint32_t TIMER_DIV32 = 8;
static const uint32_t TIMER_DIV64 = 9;
static const uint32_t TIMER_DIV128 = 10;


static const uint32_t TIMER_INIT = 262144;


static void timer_handler(void)
{
	scheduler_tick();
}

static void local_apic_timer_setup(void)
{
	const int intno = allocate_interrupt();

	register_interrupt_handler(intno, &timer_handler);

	local_apic_write(LOCAL_APIC_TIMER_DIVIDER, TIMER_DIV128);
	local_apic_write(LOCAL_APIC_TIMER_LVT, TIMER_PERIODIC | intno);
	local_apic_write(LOCAL_APIC_TIMER_INIT, TIMER_INIT);
}

void time_setup(void)
{
	local_apic_timer_setup();
}
