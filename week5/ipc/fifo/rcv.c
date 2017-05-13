#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdio.h>

#define FIFO "myfifo"

int main()
{
	mknod(FIFO, S_IFIFO | 0666, 0);

	printf("Wait for a writer...\n");
	int fd = open(FIFO, O_RDONLY);
	printf("A writer connected, receiving...\n");
	int size;
	char buf[4096];

	while ((size = read(fd, buf, sizeof(buf) - 1)) > 0) {
		buf[size] = '\0';
		printf("Received: %s\n", buf);
	}

	return 0;
}
