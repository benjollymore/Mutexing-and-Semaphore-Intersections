/*
Assignment3.c
Run Trains and update RAG 
Written by 
Benjamin Jollymore 
Johnathon Power
Latest version as of 2018-11-24 253
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
#include <errno.h>
#include <string.h>

//semaphore name definitions
#define SEM_N "/semaphore_north"
#define SEM_W "/semaphore_west"
#define SEM_S "/semaphore_south"
#define SEM_E "/semaphore_east"
#define SEM_JUNCTION "/semaphore_junction"
#define SEM_WRITE "/semaphore_write"

//used for matrix access
#define N 0
#define E 1
#define S 2
#define W 3

//error msg handler from linux manual
#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

//semaphores
sem_t *semaphore_direction;
sem_t *semaphore_right;
sem_t *junction; 
sem_t *writeFile; 

int **matrix;
int trainQuantity = 0;

void getNumOfTrains(){

	FILE *fp;  
    char c;  // To store a character read from file 
 
    // Open the file 
    fp = fopen("matrix.txt", "r"); 
  
    // Check if file exists 
    if (fp == NULL) 
    { 
        printf("Could not open file\n"); 
        exit(0);
    } 
  
    // Extract characters from file and store in character c 
    for (c = getc(fp); c != EOF; c = getc(fp)) 
        if (c == '\n') // Increment count if this character is newline 
           trainQuantity += 1; 
  
    // Close the file 
    fclose(fp); 
}

void validateSem(sem_t *testSem){
	if (testSem == SEM_FAILED) {
		perror("sem_open(3) error");
		exit(EXIT_FAILURE);
	}
}

void readMatrix()
{	
    FILE *inputFile;
    inputFile = fopen ("matrix.txt", "r");
    if (inputFile == NULL){
    	printf("Could not open file!\n");
        exit(0);
    }
    for(int i = 0; i < trainQuantity; i++)
    {
            fscanf(inputFile, "%d %d %d %d", &matrix[i][0], &matrix[i][1], &matrix[i][2], &matrix[i][3]);
    }
    fclose (inputFile); 
}

void writeMatrix(){
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
	for (int i = 0; i < trainQuantity; i++){
		
			if (fprintf(inputFile, "%d %d %d %d", matrix[i][0], matrix[i][1], matrix[i][2], matrix[i][3])<0){
				printf("Error write to File: %s, Program terminating on exit code 1.\n", fileName);
				exit(1);
			}
		
		if (fprintf(inputFile, "%s", "\n")<0){
			printf("Error write to  File: %s, Program terminating on exit code 1.\n", fileName);
			exit(1);

		}
	}
	//close file
	fclose(inputFile);
}

void editMatrix(int lock, int train, int value){
	sem_wait(writeFile); 
	readMatrix();
	matrix[lock][train] = value;
	writeMatrix();
	sem_post(writeFile);
}

int main(int argc, char *argv[]){
	//init file io sem
	writeFile = sem_open(SEM_WRITE, O_RDWR);
	validateSem(writeFile);

	int trainID = atoi(argv[1]) + 1;
	printf("Train number %d going direction %s checking in.\n",trainID, argv[0]);

	//find num of trains
	sem_wait(writeFile);
	getNumOfTrains();
	sem_post(writeFile);

	//init matrix
    matrix = malloc(sizeof(int*)*trainQuantity);
    for (int i = 0; i < trainQuantity; i++){
        matrix[i] = malloc(sizeof(int));
    }

	sem_wait(writeFile);
	readMatrix();
	sem_post(writeFile);
	
	//init controls for sems dependent on direction
	int direction;
	int right;
	char printRight;
	junction = sem_open(SEM_JUNCTION, O_RDWR);
	validateSem(junction);
	
	if (*argv[0] == 'N'){
		semaphore_direction=sem_open(SEM_N, O_RDWR);
		semaphore_right=sem_open(SEM_W, O_RDWR);
		direction = N;
		right = W;
		printRight = 'W';
	}
	else if (*argv[0] == 'E'){
		semaphore_direction=sem_open(SEM_E, O_RDWR);
		semaphore_right=sem_open(SEM_N, O_RDWR);
		direction = E;
		right = N;
		printRight = 'N';
	}
	else if (*argv[0] == 'S'){
		semaphore_direction=sem_open(SEM_S, O_RDWR);
		semaphore_right=sem_open(SEM_E, O_RDWR);
		direction = S;
		right = E;
		printRight = 'E';
	}
	else if (*argv[0] == 'W'){
		semaphore_direction=sem_open(SEM_W, O_RDWR);
		semaphore_right=sem_open(SEM_S, O_RDWR);
		direction = W;
		right = S;
		printRight = 'S';
	}
	validateSem(semaphore_direction);
	validateSem(semaphore_right);

	//wait for sems with timeout to prevent deadlock
	printf("Train %s bound requests semaphore %s\n", argv[0], argv[0]);

	//update matrix for request
	editMatrix(direction, trainID-1, 1);

	sem_wait(semaphore_direction);

	//update matrix for aquisition
	editMatrix(direction, trainID-1, 2);
	printf("Train %s bound aquires semaphore %s\n", argv[0], argv[0]);
	
	
	printf("Train %s bound requests semaphore %c\n", argv[0], printRight);

	//update matrix for request
	editMatrix(right, trainID-1, 1);

	sem_wait(semaphore_right);

	//update matrix for aquisition
	editMatrix(right, trainID-1, 2);
	
	//pass junction
	sem_wait(junction);
	printf("--->Train %s bound passing through junction.--->\n", argv[0]);
	sleep(2);
	printf("--->Train %s bound has left junction and releases it's semaphores.--->\n", argv[0]);

	//post sems
	sem_post(semaphore_direction);
	editMatrix(direction, trainID-1, 0);

	sem_post(semaphore_right);
	editMatrix(right, trainID-1, 0);
	
	sem_post(junction);

	//terminate train
	return 0;

}
