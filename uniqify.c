#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>


//expecting length to be less than 1000
#define BUF_SIZE 1000

//to be used with threads to pass info to function
typedef struct parseInfo {
		int structTaskNum;
		int **structparseFdArray;

	}parseInfoStruct;
//to be used with threads to pass info to function
typedef struct mergeInfo {
		int structTaskNum;
		int **structsortFdArray;

	}sortInfoStruct;
int wordCount = 1;
char wordTracker[BUF_SIZE] = "";
pthread_mutex_t threadLock = PTHREAD_MUTEX_INITIALIZER;
int merge(int taskNum, char **wordBuffer);

/*
Parser called #ifdef PROCESS.
Parses words "round robin" style using ParserFdArray into cleaned up tokens to be sent to the sorter
*/
void processParseInput(int taskNum, int **parserFdArray) {
	int i;
	char lineBuffer[BUF_SIZE];
	char wordBuffer[100]; 
	char *token;
	
	FILE **toSorter = malloc(taskNum * sizeof(FILE *));
	
	//assign parserFdArray[i][1] to each toSorter stream
	for (i = 0; i < taskNum; i++) {
		toSorter[i] = fdopen(parserFdArray[i][1], "w");
	}
	//while there is data to read
	while(fgets(lineBuffer, BUF_SIZE, stdin) != NULL) {
		//clean up strings
		for (i = 0; i < strlen(lineBuffer); i++) {
			if (isalpha(lineBuffer[i]) == 0) {
				//convert symbols to spaces to delimit with tokens
				lineBuffer[i] = ' ';
			} 
			else {
				//convert every character to lower
				lineBuffer[i] = tolower(lineBuffer[i]);
			}
		}

		//tokenize each word placing them into wordBuffer
		int fileNum = 0;
		token = strtok(lineBuffer, " ");
		if (token != NULL)
			sprintf(wordBuffer, "%s\n", token);
		while (token != NULL) {
			//distribute words round robin starting with toSorter[0]
			fputs(wordBuffer, toSorter[fileNum]);
			token = strtok(NULL, " ");
			if (token != NULL) {
				sprintf(wordBuffer, "%s\n", token);
			}
			//check to see if we need to restart parserFD index
			if (fileNum == (taskNum - 1)) {
				fileNum = 0;
			}
			else {
				fileNum++;
			}
		}
	}

	//close out the streams
	for (i = 0; i < taskNum; i++) {
		fclose(toSorter[i]);
	}
}

/*
Suppresor called #ifdef PROCESS. Reads values from sorterFdArray into wordBuffer to be merged
and output to stdout
*/
void processSuppressor(int taskNum, int **sorterFdArray) {
	int i = 0;
	int holder = 0; 
	FILE **input = malloc(taskNum * sizeof(FILE *));
	char **wordBuffer = malloc(taskNum * sizeof(char *));

	//open streams 
	for (i = 0; i < taskNum; i++) {
		input[i] = fdopen(sorterFdArray[i][0], "r");
		wordBuffer[i] = malloc(BUF_SIZE * sizeof(char));
		if (fgets(wordBuffer[i], BUF_SIZE, input[i]) == NULL)
			//set last value to NULL after reading values to wordBuffer from input
			wordBuffer[i] = NULL;
	}
	
	//call merge to start printing words
	int index = merge(taskNum, wordBuffer);
	while ((holder = index) + 1) {
		//set last value to NULL after reading values to wordBuffer from input
		if (fgets(wordBuffer[holder], BUF_SIZE, input[holder]) == NULL) {
			wordBuffer[holder] = NULL;		
		}
		//call merge when ready
		index = merge(taskNum, wordBuffer);
	}

	//close out streams and free wordBuffer memory
	for (i = 0; i < taskNum; i++) {
		fclose(input[i]);
		free(wordBuffer[i]);
	}

	free(wordBuffer);
	free(input);
}

