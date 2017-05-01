#include <lock.h>
#include <ints.h>
#include <threads.h>



void spin_setup(struct spinlock *lock)
{
	(void) lock;
}

/**
 * With this implementation while we hold spinlock we can't be
 * preempted from the CPU, but the CPU is still able to handle
 * interrupts.
 **/
void spin_lock(struct spinlock *lock)
{
	(void) lock;
	preempt_disable();
}

void spin_unlock(struct spinlock *lock)
{
	(void) lock;
	preempt_enable();
}


/**
 * With this implementation interrupts are masked while we're
 * holding spinlock, thus deadlock between thread and an
 * interrupt handler impossible but the CPU can't accept
 * interrupts.
 **/
int spin_lock_int_save(struct spinlock *lock)
{
	const int enabled = local_int_save();

	spin_lock(lock);
	return enabled;
}

void spin_unlock_int_restore(struct spinlock *lock, int enable)
{
	spin_unlock(lock);
	local_int_restore(enable);
}
