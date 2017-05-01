#ifndef __LOCK_H__
#define __LOCK_H__


/**
 * In a single processor system we don't need to store anything
 * inside lock. But we still going to call it spinlock.
 **/
struct spinlock { int dummy; };


void spin_setup(struct spinlock *lock);


/**
 * Use this to protect data that is cannot be accessed from
 * an interrupt handler.
 **/
void spin_lock(struct spinlock *lock);
void spin_unlock(struct spinlock *lock);

/**
 * Use this to protect datat that can be accessed from
 * an interrupt handler.
 **/
int spin_lock_int_save(struct spinlock *lock);
void spin_unlock_int_restore(struct spinlock *lock, int enable);

#endif /*__LOCK_H__*/
