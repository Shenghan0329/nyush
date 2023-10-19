#ifndef _COMMAND_H_
#define _COMMAND_H_

#include "constants.h"
#include <stdio.h>

// Control components
extern int TERMINATE;
extern int toBreak;
// Input and Output
extern char * input;
extern char * output;
extern char* currCommandName[MAX_ARG_NUMS];
// Whether to append
extern int append;

void ctrlC(int sig);
void ctrlZ(int sig);
void cd(char* dir,int argNum);
void jobs(int argNum);
void fg(int index, int argNum);
void ex(int argNum);
int run(char* path, char ** args);
int exec(int argc, char ** argv);

#endif
