/* This file will contain your solution. Modify it as you wish. */
#include <types.h>
#include <synch.h>  /* for P(), V(), sem_* */
#include <lib.h>    /* for kprintf */
#include "producerconsumer_driver.h"

/* Declare any variables you need here to keep track of and
   synchronise your bounded. A sample declaration of a buffer is shown
   below. You can change this if you choose another implementation. */

static struct pc_data buffer[BUFFER_SIZE]; /* Data item buffer */
int bufStart;   /* Points to the current data item at front of queue */
int bufEnd;     /* Points to the last element in the queue */
struct lock *bufLock; /* Lock to control access to buffer and indexes */
struct semaphore *producer_hold; /* Mutex to control blocking within 
                                    producer_send() */
struct semaphore *consumer_hold; /* Mutex to control blocking within 
                                    consumer_reveive() */
/* consumer_receive() is called by a consumer to request more data. It
   should block on a sync primitive if no data is available in your
   buffer. */

struct pc_data consumer_receive(void)
{
        /* Block if 10 consumers have emptied the buffer without a producer 
         * creating a signal. Effectively the number of items in buf. */
        P(consumer_hold);

        struct pc_data thedata;
 
        /* critical region, acquire lock */
        lock_acquire(bufLock);
        thedata = buffer[bufStart];
        bufStart = (bufStart+1) % BUFFER_SIZE;
        lock_release(bufLock);
        /* critical region, release lock */

        /* Signal to the producer that a consumer has consumed an item */
        V(producer_hold);
        return thedata;
}

/* procucer_send() is called by a producer to store data in your
   bounded buffer. */

void producer_send(struct pc_data item)
{
        /* Block if 10 producers have filled the buffer without a consumer 
         * creating a signal. Effectively the number of free spots in buf. */
        P(producer_hold);

        /* critical region, acquire lock */
        lock_acquire(bufLock);
        bufEnd = (bufEnd + 1) % BUFFER_SIZE;
        buffer[bufEnd] = item;
        lock_release(bufLock);
        /* critical region, release lock */

        /* Signal to the consumer that a producer has produced an item */
        V(consumer_hold);
        return;
}




/* Perform any initialisation (e.g. of global data) you need
   here. Note: You can panic if any allocation fails during setup */

void producerconsumer_startup(void)
{
        bufStart = 0;
        bufEnd = BUFFER_SIZE - 1;

        /* create locks and semaphores and make 
         * sure they allocated correctly */
        
        producer_hold = sem_create("producer_hold", BUFFER_SIZE);
        KASSERT(producer_hold != 0);
        consumer_hold = sem_create("consumer_hold", 0);
        KASSERT(consumer_hold != 0);
        
        bufLock = lock_create("bufLock");
        KASSERT(bufLock != 0);
}

/* Perform any clean-up you need here */
void producerconsumer_shutdown(void)
{
        /* clean up all memory */
        lock_destroy(bufLock);
        sem_destroy(producer_hold);
        sem_destroy(consumer_hold);
}

