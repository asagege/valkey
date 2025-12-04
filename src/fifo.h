/*
 * Copyright (c) Valkey Contributors
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* A space/time efficient FIFO queue of pointers.
 *
 * Implemented with an unrolled single-linked list, the implementation packs multiple pointers into
 * a single block.  This increases space efficiency and cache locality over the Valkey `list` for the
 * purpose of a simple FIFO queue.
 */

#ifndef __FIFO_H_
#define __FIFO_H_

typedef struct fifo fifo;

/* Create a new FIFO queue. */
fifo *fifoCreate(void);

/* Push an item onto the end of the queue. */
void fifoPush(fifo *q, void *ptr);

/* Look at the first item in the queue (without removing it).
 * NOTE: asserts if the queue is empty. */
void *fifoPeek(fifo *q);

/* Return and remove the first item from the queue.
 * NOTE: asserts if the queue is empty. */
void *fifoPop(fifo *q);

/* Return the number of items in the queue. */
long fifoLength(fifo *q);

/* Delete the queue.
 * NOTE: this does not free items which may be referenced by inserted pointers. */
void fifoDelete(fifo *q);

/* Joins the fifo "other" to the end of "q".  "other" becomes empty, but remains valid.
 * This is an O(1) operation. */
void fifoJoin(fifo *q, fifo *other);

/* Returns a new fifo, containing all of the items from "q".  "q" remains valid, but becomes empty.
 * This is an O(1) operation. */
fifo *fifoPopAll(fifo *q);

#endif
