/* student's roll number is given through command line arguments */

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h> /* fork() */
#include<sys/ipc.h> /* shmget(2) */
#include<sys/shm.h> /* shmget(2) shmat(2) */
#include<sys/types.h> /* shmat(2) */
#include<sys/wait.h> /* wait() */
#include<signal.h> /* for signal(2) kill(2) */
#include<sys/sem.h> /* semget(2) semctl(2) */

#define NO_SEM	1
#define P(s) semop(s, &Pop, 1);
#define V(s) semop(s, &Vop, 1);

int shmid;
int *roll;

/* struct sembuf has the following fields
   unsigned short sem_num; semaphore number
   short          sem_op;  semaphore operation
   short          sem_flg; operation flags */
struct sembuf Pop;
struct sembuf Vop;

/* if attendance marked, detach & exit
   or if roll number out of range, detach & exit.
   or if attendance already marked, detach & exit. 
   signal received from "teacher" process */
void sigHandler(int signum) {
	int status;
	if(signum==SIGINT) printf("Attendance Marked.\n");
	else if(signum==SIGUSR1) printf("Invalid Roll Number.\n");
	else if(signum==SIGUSR2) printf("Attendance already marked.\n");	
	shmdt((void *)roll);
	exit(0);
}


int main(int argc,char *argv[]){
	
	signal(SIGINT, sigHandler);
	signal(SIGUSR1, sigHandler);	
	signal(SIGUSR2, sigHandler);

	/* creating key */	
	key_t semkey;	
	semkey = ftok("/mnt/c/Users/Vibhash Chandra/Desktop/Study/Operating system Concepts/Operating System Lab Git/Semaphore and Shared Memory/semKey", 1);	
	int status,semid;
	
	union semun {
		int 		val;    /* Value for SETVAL */
		struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
		unsigned short  *array;  /* Array for GETALL, SETALL */
		struct seminfo  *__buf;  /* Buffer for IPC_INFO (Linux-specific) */
	}valArg;
	
	Pop.sem_num = 0;
	Pop.sem_op = -1;
	Pop.sem_flg = 0;

	Vop.sem_num = 0;
	Vop.sem_op = 1;
	Vop.sem_flg = 0;
	
	//       int semget(key_t key, int nsems, int semflg);
	semid = semget(semkey, NO_SEM, 0777);
	if(semid == -1) {
		perror("semget() failed");
		exit(1);
	}
		
	key_t shmkey;	
	shmkey = ftok("/mnt/c/Users/Vibhash Chandra/Desktop/Study/Operating system Concepts/Operating System Lab Git/Semaphore and Shared Memory/shmKey", 1);
	if((shmid = shmget(shmkey,sizeof(int),0777))==-1){
		perror("Shmget() FAILED!");
		exit(1);
	}
	
	roll = (int *)shmat(shmid,NULL,0);
	if(roll==(void *)-1){
		perror("Attaching Shared segment to the address space of child process FAILED!");
		exit(1);
	}		
	
	printf("Semaphore Value (Outside While Loop) = %d\n",semctl(semid, 0, GETVAL, valArg));
	/* student gives its attendance (sets "roll" = argument send via terminal i.e. student's roll number) */
	while(1){ 
		if(*roll==-1){
			printf("Semaphore Value (above wait) = %d\n",semctl(semid, 0, GETVAL, valArg));
			P(semid);								
			printf("Semaphore Value (below wait) = %d\n",semctl(semid, 0, GETVAL, valArg)); 
			*roll = atoi(argv[1]); /* critical section */
		}
	}
}		

