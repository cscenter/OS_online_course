#include <sys/wait.h> /* waitpid */
#include <unistd.h> /* fork */
#include <stdio.h>


int main()
{
	static int value = 42;
	const pid_t pid = fork();

	if (pid == -1) {
		perror("failed to fork");
		return 1;
	}

	if (!pid) value = 43;
	else waitpid(pid, NULL, 0);

	printf("value (%p) = %d\n", &value, value);

	return 0;
};
