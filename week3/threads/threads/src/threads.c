#include <stdint.h>

#include <threads.h>
#include <buddy.h>
#include <slab.h>
#include <ints.h>


struct switch_frame {
	uint64_t rflags;
	uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r12;
	uint64_t rbp;
	uint64_t rbx;
	uint64_t rip;
} __attribute__((packed));



static const int TIMESLICE = 5;

static struct slab_cache cache;
static struct list_head ready;
static struct thread *current;
static struct thread *idle;
static int remained_time;


static void thread_place(struct thread *me)
{
	if (current->state == THREAD_FINISHED)
		current->state = THREAD_DEAD;
	current = me;
	remained_time = TIMESLICE;
}

void thread_entry(struct thread *me, int (*fptr)(void *), void *arg)
{
	thread_place(me);
	local_int_enable();
	
	thread_exit(fptr(arg));
}

static void switch_threads(struct thread *from, struct thread *new)
{
	extern void __switch_threads(void **, void *);

	__switch_threads(&from->context, new->context);
}

static struct thread *thread_alloc(void)
{
	return slab_cache_alloc(&cache);
}

static void thread_free(struct thread *thread)
{
	slab_cache_free(&cache, thread);
}


struct thread *__thread_create(int stack_order, int (*fptr)(void *), void *arg)
{
	struct thread *thread = thread_alloc();

	if (!thread)
		return thread;

	thread->stack_order = stack_order;
	thread->stack_phys = buddy_alloc(stack_order);

	if (!thread->stack_phys) {
		thread_free(thread);
		return 0;
	}

	const size_t stack_size = PAGE_SIZE << stack_order;

	char *ptr = va(thread->stack_phys);
	struct switch_frame *frame = (struct switch_frame *)(ptr +
				(stack_size - sizeof(*frame)));

	extern void __thread_entry(void);

	frame->rip = (uint64_t)&__thread_entry;
	frame->r15 = (uint64_t)thread;
	frame->r14 = (uint64_t)fptr;
	frame->r13 = (uint64_t)arg;
	frame->rflags = 2;

	thread->context = frame;
	return thread;
}

struct thread *thread_create(int (*fptr)(void *), void *arg)
{
	const int DEFAULT_STACK_ORDER = 3; /* 16Kb stack */
	return __thread_create(DEFAULT_STACK_ORDER, fptr, arg);
}

void thread_start(struct thread *thread)
{
	const int enabled = local_int_save();

	thread->state = THREAD_ACTIVE;
	list_add_tail(&thread->ll, &ready);
	local_int_restore(enabled);
}

struct thread *thread_current(void)
{
	return current;
}


void thread_exit(int ret)
{
	struct thread *me = current;

	me->retval = ret;
	me->state = THREAD_FINISHED;
	schedule();
}

void thread_join(struct thread *thread, int *ret)
{
	while (thread->state != THREAD_DEAD)
		schedule();

	if (ret)
		*ret = thread->retval;
}

void thread_destroy(struct thread *thread)
{
	buddy_free(thread->stack_phys, thread->stack_order);
	thread_free(thread);
}


void schedule(void)
{
	struct thread *me = current;
	struct thread *next = 0;

	const int enabled = local_int_save();

	/* check the next in the list */
	if (!list_empty(&ready)) {
		next = (struct thread *)ready.next;
		list_del(&next->ll);
	}

	/**
	 * if there is no next and the current thread is about to block
	 * use a special idle thread
	 **/
	if (me->state != THREAD_ACTIVE && !next)
		next = idle;

	if (!next) {
		thread_place(me);
		local_int_restore(enabled);
		return;
	}

	/**
	 * if the current thread is still active and not the special idle
	 * thread, then add it to the end of the ready queue
	 **/
	if (me->state == THREAD_ACTIVE && me != idle)
		list_add_tail(&me->ll, &ready);

	switch_threads(me, next);
	thread_place(me);
	local_int_restore(enabled);
}

void scheduler_tick(void)
{
	if (--remained_time > 0)
		return;
	schedule();
}

void scheduler_idle(void)
{
	local_int_enable();
	while (1) {
		schedule();
		__asm__ ("hlt");
	}
}


void scheduler_setup(void)
{
	static struct thread main;

	main.state = THREAD_ACTIVE;
	current = &main;
	idle = &main;

	list_init(&ready);
	slab_cache_setup(&cache, sizeof(struct thread));
}
