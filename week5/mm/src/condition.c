#include <condition.h>
#include <threads.h>
#include <mutex.h>


struct generic_lock {
	void (*lock)(struct generic_lock *);
	void (*unlock)(struct generic_lock *);
};


struct generic_mutex {
	struct generic_lock lock;
	struct mutex *mutex;
};


static void generic_mutex_lock(struct generic_lock *lock)
{
	struct generic_mutex *mutex = (struct generic_mutex *)lock;

	mutex_lock(mutex->mutex);
}

static void generic_mutex_unlock(struct generic_lock *lock)
{
	struct generic_mutex *mutex = (struct generic_mutex *)lock;

	mutex_unlock(mutex->mutex);
}


struct generic_spinlock {
	struct generic_lock lock;
	struct spinlock *spin;
};


static void generic_spin_lock(struct generic_lock *lock)
{
	struct generic_spinlock *spin = (struct generic_spinlock *)lock;

	spin_lock(spin->spin);
}

static void generic_spin_unlock(struct generic_lock *lock)
{
	struct generic_spinlock *spin = (struct generic_spinlock *)lock;

	spin_unlock(spin->spin);
}


static void generic_spin_lock_int(struct generic_lock *lock)
{
	struct generic_spinlock *spin = (struct generic_spinlock *)lock;

	spin_lock_int_save(spin->spin);
}

static void generic_spin_unlock_int(struct generic_lock *lock)
{
	struct generic_spinlock *spin = (struct generic_spinlock *)lock;

	spin_unlock_int_restore(spin->spin, 1);
}



struct condition_wait {
	struct list_head ll;
	struct thread *thread;
};


void condition_setup(struct condition *cv)
{
	spin_setup(&cv->lock);
	list_init(&cv->wait);
}

static void __wait(struct condition *cv, struct generic_lock *lock)
{
	struct thread *me = thread_current();
	struct condition_wait wait;

	spin_lock(&cv->lock);
	wait.thread = me;
	list_add_tail(&wait.ll, &cv->wait);
	thread_block();
	lock->unlock(lock);
	spin_unlock(&cv->lock);

	schedule();

	lock->lock(lock);
}

void condition_wait_spin(struct condition *cv, struct spinlock *lock)
{
	struct generic_spinlock l = {
		{ &generic_spin_lock, &generic_spin_unlock },
		lock
	};

	__wait(cv, &l.lock);
}

void condition_wait_spin_int(struct condition *cv, struct spinlock *lock)
{
	struct generic_spinlock l = {
		{ &generic_spin_lock_int, &generic_spin_unlock_int },
		lock
	};

	__wait(cv, &l.lock);
}

void condition_wait(struct condition *cv, struct mutex *lock)
{
	struct generic_mutex l = {
		{ &generic_mutex_lock, &generic_mutex_unlock },
		lock
	};

	__wait(cv, &l.lock);
}


void notify_one(struct condition *cv)
{
	const int enabled = spin_lock_int_save(&cv->lock);

	if (!list_empty(&cv->wait)) {
		struct condition_wait *wait =
					(struct condition_wait *)cv->wait.next;

		list_del(&wait->ll);
		thread_wake(wait->thread);
	}
	spin_unlock_int_restore(&cv->lock, enabled);
}

void notify_all(struct condition *cv)
{
	const int enabled = spin_lock_int_save(&cv->lock);
	struct list_head list;
	struct list_head *head = &list;

	list_init(&list);
	list_splice(&cv->wait, &list);
	spin_unlock_int_restore(&cv->lock, enabled);

	for (struct list_head *ptr = head->next; ptr != head; ) {
		struct condition_wait *wait = (struct condition_wait *)ptr;

		ptr = ptr->next;
		thread_wake(wait->thread);
	}
}
