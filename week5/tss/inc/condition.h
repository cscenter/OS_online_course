#ifndef __CONDITION_H__
#define __CONDITION_H__

#include <lock.h>
#include <list.h>


struct condition {
	struct spinlock lock;
	struct list_head wait;
};

struct mutex;


void condition_setup(struct condition *cv);

/* Two versions of wait: one works with spinlock, other works with mutex. */
void condition_wait_spin(struct condition *cv, struct spinlock *lock);
void condition_wait_spin_int(struct condition *cv, struct spinlock *lock);
void condition_wait(struct condition *cv, struct mutex *lock);

void notify_one(struct condition *cv);
void notify_all(struct condition *cv);

#endif /*__CONDITION_H__*/
