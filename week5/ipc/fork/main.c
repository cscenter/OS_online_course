#include <stdio.h>
#include <unistd.h>


int main()
{
	const pid_t pid = fork();

	if (pid < 0) {
		printf("fork failed\n");
		return -1;
	}

	if (pid)
		printf("%d: I'm your father %d\n",
			(int)getpid(), (int)pid);
	else
		printf("%d: Noooooo!!!\n",
			(int)getpid());

	return 0;
}