/*
Parser called #ifdef THREAD.
Parses words "round robin" style using ParserFdArray passed by struct into cleaned up tokens to be sent to the sorter
*/
void *threadParseInput(parseInfoStruct *parseStruct) {	
	int i;
	char lineBuffer[BUF_SIZE];
    char wordBuffer[100];
    char *token;

    FILE **toSorter = malloc(parseStruct->structTaskNum * sizeof(FILE *));

	#ifdef THREAD
   		pthread_mutex_lock(&threadLock);
	#endif

    //assign streams to output from parsed buffers
    for (i = 0; i < parseStruct->structTaskNum; i++) {
        toSorter[i] = fdopen(parseStruct->structparseFdArray[i][1], "w");
    }

	
	#ifdef THREAD
		pthread_mutex_unlock(&threadLock);
	#endif

    while(fgets(lineBuffer, BUF_SIZE, stdin) != NULL) {
        for (i = 0; i < strlen(lineBuffer); i++) {
            if (isalpha(lineBuffer[i]) == 0) {
                //convert symbols to spaces to delimit with tokens
                lineBuffer[i] = ' ';
            } 
            else {
                //convert every character to lower
                lineBuffer[i] = tolower(lineBuffer[i]);
            }
        }
        //create tokens and store them in wordBUffer
        int fileNum = 0;
        token = strtok(lineBuffer, " ");
        if (token != NULL)
            sprintf(wordBuffer, "%s\n", token);
        while (token != NULL) {
            //ditribute words roung robin starting with toSorter[0]
            fputs(wordBuffer, toSorter[fileNum]);
            token = strtok(NULL, " ");
            if (token != NULL) {
                sprintf(wordBuffer, "%s\n", token);
            }
            if (fileNum == (parseStruct->structTaskNum - 1)) {
                fileNum = 0;
            }
            else {
                fileNum++;
            }
        }
    }

    for (i = 0; i < parseStruct->structTaskNum; i++) {
        fclose(toSorter[i]);
    }
	return NULL;
}


/*
Suppresor called #ifdef THREAD. Reads values from sorterFdArray into wordBuffer to be merged
and output to stdout.
*/
void *threadSuppressor(sortInfoStruct *sortStruct) {
	int i = 0;
	int holder = 0;
	FILE **input = malloc(sortStruct->structTaskNum * sizeof(FILE *));
	char **wordBuffer = malloc(sortStruct->structTaskNum * sizeof(char *));
	
	#ifdef THREAD
		pthread_mutex_lock(&threadLock);
	#endif

	for (i = 0; i < sortStruct->structTaskNum; i++) {
		input[i] = fdopen(sortStruct->structsortFdArray[i][0], "r");
		wordBuffer[i] = malloc(BUF_SIZE * sizeof(char));
		if (fgets(wordBuffer[i], BUF_SIZE, input[i]) == NULL)
			//set last value to NULL after reading values to wordBuffer from input
			wordBuffer[i] = NULL;
	}
	
	int index = merge(sortStruct->structTaskNum, wordBuffer);

	while ((holder = index) + 1) {
		//set last value to NULL after reading values to wordBuffer from input
		if (fgets(wordBuffer[holder], BUF_SIZE, input[holder]) == NULL) {
			wordBuffer[holder] = NULL;		
		}
		//call merge once ready
		index = merge(sortStruct->structTaskNum, wordBuffer);
	}
	
	#ifdef THREAD
		pthread_mutex_unlock(&threadLock);
	#endif

	//closes streams and frees wordBuffer memory allocation
	for (i = 0; i < sortStruct->structTaskNum; i++) {
		fclose(input[i]);
		free(wordBuffer[i]);
	}
	free(wordBuffer);
	free(input);
	return NULL;
}

int merge(int taskNum, char **wordBuffer) {
	int i = 0; 
	int holder = 0;
	//increments minimum word index holder
	while (wordBuffer[holder] == NULL) {
		if (holder == (taskNum - 1))
			return -1;
		holder++;
	}

	for (i = holder + 1; i < taskNum; i++) {
		if (wordBuffer[i] != NULL) {
			//if words do not match set i = holder
			if (strcmp(wordBuffer[i], wordBuffer[holder]) < 0) {
				holder = i;
			}
		}
	}
	//compare buffers to count word occurrences
	if (strcmp(wordBuffer[holder], wordTracker) == 0) {
		wordCount++;
	}
	else {
		//print out word count and word
		if (wordTracker != NULL && strcmp(wordTracker, "") != 0) {
			printf("Count: %d, Word: %s", wordCount, wordTracker);
		}
		//reset starting position
		strcpy(wordTracker, wordBuffer[holder]);
		wordCount = 1;
			
	}
	return holder;
}

