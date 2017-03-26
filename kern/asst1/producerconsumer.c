/* This file will contain your solution. Modify it as you wish. */
#include <types.h>
#include <lib.h>    /* for kprintf */
#include "producerconsumer_driver.h"
#include <synch.h>  /* for P(), V(), sem_* */

/* Declare any variables you need here to keep track of and
   synchronise your bounded. A sample declaration of a buffer is shown
   below. You can change this if you choose another implementation. */

static struct pc_data buffer[BUFFER_SIZE];

static struct semaphore *mutex;
static struct semaphore *empty;
static struct semaphore *full;
// static struct lock *bufStartLock;
struct bufInfo {
        int start;
        int end;
};

static struct bufInfo BufInfo;


/* consumer_receive() is called by a consumer to request more data. It
   should block on a sync primitive if no data is available in your
   buffer. */

struct pc_data consumer_receive(void)
{
        P(full);
        P(mutex);

        struct pc_data thedata;
        thedata = buffer[BufInfo.start];
        // kprintf("removed at %d\n", BufInfo.start);
        BufInfo.start = (BufInfo.start + 1) % BUFFER_SIZE;

        V(mutex);
        V(empty);

        return thedata;
}

/* procucer_send() is called by a producer to store data in your
   bounded buffer. */

void producer_send(struct pc_data item)
{
        P(empty);
        P(mutex);

        buffer[BufInfo.end] = item;
        // kprintf("inserted at %d\n", BufInfo.end);
        BufInfo.end = (BufInfo.end + 1) % BUFFER_SIZE;

        V(mutex);
        V(full);

}




/* Perform any initialisation (e.g. of global data) you need
   here. Note: You can panic if any allocation fails during setup */

void producerconsumer_startup(void)
{
        BufInfo.start = 0;
        BufInfo.end = 0;

        mutex = sem_create("mutex", 1);
        KASSERT(mutex);

        // number of empty slots
        empty = sem_create("empty", BUFFER_SIZE);
        KASSERT(empty);

        // number of full slots
        full = sem_create("full", 0);
        KASSERT(full);

}

/* Perform any clean-up you need here */
void producerconsumer_shutdown(void)
{
        // dun
        sem_destroy(mutex);
        sem_destroy(empty);
        sem_destroy(full);
}

