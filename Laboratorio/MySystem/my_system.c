#include <stdlib.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
	if (argc!=2) {
		fprintf(stderr, "Usage: %s <command>\n", argv[0]);
		exit(1);
	}

	return system(argv[1]);
}

int system(const char *command)
{
	int stat;
	pid_t child_pid;
	
	child_pid = fork();
	if (child_pid == 0) {
		/* proceso hijo */
		execl("/bin/bash","bash","-c",command,(char *) NULL);
	}
	else if (child_pid > 0) {
		/* proceso padre */
		wait(&stat);
		if (WIFEXITED(stat)) {
			stat = WEXITSTATUS(stat);
		}
		else {
			fprintf(stderr, "Error while terminating child\n");
			exit(1);
		}
	}
	else {
		/* error en el proceso */
		fprintf(stderr, "Error while executing fork\n");
		exit(1);
	}

	return stat;
}