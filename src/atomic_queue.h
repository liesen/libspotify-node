/**
  Operations on lock-free and thread-safe singly linked LIFO queues.

  Modelled after the Darwin/XNU OSAtomic queue.

  Example:

    typedef struct elem {
      long data1;
      struct elem *link;
      int data2;
    } elem_t;

    elem_t fred, mary, *p;
    nt_atomic_queue q = NT_ATOMIC_QUEUE_INIT;

    nt_atomic_enqueue( &q, &fred, offsetof(elem_t,link) );
    nt_atomic_enqueue( &q, &mary, offsetof(elem_t,link) );

    p = nt_atomic_dequeue( &q, offsetof(elem_t,link) );

  In this example, the call of nt_atomic_dequeue() will return a ptr to mary.



  Copyright (c) 2009 Notion <http://notion.se/>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#ifndef _NT_ATOMIC_QUEUE_H_
#define _NT_ATOMIC_QUEUE_H_

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
  Queue type
*/
typedef volatile struct {
  volatile void *opaque1;
  volatile size_t opaque2;
#if defined(__GNUC__) && ( defined(__x86_64__) || defined(__LP64__) )
} nt_atomic_queue __attribute__((aligned(16)));
#else
} nt_atomic_queue;
#endif

/**
  Initialize a queue.
*/
#define NT_ATOMIC_QUEUE_INIT (nt_atomic_queue) { NULL, 0L }

inline void nt_atomic_queue_init(nt_atomic_queue *queue) {
  queue->opaque1 = NULL;
  queue->opaque2 = 0L;
}

// workhorse prototypes
#ifdef __LP64__

void nt_atomic_enqueue64_mp(nt_atomic_queue *queue, void *elem, size_t offset);
void *nt_atomic_dequeue64_mp(nt_atomic_queue *queue, size_t offset);
void *nt_atomic_dequeue_ifnexteq64_mp(nt_atomic_queue *queue, size_t offset, void *cmpptr);

void nt_atomic_enqueue64_up(nt_atomic_queue *queue, void *elem, size_t offset);
void *nt_atomic_dequeue64_up(nt_atomic_queue *queue, size_t offset);
void *nt_atomic_dequeue_ifnexteq64_up(nt_atomic_queue *queue, size_t offset, void *cmpptr);

#else

void nt_atomic_enqueue32_mp(nt_atomic_queue *queue, void *elem, size_t offset);
void *nt_atomic_dequeue32_mp(nt_atomic_queue *queue, size_t offset);
void *nt_atomic_dequeue_ifnexteq32_mp(nt_atomic_queue *queue, size_t offset, void *cmpptr);

void nt_atomic_enqueue32_up(nt_atomic_queue *queue, void *elem, size_t offset);
void *nt_atomic_dequeue32_up(nt_atomic_queue *queue, size_t offset);
void *nt_atomic_dequeue_ifnexteq32_up(nt_atomic_queue *queue, size_t offset, void *cmpptr);

void *nt_atomic_dequeue32_on64_mp(nt_atomic_queue *queue, size_t offset);
void *nt_atomic_dequeue_ifnexteq32_on64_mp(nt_atomic_queue *queue, size_t offset, void *cmpptr);

void *nt_atomic_dequeue32_on64_up(nt_atomic_queue *queue, size_t offset);
void *nt_atomic_dequeue_ifnexteq32_on64_up(nt_atomic_queue *queue, size_t offset, void *cmpptr);

#endif

/**
  Enqueue (push) an element in @queue.

  @param queue  pointer to a nt_atomic_queue.
  @param elem   the element to enqueue.
  @param offset offset in bytes to the link pointer of @elem. Usually
                offsetof(my_struct, link_member).
*/
inline void nt_atomic_enqueue(nt_atomic_queue *queue, void *elem, size_t offset) {
#ifdef __LP64__
  nt_atomic_enqueue64_mp(queue, elem, offset);
#else
  nt_atomic_enqueue32_mp(queue, elem, offset);
#endif
}

/**
  Dequeue (pop) the first (top/next) element from a @queue.

  @param queue  pointer to a nt_atomic_queue
  @param offset offset in bytes to the link pointer. Usually
                offsetof(my_struct, link_member).
  @returns pointer to a dequeued element or NULL if the queue is empty.
*/
inline void *nt_atomic_dequeue(nt_atomic_queue *queue, size_t offset) {
#ifdef __LP64__
  nt_atomic_dequeue64_mp(queue, offset);
#else
  nt_atomic_dequeue32_mp(queue, offset);
#endif
}

/**
  CAS-version of nt_atomic_dequeue.

  Returns the first (next) element if it has the same address as @cmpptr,
  otherwise the second element, if any, is returned.

  @param queue  pointer to a nt_atomic_queue
  @param offset offset in bytes to the link pointer. Usually
                offsetof(my_struct, link_member).
  @param cmpptr compare next element to this address; if they are the same, the
                next element is returned. Otherwise the second element, if any,
                is returned.
  @returns pointer to a dequeued element or NULL if the queue is empty.
*/
inline void *nt_atomic_dequeue_ifnexteq(nt_atomic_queue *queue, size_t offset, void *cmpptr) {
#ifdef __LP64__
  nt_atomic_dequeue_ifnexteq64_mp(queue, offset, cmpptr);
#else
  nt_atomic_dequeue_ifnexteq32_mp(queue, offset, cmpptr);
#endif
}


#ifdef __cplusplus
}
#endif

#endif // _NT_ATOMIC_QUEUE_H_
