#include <stdint.h>

#include <buddy.h>
#include <ints.h>
#include <mm.h>
#include <paging.h>
#include <slab.h>
#include <string.h>
#include <threads.h>


#define IOMAP_BITS	(1 << 16)
#define IOMAP_WORDS	(IOMAP_BITS / sizeof(unsigned long))

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

struct tss {
	uint32_t rsrv0;
	uint64_t rsp[3];
	uint64_t rsrv1;
	uint64_t ist[7];
	uint64_t rsrv2;
	uint16_t rsrv3;
	uint16_t iomap_base;
	unsigned long iomap[IOMAP_WORDS + 1];
} __attribute__((packed));


static const int TIMESLICE = 5;

static struct tss tss __attribute__((aligned (PAGE_SIZE)));

static struct slab_cache cache;
static struct spinlock ready_lock;
static struct list_head ready;
static struct thread *current;
static struct thread *idle;
static int remained_time;
static int preempt_count;


static void thread_place(struct thread *me)
{
	spin_lock(&current->lock);
	if (current->state == THREAD_FINISHED) {
		current->state = THREAD_DEAD;
		notify_one(&current->cv);
	}
	spin_unlock(&current->lock);

	current = me;
	remained_time = TIMESLICE;
	tss.rsp[0] = (uint64_t)va(me->stack_phys +
				(PAGE_SIZE << me->stack_order));
	cr3_write(me->mm->cr3);
}

int thread_entry(struct thread *me, int (*fptr)(void *), void *arg)
{
	thread_place(me);
	local_int_enable();

	return fptr(arg);
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

	thread->mm = mm_create();
	if (!thread->mm) {
		buddy_free(thread->stack_phys, stack_order);
		thread_free(thread);
		return 0;
	}

	const size_t stack_size = PAGE_SIZE << stack_order;

	char *ptr = va(thread->stack_phys);
	struct frame *regs =
		(struct frame *)(ptr + stack_size - sizeof(*regs));
	struct switch_frame *frame =
		(struct switch_frame *)((char *)regs - sizeof(*frame));

	extern void __thread_entry(void);
	extern void __thread_exit(void);


	memset(regs, 0, sizeof(*regs));
	memset(frame, 0, sizeof(*frame));

	frame->rip = (uint64_t)&__thread_entry;
	frame->r15 = (uint64_t)thread;
	frame->r14 = (uint64_t)fptr;
	frame->r13 = (uint64_t)arg;
	frame->rflags = 2;

	regs->cs = KERNEL_CS;
	regs->rsp = (uint64_t)ptr + stack_size;
	regs->rip = (uint64_t)&__thread_exit;

	spin_setup(&thread->lock);
	condition_setup(&thread->cv);
	thread->context = frame;
	thread->regs = regs;
	return thread;
}

struct thread *thread_create(int (*fptr)(void *), void *arg)
{
	const int DEFAULT_STACK_ORDER = 3; /* 16Kb stack */
	return __thread_create(DEFAULT_STACK_ORDER, fptr, arg);
}

void thread_start(struct thread *thread)
{
	const int enabled = spin_lock_int_save(&ready_lock);

	thread->state = THREAD_ACTIVE;
	list_add_tail(&thread->ll, &ready);
	spin_unlock_int_restore(&ready_lock, enabled);
}

void thread_wake(struct thread *thread)
{
	thread_start(thread);
}

struct thread *thread_current(void)
{
	return current;
}


void thread_block(void)
{
	struct thread *me = current;
	const int enabled = spin_lock_int_save(&me->lock);

	me->state = THREAD_BLOCKED;
	spin_unlock_int_restore(&me->lock, enabled);
}

void thread_exit(int ret)
{
	struct thread *me = current;
	const int enabled = spin_lock_int_save(&me->lock);

	me->retval = ret;
	me->state = THREAD_FINISHED;
	spin_unlock_int_restore(&me->lock, enabled);
	schedule();
}

void thread_join(struct thread *thread, int *ret)
{
	int enabled = spin_lock_int_save(&thread->lock);

	while (thread->state != THREAD_DEAD)
		condition_wait_spin_int(&thread->cv, &thread->lock);

	spin_unlock_int_restore(&thread->lock, enabled);

	if (ret)
		*ret = thread->retval;
}

void thread_destroy(struct thread *thread)
{
	buddy_free(thread->stack_phys, thread->stack_order);
	mm_release(thread->mm);
	thread_free(thread);
}


void preempt_disable(void)
{
	const int enabled = local_int_save();

	++preempt_count;
	local_int_restore(enabled);
}

void preempt_enable(void)
{
	const int enabled = local_int_save();

	--preempt_count;
	local_int_restore(enabled);
}


void schedule(void)
{
	struct thread *me = current;
	struct thread *next = 0;

	const int enabled = local_int_save();

	/* check if preemptition enabled */
	if (preempt_count) {
		local_int_restore(enabled);
		return;
	}

	/* check the next in the list */
	spin_lock(&ready_lock);
	if (!list_empty(&ready)) {
		next = (struct thread *)ready.next;
		list_del(&next->ll);
	}
	spin_unlock(&ready_lock);

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
	spin_lock(&ready_lock);
	if (me->state == THREAD_ACTIVE && me != idle)
		list_add_tail(&me->ll, &ready);
	spin_unlock(&ready_lock);

	switch_threads(me, next);
	thread_place(me);
	local_int_restore(enabled);
}

void scheduler_tick(void)
{
	if (remained_time)
		--remained_time;

	if (remained_time <= 0)
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


/* TSS descriptor in the GDT has special format */
struct tss_desc {
	uint64_t low;
	uint64_t high;
} __attribute__((packed));


static void tss_desc_setup(struct tss_desc *desc, const struct tss *tss)
{
	const uint64_t limit = sizeof(*tss) - 1;
	const uint64_t base = (uint64_t)tss;

	desc->high = base >> 32;
	desc->low = (limit & 0xffffull) | ((base & 0xffffffull) << 16) |
		/* 9 is TSS descriptor type value + present bit */
		(9ull << 40) | (1ull << 47) |
		((limit & 0xf0000ull) << 32) | ((base & 0xff000000ull) << 32);
}

static void tr_write(uint16_t sel)
{
	__asm__ ("ltr %0" : : "a"(sel));
}

static void tss_setup(void)
{
	const int tss_index = TSS_SEL >> 3;
	uint64_t *gdt = gdt_base();
	struct tss_desc desc;

	tss.iomap_base = offsetof(struct tss, iomap);
	memset(tss.iomap, 0xff, sizeof(tss.iomap));
	tss_desc_setup(&desc, &tss);
	memcpy(&gdt[tss_index], &desc, sizeof(desc));
	tr_write(TSS_SEL);
}

void scheduler_setup(void)
{
	extern char bootstrap_stack_top[];
	static struct thread main;
	static struct mm mm;

	main.state = THREAD_ACTIVE;
	main.stack_phys = pa(bootstrap_stack_top) - PAGE_SIZE;
	main.stack_order = 0;

	list_init(&mm.vmas);
	mm.cr3 = initial_cr3;
	mm.pt = addr_page(initial_cr3);

	main.mm = &mm;
	current = &main;
	idle = &main;

	spin_setup(&ready_lock);
	list_init(&ready);
	slab_cache_setup(&cache, sizeof(struct thread));
	tss_setup();
}
