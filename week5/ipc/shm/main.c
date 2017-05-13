#include <sys/types.h>
#include <sys/shm.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	const key_t id = 3148;
	const int size = 4096;
	const int fd = shmget(id, size, 0666 | IPC_CREAT);

	if (fd < 0) {
		printf("shmget failed\n");
		return -1;
	}

	void *ptr = shmat(fd, (void *)0, 0);

	if (ptr == (void *)-1) {
		printf("shmat failed\n");
		return -1;
	}

	char c;
	while ((c = getopt(argc, argv, "p:g")) != -1) {
		switch (c) {
		case 'p':
			strcpy(ptr, optarg);
			break;
		case 'g':
			puts(ptr);
			break;
		}
	}
	shmdt(ptr);

	return 0;
}
