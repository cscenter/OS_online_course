#ifndef __MUTEX_H__
#define __MUTEX_H__

#include <lock.h>
#include <list.h>

/**
 * Mutex guaranties mutual exclusion as spinlock does, but if a
 * thread tries to acquire a mutex while it's being held by
 * another thread the thread trying to acquire the mutex will
 * be blocked.
 **/
struct thread;

struct mutex {
	struct spinlock lock;
	struct list_head wait;
	struct thread *owner;
};


void mutex_setup(struct mutex *mutex);

void mutex_lock(struct mutex *mutex);
void mutex_unlock(struct mutex *mutex);

#endif /*__MUTEX_H__*/
