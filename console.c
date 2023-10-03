#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "console.h"

char * invalidCommand = "Error: invalid command\n";
char * invalidDirectory = "Error: invalid directory\n";
char * invalidProgram = "Error: invalid program\n";
char * invalidFile = "Error: invalid file\n";
char * suspendedJobs = "Error: there are suspended jobs\n";

char * builtInCommands[] = {"cd","jobs","fg","exit"};
char * signals[] = {"|", ">", ">>", "<"};

// Check if a string is a signal, return 1 if true
int isSignal(char* str){
    for(int j = 0; j < 4; j++){
        if(strcmp(str,signals[j])==0){
            return 1;
        }
    }
    return 0;
}
// Check if a string is a command or buildin command, return 1 if true
int isCmd(char* str){
  for(int i = 0; str[i] != '\0'; ++i){
    if(str[i] == '>'||str[i] == '<'||str[i] == '|'||str[i] == '*'||str[i] == '!'||str[i] == '`'||str[i] == '\''||str[i] == '\"'){
      return 0;
    }
  }
  return 1;
}
// CHECK IF COMMANDS ARE VALID, return -1 if not valid
int isValid(int argc, char ** argv){
    // CASE 0: Not start or end with signals
    if(isSignal(argv[argc-1])==1||isSignal(argv[0])==1){
        return -1;
    }
    int noLess = 0;
    for(int i = 0; i < argc; i++){
        int isSign = isSignal(argv[i]);
        // CASE 0.0: Valid string, must be command, arg, filename, or signal
        if(isCmd(argv[i])==0&&isSign==0){
            return -1;
        }
        // CASE 1: Things after built in arguments, consider in their functions sperately
        // CASE 2: Signals
        if(isSign==1){
            // 1) Two signals not next to each others
            if(isSignal(argv[i+1])==1){
                return -1;
            }
            // 2) <, the second argument following must be non-"<" signal
            if(strcmp(argv[i],"<")==0){
                if(i+2<argc&&((isSignal(argv[i+2])==0)||strcmp(argv[i+2],"<")==0)){
                    return -1;
                }
            }
            // 3) |, no < after, the next argument is not BuiltIn
            if(strcmp(argv[i],"|")==0)
                noLess = 1;
            // 4) >, >>, if exists second argument following, it must be <
            if(strcmp(argv[i],">")==0||strcmp(argv[i],">>")==0){
                if(i+2<argc&&(!strcmp(argv[i+2],"<")==0)){
                    return -1;
                }
            }
        }
        if(noLess==1&&strcmp(argv[i],"<")==0){
            return -1;
        }
    }
    return 0;
}

// BUILDIN CD
void cd(char* dir,int argNum){
    // Check if too many or too few arguments
    if(argNum != 1){
        printf("%s",invalidCommand);
        return;
    }
    // Reference: https://www.geeksforgeeks.org/how-to-check-a-file-or-directory-exists-in-cpp/
    // Check if Directory valid
    struct stat sb;
    if (stat(dir, &sb) != 0){
        printf("%s",invalidDirectory);
        return;
    }
    char s[1000];
    getcwd(s, 1000);
    chdir(dir);
    return;
}
// BUITDIN EXIT
void ex(pid_t * suspended, int argNum){
    if(argNum != 0){
        printf(invalidCommand);
        return;
    }else if(suspended[0] != 0){
        printf(suspendedJobs);
        return;
    }
    raise(SIGTERM);
}

// Reference: inclass notes
int run(char* path, char ** args,int append, char* input, char* output, int* fd, int* fdd, int terminate){
    pipe(fd);
    pid_t pid=fork();
    if(pid==0){   
        // Handle Input and Output
        // Reference: https://www.geeksforgeeks.org/dup-dup2-linux-system-call/
        // Reference: https://www.geeksforgeeks.org/input-output-system-calls-c-create-open-close-read-write/
        if (input != NULL) {
            int fd1;
            if((fd1=open(input, O_RDONLY)) < 0){
                printf(invalidFile);
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
                printf(invalidFile);
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
        dup2(*fdd, 0);
        if (terminate != 1) {
            dup2(fd[1], 1);
        }
        close(fd[0]); 
        execv(path,args);
        printf(invalidProgram);
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
        waitpid(pid, &status, 0);
        close(fd[1]);
		*fdd = fd[0];
        return 0;
    }
}

// Execute Command
int exec(int argc, char ** argv, char * input, char * output, int append, int* fd, int* fdd, int terminate){
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
        result = run(path,argv,append, input, output, fd, fdd, terminate);
    }
    // Case 2: cmd starts with /
    else if(cmd[0]=='/'){
        result = run(cmd,argv,append,input, output, fd, fdd, terminate);
    }
    // Case 3: cmd relative path
    else{
        int cmdLen = strlen(cmd); char path[cmdLen+3];
        path[0] = '.'; path[1] = '/'; 
        for(int i = 0; i<cmdLen; i++){
            path[i+2] = cmd[i];
        }
        path[cmdLen+2] = '\0';
        result = run(path, argv, append, input, output, fd, fdd, terminate);   
    }
    return result;
}

// [nyush lab2]$
// argc: number of arguments
// argv: an array of arguments
void manipulate_args(int argc, char ** argv, pid_t * suspended){
    if(isValid(argc,argv)==-1){
        printf("%s",invalidCommand);
        return;
    }
    // Pipe
    int fd[2];
    int fdd = 0;
    int terminate = 0;
    // Store current command, pipe, result of command, and whether to execute
    char ** currCmd = (char**) malloc((argc+1)*sizeof(char*));
    char * input;
    char * output;
    input = NULL;
    output = NULL;
    int acceptCommand = 1;
    int append = 0;
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
    if(strcmp(argv[0], "exit") == 0){
        ex(suspended,argc-1);
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
                terminate = 1;
            currResult = exec(cmdSize, currCmd,input,output, append,fd,&fdd, terminate);
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
// Retrieve the length of the string
// int len;  
// for (len = 0; argv[i][len] != '\0'; ++len);
// printf("%s ",argv[i]);

