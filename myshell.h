#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include<errno.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<fcntl.h>

void newfork(char ** args, int ampCount, int outCount){
    fflush(stdout); //clear the stdin buffer to prevent print errors
    int fileNum;

    pid_t  pid; //holds pid of the child process

    pid = fork();
        
    if (pid == 0) { //child
        if (outCount == 1){ //if there is a redirect
            int fileIndex;

            //loop to find file name
            for (int j = 0; j < 32; j++){ //32 is max words in 1 command
                if (!strcmp(args[j], ">")){
                    fileIndex = j + 1; //index is one after carror
                    break; //exit once found
                }
            }
            close(STDOUT_FILENO); //write
            close(STDIN_FILENO);

            args[fileIndex - 1] = NULL;

            fileNum = open(args[fileIndex], O_WRONLY | O_CREAT, 0777); //open file
            dup2(fileNum, STDOUT_FILENO); //write
            close(fileNum);
        }
        execve(args[0], args, NULL); //run command
        exit (0);
    } 
    else if (pid > 0) { //parent
        if (!ampCount) { //wait if there isn't an amp
            pid = waitpid(pid, NULL, 0); 
        }
    } 
    else { //error 
        printf("Error in fork.");
        perror("Error in fork.");
    }
    return;
}

int addSpaces(char * array, const char * specialChar){ //allows additional input parsing
    char tempArray[512]; //holds strings that need to be concatenate onto finalArray
    char finalArray[512]; //holds final array that will be transferred into input
    
    memset(finalArray, '\0', sizeof(finalArray)); //set every value in array to null
    memset(tempArray, '\0', sizeof(tempArray)); //set every value in array to null
    size_t lengthArr = sizeof(array)/sizeof(array[0]); //calculates the number of characters in the array

    char * charFound = NULL; //pointer to location in array where character was found
    char * arrayPtr = array;

    int foundBool = 1; //checks if the character was found in the array
    int perfectBool = 1; //set to 0 if a change is made to the string
    int charCount = 0;

    charFound = strpbrk(arrayPtr, specialChar); //find first special character in array

    if (charFound == NULL){
        strcat(finalArray, array);
        foundBool = 0;
    }

    while (charFound != NULL){
        //printf("Char %s found!\n", specialChar);
        int editedBool = 0;

        int wordLength = strcspn(arrayPtr, specialChar); //find number of spaces before end of word

        if (*(charFound - 1) != ' '){ //no space before special char
            //printf("No space before char\n");
            memset(tempArray, '\0', sizeof(tempArray)); //reset temp array

            for (int i = 0; i < wordLength; i++){
                tempArray[i] = arrayPtr[i];
            }

            //add array and space to final array
            strcat(finalArray, tempArray);
            strcat(finalArray, " ");
            strcat(finalArray, specialChar);

            editedBool = 1; //the space before has been handled
            perfectBool = 0;
        }

        if (*(charFound + 1) != ' '){ //no space after special char
            //printf("No space after char\n");
            memset(tempArray, '\0', sizeof(tempArray)); //reset temp array

            if (editedBool){ //if no space run before on this character, add space after
                strcat(finalArray, " ");
            }
            else if(!editedBool){ //if space before this character, get entire command part and cat on the space
                for (int i = 0; i < (wordLength + 1); i++){
                    tempArray[i] = arrayPtr[i];
                }

            strcat(finalArray, tempArray);
            strcat(finalArray, " ");

            perfectBool = 0;
            }
        }
        //reset for next loop
        editedBool = 0;
        charCount++;
        arrayPtr = (charFound + 1);
        charFound = strpbrk(charFound + 1, specialChar);
    }
    //after processing all commands before last
    if (perfectBool){
        arrayPtr = array;
    }
    
    if (foundBool){
        for (int i = 0; i < strlen(arrayPtr); i++){
            tempArray[i] = arrayPtr[i];
        }
        strcat(finalArray, tempArray);
    }

    //set edited array values to array
    for (int i = 0; i < strlen(finalArray); i++){
        array[i] = finalArray[i];
    }

    //printf("The result is '%s'\n", finalArray);
    return charCount;
}