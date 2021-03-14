#include<stdio.h>
#include<stdlib.h>
#include<unistd.h> /* fork() */
#include<sys/ipc.h> /* shmget(2) semget(2) semctl(2) */
#include<sys/shm.h> /* shmget(2) shmat(2) semget(2) semctl(2)*/
#include<sys/types.h> /* shmat(2) semget(2) semctl(2)*/
#include<sys/wait.h> /* wait() */
#include<signal.h> /* for signal(2) kill(2) */
#include<sys/sem.h> /* semget(2) semctl(2) */

#define NUM_ROLLS 1024
#define NO_SEM	1
#define P(s) semop(s, &Pop, 1);
#define V(s) semop(s, &Vop, 1);

/* Attendance Register */
int A[NUM_ROLLS] = {0};
int shmid,semid;
int *roll;

/* struct sembuf has the following fields
   unsigned short sem_num; semaphore number
   short          sem_op;  semaphore operation
   short          sem_flg; operation flags */
struct sembuf Pop;
struct sembuf Vop;

/* stores "student" process' id */
struct student{
	pid_t pid;
	int status;
}students[NUM_ROLLS];	

/* no. of students (entered by "teacher") */
int n;

/* prints the attendance register */
void printR(){
	printf("\n");
	for(int i=1;i<=n;i++){ 
		printf("Attendance of R.No. %d: ",i);
		if(students[i].status==0) printf("Absent.\n");
		else printf("Present with pid: %d\n",students[i].pid);
	}
}

/* marks the segment to be destroyed and kill the process when Ctrl+C pressed */
void releaseSHM(int signum) {
	int status;
	/* prints Attendance of students */
	printR();	
	shmdt((void *)roll);
	semctl(semid,0,IPC_RMID);
        status = shmctl(shmid, IPC_RMID, NULL); 
        if (status == 0) {
                fprintf(stderr, "Removed shared memory with id = %d.\n", shmid);
        } else if (status == -1) {
                fprintf(stderr, "Cannot remove shared memory with id = %d.\n", shmid);
        } else {
                fprintf(stderr, "shmctl() returned wrong value while removing shared memory with id = %d.\n", shmid);
        }
        status = kill(0, SIGKILL);
        if (status == 0) {
                fprintf(stderr, "kill successful.\n");
        } else if (status == -1) {
                perror("kill failed.\n");
                fprintf(stderr, "Cannot remove shared memory with id = %d.\n", shmid);
        } else {
                fprintf(stderr, "kill(2) returned wrong value.\n");
        }
}

int main(){
	key_t semkey;	
	semkey = ftok("/mnt/c/Users/Vibhash Chandra/Desktop/Study/Operating system Concepts/Operating System Lab Git/Semaphore and Shared Memory/semKey", 1);	
	signal(SIGINT,releaseSHM);
	int status,num=0;
	union semun {
		int              val;    /* Value for SETVAL */
		struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
		unsigned short  *array;  /* Array for GETALL, SETALL */
		struct seminfo  *__buf;  /* Buffer for IPC_INFO (Linux-specific) */
	}setvalArg;

	setvalArg.val = 1;

	Pop.sem_num = 0;
	Pop.sem_op = -1;
	Pop.sem_flg = SEM_UNDO;

	Vop.sem_num = 0;
	Vop.sem_op = 1;
	Vop.sem_flg = SEM_UNDO;
	
	//       int semget(key_t key, int nsems, int semflg);
	semid = semget(semkey, NO_SEM, IPC_CREAT | 0777);
	if(semid == -1) {
		perror("semget() failed");
		exit(1);
	}

	// int semctl(int semid, int semnum, int cmd, ...);
	status = semctl(semid, 0, SETVAL, setvalArg);
	if(status == -1) {
		perror("semctl() failed");
		exit(1);
	}
	
	struct shmid_ds data,*buff;
	buff = &data;
		
	key_t shmkey;	
	shmkey = ftok("/mnt/c/Users/Vibhash Chandra/Desktop/Study/Operating system Concepts/Operating System Lab Git/Semaphore and Shared Memory/shmKey", 1);	
	if((shmid = shmget(shmkey,sizeof(int),IPC_CREAT|0777))==-1){
		perror("Shmget() FAILED!");
		exit(1);
	}
	roll = (int *)shmat(shmid,NULL,0);
	if(roll==(void *)-1){
		perror("Attaching Shared segment to the address space of teacher process FAILED!");
		exit(1);
	}	
	
	printf("Enter number of students: ");
	scanf("%d",&n);
	printf("Teacher Taking Attendance.\n");	
	/* set roll no. to -1 */
	*roll = -1;
	while(1){ 
		/* whenever roll != -1, mark attendance of the student (*roll) and reset roll no. to -1 
		   Also, student performs the wait operation but not the signal operation. Therefore, no other 
		   student can access the shared memory segment till parent performs the signal operation.*/

		if(*roll!=-1){
			shmctl(shmid,IPC_STAT,buff);
			if(*roll<1||*roll>n){ // if roll number sent by "student" process out of range		
				printf("Roll No. Out of Range!\n");												
				kill(buff->shm_lpid,SIGUSR1);
				*roll = -1; /* critical section */
				printf("Semaphore Value (above signal) = %d\n",semctl(semid, 0, GETVAL, setvalArg));
				V(semid);												
				printf("Semaphore Value (below signal) = %d\n",semctl(semid, 0, GETVAL, setvalArg)); 
			}			
			else if(students[*roll].status!=1){	
				students[*roll].status = 1;
				students[*roll].pid = buff->shm_lpid;					
				kill(buff->shm_lpid,SIGINT);			
				printf("Attendance marked of roll number: %d\n",*roll);
				*roll = -1; /* critical section */
				printf("Semaphore Value (above signal) = %d\n",semctl(semid, 0, GETVAL, setvalArg));
				V(semid);												
				printf("Semaphore Value (below signal) = %d\n",semctl(semid, 0, GETVAL, setvalArg)); 
				num+=1;	
			}
			else{				
				printf("Attendance Already Marked!\n");							
				kill(buff->shm_lpid,SIGUSR2); 
				*roll = -1; /* critical section */
				printf("Semaphore Value (above signal) = %d\n",semctl(semid, 0, GETVAL, setvalArg));
				V(semid);												
				printf("Semaphore Value (below signal) = %d\n",semctl(semid, 0, GETVAL, setvalArg));
			}
			if(num==n) {break;}
		}						
	}
	releaseSHM(SIGKILL);
}			
	
