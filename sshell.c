/**************************************************************
 Simple Shell Implementation
 This Shell implements the following:
 1)The builtin commands: cd, pwd, and exit
 2)Input and Output Redirection
 3)Piping
 4)Background processes
 Authored by: Malini Pathakota & Srikavya Dindu
 **************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <ctype.h>
#include <stdlib.h>

#define CMD_LINE 1024

int redirectInFlag;
int redirectOutFlag;
int inRedirectLocation;
int outRedirectLocation;

enum{
    ERR_INVAL_CMDLINE,
    ERR_CMD_NOTFOUND,
    ERR_NO_DIR,
    ERR_CNTOPEN_INPTFILE,
    ERR_NO_INPTFILE,
    ERR_CNTOPEN_OUTPTFILE,
    ERR_NO_OUTPTFILE,
    ERR_MISLOC_INPT_REDIR,
    ERR_MISLOC_OUTPT_REDIR,
    ERR_MISLOC_BACKGRND,
    ERR_ACTIVEJOB_RNNING
};

// Prints the appropriate error message based on the error code parameter
void errorMessages(int errCode){
    switch(errCode){
        case ERR_INVAL_CMDLINE:
            fprintf(stderr, "Error: invalid command line\n");
            break;
        case ERR_CMD_NOTFOUND:
            fprintf(stderr, "Error: command not found\n");
            break;
        case ERR_NO_DIR:
            fprintf(stderr, "Error: no such directory\n");
            break;
        case ERR_CNTOPEN_INPTFILE:
            fprintf(stderr, "Error: cannot open input file\n");
            break;
        case ERR_NO_INPTFILE:
            fprintf(stderr, "Error: no input file\n");
            break;
        case ERR_CNTOPEN_OUTPTFILE:
            fprintf(stderr, "Error: cannot open output file\n");
            break;
        case ERR_NO_OUTPTFILE:
            fprintf(stderr, "Error: no output file\n");
            break;
        case ERR_MISLOC_INPT_REDIR:
            fprintf(stderr, "Error: mislocated input redirection\n");
            break;
        case ERR_MISLOC_OUTPT_REDIR:
            fprintf(stderr, "Error: mislocated output redirection\n");
            break;
        case ERR_MISLOC_BACKGRND:
            fprintf(stderr, "Error: mislocated background sign\n");
            break;
        case ERR_ACTIVEJOB_RNNING:
            fprintf(stderr, "Error: active jobs still running\n");
            break;
    }
}

// Parses through inputted string to create spaces between all the arguments and meta characters
char *createSpaces(char *enteredLine){
    int charSize = sizeof(char);
    char *spaced = (char *)malloc((CMD_LINE * 2) * charSize);
    
    int i = 0;
    int j = 0;
    while(i < strlen(enteredLine))
    {
        // Creates spaces around meta characters
        if(enteredLine[i] == '>' || enteredLine[i] == '<' || enteredLine[i] == '|' || enteredLine[i] == '&'){
            spaced[j] = ' ';
            j++;
            spaced[j] = enteredLine[i];
            j++;
            spaced[j] = ' ';
            j++;
        }
        else{
            spaced[j] = enteredLine[i];
            j++;
        }
        i++;
    }
    
    // Adds a null character at the end of the spaced out string
    spaced[j] = '\0';
    j++;
    char *end = spaced + strlen(spaced) - 1;
    *end = '\0';
    
    return spaced;
}

// Returns the length of the array of arguments
int findLength(char *args[]){
    int len = 0;
    while(args[len] != NULL){
        len++;
    }
    
    return len;
}

// Checks for errors regarding the meta characters being in the wrong locations or not having the correct arguments
int parseError(char *args[]){
    int errorFlag = 0;
    int len = findLength(args);
    
    // Sets errorFlag for errors relating to '<', '>' and '|'
    int i = 0;
    while(args[i] != NULL){
        if(strcmp("<",args[i]) == 0 || strcmp(">",args[i]) == 0 || strcmp("|",args[i]) == 0){
            if(i == 0){
                errorMessages(ERR_INVAL_CMDLINE);
                errorFlag = 1;
            }
            else if(i != 0){
                if(args[i-1] == NULL || args[i+1] == NULL){
                    if(strcmp(">",args[i]) == 0){
                        errorMessages(ERR_NO_OUTPTFILE);
                    }
                    else if(strcmp("<",args[i]) == 0){
                        errorMessages(ERR_NO_INPTFILE);
                    }
                    else{
                        errorMessages(ERR_INVAL_CMDLINE);
                    }
                    errorFlag = 1;
                }
            }
        }

        // Sets errorFlag for errors relating to '&'
        if(strcmp("&",args[i]) == 0){
            if(i != len - 1 || i == 0){
                errorMessages(ERR_MISLOC_BACKGRND);
                errorFlag = 1;
            }
        }
        i++;
    }
    return errorFlag;
}

// Prints the appropriate '+ completed' line after running each command
void printCommandStatus(char input[], int status){
    int len = strlen(input);
    strcpy(&input[len-1],"");
    fprintf(stderr, "+ completed '%s' [%d]\n", input, status);
}

// Implements the 'exit' and 'pwd' built in commands
int builtInCommands(char *args[]){
    if(strcmp("exit",args[0]) == 0){
        fprintf(stderr, "Bye...\n");
        exit(0);
    }
    if(strcmp("pwd",args[0]) == 0){
        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        return 1;
    }
    
    return 0;
}

// Checks if process needs to run in the background
int runBackground(char *args[]){
    int len = findLength(args);
    if(strcmp("&",args[len-1]) == 0){
        return 1;
    }
    return 0;
}

// Parses arguments into a format that can be used for input and output redirection
char *saveCombinedArgs(char *args[], char *combinedArgs[]){
    int len = findLength(args);
    int n = 0;
    int m = 0;
    while(args[n] != NULL){
        if(strcmp(args[n],"<") != 0 && strcmp(args[n],">") != 0){
            combinedArgs[m] = args[n];
        }
        else{
            m--;
        }
        n++;
        m++;
    }
    
    return *combinedArgs;
}

// Clears all the elements in the array of arguments
char *clearArgs(char *args[]){
    int len = findLength(args);
    int i = 0;
    
    while(i < len){
        args[i] = NULL;
        i++;
    }
    return *args;
}

// Sets flags to indicated whether the commands calls for input redirection or output redirection
// Returns a pointer to an array of parsed arguments to be used for input and output redirection
char *redirectionHandler(char *args[], char *combinedArgs[]){
    int i = 0;
    while(args[i] != NULL){
        if(strcmp(args[i],"<") == 0 || strcmp(args[i],">") == 0 || strcmp(args[i],"|") == 0){
            if(strcmp(args[i],"<") == 0){
                redirectInFlag = 1;
                inRedirectLocation = i;
            }
            else{
                redirectInFlag = 0;
            }
            
            if(strcmp(args[i],">") == 0){
                redirectOutFlag = 1;
                outRedirectLocation = i;
            }
            else{
                redirectOutFlag = 0;
            }

            *combinedArgs = clearArgs(combinedArgs);
            *combinedArgs = saveCombinedArgs(args, combinedArgs);
        }
        i++;
    }
    return *combinedArgs;
}

// Handles regular commands and redirection that do not use piping
// Forks
void commandHandler(char *args[], char input[], int backgroundFlag){
    int retval;
    int fd;
    
    int pid = fork();
    if (pid == 0){
        if(backgroundFlag == 1){
            args[findLength(args)-1] = NULL;
        }
        if(redirectInFlag == 1){
            fd = open(args[inRedirectLocation], O_RDONLY);
            if(fd < 0){
                errorMessages(ERR_CNTOPEN_INPTFILE);
                exit(0);
            }
            else{
                dup2(fd, 0);
                close(fd);
                args[inRedirectLocation]=NULL;
            }
        }
        else if(redirectOutFlag == 1){
            fd = open(args[outRedirectLocation], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            if(fd < 0){
                errorMessages(ERR_CNTOPEN_OUTPTFILE);
                exit(0);
            }
            else{
                dup2(fd, 1);
                close(fd);
                args[outRedirectLocation]=NULL;
            }
        }
        execvp(args[0], args);
        exit(1);
    }
    else if( pid < 0){
        perror("fork");
    }
    else{
        if(backgroundFlag == 1){
            // If the backgroundFlag is set, the parent does not wait for the child and the process runs in the background
            // waitpid(-1, &retval, WNOHANG);
        }
        else
        {
            waitpid(-1, &retval, 0);
            if(retval==256){
                errorMessages(ERR_CMD_NOTFOUND);
            }
            else{
                printCommandStatus(input, retval);
            }
        }
    }
}

// Returns the number of pipes in the entered command
int numberOfPipes(char *args[]){
    int pipes = 0;
    
    int i = 0;
    while(args[i]!=NULL){
        if(strcmp("|",args[i]) == 0){
            pipes++;
        }
        i++;
    }
    
    return pipes;
}

// Forks and saves output of command to be used for input of next command
void singlePipe(char *firstArgs[], int pipeInput, int pipeOutput, char input[]){
    char *combinedArgs[CMD_LINE];
    *combinedArgs = redirectionHandler(firstArgs, combinedArgs);

    // Forks and passes output of command into input of next command
    int pid = fork();
    if (pid == 0){
        if (pipeInput != 0){
            close(STDIN_FILENO);
            dup(pipeInput);
            close(pipeInput);
        }
        if (pipeOutput != 1){
            close(STDOUT_FILENO);
            dup(pipeOutput);
            close(pipeOutput);
        }
        // If the command uses redirection, control is passed to commandHandler to check for redirectionFlags and execute
        // the appropriate commands
        if(redirectInFlag == 1 || redirectOutFlag == 1){
            commandHandler(combinedArgs, input, 0);
        }
        else{
            execvp(firstArgs[0],firstArgs);
        }
    }
}

// Handles commands that use pipes by parsing through arguments to separate the commands on either sides of the pipes
// and passing each command through the singlePipe() method to use the output of one command as input for the next
void createPipes(char *args[], char input[]){
    char *firstArgs[CMD_LINE];
    char *lastArgs[CMD_LINE];
    int numPipes = numberOfPipes(args);

    // Parses through arguments to find the set of arguments after the last pipe
    int i = 0;
    int j = 0;
    int counter = 0;
    while(args[i] != NULL){
        if(strcmp(args[i],"|") == 0){
            counter++;
            i++;
        }
        if(counter == numPipes){
            lastArgs[j] = args[i];
            j++;
        }
        i++;
    }

    // Creates pipe and for each '|' found in the inputted command, finds the set of arguments to recieve output from
    // to use as input for next set of arguments
    int fd[2];
    int pipeInput;
    int inc = 0;
    int n = 0;
    while(inc < numPipes){
        // Parses arguments to find next set of commands to execute and then pass the output of into pipe
        int m = 0;
        while(args[n] != NULL){
            if(strcmp(args[n],"|") == 0){
                n++;
                break;
            }
            else{
                firstArgs[m] = args[n];
            }
            n++;
            m++;
        }

        // Passes parsed arguments into singlePipe() method and saves input to be used for next command in next loop
        pipe(fd);
        singlePipe(firstArgs, pipeInput, fd[1], input);
        close(fd[1]);
        pipeInput = fd[0];

        clearArgs(firstArgs);
        inc++;
    }
    if(pipeInput != 0){
        close(STDIN_FILENO);
        dup(pipeInput);
    }
    execvp(lastArgs[0],lastArgs);
}

// Main method that calls all the appropriate functions
int main(void){
    char *args[CMD_LINE];
    char *combinedArgs[CMD_LINE];
    
    while(1){
        redirectInFlag = 0;
        redirectOutFlag = 0;
        
        // Prints 'sshell$' and recieves input from user
        printf("sshell$ ");
        fflush(stdout);
        char input[CMD_LINE];
        fgets(input, CMD_LINE, stdin);
        
        if(!isatty(STDIN_FILENO)){
            printf("%s", input);
            fflush(stdout);
        }
        
        if(input[0] == '\n'){
            continue;
        }
        
        // Creates spaces around all meta characters of input and tokenizes it based on the ' ' delimiter
        char *spaced = createSpaces(input);
        char *arg = strtok(spaced," ");
        int i = 0;
        while (arg){
            args[i] = arg;
            i++;
            arg = strtok(NULL, " ");
        }
        
        // Executes the 'cd' built in command if necessary
        args[i] = NULL;
        if(strcmp("cd",args[0]) == 0){
            if(chdir(args[1]) == 0){
                printCommandStatus(input,0);
            }
            else if(chdir(args[1]) == -1){
                errorMessages(ERR_NO_DIR);
            }
            continue;
        }
        
        // Runs the 'exit' and 'pwd' built in commands if necessary
        builtInCommands(args);
        
        // Executes regular commands or commands regarding meta characters if no errors were found
        if(parseError(args) == 0){
            // Sets backgroundFlag if '&' is found in command
            int backgroundFlag = runBackground(args);
            
            // Passes control to createPipes() method if any pipes are found in command
            int numpipe = numberOfPipes(args);
            if(numpipe != 0){
                createPipes(args, input);
                continue;
            }
            else{
                // Otherwise, handles commands that involve redirection with no pipes or regular commands
                *combinedArgs = redirectionHandler(args, combinedArgs);
                if(redirectInFlag == 1 || redirectOutFlag == 1){
                    commandHandler(combinedArgs, input, backgroundFlag);
                }
                else{
                    commandHandler(args, input, backgroundFlag);
                }
            }
        }
    }
    return 0;
}
