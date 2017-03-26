#include <types.h>
#include <lib.h>
#include <synch.h>
#include <test.h>
#include <thread.h>

#include "paintshop.h"
#include "paintshop_driver.h"


/*
 * **********************************************************************
 * YOU ARE FREE TO CHANGE THIS FILE BELOW THIS POINT AS YOU SEE FIT
 *
 */

/* lock for tints when used to mix */
static struct lock *tint_hold[NCOLOURS];    
/* locks for using the buffer and buffer indicies */
static struct lock *bufLock;
/* sem for blocking customers while paint is mixed */
static struct semaphore *customer_hold[NCUSTOMERS]; 
/* sem for blocking sales people from pulling next order while there isn't any */
static struct semaphore *order_hold;        
/* sem for blocking customers from making orders whilst buffer is full */                                           
static struct semaphore *buffer_hold;
/* buffer for holding at most NCUSTOMERS order */
static struct paintorder* orderBuf[NCUSTOMERS];
/* index for start and then end of the order buffer */
static int bufStart;
static int bufEnd;
void sort_tints(unsigned int *tints);
/*
 * **********************************************************************
 * FUNCTIONS EXECUTED BY CUSTOMER THREADS
 * **********************************************************************
 */

/*
 * order_paint()
 *
 * Takes one argument referring to the order to be filled. The
 * function makes the order available to staff threads and then blocks
 * until the staff have filled the can with the appropriately tinted
 * paint.
 */

void order_paint(struct paintorder *order)
{
        /* reduce counter so that we don't exceed NCUSTOMERS orders */
        P(buffer_hold);

        /* get bufLock to modify buffer and indicies */
        lock_acquire(bufLock);
        bufEnd = (bufEnd + 1) % NCUSTOMERS;
        order->order_owner = bufEnd;
        orderBuf[bufEnd] = order;
        lock_release(bufLock);

        /* give shop staff notification that there is an order ready */
        V(order_hold);

        /* this will block the customer until a staff unblocks him/her */
        P(customer_hold[order->order_owner]);
}



/*
 * **********************************************************************
 * FUNCTIONS EXECUTED BY PAINT SHOP STAFF THREADS
 * **********************************************************************
 */

/*
 * take_order()
 *
 * This function waits for a new order to be submitted by
 * customers. When submitted, it returns a pointer to the order.
 *
 */

struct paintorder *take_order(void)
{
        /* block until an order is ready */
        P(order_hold);

        /* get buflock to modify buffer and indicies */
        lock_acquire(bufLock);
        struct paintorder *ret = orderBuf[bufStart];
        bufStart = (bufStart + 1) % NCUSTOMERS;
        lock_release(bufLock);

        /* let customers know there is one more position to place an order */
        V(buffer_hold);

        return ret;
}


/*
 * sort_tints()
 *
 * Uses insertion sort to sort the tints so that lock_acquire can lock
 * the tint locks in order and not cause deadlocks between threads. This
 * algorithm is suitable as we were told that there will be PAINT_COMPLEXITY
 * in the low tens of numbers, and recursion at this low N is a large overhead.
 */
void sort_tints(unsigned int *tints)
{
   int i, j;
   unsigned int el;
   for (i = 1; i < PAINT_COMPLEXITY; i++)
   {
       j = i-1;
       el = tints[i];
       while (j >= 0 && tints[j] > el)
       {
           tints[j+1] = tints[j];
           j = j-1;
       }
       tints[j+1] = el;
   }
}


/*
 * fill_order()
 *
 * This function takes an order provided by take_order and fills the
 * order using the mix() function to tint the paint.
 *
 * NOTE: IT NEEDS TO ENSURE THAT MIX HAS EXCLUSIVE ACCESS TO THE
 * REQUIRED TINTS (AND, IDEALLY, ONLY THE TINTS) IT NEEDS TO USE TO
 * FILL THE ORDER.
 */

void fill_order(struct paintorder *order)
{

        int i;
        int tint; /* index tints, starting at 0 rather than 1 */
        unsigned int *tints = order->requested_tints;
        sort_tints(tints); /* sort to prevent deadlocking later */
        unsigned char locked_tints[NCOLOURS] = {0}; /* prevent double-locking
                                                       a lock for no reason */

        /* lock tint locks which correspond to ones that this order needs */
        for (i = 0; i < PAINT_COMPLEXITY; i++) {
                tint = tints[i] - 1;
                if (tints[i] && !locked_tints[tint]) {
                        lock_acquire(tint_hold[tint]);
                        locked_tints[tint] = 1;
                }
        }
        mix(order);
        for (i = (PAINT_COMPLEXITY - 1); i >= 0; i--) {
                tint = tints[i] - 1;
                if (tints[i] && locked_tints[tint]) {
                        lock_release(tint_hold[tint]); 
                        locked_tints[tint] = 0;
                }
        }
}


/*
 * serve_order()
 *
 * Takes a filled order and makes it available to (unblocks) the
 * waiting customer.
 */

void serve_order(struct paintorder *order)
{       
        /* notify the blocked customer that they may continue */
        V(customer_hold[order->order_owner]);
}



/*
 * **********************************************************************
 * INITIALISATION AND CLEANUP FUNCTIONS
 * **********************************************************************
 */


/*
 * paintshop_open()
 *
 * Perform any initialisation you need prior to opening the paint shop
 * to staff and customers. Typically, allocation and initialisation of
 * synch primitive and variable.
 */

void paintshop_open(void)
{
        int i;
        bufStart = 0;
        bufEnd = NCUSTOMERS - 1;
        bufLock = lock_create("buf_lock");
        order_hold = sem_create("order_hold_sem", 0); 
        buffer_hold = sem_create("buffer_hold_sem", NCUSTOMERS);
        /* create and name semaphores for each customer */
        for (i = 0; i < NCUSTOMERS; i++){
                char *semName = (char *)kmalloc(21);
                snprintf(semName, 21, "customer_hold_sem_%d", i);
                customer_hold[i] = sem_create(semName, 0);
        }
        /* create and name locks for each tint */
        for (i = 0;  i < NCOLOURS; i++) {
                char *lockName = (char *)kmalloc(8);
                snprintf(lockName, 8, "tint_%d", i+1);
                tint_hold[i] = lock_create(lockName);
        }
}

/*
 * paintshop_close()
 *
 * Perform any cleanup after the paint shop has closed and everybody
 * has gone home.
 */

void paintshop_close(void)
{
        int i;
        /* clean up ALL THE THINGS */
        lock_destroy(bufLock);
        sem_destroy(order_hold);
        sem_destroy(buffer_hold);
        for (i = 0; i < NCOLOURS; i++) {
                lock_destroy(tint_hold[i]);
        }
        for (i = 0; i < NCUSTOMERS; i++) {
                sem_destroy(customer_hold[i]);
        }
}

