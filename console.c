#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdarg.h>
#include "console.h"

char * invalidCommand = "Error: invalid command\n";
char * invalidDirectory = "Error: invalid directory\n";

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
// CHECK IF COMMANDS ARE VALID, return -1 if not valid
int isValid(int argc, char ** argv){
    // CASE 0: Not start or end with signals
    if(isSignal(argv[argc-1])==1||isSignal(argv[0])==1){
        return -1;
    }
    int noLess = 0;
    for(int i = 0; i < argc; i++){
        // CASE 0.0: Valid string, must be command, arg, filename, or signal

        // CASE 1: Things after built in arguments, consider in their functions sperately
        // CASE 2: Signals
        if(isSignal(argv[i])==1){
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
// [nyush lab2]$
void manipulate_args(int argc, char ** argv){
    if(isValid(argc,argv)==-1){
        printf("%s",invalidCommand);
    }
    // argc: number of arguments
    // argv: an array of arguments
    for (int i = 0; i < argc; ++i) {
        // Retrieve the length of the string
        // int len;  
        // for (len = 0; argv[i][len] != '\0'; ++len);
        // printf("%s ",argv[i]);

        // BUILTIN cd
        if(strcmp(argv[0], "cd")==0){
            if(cd(argv[1],argc-1)==-1){
                printf("%s",invalidCommand);
                break;
            }else if(cd(argv[1],argc-1)==-2){
                printf("%s",invalidDirectory);
                break;
            }
        }
    }
}
