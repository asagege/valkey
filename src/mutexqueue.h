/*
 * Copyright (c) Valkey Contributors
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * A thread-safe queue, protected by a mutex.
 *
 * Supports:
 *   - Adding an item to the end of the queue
 *   - Adding a list of items (fifo) to the end of the queue
 *   - Insertion of a priority item at the beginning of the queue (but after existing priority items)
 *   - Removing an item from the beginning of the queue
 *   - Removing ALL items as (as new fifo) from the queue
 *   - Synchronous waiting on the queue for new items
 *
 * The caller is responsible for memory management for items in the queue.
 */

#ifndef __MUTEXQUEUE_H
#define __MUTEXQUEUE_H

#include <stdbool.h>
#include "fifo.h"

/* The mutexQueue is an opaque structure.  */
typedef struct mutexQueue mutexQueue;

/* Create an empty queue. */
mutexQueue *mutexQueueCreate(void);

/* Release an empty queue. */
void mutexQueueRelease(mutexQueue *theQueue);

/* Number of items in the queue. */
unsigned long mutexQueueLength(mutexQueue *theQueue);

/* Insert a priority item at the beginning of the queue (but after existing priority items). */
void mutexQueueAddPriority(mutexQueue *theQueue, void *value);

/* Insert an item at the end of the queue. */
void mutexQueueAdd(mutexQueue *theQueue, void *value);

/* Insert multiple items (from a fifo) to the end of the queue. */
void mutexQueueAddMultiple(mutexQueue *theQueue, fifo *valueFifo);

/* Retrieves the first item off the queue (or NULL if queue is empty). */
void *mutexQueuePop(mutexQueue *theQueue, bool blocking);

/* Retrieves all items from the queue as a fifo (or NULL if the queue is empty). */
fifo *mutexQueuePopAll(mutexQueue *theQueue, bool blocking);

#endif
