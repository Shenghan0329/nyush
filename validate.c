#include <string.h>
#include "validate.h"

// Signals and builtin commands
const char * builtInCommands[] = {"cd","jobs","fg","exit"};
const char * signals[] = {"|", ">", ">>", "<"};

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