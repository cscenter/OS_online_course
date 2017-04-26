#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


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


struct thread {
	void *context;
};


void switch_threads(struct thread *from, struct thread *to)
{
	void __switch_threads(void **prev, void *next);

	__switch_threads(&from->context, to->context);
}

struct thread *__create_thread(size_t stack_size, void (*entry)(void))
{
	const size_t size = stack_size + sizeof(struct thread);
	struct switch_frame frame;
	struct thread *thread = malloc(size);

	if (!thread)
		return thread;

	memset(&frame, 0, sizeof(frame));
	frame.rip = (uint64_t)entry;

	thread->context = (char *)thread + size - sizeof(frame);
	memcpy(thread->context, &frame, sizeof(frame));
	return thread;
}

struct thread *create_thread(void (*entry)(void))
{
	const size_t default_stack_size = 4096;

	return __create_thread(default_stack_size, entry);
}

void destroy_thread(struct thread *thread)
{
	free(thread);
}


static struct thread _thread0;
static struct thread *thread[3];


static void thread_entry1(void)
{
	printf("In thread1, switching to thread2...\n");
	switch_threads(thread[1], thread[2]);
	printf("Back in thread1, switching to thread2...\n");
	switch_threads(thread[1], thread[2]);
	printf("Back in thread1, switching to thread0...\n");
	switch_threads(thread[1], thread[0]);
}

static void thread_entry2(void)
{
	printf("In thread2, switching to thread1...\n");
	switch_threads(thread[2], thread[1]);
	printf("Back in thread2, switching to thread1...\n");
	switch_threads(thread[2], thread[1]);
}

int main(void)
{
	thread[0] = &_thread0;
	thread[1] = create_thread(&thread_entry1);
	thread[2] = create_thread(&thread_entry2);

	printf("In thread0, switching to thread1...\n");
	switch_threads(thread[0], thread[1]);
	printf("Retunred to thread 0\n");

	destroy_thread(thread[2]);
	destroy_thread(thread[1]);

	return 0;
}
