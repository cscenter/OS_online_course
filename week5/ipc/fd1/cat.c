#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <stddef.h>
#include <stdio.h>


static int write_all(int fd, const char *buf, size_t size)
{
	size_t pos = 0;

	while (pos != size) {
		ssize_t written = write(fd, buf + pos, size - pos);

		if (written < 0)
			return -1;
		pos += written;
	}
	return 0;
}

int main(int argc, char **argv)
{
	char buf[4096];

	for (int i = 1; i < argc; ++i) {
		int fd = open(argv[i], O_RDONLY);

		if (fd < 0) {
			fprintf(stderr, "failed to open %s\n", argv[i]);
			return -1;
		}

		ssize_t rd;

		while ((rd = read(fd, buf, sizeof(buf)))) {
			if (rd < 0) {
				fprintf(stderr, "failed to read %s\n", argv[i]);
				close(fd);
				return -1;
			}

			if (write_all(1, buf, rd)) {
				fprintf(stderr, "failed to write to stdout\n");
				close(fd);
				return -1;
			}
		}
		close(fd);
	}
	return 0;
}
