#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <sys/wait.h>

#include "console.h"
#include "validate.h"
#include "command.h"

// [nyush lab2]$
// argc: number of arguments
// argv: an array of arguminvalidsuspents
void manipulate_args(int argc, char ** argv, int toBreak){
    if(toBreak == 1){
        printf("\n");
        return;
    }
    if(isValid(argc,argv)==-1){
        fprintf(stderr,"Error: invalid command\n");
        return;
    }
    TERMINATE = 0;
    // Store current command, pipe, result of command, and whether to execute
    char ** currCmd = (char**) malloc((argc+1)*sizeof(char*));
    int acceptCommand = 1;
    int cmdSize = 0;
    char currSignal[2];
    currSignal[1] = '\0';
    int currResult;
    int toExe = 0;

    // Builtin commands with highest priority
    // BUILTIN cd
    if(strcmp(argv[0], "cd") == 0){
        cd(argv[1],argc-1);
        return;
    }
    // BUILDIN jobs
    if(strcmp(argv[0], "jobs") == 0){
        jobs(argc-1);
        return;
    }
    // BUILDIN fg
    if(strcmp(argv[0], "fg") == 0){
        if(argv[1]){
            fg(atoi(argv[1]),argc-1);
        }else{
            fg(666,argc-1);
        }
        return;
    }
    // printf("%d", strcmp(argv[0], "exit"));
    // BUILDIN exit
    if(strcmp(argv[0], "exit") == 0){
        ex(argc-1);
        return;
    }
    
    // Signals and other commands
    for (int i = 0; i < argc; ++i) {
        // Face signals
        // if(isSignal(argv[i])==1){
        if(strcmp(argv[i],"|")==0){
            strncpy(currSignal,argv[i],2);
            toExe = 1;
            acceptCommand = 1;
        }
        // Store commands
        else{
            if(strcmp(argv[i],"<")==0){
                input = argv[i+1];
                acceptCommand = 0;
            }else if(strcmp(argv[i],">")==0||strcmp(argv[i],">>")==0){
                append = strcmp(argv[i],">>") == 0 ? 1 : 0;
                output = argv[i+1];
                acceptCommand = 0;
            }
            if(acceptCommand == 1){
                currCmd[cmdSize] = argv[i];
                cmdSize ++;
            }
        }
        // execute command when the argv end or toExe==1, then empty the currCmd
        if(i == argc-1 || toExe == 1){
            currCmd[cmdSize] = NULL;
            if(i == argc - 1)
                TERMINATE = 1;
            currResult = exec(cmdSize,currCmd);
            // if exec fails, return error and free(currCmd)
            if(currResult != 0){
                free(currCmd);
                return;
            }
            cmdSize = 0;
            free(currCmd);
            currCmd = (char**) malloc((argc+1)*sizeof(char*));
            toExe = 0;
            input = NULL;
            output = NULL;
        }
    }
    free(currCmd);
}

int shell() {
  signal(SIGINT, ctrlC);
  signal(SIGTSTP, ctrlZ);
  // SHELL
  char usrInput[MAX_INPUT_LENGTH];
  int flag = 0;
  while(flag == 0){
    // Print Shell Prompt
    // Reference: https://stackoverflow.com/questions/298510/how-to-get-the-current-directory-in-a-c-program
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        char dir[100];
        char * currDir = strtok(cwd, "/"); // Split by space
        while (currDir != NULL) {
            // printf(currDir);
            strncpy(dir,currDir,100);
            currDir = strtok(NULL, "/");
        }
        printf("[nyush %s]$ ", dir);
        fflush(stdout);
    } else {
        perror("getcwd() error");
        return 1;
    }

    // Read user input
    if (fgets(usrInput, sizeof(usrInput), stdin) == NULL) {
      break; 
    }
    
    usrInput[strcspn(usrInput, "\n")] = '\0';
    // Process user input as parameters of manipulate_args()
    char *token;
    char *args[MAX_ARG_NUMS]; 
    int num = 0;
    if(strlen(usrInput)==0){
      continue;
    }
    if(toBreak == 1){
      toBreak = 0;
      continue;
    }
    token = strtok(usrInput, " "); // Split by space
    while (token != NULL) {
        currCommandName[num] = token;
        args[num] = token;
        token = strtok(NULL, " ");
        num++;
    }
    currCommandName[num] = NULL;
    // Call Terminal function to process the arguments
    manipulate_args(num, args, toBreak);
    toBreak = 0;
  }
  return 0;
}