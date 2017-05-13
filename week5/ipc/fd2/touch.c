#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdio.h>

int main(int argc, char **argv)
{
	int perm = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

	for (int i = 1; i < argc; ++i) {
		int fd = open(argv[i], O_WRONLY | O_CREAT, perm);

		if (fd < 0) {
			fprintf(stderr, "Failed to create %s\n", argv[i]);
			return -1;
		}
		close(fd);
	}
	return 0;
}
