#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <sys/wait.h>
#include <unistd.h>


int main()
{
	const pid_t pid = fork();

	if (pid < 0) {
		printf("fork failed\n");
		return -1;
	}

	if (pid) {
		int status;

		waitpid(pid, &status, 0);
		printf("Return code: %d\n", WEXITSTATUS(status));
	} else {
		for (char c = 'n'; tolower(c) != 'y'; scanf("%c", &c))
			printf("Kill me? (y/n)\n");
		exit(42);
	}
	return 0;
}
