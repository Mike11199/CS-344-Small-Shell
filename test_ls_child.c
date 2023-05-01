#include <sys/wait.h> // for waitpid
#include <stdio.h>    // for printf and perror
#include <stdlib.h>   // for exit
#include <unistd.h>   // for execv, getpid, fork

/*
  The following program forks a child process. The child process then replaces
  the program using execv to run "/bin/ls". The parent process waits for the
  child process to terminate.
*/
int main(){
	char *newargv[] = { "/bin/ls", "-al", NULL };
  char *newargv2[] = { "ls", NULL };
	int childStatus;

	// Fork a new process
	pid_t spawnPid = fork();

	switch(spawnPid){
	case -1:
		perror("fork()\n");
		exit(1);
		break;
	case 0:
		// In the child process
		printf("CHILD(%d) running ls command\n", getpid());
		// Replace the current program with "/bin/ls"
		//execv(newargv[0], newargv);
    execvp(newargv2[0], newargv2);
		// exec only returns if there is an error
		perror("execve");
		exit(2);
		break;
	default:
		// In the parent process
		// Wait for child's termination
		spawnPid = waitpid(spawnPid, &childStatus, 0);
		printf("PARENT(%d): child(%d) terminated. Exiting\n", getpid(), spawnPid);
		exit(0);
		break;
	} 
}
