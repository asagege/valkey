/*
 * Copyright (c) Valkey Contributors
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* mutexQueue - A thread-safe wrapper around FIFO */

#include "mutexqueue.h"
#include "serverassert.h"
#include "zmalloc.h"
#include <pthread.h>


struct mutexQueue {
    fifo *priority_fifo;      /* Ordered list of priority insertions */
    fifo *normal_fifo;        /* Ordered list of normal insertions */
    pthread_mutex_t mutex;    /* Mutex to lock shared access */
    pthread_cond_t notify_cv; /* Condition variable to notify waiting threads */
};


/* Create an empty queue.  */
mutexQueue *mutexQueueCreate(void) {
    mutexQueue *mq;
    mq = zmalloc(sizeof(*mq));

    pthread_mutex_init(&mq->mutex, NULL);
    pthread_cond_init(&mq->notify_cv, NULL);
    mq->priority_fifo = fifoCreate();
    mq->normal_fifo = fifoCreate();
    return mq;
}


/* Release an empty queue.
 * Note:  The queue must be empty before calling release.  The quickest way to empty the queue is to
 *        call mutexQueuePopAll - which returns the items in a new fifo.  It is the caller's
 *        responsibility to free memory (as necessary) for any items.
 * Note:  Behavior is undefined if other threads are accessing the queue.  */
void mutexQueueRelease(mutexQueue *theQueue) {
    assert(mutexQueueLength(theQueue) == 0);
    mutexQueue *mq = theQueue;

    pthread_mutex_destroy(&mq->mutex);
    pthread_cond_broadcast(&mq->notify_cv);
    pthread_cond_destroy(&mq->notify_cv);

    fifoDelete(mq->priority_fifo);
    fifoDelete(mq->normal_fifo);

    zfree(mq);
}


/* Internal routine assumes mutex already locked.  */
static inline unsigned long mutexQueueLengthInternal(mutexQueue *mq) {
    return fifoLength(mq->priority_fifo) + fifoLength(mq->normal_fifo);
}


/* Number of items in the queue.  */
unsigned long mutexQueueLength(mutexQueue *theQueue) {
    mutexQueue *mq = theQueue;

    pthread_mutex_lock(&mq->mutex);

    unsigned long len = mutexQueueLengthInternal(mq);

    pthread_mutex_unlock(&mq->mutex);
    return len;
}


/* Insert a priority item at the beginning of the queue (but after existing priority items).  */
void mutexQueueAddPriority(mutexQueue *theQueue, void *value) {
    mutexQueue *mq = theQueue;

    pthread_mutex_lock(&mq->mutex);

    bool mustSignal = (mutexQueueLengthInternal(theQueue) == 0);
    fifoPush(mq->priority_fifo, value);
    if (mustSignal) pthread_cond_broadcast(&mq->notify_cv);

    pthread_mutex_unlock(&mq->mutex);
}


/* Insert an item at the end of the queue.  */
void mutexQueueAdd(mutexQueue *theQueue, void *value) {
    mutexQueue *mq = theQueue;

    pthread_mutex_lock(&mq->mutex);

    bool mustSignal = (mutexQueueLengthInternal(theQueue) == 0);
    fifoPush(mq->normal_fifo, value);
    if (mustSignal) pthread_cond_broadcast(&mq->notify_cv);

    pthread_mutex_unlock(&mq->mutex);
}


/* Insert a fifo of items at the end of the queue.  This removes the items from the source fifo!  */
void mutexQueueAddMultiple(mutexQueue *theQueue, fifo *valueFifo) {
    mutexQueue *mq = theQueue;

    if (fifoLength(valueFifo) == 0) return;

    pthread_mutex_lock(&mq->mutex);

    bool mustSignal = (mutexQueueLengthInternal(theQueue) == 0);
    fifoJoin(mq->normal_fifo, valueFifo);
    if (mustSignal) pthread_cond_broadcast(&mq->notify_cv);

    pthread_mutex_unlock(&mq->mutex);
}


/* Retrieves the first item off the queue (or NULL if queue is empty).
 * If 'blocking' is true, this method will block until an item is available.  */
void *mutexQueuePop(mutexQueue *theQueue, bool blocking) {
    mutexQueue *mq = theQueue;
    void *value = NULL;

    pthread_mutex_lock(&mq->mutex);

    if (blocking) {
        while (mutexQueueLengthInternal(mq) == 0) {
            pthread_cond_wait(&mq->notify_cv, &mq->mutex);
        }
    }

    if (fifoLength(mq->priority_fifo) > 0) {
        value = fifoPop(mq->priority_fifo);
    } else if (fifoLength(mq->normal_fifo) > 0) {
        value = fifoPop(mq->normal_fifo);
    }

    pthread_mutex_unlock(&mq->mutex);
    return value;
}


/* Retrieves all items from the queue as a fifo (or NULL if the queue is empty).
 * If 'blocking' is true, this method will block until an item is available.  */
fifo *mutexQueuePopAll(mutexQueue *theQueue, bool blocking) {
    mutexQueue *mq = theQueue;
    fifo *result = NULL;

    pthread_mutex_lock(&mq->mutex);

    if (blocking) {
        while (mutexQueueLengthInternal(mq) == 0) {
            pthread_cond_wait(&mq->notify_cv, &mq->mutex);
        }
    }

    if (mutexQueueLengthInternal(mq) > 0) {
        result = fifoCreate();
        fifoJoin(result, mq->priority_fifo);
        fifoJoin(result, mq->normal_fifo);
    }

    pthread_mutex_unlock(&mq->mutex);
    return result;
}
