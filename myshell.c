#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include<errno.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/types.h>
#include <signal.h>

#include "myshell.h"

void newfork(char ** args, int ampCount, int outCount);
int addSpaces(char * array, const char * specialChar);

int main(int argc, char* argv[]) {
    //input supression
    int supressBool = 0; //if 0, does not surpress input

    for (int i = 0; i < argc; i++){
        if (!(strcmp(argv[i], "-n"))){ //checks each arg to see if it's -n
            supressBool = 1;
        } 
    }

    //variables for parse
    int exitFlag = 0; //tells program to terminate main loop

    char userInput[512]; //sets maximum size of user input to 512
    char* spaceArray[32]; //array that can hold pointers to 32 character arrays

    const char delimiter[]=" "; //sets space as the character that will mark separations between words
    char* space = NULL; //stores pointer to spaces found by strpbrk

    int wordCounter = 0; //stores the number of words passed in the current line for parsing loop

    //variables for fork
    pid_t  pid; //holds pid of the child process
    char commandInput[512]; //sets maximum size of user input to 512
    char* args[31]; //holds arguments for execve
    int waitStatus;

    //shell loop
    while (exitFlag == 0){
        //reset loop variables
        wordCounter = 0;

        signal(SIGCHLD, SIG_IGN); //parents ignores children when recieved, and they are reaped

        if (supressBool == 0) {
            printf("my_shell$ "); //prompt input from user
        }

        //get and format user input
        char* currentLine = fgets(userInput, 512, stdin); //reads up to 512 characters from stdin and stores them in userInput array
        
        if (currentLine == NULL){ //breaks loop if ctrl+d is pressed
            break;
        }
        
        currentLine[strcspn(currentLine, "\n")] = ' '; //removes newline character from input string by calculating the number of characters before it appears and adds a space to flag last word
        char* currentWord = currentLine; //the currentWord pointer is now set to the same location as the start of the current input line

        //functions to put spaces before and after special characters if there isn't one already, and check if these characters appear
        int pipeCount = addSpaces(currentLine, "|"); //pipecount holds the number of pipes in the input
        int inputCount = addSpaces(currentLine, "<");
        int outputCount = addSpaces(currentLine, ">");
        int ampCount = addSpaces(currentLine, "&");
        //int ampCount = 0;

        //loop to parse string
        space = strpbrk(currentLine, delimiter); //finds the first space in the current line
        
        while (space != NULL){
            int wordLength = strcspn(currentWord, " "); //find number of spaces before end of word
            currentWord[wordLength] = '\0'; //set the space at the end of the word to a null

            spaceArray[wordCounter] = space - wordLength; //sets the pointer to first character of each word

            currentWord = space + 1; //move current word pointer to next word in string
            space = strpbrk(space + 1, delimiter); //sets starting position at character after found space and looks for next space

            wordCounter++; //notes that we have gone through the loop once
        }
        spaceArray[wordCounter] = NULL; //mark end of char* array

        //***************STRINGARRAY HAS ALL USER INPUTS SEPARATED BY SPACES HERE (don't edit above line)***************

        //Process to break up input into multiple components

        //file is for debugging
        FILE *fptr;
        remove("./defaultFile"); //make sure file is clear
        fptr = fopen("./defaultFile","w+"); //make a new file to write and read from
        
        //----
        //make matrix with commands
        int commandCount = 0; //holds the number of commands
        int index = 0; //holds the word number for *current line*
        
        char * commandArray[pipeCount + 1][32]; //each row is a new command, up to 32 words per command
        memset(commandArray, '\0', sizeof(commandArray)); //set all characters in array to null
        
        currentWord = currentLine; //reset current word pointer to start of input line

        //separate commands at each pipe
        for (int i = 0; i < wordCounter; i++){
            if (strcmp(*(spaceArray+i), "|")){ //if the strings are not the same, write to file
                fprintf(fptr, "%s ", *(spaceArray+i)); //write the next word in the space array
                commandArray[commandCount][index] = *(spaceArray+i);

                //locate input character if it exists
                if (!strcmp(*(spaceArray+i), "<")){ //test if input character is <
                
                }
                index++;
            }
            else{ //if pipe found, add newline
                fprintf(fptr, "\n");
                index = 0;
                commandCount++;
            }
        }

        //find arg array and fork
        if (pipeCount == 0){ //if there is only one command
            newfork(commandArray[0], ampCount, outputCount);
        }
        
        else{ //there is a pipe
            pid_t  pid; //holds pid of the child process

            FILE * writePtr; //holds file pointer for output if needed
            int fileNum; //holds fd of file if needed

            int pipeFD[2]; //pipeFile[0] = read; pipeFile[1] = write
            pipe(pipeFD);

            for (int i = 0; i < (pipeCount + 1); i++){
                fflush(stdout); //clear the stdin buffer to prevent print errors
                pid = fork(); //gets pid of each child process
                
                if (pid == 0){ //child
                    if (i == 0){ //first command just writes
                        //close(STDOUT_FILENO);
                        dup2(pipeFD[1], STDOUT_FILENO); // set fd id of pipe[1] to the same as fdout so exec writes to stdout
                        execve(commandArray[i][0], commandArray[i], NULL);
                        exit(0);
                    }
                    else if (i != pipeCount){ //any following command reads and writes
                        close(STDOUT_FILENO);
                        close(STDIN_FILENO);
                        dup2(pipeFD[0], STDIN_FILENO); //read
                        dup2(pipeFD[1], STDOUT_FILENO); //write
                        execve(commandArray[i][0], commandArray[i], NULL);
                        exit(0);
                    }
                    else { //last command just reads
                        int fileIndex = 0;

                        close(STDIN_FILENO);
                        dup2(pipeFD[0], STDIN_FILENO); //read

                        if (outputCount == 1){ //if there is a redirect
                            int fileIndex;

                            //loop to find file name
                            for (int j = 0; j < 32; j++){ //32 is max words in 1 command
                                if (!strcmp(commandArray[i][j], ">")){
                                    fileIndex = j + 1; //index is one after carror
                                    break; //exit once found
                                }
                            }
                            fileNum = open(commandArray[i][fileIndex], O_WRONLY | O_CREAT, 0777); //open file
                            commandArray[i][fileIndex - 1] = NULL;

                            dup2(fileNum, STDOUT_FILENO); //write
                            close(fileNum);
                        }
                        execve(commandArray[i][0], commandArray[i], NULL);
                        exit(0);
                    }
                }
                else if (pid > 0){ //parent
                    //if (outputCount == 1) close(fileNum);
                    close(pipeFD[1]); //close write end
                    
                    if (!ampCount) { //wait if there isn't an amp
                        pid = waitpid(pid, NULL, 0);
                    }
                }
                else{ //error
                    printf("Error in fork.");
                    perror("Error in fork.");
                }
            }

            //now all pipes have run
            close(pipeFD[0]);
            close(pipeFD[1]);
        }

        fclose(fptr);
    }
    printf("\n");
    return 0;
}