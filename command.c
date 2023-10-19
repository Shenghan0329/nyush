#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <limits.h>
#include <sys/wait.h>

#include "command.h"
#include "errors.h"

// Global Storage
// Store process and command names
static pid_t suspendedProcess[MAX_SUSPEND];
int processIndex = 0;
static char** suspendedCommandName[MAX_SUSPEND];
static pid_t currentProcess = 0;

// Control components
int TERMINATE = 0;
int toBreak = 0;
// Input and Output
char * input = NULL;
char * output = NULL;
char* currCommandName[MAX_ARG_NUMS];
// Whether to append
int append = 0;

// Pipe
static int fd[2];
static int fdd = 0;

// Handle Signal
void ctrlC(int sig){
    if(sig != -1){
    }
    if(currentProcess != 0){
        kill(currentProcess,SIGTERM);
        currentProcess = 0;
    }
    toBreak = 1;
}
void ctrlZ(int sig){
    if(sig != -1){
    }
    if(currentProcess != 0){
      kill(currentProcess,SIGTSTP);
      suspendedProcess[processIndex] = currentProcess;
      int i = 0;
      char** copyName = (char**) malloc(100*sizeof(char*));
      while(currCommandName[i] != NULL){
        char* copyArg = (char*) malloc(100*sizeof(char));
        strncpy(copyArg,currCommandName[i],100);
        copyName[i] = copyArg;
        i++;
      }
      copyName[i] = NULL;
      suspendedCommandName[processIndex] = copyName;
      processIndex ++;
      currentProcess = 0;
    }
    printf("\n");
    toBreak = 1;
}

// BUILDIN CD
void cd(char* dir,int argNum){
    // Check if too many or too few arguments
    if(argNum != 1){
        fprintf(stderr,invalidCommand);
        return;
    }
    // Reference: https://www.geeksforgeeks.org/how-to-check-a-file-or-directory-exists-in-cpp/
    // Check if Directory valid
    struct stat sb;
    if (stat(dir, &sb) != 0){
        fprintf(stderr,invalidDirectory);
        return;
    }
    char s[1000];
    getcwd(s, 1000);
    chdir(dir);
    return;
}
// BUILDIN JOBS
void jobs(int argNum){
    if(argNum != 0){
        fprintf(stderr,invalidCommand);
        return;
    }
    if(suspendedProcess[0] != 0){
        for(int i = 0; i<processIndex; i++){
            printf("[%d]",i+1);
            int j = 0;
            while(suspendedCommandName[i][j] != NULL){
                printf(" %s",suspendedCommandName[i][j++]);
            }
            printf("\n");
        }
        
    }
    return;
}
// BUILDIN FG
void fg(int index, int argNum){
    if(argNum != 1){
        fprintf(stderr,invalidCommand);
        return;
    }
    if(index<1 || index > processIndex){
        fprintf(stderr,invalidJob);
        return;
    }
    kill(suspendedProcess[index-1], SIGCONT);
    int status;
    waitpid(suspendedProcess[index-1], &status, WUNTRACED);
    int currProcess = suspendedProcess[index-1];
    char** currCommandName = suspendedCommandName[index-1];
    if(status == 0){
        int x = 0;
        while(suspendedCommandName[index-1][x] != NULL){
            free(suspendedCommandName[index-1][x]);
            x++;
        }
        free(suspendedCommandName[index-1]);
    }
    for(int i = index-1; i<processIndex-1; i++){
        suspendedCommandName[i] = suspendedCommandName[i+1];
        suspendedProcess[i] = suspendedProcess[i+1];
    }
    if(status == 0){
        suspendedCommandName[processIndex-1] = NULL;
        suspendedProcess[processIndex-1] = 0;
        processIndex --;
    }else{
        suspendedCommandName[processIndex-1] = currCommandName;
        suspendedProcess[processIndex-1] = currProcess;
    }
    return;
}
// BUITDIN EXIT
void ex(int argNum){
    if(argNum != 0){
        fprintf(stderr,invalidCommand);
        return;
    }else if(suspendedProcess[0] != 0){
        fprintf(stderr,suspendedJobs);
        return;
    }
    exit(0);
}

