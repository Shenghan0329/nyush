#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <signal.h>
#include "console.h"

#define MAX_INPUT_LENGTH 1024
#define MAX_ARG_NUMS 100

static pid_t suspendedProcess[100];
int processIndex = 0;
static char* suspendedProcessName[100];
static pid_t currentProcess = 0;

int toBreak = 0;
void ctrlC(int sig){
    if(sig != -1){
    }
    if(currentProcess != 0){
        kill(currentProcess,SIGTERM);
        currentProcess = 0;
    }
    toBreak = 1;
    printf("\n");
}
void ctrlZ(int sig){
    if(sig != -1){
    }
    if(currentProcess != 0){
      kill(currentProcess,SIGTSTP);
      suspendedProcess[processIndex] = currentProcess;
      processIndex ++;
      currentProcess = 0;
    }
    toBreak = 1;
}

int main() {
  signal(SIGINT, ctrlC);
  signal(SIGQUIT, ctrlZ);
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
      continue; 
    }
    
    input[strcspn(input, "\n")] = '\0';
    // Process user input as parameters of manipulate_args()
    char *token;
    char *args[10]; 
    int num = 0;
    if(strlen(input)==0){
      continue;
    }
    token = strtok(input, " "); // Split by space
    while (token != NULL) {
        args[num++] = token;
        token = strtok(NULL, " ");
    }
    if(toBreak == 1){
      toBreak = 0;
      continue;
    }
    // Call Terminal function to process the arguments
    manipulate_args(num, args, toBreak,&currentProcess, suspendedProcess);
    toBreak = 0;
  }
  return 0;
}
