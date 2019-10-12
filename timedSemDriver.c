/*
Assignment3.c
Trains go through junctions and stuff
Written by 
Benjamin Jollymore A00400128
Johnathon Power A00405363
Latest version as of 2018-11-23
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
#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)
#define INITIAL_VALUE 1

void unlinkSems(){
	sem_unlink(SEM_N);
	sem_unlink(SEM_S);
	sem_unlink(SEM_E);
	sem_unlink(SEM_W);
	sem_unlink(SEM_JUNCTION);
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

	//init sems
	sem_t *semaphore_north = sem_open(SEM_N, O_CREAT | O_EXCL, SEM_PERMS, INITIAL_VALUE);
	sem_t *semaphore_west = sem_open(SEM_W, O_CREAT | O_EXCL, SEM_PERMS, INITIAL_VALUE);
	sem_t *semaphore_south = sem_open(SEM_S, O_CREAT | O_EXCL, SEM_PERMS, INITIAL_VALUE);	
	sem_t *semaphore_east = sem_open(SEM_E, O_CREAT | O_EXCL, SEM_PERMS, INITIAL_VALUE);
	sem_t *junction = sem_open(SEM_JUNCTION, O_CREAT | O_EXCL, SEM_PERMS, INITIAL_VALUE);

	//check for issues with semaphore initialization
	validateSem(semaphore_north);
	validateSem(semaphore_west);
	validateSem(semaphore_east);
	validateSem(semaphore_south);
	validateSem(junction);	

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

	//Spawn new trains with their respective directions
	pid_t i;
	for (int i = 0; i < strlen(sequence); i++){
		//prep data for exec
		char* args[] = {sequence[i]};
		//fork & exec from train.c executable
		pid_t i = fork();
		if (i == 0)
		{
			execl("newTrain", args, (char *)NULL);
			perror("exec failure");
   			exit(1);
		}
	}
	
	//ludicrously long sleep kinda lets it work
	//Wait for trains
	int trainStatus = 0;
	while ((i = wait(&trainStatus)) > 0);
	
	//unlink semaphores and 
	printf("All trains are finished: Unlinking semaphores\n");
	unlinkSems();
	printf("manager terminating\n");
	return 0;
}
