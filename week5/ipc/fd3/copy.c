#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdio.h>


static void copy(void)
{
	int c;

	while ((c = getchar()) != EOF)
		putchar(c);
}

int main(int argc, char **argv)
{
	if (argc != 3) {
		fprintf(stderr, "Exactly two arguments expected\n");
		return -1;
	}

	int from = open(argv[1], O_RDONLY);
	
	if (from < 0) {
		fprintf(stderr, "Failed to open %s\n", argv[1]);
		return -1;
	}

	int err = dup2(from, 0);
	close(from);

	if (err < 0) {
		fprintf(stderr, "Failed to duplicate\n");
		return -1;
	}

	int perm = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
	int to = open(argv[2], O_WRONLY | O_CREAT, perm);

	if (to < 0) {
		fprintf(stderr, "Failed to open %s\n", argv[2]);
		return -1;
	}
		
	err = dup2(to, 1);
	close(to);

	if (err < 0) {
		fprintf(stderr, "Failed to duplicate\n");
		return -1;
	}

	copy();

	return 0;
}
