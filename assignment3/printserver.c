#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h> 
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>

#define SIZE 5
int NUMB_THREADS;
int NUMB_PRODUCER;
//functionalties still needed: ctrlc graceful termination, LIFO,
typedef struct{
    int bytes;
    int parentid;
    time_t timeAdded;
}globalbuffer;

globalbuffer *gloalbufferarray;
int buffersize = sizeof(globalbuffer) *30;
int buffer_index;
int fifo_index =-1;
 
pthread_mutex_t buffer_mutex;
bool iswritten = true;

pid_t parentpid;
pthread_mutex_t * mutexarray;
int mutexsize = sizeof(pthread_mutex_t)*1;

int* boolsharedmem;
int* totaljobmem;
int* totalbytemem;
int* processidmem;
int* threadidmem;
int* consumertotalreadmem;

double totalwaittime;
    int   shmid,shmid2,shmid3,shmid4,shmid5,shmid6,shmid7,shmid8,shmid9,shmid10;
    /* a pointer to the shared memory segment */
    int *intsharedmem;
 
 typedef struct mycountingsem{
    int val;
    sem_t gate;
    sem_t mutex;
}countingsem;


void mycountingsem_init(struct mycountingsem *sem,int k){
   sem->val = k;
    if(k>=1) sem_init(&sem->gate,1,1);
    else{
        sem_init(&sem->gate,1,0);
    }
    sem_init(&sem->mutex,1,1);
}

void mycountingsem_wait(struct mycountingsem *sem){
    int semval;
    sem_getvalue(&sem->gate,&semval);
    sem_wait(&sem->gate);
    sem_wait(&sem->mutex);
    (sem->val)--;
    if(sem->val>0){
        sem_post(&sem->gate);
    }
    sem_post(&sem->mutex);
}

void mycountingsem_post(struct mycountingsem *sem){
    sem_wait(&sem->mutex);
    (sem->val)++;
    if(sem->val==1){
    int semval;
    sem_getvalue(&sem->gate,&semval);
        sem_post(&sem->gate);
    }
    sem_post(&sem->mutex);
}

countingsem* countingsemarray;
 
void insertbuffer(globalbuffer value, int* sharedmemint) {
    if (*sharedmemint < 30) {
        //printf("inserting\n");
        gloalbufferarray[*sharedmemint] = value;
        *sharedmemint = (*sharedmemint+1)%30;
    } else {
        printf("Buffer overflow%d\n",*sharedmemint);
    }
}
 
globalbuffer dequeuebuffer() {
    if (buffersize >= 0) {// change this check for something not sure yet
        fifo_index = (fifo_index+1)%30;
        //printf("fifo_index = %d\n",fifo_index);
        return gloalbufferarray[fifo_index];
    } else {
        printf("Buffer underflow2\n");
        return;
    }
   // return 0;
}

 int p =1;
