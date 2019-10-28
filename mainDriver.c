/*
Assignment3.c
Spawn Trains and check RAG for deadlock conditions
Written by 
Benjamin Jollymore 
Latest version as of 2018-11-24
Run on Ubuntu 18.04
**/

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

//define locations for named semaphores
#define SEM_N "/semaphore_north"
#define SEM_W "/semaphore_west"
#define SEM_S "/semaphore_south"
#define SEM_E "/semaphore_east"
#define SEM_JUNCTION "/semaphore_junction"
#define SEM_WRITE "/semaphore_write"
#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)
#define INITIAL_VALUE 1

int **matrix;

void initMatrix(int trainNum){
//get train sequence from file. Maximum 100 trains.
	char fileName[] = "matrix.txt";
	
	//open file
	FILE *inputFile;
	inputFile = fopen(fileName, "w");
	//ensure file is accessable
	if (inputFile == NULL) {
		printf("Error Opening File: %s, Program terminating on exit code 1.\n", fileName);
		exit(1);
	}
	//Send file content to array with err checking
	for (int i = 0; i < trainNum; i++){
		for (int j = 0; j < 4; j++){

			if (fprintf(inputFile, "%s", "0 ")<0){
				printf("Error Reading File: %s, Program terminating on exit code 1.\n", fileName);
				exit(1);
			}
		}
		if (fprintf(inputFile, "%s", "\n")<0){
			printf("Error Reading File: %s, Program terminating on exit code 1.\n", fileName);
			exit(1);

		}
	}
	//close file
	fclose(inputFile);

}

void readMatrix(int rows)
{
    FILE *inputFile;
    inputFile = fopen ("matrix.txt", "r");
    if (inputFile == NULL)
        exit(0);

    for(int i = 0; i < rows; i++)
    {
            fscanf(inputFile, "%d %d %d %d ", &matrix[i][0], &matrix[i][1], &matrix[i][2], &matrix[i][3]);
    }

    fclose (inputFile); 
}

int checkDeadlock(int rows){
	printf("Deadlock Check Active\n");
    int deadlock = 0;
    int lockedNodes = 0;

    //check cycle
    for(int i = 0; i < rows; i++) {
        //check for 2 in each row
        for(int j = 0; j < 4; j++){
            //if 2 found, stop loop, check for 1
            if (matrix[i][j] == 2){
                //check row for 1
                for(int k = 0; k < 4; k++){
                    //if 1, stop loop, increment locked nodes
                    if(matrix[k][i] == 1){
                        lockedNodes++;
                        break; //break k loop
                    }   //end k
                } //end if 1
                break; // break j loop
            }  //end if 2
        } //end j loop
    } // end i loop

    //if cycle, return deadlock
    if(lockedNodes >= 2){
        deadlock = 1;
        printf("Deadlock Detected!\n");
		printf("North Train holds North lock, Requests East lock\n");
		printf("East Train holds East lock, Requests South lock\n");
		printf("South Train holds South lock, Requests West lock\n");
		printf("West Train holds West lock, Requests West lock\n");
    }
    else{
    	printf("No deadlock detected.\n");
    }
    
    return deadlock;
}


void unlinkSems(){
	sem_unlink(SEM_N);
	sem_unlink(SEM_S);
	sem_unlink(SEM_E);
	sem_unlink(SEM_W);
	sem_unlink(SEM_JUNCTION);
	sem_unlink(SEM_WRITE);
}

void validateSem(sem_t *testSem){
	if (testSem == SEM_FAILED) {
		perror("sem_open(3) error");
		exit(EXIT_FAILURE);
	}
	if (sem_close(testSem) < 0) {
		perror("sem_close(3) failed");
        // We ignore possible sem_unlink(3) errors here
		sem_unlink(SEM_N);
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char *argv[]) {

	//unlink all sems to prevent possible corruption in kernel
	unlinkSems();
	
	//deadlock probability
	printf("p\n");
	double p = atof(argv[1]);

	//init sems
	sem_t *semaphore_north = sem_open(SEM_N, O_CREAT | O_EXCL, SEM_PERMS, INITIAL_VALUE);
	sem_t *semaphore_west = sem_open(SEM_W, O_CREAT | O_EXCL, SEM_PERMS, INITIAL_VALUE);
	sem_t *semaphore_south = sem_open(SEM_S, O_CREAT | O_EXCL, SEM_PERMS, INITIAL_VALUE);	
	sem_t *semaphore_east = sem_open(SEM_E, O_CREAT | O_EXCL, SEM_PERMS, INITIAL_VALUE);
	sem_t *junction = sem_open(SEM_JUNCTION, O_CREAT | O_EXCL, SEM_PERMS, INITIAL_VALUE);
	sem_t *writeFile = sem_open(SEM_WRITE, O_CREAT | O_EXCL, SEM_PERMS, INITIAL_VALUE);

	//check for issues with semaphore initialization
	validateSem(semaphore_north);
	validateSem(semaphore_west);
	validateSem(semaphore_east);
	validateSem(semaphore_south);
	validateSem(junction);	
	validateSem(writeFile);

	//get train sequence from file. Maximum 100 trains.
	char fileName[] = "sequence.txt";
	char sequence[100];
	//open file
	FILE *inputFile;
	inputFile = fopen(fileName, "r");
	//ensure file is accessable
	if (inputFile == NULL) {
		printf("Error Opening File: %s, Program terminating on exit code 1.\n", fileName);
		exit(1);
	}
	//Send file content to array with err checking
	if (fscanf(inputFile, "%s", sequence)<0){
		printf("Error Reading File: %s, Program terminating on exit code 1.\n", fileName);
		exit(1);
	}
	//close file
	fclose(inputFile);
	printf("Train generation sequence: %s\n", sequence);
	initMatrix(strlen(sequence));

	matrix = malloc(sizeof(int*)*strlen(sequence));
    for (int i = 0; i < strlen(sequence); i++){
        matrix[i] = malloc(sizeof(int));
    }

    readMatrix(strlen(sequence));
	//Spawn new trains with their respective directions
	pid_t i;
	for (int j = 0; j < strlen(sequence); j++){
		//prep data for exec
		char* args[] = {sequence[j]};
		//fork & exec from train.c executable
		pid_t i = fork();
		char str[10];
		snprintf(str, 10, "%d", j);
		if (i == 0)
		{
			execl("newTrain", args, str, (char *) NULL);
			perror("exec failure");
			exit(1);
		}
		sleep(1-p);
	}
	
	//Wait for trains
	int trainStatus = 0;
	while ((i = wait(&trainStatus)) > 0){
		sleep(1);
		readMatrix(strlen(sequence));
		if(checkDeadlock(strlen(sequence))){
			exit(1);
		}
	}
	
	//unlink semaphores and 
	printf("All trains are finished: Unlinking semaphores\n");
	unlinkSems();
	printf("manager terminating\n");
	return 0;
}
