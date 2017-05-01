#ifndef __APIC_H__
#define __APIC_H__


#include <stdint.h>


#define LOCAL_APIC_TPR			0x80
#define LOCAL_APIC_EOI			0xb0
#define LOCAL_APIC_DFR			0xd0
#define LOCAL_APIC_LDR			0xe0
#define LOCAL_APIC_SPURIOUS		0xf0
#define LOCAL_APIC_TIMER_LVT		0x320
#define LOCAL_APIC_TIMER_INIT		0x380
#define LOCAL_APIC_TIMER_COUNT		0x390
#define LOCAL_APIC_TIMER_DIVIDER	0x3e0


void local_apic_write(int reg, uint32_t value);
uint32_t local_apic_read(int reg);

void local_apic_setup(void);

#endif /*__APIC_H__*/
