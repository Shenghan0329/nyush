#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include "console.h"

#define MAX_INPUT_LENGTH 1024
#define MAX_ARG_NUMS 100

pid_t suspended[100];
pid_t currentProcess;

int main() {
  // SHELL
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
    manipulate_args(num, args, suspended);
  }
  return 0;
}