/*
called when setting up pipes initially to close all necessary pipes
*/
void closePipes(int **FdArray, int taskNum) {
    for (int i = 0; i <= taskNum; i++) {
        for (int j = 0; j < 2; j++) {
            //Close all existing pipes 
            close(FdArray[i][j]);
        }
    }
}

/*
Main function sets up pipes, creates a parser and sorter file descriptor array
Handles #ifdef PROCESS and #ifdef THREAD 
*/
int main(int argc, char *argv[]) {
	int taskNum = atoi(argv[1]);
	//stores parser file descriptors in an array
	int **parserFdArray = malloc(taskNum * sizeof(int *));
	//stores sorter file descriptors in an array
	int **sorterFdArray = malloc(taskNum * sizeof(int *));
	pid_t *pid = malloc(taskNum * sizeof(int));
	int i;

	//allocate memory for each pipe in each i array
	for (i = 0; i < taskNum; i++) {
		parserFdArray[i] = malloc(2 * sizeof(int));
		sorterFdArray[i] = malloc(2 * sizeof(int));
	}
	pid_t spid;
	for (i = 0; i < taskNum; i++) {
		//create pipes from parserFdArray and sorterFdArray
		if (pipe(parserFdArray[i]) < 0) {
			fprintf(stderr,"Failed to get pipe.\n");
			exit(EXIT_FAILURE);
		} 
		if (pipe(sorterFdArray[i]) < 0) {
			fprintf(stderr,"Failed to get pipe.\n");
			exit(EXIT_FAILURE);
		}
		//forks n = taskNum processes
		spid = fork();
		if (spid == -1) {
			perror("Fork failed.");
			exit(EXIT_FAILURE);
		}
		//Child closes write from parser and closes read from sort/merge //
		if (spid == 0) {
			//redirect pipes in sorterFdArray and parserFdArray 
			dup2(sorterFdArray[i][1], 1);
			dup2(parserFdArray[i][0], 0);
			//close all pipes
			closePipes(parserFdArray, i);
            closePipes(sorterFdArray, i);
			//calls sort function
		    execlp("sort", "sort", (char *)NULL);
		    exit(EXIT_FAILURE);
		}
		else {
			close(parserFdArray[i][0]);
			close(sorterFdArray[i][1]);
			pid[i] = spid; 
		}	
	}

	#ifdef PROCESS
		//calls parser function with parserFdArray pointer array
		processParseInput(taskNum, parserFdArray);

		//forks merger process
		int mergerPid;
		mergerPid = fork();
		if (mergerPid == -1) {
			perror("Fork failed.");
		}
		else if (mergerPid == 0) {
			processSuppressor(taskNum, sorterFdArray);
		}
		else {
			waitpid(mergerPid, NULL, 0);
		}

	#endif


	#ifdef THREAD
		
		//creates a struct that stores the number of tasks and the parse file descriptor array so that both variables can be passed to functions using thread
		parseInfoStruct *parseFunctionStruct = malloc(sizeof(parseInfoStruct));
		parseFunctionStruct->structTaskNum = taskNum;
		parseFunctionStruct->structparseFdArray = malloc(taskNum * sizeof(int *));
		parseFunctionStruct->structparseFdArray = parserFdArray;

		//creates a struct that stores the number of tasks and the sort file descriptor array so that both variables can be passed to functions using thread
		sortInfoStruct *sortFunctionStruct = malloc(sizeof(sortInfoStruct));
		sortFunctionStruct->structTaskNum = taskNum;
		sortFunctionStruct->structsortFdArray = malloc(taskNum * sizeof(int *));
		sortFunctionStruct->structsortFdArray = sorterFdArray;

		//creates array and threads for array, and calls parse and sort functions
		pthread_t threadArray[2];
	    pthread_create(&threadArray[0], NULL, (void*)(void*)threadParseInput, parseFunctionStruct);
		pthread_create(&threadArray[1], NULL, (void*)(void*)threadSuppressor, sortFunctionStruct);

		for (int t = 0; t < 2; t++) {
	        pthread_join(threadArray[t], NULL);
	    }
		
		//frees previously allocated memory for thread structs 
		free(parseFunctionStruct);
		free(sortFunctionStruct);
	
	#endif
	
	free(parserFdArray);
	free(sorterFdArray);
	free(pid);

	for (i = 0; i < taskNum; i++) {
		wait(NULL);
	}
	return 0;
}