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

//error msg handler from linux manual
#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

//timed semaphores
sem_t *semaphore_direction;
sem_t *semaphore_right;
sem_t *junction; 

//handler for semaphore errors modified from linux manual
static void
handler(int sig)
{
    write(STDOUT_FILENO, "Call from handler\n", 24);
    if (sem_post(semaphore_direction) == -1) {
        write(STDERR_FILENO, "semaphore_direction failed\n", 18);
        _exit(EXIT_FAILURE);
    }
    if (sem_post(semaphore_right) == -1) {
        write(STDERR_FILENO, "semaphore_right failed\n", 18);
        _exit(EXIT_FAILURE);
    }
    if (sem_post(junction) == -1) {
        write(STDERR_FILENO, "junction failed\n", 18);
        _exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]){

	//time control unit for semaphore timeout
	struct timespec ts;
	if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
	{
		/* handle error */
		return -1;
	}
	ts.tv_sec += 10;
	
	printf("Train going direction %s checking in.\n", argv[0]);

	//init controls for sems dependent on direction
	char right;
	junction = sem_open(SEM_JUNCTION, O_RDWR);
	if (*argv[0] == 'N'){
		semaphore_direction=sem_open(SEM_N, O_RDWR);
		semaphore_right=sem_open(SEM_W, O_RDWR);
		right = 'W';

	}
	else if (*argv[0] == 'E'){
		semaphore_direction=sem_open(SEM_E, O_RDWR);
		semaphore_right=sem_open(SEM_N, O_RDWR);
		right = 'N';
	}
	else if (*argv[0] == 'S'){
		semaphore_direction=sem_open(SEM_S, O_RDWR);
		semaphore_right=sem_open(SEM_E, O_RDWR);
		right = 'E';
	}
	else if (*argv[0] == 'W'){
		semaphore_direction=sem_open(SEM_W, O_RDWR);
		semaphore_right=sem_open(SEM_S, O_RDWR);
		right = 'S';
	}

	//wait for sems with timeout to prevent deadlock
	printf("Train %s bound requests semaphore %s\n", argv[0], argv[0]);
	int s;
	while ((s = sem_timedwait(semaphore_direction, &ts)) == -1 && errno == EINTR)
        continue;       /* Restart if interrupted by handler */
    if (s == -1) {
        if (errno == ETIMEDOUT)
            printf("!!Deadlock in direction request!! Releasing single semaphore %s to break cycle with remaining 3.\n", argv[0]);
        else
            perror("sem_timedwait");
    }

	printf("Train %s bound requests semaphore %c\n", argv[0], right);
    while ((s = sem_timedwait(semaphore_right, &ts)) == -1 && errno == EINTR)
        continue;       /* Restart if interrupted by handler */
    if (s == -1) {
        if (errno == ETIMEDOUT)
            printf("!!Deadlock in right request!! Releasing single semaphore %c to break cycle with remaining 3.\n", right);
        else
            perror("sem_timedwait");
    }

//	sem_timedwait(semaphore_direction,&ts);
	//sem_timedwait(semaphore_right,&ts);
	
	//pass junction
	sem_wait(junction);
	printf("--->Train %s bound passing through junction.--->\n", argv[0]);
	sleep(2);
	printf("--->Train %s bound has left junction and releases it's semaphores.--->\n", argv[0]);

	//post sems
	sem_post(semaphore_direction);
	sem_post(semaphore_right);
	sem_post(junction);

	//terminate train
	return 0;

}