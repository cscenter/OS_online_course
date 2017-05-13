#include <stdint.h>

#include <memory.h>
#include <ioport.h>
#include <apic.h>
#include <ints.h>
#include <print.h>


static uintptr_t LOCAL_APIC_BASE;

/**
 * Just like with video buffer, local APIC registers mapped in memory.
 * This function checks APIC BASE MSR (Model Specific Register), to find
 * physical address where local APIC registers mapped.
 **/
static uintptr_t local_apic_base(void)
{
	const uint64_t APIC_BASE_MSR = 0x1b;

	uint32_t low, high;

	__asm__ ("rdmsr" : "=a"(low), "=d"(high) : "c"(APIC_BASE_MSR));
	return (((uintptr_t)high << 32) | low) & ~((uintptr_t)(1 << 12) - 1);
}

/**
 * Even though we are going to use only local APIC for simplicity, we need to
 * disable legacy PIC to avoid problems. So the function setups PIC and than
 * masks all interrupts from PIC effectively disabling it. But since i
 * provide the code without any explanation it's might look a bit magical.
 **/
static void i8259_disable(void)
{
	const int MCMD = 0x20;
	const int MDATA = 0x21;

	const int SCMD = 0xa0;
	const int SDATA = 0xa1;


	out8(MCMD, (1 << 0) | (1 << 4));
	out8(SCMD, (1 << 0) | (1 << 4));

	out8(MDATA, 0x20);
	out8(SDATA, 0x28);

	out8(MDATA, (1 << 2));
	out8(SDATA, 2);

	out8(MDATA, 1);
	out8(SDATA, 1);

	out8(MDATA, 0xff);
	out8(SDATA, 0xff);
}

uint32_t local_apic_read(int offs)
{
	uint32_t value;

	__asm__ volatile ("movl (%1), %0"
		: "=r"(value)
		: "r"(LOCAL_APIC_BASE + offs));
	return value;
}

void local_apic_write(int offs, uint32_t value)
{
	__asm__ volatile ("movl %0, (%1)"
		:
		: "r"(value), "r"(LOCAL_APIC_BASE + offs));
}

void local_apic_setup(void)
{
	/**
	 * Since we don't support SMP, we have only one local APIC, so only one
	 * possible value. Generally every local APIC must have unique logical
	 * id.
	 **/
	const uint32_t LDR = (1 << 24);
	/**
	 * DFR stores 'interrupt model'. There are two possible interrupts
	 * models and since again we have only one local APIC, both of them
	 * works for us but i'm going to use 'flat model' because it's simpler.
	 **/
	const uint32_t DFR = (0xf << 28);
	/**
	 * TPR (Task-Priority Register) allows to block some low-priority
	 * interrupts, but since we are not going to use this functionality we
	 * just set it to zero.
	 **/
	const uint32_t TPR = 0;
	/**
	 * To actually enable local APIC we need to set spuriuos interrupt
	 * vector and "APIC Software Enable" bit in spurious interrupt register.
	 **/
	const uint32_t SPURIOUS = INTNO_SPURIOUS | (1 << 8);

	i8259_disable();

	const uintptr_t local_apic_phys = local_apic_base();

	LOCAL_APIC_BASE = (uintptr_t)va(local_apic_phys);
	printf("Local APIC base address 0x%llx\n",
				(unsigned long long)local_apic_phys);

	local_apic_write(LOCAL_APIC_TPR, TPR);
	local_apic_write(LOCAL_APIC_LDR, LDR);
	local_apic_write(LOCAL_APIC_DFR, DFR);
	local_apic_write(LOCAL_APIC_SPURIOUS, SPURIOUS);
}
