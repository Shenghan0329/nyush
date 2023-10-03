#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/wait.h>
#include "console.h"

#define MAX_INPUT_LENGTH 1024
#define MAX_ARG_NUMS 100
// static void
// pipeline(char ***cmd)
// {
// 	int fd[2];
// 	pid_t pid;
// 	int fdd = 0;				/* Backup */

// 	while (*cmd != NULL) {
// 		pipe(fd);				/* Sharing bidiflow */
// 		if ((pid = fork()) == -1) {
// 			perror("fork");
// 			exit(1);
// 		}
// 		else if (pid == 0) {
// 			dup2(fdd, 0);
// 			if (*(cmd + 1) != NULL) {
// 				dup2(fd[1], 1);
// 			}
// 			close(fd[0]);
// 			execvp((*cmd)[0], *cmd);
// 			exit(1);
// 		}
// 		else {
// 			wait(NULL); 		/* Collect childs */
// 			close(fd[1]);
// 			fdd = fd[0];
// 			cmd++;
// 		}
// 	}
// }

int main() {
  // char *ls[] = {"ls", "-al", NULL};
	// char *rev[] = {"rev", NULL};
	// char *nl[] = {"nl", NULL};
	// char *cat[] = {"cat", "-e", NULL};
	// char **cmd[] = {ls, rev, nl, cat, NULL};

	// // pipeline(cmd);
  char input[MAX_INPUT_LENGTH];
  int flag = 0;
  while(flag == 0){
    // Print Shell Prompt
    // Reference: https://stackoverflow.com/questions/298510/how-to-get-the-current-directory-in-a-c-program
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("[nyush %s]$ ", cwd);
    } else {
        perror("getcwd() error");
        return 1;
    }

    // Read user input
    if (fgets(input, sizeof(input), stdin) == NULL) {
      break; 
    }
    input[strcspn(input, "\n")] = '\0';

    // Process user input as parameters of manipulate_args()
    char *token;
    char *args[10]; 
    int num = 0;
    token = strtok(input, " "); // Split by space
    while (token != NULL) {
        args[num++] = token;
        token = strtok(NULL, " ");
    }

    // Call Terminal function to process the arguments
    manipulate_args(num, args);
  }
  return 0;
}