void *consumer(void *thread_n) {
    int thread_numb = *(int *)thread_n;
    globalbuffer value;
    threadidmem[p]=pthread_self();
    //printf("%d %d\n",p, threadidmem[p]);
    p++;
    int i=0;
    int lastjob=0;
    sleep(8);
     while(true){

         if(*boolsharedmem ==0 && mycountingsem_getval(&countingsemarray[1])==0){
             //printf("broke..\n");
             break;
         }

        mycountingsem_wait(&countingsemarray[1]); //empty sem

        pthread_mutex_lock(&mutexarray[0]);
        value = dequeuebuffer(value);
        consumertotalreadmem[0] = consumertotalreadmem[0]+1;
        consumertotalreadmem[1] = consumertotalreadmem[1]+value.bytes;
       totalwaittime += time(NULL)-value.timeAdded;
        if(value.parentid==lastjob){
            sleep((double)rand() / (double)RAND_MAX);
            printf("same parent. sleeping...\n");
        }
        lastjob=value.parentid;
        pthread_mutex_unlock(&mutexarray[0]);

        mycountingsem_post(&countingsemarray[0]); //full sem
        printf("Consumer %d dequeue %d, %d from buffer\n", thread_numb, value.parentid, value.bytes);
   }
//    for(int i =0; i<NUMB_THREADS;i++)
//     pthread_cancel(thread[i]);
    printf("thread exited\n");
    pthread_mutex_destroy(&mutexarray[0]);
    sem_destroy(&countingsemarray[0]);
    sem_destroy(&countingsemarray[1]);
    pthread_exit(0);
}
 
 void signal_handler(int no){
        if(no==SIGINT){
        if(getpid()!=parentpid){
            //printf("non parent exit\n");
            exit(0);
        }
        
    if(getpid()== parentpid){
        //printf("parentpid %d\n",parentpid);

    pthread_mutex_destroy(&mutexarray[0]);
    sem_destroy(&countingsemarray[0]);
    sem_destroy(&countingsemarray[1]);
        for(int i=1; i<=threadidmem[0];i++){
           // printf("hi %d\n",threadidmem[i]);
            //pthread_cancel(pthread_self());
            pthread_kill(&threadidmem[i], SIGKILL);
            printf("canceled thread %d\n",threadidmem[i]);
         }

         for(int k=1;k<=processidmem[0];k++){
            printf("killed process %d\n",processidmem[k]);
            kill(processidmem[k],SIGKILL);
         }
           // *consumertotalreadmem
         //=0;
        //printf("inside handler\n");
        /** now detach the shared memory segment */
                if (shmdt(gloalbufferarray) == -1) {
                    fprintf(stderr, "Unable to detach\n");
                }
            /** now remove the shared memory segment */
                shmctl(shmid, IPC_RMID, NULL);

                if (shmdt(countingsemarray) == -1) {
                    fprintf(stderr, "Unable to detach\n");
                }

                shmctl(shmid2, IPC_RMID, NULL);

                if (shmdt(mutexarray) == -1) {
                    fprintf(stderr, "Unable to detach\n");
                }

                shmctl(shmid3, IPC_RMID, NULL);

                if (shmdt(intsharedmem) == -1) {
                    fprintf(stderr, "Unable to detach\n");
                }

                shmctl(shmid4, IPC_RMID, NULL);

                if (shmdt(boolsharedmem) == -1) {
                    fprintf(stderr, "Unable to detach\n");
                }

                shmctl(shmid5, IPC_RMID, NULL);

                if (shmdt(totaljobmem) == -1) {
                    fprintf(stderr, "Unable to detach\n");
                }

                shmctl(shmid6, IPC_RMID, NULL);
                //printf("parent exited.\n");

                if (shmdt(totalbytemem) == -1) {
                    fprintf(stderr, "Unable to detach\n");
                }

                shmctl(shmid7, IPC_RMID, NULL);
                if (shmdt(processidmem) == -1) {
                    fprintf(stderr, "Unable to detach\n");
                }

                shmctl(shmid8, IPC_RMID, NULL);

                if (shmdt(threadidmem) == -1) {
                    fprintf(stderr, "Unable to detach\n");
                }

                shmctl(shmid9, IPC_RMID, NULL);

                if (shmdt(consumertotalreadmem
            ) == -1) {
                    fprintf(stderr, "Unable to detach\n");
                }
            /** now remove the shared memory segment */
                shmctl(shmid10, IPC_RMID, NULL);

                //detached=0;

                
                printf("finished detatching\n");
                exit(0); 
        }
    }   
}
int main(int argc, char *argv[]) {
    // printf("Enter the number of producer processes.\n");
    // scanf("%d",&NUMB_PRODUCER);

    // printf("Enter the number of consumer threads.\n");
    // scanf("%d",&NUMB_THREADS);
    NUMB_PRODUCER = atoi(argv[1]);
    NUMB_THREADS = atoi(argv[2]);
    buffer_index = 0;
    signal(SIGINT,signal_handler);

    /* the size (in bytes) of the shared memory segment */
    const int       segment_size = buffersize;

    if( (shmid = shmget(IPC_PRIVATE, segment_size, IPC_CREAT | 0666)) < 0 )
    {
        perror("shmget");
        exit(1);
    }

    /** attach the shared memory segment */

        if( (gloalbufferarray= shmat(shmid, NULL, 0)) == (globalbuffer *) -1 )
    {
        perror("shmat");
        exit(1);
    }
    
    	//printf("shared memory segment %d attached at address %p\n", shmid, gloalbufferarray);

    //mycountingsem_init(&full_sem0,30);
    //mycountingsem_init(&empty_sem0,0);
    int countingsemsize = sizeof(countingsem)*2;

       if( (shmid2 = shmget(IPC_PRIVATE, countingsemsize, IPC_CREAT | 0666)) < 0 )
    {
        perror("shmget");
        exit(1);
    }

    /** attach the shared memory segment */

        if( (countingsemarray= shmat(shmid2, NULL, 0)) == (countingsem *) -1 )
    {
        perror("shmat");
        exit(1);
    }
        mycountingsem_init(&countingsemarray[0],30);
        mycountingsem_init(&countingsemarray[1],0);
    
    	//printf("shared memory segment %d attached at address %p\n", shmid2, countingsemarray);
        //countingsemarray[0] = full_sem0;
        //countingsemarray[1] = empty_sem0;

    if( (shmid3 = shmget(IPC_PRIVATE, mutexsize, IPC_CREAT | 0666)) < 0 )
    {
        perror("shmget");
        exit(1);
    }

    /** attach the shared memory segment */

        if( (mutexarray= shmat(shmid3, NULL, 0)) == (pthread_mutex_t *) -1 )
    {
        perror("shmat");
        exit(1);
    }

    //mutexarray[0]=buffer_mutex;
    pthread_mutex_init(&mutexarray[0], NULL);

    if( (shmid4 = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0 )
    {
        perror("shmget");
        exit(1);
    }

    /** attach the shared memory segment */

        if( (intsharedmem= shmat(shmid4, NULL, 0)) == (int *) -1 )
    {
        perror("shmat");
        exit(1);
    }
    *intsharedmem =0;

        if( (shmid5 = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0 )
    {
        perror("shmget");
        exit(1);
    }

    /** attach the shared memory segment */

        if( (boolsharedmem= shmat(shmid5, NULL, 0)) == (int *) -1 )
    {
        perror("shmat");
        exit(1);
    }
    *boolsharedmem =1;

    if( (shmid6 = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0 )
    {
        perror("shmget");
        exit(1);
    }

    /** attach the shared memory segment */

        if( (totaljobmem= shmat(shmid6, NULL, 0)) == (int *) -1 )
    {
        perror("shmat");
        exit(1);
    }
    *totaljobmem =0;

        if( (shmid7 = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0 )
    {
        perror("shmget");
        exit(1);
    }

    /** attach the shared memory segment */

        if( (totalbytemem= shmat(shmid7, NULL, 0)) == (int *) -1 )
    {
        perror("shmat");
        exit(1);
    }
    *totalbytemem =0;

    if( (shmid8 = shmget(IPC_PRIVATE, sizeof(int) *NUMB_PRODUCER+1, IPC_CREAT | 0666)) < 0 )
    {
        perror("shmget");
        exit(1);
    }

    /** attach the shared memory segment */

    if( (processidmem= shmat(shmid8, NULL, 0)) == (int *) -1 )
    {
        perror("shmat");
        exit(1);
    }
    processidmem[0]=NUMB_PRODUCER;


            if( (shmid9 = shmget(IPC_PRIVATE, sizeof(int)*NUMB_THREADS+1, IPC_CREAT | 0666)) < 0 )
    {
        perror("shmget");
        exit(1);
    }

    /** attach the shared memory segment */

        if( (threadidmem= shmat(shmid9, NULL, 0)) == (int *) -1 )
    {
        perror("shmat");
        exit(1);
    }
    threadidmem[0]=NUMB_THREADS;
    //*totalbytemem =0;

    if( (shmid10 = shmget(IPC_PRIVATE, sizeof(int)*2, IPC_CREAT | 0666)) < 0 )
    {
        perror("shmget");
        exit(1);
    }

    /** attach the shared memory segment */

        if( (consumertotalreadmem = shmat(shmid10, NULL, 0)) == (int *) -1 )
    {
        perror("shmat");
        exit(1);
    }
    consumertotalreadmem[0]=0;//total jobs read
    consumertotalreadmem[1]=0; //total bytes read

    pthread_t thread[NUMB_THREADS];
    int thread_numb[NUMB_THREADS];
    int i;
    //printf("adding consumers\n");
    for (i = 0; i < NUMB_THREADS; ) {
        thread_numb[i] = i;
        pthread_create(thread + i, // pthread_t *t
                    NULL, // const pthread_attr_t *attr
                    consumer, // void *(*start_routine) (void *)
                    thread_numb + i);  // void *arg
        //threadidmem[i+1]=thread+i;
        i++;
    }

    parentpid = getpid();
    int z =0;
    time_t begin = time(NULL);
    for(int i=0; i<NUMB_PRODUCER;i++){
        int jobs = rand() % 30+1;
        srand(time(NULL) ^ (getpid()<<16));
        sleep(1);
        if(fork()==0){
           // if(i!=0){
            processidmem[i+1]=getpid();
            //printf("numb producer %d %d\n",i+1,getpid());
            //}
            int j=0;
             while (j++ < jobs) {
                time_t timeStart = time(NULL);
                sleep(rand() % 8);
                int valueinbytes = rand()% (1000-100+1)+100;
                globalbuffer job = {.bytes = valueinbytes, .parentid = getpid(),.timeAdded=timeStart};
                mycountingsem_wait(&countingsemarray[0]); // full sem

                pthread_mutex_lock(&mutexarray[0]);
                insertbuffer(job,intsharedmem);
                (*totaljobmem)++;
                (*totalbytemem) = (*totalbytemem) + valueinbytes;
                pthread_mutex_unlock(&mutexarray[0]);

                mycountingsem_post(&countingsemarray[1]); //empty sem
                printf("Producer %d added %d to buffer\n", getpid(), valueinbytes);
            }
            //printf("child finished.\n");
            exit(0);
        }
    }
    if(getpid()==parentpid){// check to ensure only main parent runs
            //printf("waiting on waits\n");

            for(int i=0;i<NUMB_PRODUCER;i++){
            wait(NULL);
            
            }
            printf("Producers submitted %d total jobs totaling %d bytes\n",*totaljobmem,*totalbytemem);
            *boolsharedmem =0;


            sem_destroy(&countingsemarray[0]);
            sem_destroy(&countingsemarray[1]);
            sem_destroy(&mutexarray[0]);
            printf("Consumers processed %d jobs totaling %d bytes\n",consumertotalreadmem[0],consumertotalreadmem[1]);
            printf("Average wait time = %f\n",totalwaittime / *totaljobmem);
            
            /** now detach the shared memory segment */
                if (shmdt(gloalbufferarray) == -1) {
                    fprintf(stderr, "Unable to detach\n");
                }
            /** now remove the shared memory segment */
                shmctl(shmid, IPC_RMID, NULL);
                //return 0;
            //}
                /** now detach the shared memory segment */
                if (shmdt(countingsemarray) == -1) {
                    fprintf(stderr, "Unable to detach\n");
                }
            /** now remove the shared memory segment */
                shmctl(shmid2, IPC_RMID, NULL);

                        /** now detach the shared memory segment */
                if (shmdt(mutexarray) == -1) {
                    fprintf(stderr, "Unable to detach\n");
                }
            /** now remove the shared memory segment */
                shmctl(shmid3, IPC_RMID, NULL);

                if (shmdt(intsharedmem) == -1) {
                    fprintf(stderr, "Unable to detach\n");
                }
            /** now remove the shared memory segment */
                shmctl(shmid4, IPC_RMID, NULL);

                if (shmdt(boolsharedmem) == -1) {
                    fprintf(stderr, "Unable to detach\n");
                }
            /** now remove the shared memory segment */
                shmctl(shmid5, IPC_RMID, NULL);

                if (shmdt(totaljobmem) == -1) {
                    fprintf(stderr, "Unable to detach\n");
                }
            /** now remove the shared memory segment */
                shmctl(shmid6, IPC_RMID, NULL);
                //printf("parent exited.\n");

                if (shmdt(totalbytemem) == -1) {
                    fprintf(stderr, "Unable to detach\n");
                }
            /** now remove the shared memory segment */
                shmctl(shmid7, IPC_RMID, NULL);

                if (shmdt(processidmem) == -1) {
                    fprintf(stderr, "Unable to detach\n");
                }
            /** now remove the shared memory segment */
                shmctl(shmid8, IPC_RMID, NULL);

                if (shmdt(threadidmem) == -1) {
                    fprintf(stderr, "Unable to detach\n");
                }
            /** now remove the shared memory segment */
                shmctl(shmid9, IPC_RMID, NULL);
        
                if (shmdt(consumertotalreadmem) == -1) {
                    fprintf(stderr, "Unable to detach\n");
                }
            /** now remove the shared memory segment */
                shmctl(shmid10, IPC_RMID, NULL);

            time_t end = time(NULL);
            printf("Total time taken by CPU: %d seconds\n", (end-begin));
            return 0;
                    exit(0);
    }
}
int mycountingsem_getval(struct mycountingsem *p){
    //printf("%d\n",p->val);
    return p->val;
}