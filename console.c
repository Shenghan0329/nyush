#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <sys/wait.h>

#include "console.h"

char * invalidCommand = "Error: invalid command\n";
char * invalidDirectory = "Error: invalid directory\n";
char * invalidProgram = "Error: invalid program\n";

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
int cd(char* dir,int argNum){
    // Check if too many or too few arguments
    if(argNum != 1)
        return -1;
    // Reference: https://www.geeksforgeeks.org/how-to-check-a-file-or-directory-exists-in-cpp/
    // Check if Directory valid
    struct stat sb;
    if (stat(dir, &sb) != 0)
        return -2;
    char s[100];
    getcwd(s, 100);
    chdir(dir);
    return 0;
}

// Reference: inclass notes
int run(char* path, char ** args,int argc){
    // printf("%s %d ", path, (int) strlen(path));
    // printf("\n");
    pid_t pid=fork();
    if(pid==0){
        for(int i = 0; i< argc; i++){
            // printf("%s %d\n", args[i], (int) strlen(args[i]));
        }
        execv(path,args);
        printf(invalidProgram);
        exit(-1);
    }
    int status; 
    // Get child return, reference: https://www.geeksforgeeks.org/exit-status-child-process-linux/
    waitpid(pid, &status, 0);
    if ( WIFEXITED(status) > 0 ){
        return -1;
    }
    return 0;
}
// Execute Command
int exec(int argc, char ** argv){
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
        result = run(path,argv,argc);
    }
    // Case 2: cmd starts with /
    else if(cmd[0]=='/'){
        result = run(cmd,argv,argc);
    }
    // Case 3: cmd relative path
    else{
        int cmdLen = strlen(cmd); char path[cmdLen+3];
        path[0] = '.'; path[1] = '/'; 
        for(int i = 0; i<cmdLen; i++){
            path[i+2] = cmd[i];
        }
        path[cmdLen+2] = '\0';
        result = run(path, argv, argc);   
    }
    return result;
}

// [nyush lab2]$
// argc: number of arguments
// argv: an array of arguments
void manipulate_args(int argc, char ** argv){
    if(isValid(argc,argv)==-1){
        printf("%s",invalidCommand);
        return;
    }
    // Store current command, pipe, result of command, and whether to execute
    char ** currCmd = (char**) malloc((argc+1)*sizeof(char*));
    int cmdSize = 0;
    char currSignal[2];
    currSignal[1] = '\0';
    int currResult;
    int toExe = 0;

    // Builtin commands with highest priority
    // BUILTIN cd
    // if(strcmp(argv[0], "cd")==0){
    //     if(cd(argv[1],argc-1)==-1){
    //         printf("%s",invalidCommand);
    //     }else if(cd(argv[1],argc-1)==-2){
    //         printf("%s",invalidDirectory);
    //     }
    //     return;
    // }
    
    // Signals and other commands
    for (int i = 0; i < argc; ++i) {
        // Face signals
        if(isSignal(argv[i])==1){
            strncpy(currSignal,argv[i],2);
            toExe = 1;
        }
        // Store commands
        else{
            currCmd[cmdSize] = argv[i];
            cmdSize ++;
        }
        // execute command when the argv end or toExe==1, then empty the currCmd
        if(i == argc-1 || toExe == 1){
            currCmd[cmdSize] = NULL;
            currResult = exec(cmdSize, currCmd);
            // if exec fails, return error and free(currCmd)
            if(currResult != 0){
                free(currCmd);
                return;
            }
            cmdSize = 0;
            free(currCmd);
            currCmd = (char**) malloc((argc+1)*sizeof(char*));
            toExe = 0;
        }
    }
    free(currCmd);
}
// Retrieve the length of the string
// int len;  
// for (len = 0; argv[i][len] != '\0'; ++len);
// printf("%s ",argv[i]);

