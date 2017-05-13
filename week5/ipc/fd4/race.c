#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdio.h>

static const char *FILE_NAME = "data";


static void read_file(int fd)
{
	char buf[2];
	ssize_t rd;

	while ((rd = read(fd, buf, sizeof(buf) - 1))) {
		if (rd < 0) {
			fprintf(stderr, "Failed to read\n");
			return;
		}

		buf[rd] = '\0';
		printf("%d: %s\n", getpid(), buf);
	}
}

int main(void)
{
	int fd = open(FILE_NAME, O_RDONLY);

	if (fd < 0) {
		fprintf(stderr, "Failed to open %s\n", FILE_NAME);
		return -1;
	}

	if (fork() < 0) {
		fprintf(stderr, "Failed to fork\n");
		close(fd);
		return -1;
	}

	read_file(fd);
	
	close(fd);
	return 0;
}
