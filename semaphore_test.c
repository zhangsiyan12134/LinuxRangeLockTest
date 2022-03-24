
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>

sem_t lock;

void* thread(void* arg)
{
    pthread_t thId = pthread_self();
    //wait
    sem_wait(&lock);
    printf("\nThread %ld Entered..\n", thId);
  
    //critical section
    sleep(5);
      
    //signal
    printf("\nThread %ld Just Exiting...\n", thId);
    sem_post(&lock);
}
  

int main(int argc, char **argv)
{

    sem_init(&lock, 0, 3);      //init the semaphore to 4 (i.e. only 4 concurrent reading is possible)
    pthread_t t1, t2, t3, t4;
    pthread_create(&t1,NULL,thread,NULL);
    sleep(1);
    pthread_create(&t2,NULL,thread,NULL);
    sleep(1);
    pthread_create(&t3,NULL,thread,NULL);
    sleep(1);
    pthread_create(&t4,NULL,thread,NULL);
    pthread_join(t1,NULL);
    pthread_join(t2,NULL);
    pthread_join(t3,NULL);
    pthread_join(t4,NULL);
    sem_destroy(&lock);
    return 0;
}