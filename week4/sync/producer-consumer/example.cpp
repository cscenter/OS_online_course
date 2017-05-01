#include <condition_variable>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <thread>

#include <unistd.h>


/* Wrap C++ synchronization primitives in the interface showed in videos. */
struct condition {
	std::condition_variable cv;
};

struct lock {
	std::mutex mtx;
};


void wait(struct condition *cv, struct lock *lock)
{
	std::unique_lock<std::mutex> guard(lock->mtx, std::adopt_lock_t());

	cv->cv.wait(guard);
	guard.release();
}

void notify_one(struct condition *cv)
{
	cv->cv.notify_one();
}

void notify_all(struct condition *cv)
{
	cv->cv.notify_all();
}


void lock(struct lock *lock)
{
	lock->mtx.lock();
}

void unlock(struct lock *lock)
{
	lock->mtx.unlock();
}


struct lock;
void lock(struct lock *lock);
void unlock(struct lock *lock);

struct condition;
void wait(struct condition *cv, struct lock *lock);
void notify_one(struct condition *cv);
void notify_all(struct condition *cv);

struct condition cv;
struct lock mtx;
int value;
int valid_value;
int done;

void produce(int x)
{
	lock(&mtx);
	while (valid_value)
		wait(&cv, &mtx);
	value = x;
	valid_value = 1;
	notify_one(&cv);
	unlock(&mtx);
}

void finish(void)
{
	lock(&mtx);
	done = 1;
	notify_all(&cv);
	unlock(&mtx);
}

int consume(int *x)
{
	int ret = 0;
	lock(&mtx);

	while (!valid_value && !done)
		wait(&cv, &mtx);

	if (valid_value) {
		*x = value;
		valid_value = 0;
		notify_one(&cv);
		ret = 1;
	}
	unlock(&mtx);
	return ret;
}

void producer(void)
{
	for (int i = 0; i != 5; ++i) {
		const int x = rand();

		printf("produce %d\n", x);
		produce(x);
		sleep(3);
	}
	finish();
}

void consumer(void)
{
	int x;

	while (consume(&x)) {
		printf("consumed %d\n", x);
	}
}

int main(void)
{
	std::thread prod(&producer);
	std::thread cons(consumer);

	cons.join();
	prod.join();

	return 0;
}
