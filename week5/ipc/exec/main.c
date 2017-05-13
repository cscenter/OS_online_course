#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stddef.h>
#include <string.h>

#include <sys/wait.h>
#include <unistd.h>


static char **parse_cmd(const char *str)
{
	char **args = malloc(128 * sizeof(*args));
	size_t size = 0;
	size_t cap = 127;

	for (size_t i = 0; str[i];) {
		while (str[i] && isspace(str[i]))
			++i;

		if (!str[i])
			break;

		if (size == cap) {
			cap = 2 * (cap + 1) - 1;
			args = realloc(args, sizeof(*args) * (cap + 1));
		}

		const size_t p = i;

		while (str[i] && !isspace(str[i]))
			++i;

		args[size] = malloc((i - p + 1) * sizeof(char));
		strncpy(args[size], &str[p], i - p);
		args[size][i - p] = '\0';
		args[++size] = NULL;
	}
	return args;
}

static void run_cmd(const char *cmd)
{
	const pid_t pid = fork();

	if (pid < 0) {
		printf("fork failed\n");
		return;
	}

	if (pid) {
		waitpid(pid, NULL, 0);
		return;
	}

	char **args = parse_cmd(cmd);

	execvp(args[0], args);
	printf("exec failed\n");
}

int main()
{
	while (1) {
		size_t size = 0;
		char *cmd = 0;
		const ssize_t read = getline(&cmd, &size, stdin);

		if (read < 0) {
			free(cmd);
			break;
		}

		if (cmd[read - 1] == '\n')
			cmd[read - 1] = '\0';

		if (!strcmp(cmd, "exit")) {
			free(cmd);
			break;
		}

		run_cmd(cmd);
		free(cmd);
	}
	return 0;
}