// Reference: inclass notes
int run(char* path, char ** args){
    pipe(fd);
    pid_t pid=fork();
    currentProcess = pid;
    if(pid==0){ 
        // Handle Input and Output
        // Reference: https://www.geeksforgeeks.org/dup-dup2-linux-system-call/
        // Reference: https://www.geeksforgeeks.org/input-output-system-calls-c-create-open-close-read-write/
        if (input != NULL) {
            int fd1;
            if((fd1=open(input, O_RDONLY)) < 0){
                fprintf(stderr,invalidFile);
                exit(-1);
            }
            if (dup2(fd1, STDIN_FILENO) == -1) {
                printf("Dup2 input failed\n");
            }
            close(fd1);
        }
        if (output != NULL) {
            int fd2;
            // Reference: https://stackoverflow.com/questions/23092040/how-to-open-a-file-which-overwrite-existing-content
            fd2 = append == 0 ? open(output, O_WRONLY | O_CREAT | O_TRUNC,0644):open(output, O_WRONLY | O_APPEND | O_CREAT,0644);
            if (fd2< 0) {
                fprintf(stderr,invalidFile);
                exit(-1);
            }
            if (dup2(fd2, STDOUT_FILENO) == -1) {
                printf("Dup2 output failed\n");
            }
            close(fd2);
        }
        // Handle pipes
        // Reference: https://www.geeksforgeeks.org/pipe-system-call/   
        // Reference: https://gist.github.com/aspatic/93e197083b65678a132b9ecee53cfe86
        dup2(fdd, 0);
        if (TERMINATE != 1) {
            dup2(fd[1], 1);
        }
        close(fd[0]); 
        execv(path,args);
        fprintf(stderr,invalidProgram);
        exit(-1);
    }
    // Unfortunately if fork fails
    else if(pid<0){
        printf("Unable to create child process\n");
        return -1;
    }
    // Parent process
    else{
        int status; 
        // Get child return, reference: https://www.geeksforgeeks.org/exit-status-child-process-linux/
        waitpid(pid, &status, WUNTRACED);
        int stopped = WIFSTOPPED(status);
        if(stopped == 1){
            suspendedProcess[processIndex] = currentProcess;
            int i = 0;
            char** copyName = (char**) malloc(100*sizeof(char*));
            while(currCommandName[i] != NULL){
                char* copyArg = (char*) malloc(100*sizeof(char));
                strncpy(copyArg,currCommandName[i],100);
                copyName[i] = copyArg;
                i++;
            }
            copyName[i] = NULL;
            suspendedCommandName[processIndex] = copyName;
            processIndex ++;
            currentProcess = 0;
        }
        close(fd[1]);
		fdd = fd[0];
        currentProcess = 0;
        return 0;
    }
}

// Execute Command
int exec(int argc, char ** argv){
    // handle Signals
    if (argc == 0){
        return -1;
    }
    char * cmd = argv[0];
    int result;
    // Case 1: cmd no /
    if(strchr(cmd, '/') == NULL){
        // I want to append cmd to "/usr/bin/", but not sure how to do it properly for this error:
        // error: variable-sized object may not be initialized (char path[9+strlen(cmd)] = "/usr/bin")
        int cmdLen = strlen(cmd); char path[cmdLen+10];
        path[0] = '/'; path[1] = 'u'; path[2] = 's'; path[3] = 'r'; path[4] = '/'; path[5] = 'b'; path[6] = 'i'; path[7] = 'n'; path[8] = '/'; 
        for(int i = 0; i<cmdLen; i++){
            path[i+9] = cmd[i];
        }
        path[cmdLen+9] = '\0';
        result = run(path,argv);
    }
    // Case 2: cmd starts with /
    else if(cmd[0]=='/'){
        result = run(cmd,argv);
    }
    // Case 3: cmd relative path
    else{
        int cmdLen = strlen(cmd); char path[cmdLen+3];
        path[0] = '.'; path[1] = '/'; 
        for(int i = 0; i<cmdLen; i++){
            path[i+2] = cmd[i];
        }
        path[cmdLen+2] = '\0';
        result = run(path, argv);   
    }
    return result;
}