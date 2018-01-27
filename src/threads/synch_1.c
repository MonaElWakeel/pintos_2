/* This file is derived from source code for the Nachos
   instructional operating system.  The Nachos copyright notice
   is reproduced in full below. */

/* Copyright (c) 1992-1996 The Regents of the University of California.
   All rights reserved.

   Permission to use, copy, modify, and distribute this software
   and its documentation for any purpose, without fee, and
   without written agreement is hereby granted, provided that the
   above copyright notice and the following two paragraphs appear
   in all copies of this software.

   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.
*/

#include "threads/synch.h"
#include <stdio.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/* Initializes semaphore SEMA to VALUE.  A semaphore is a
   nonnegative integer along with two atomic operators for
   manipulating it:

   - down or "P": wait for the value to become positive, then
     decrement it.

   - up or "V": increment the value (and wake up one waiting
     thread, if any). */
void
sema_init (struct semaphore *sema, unsigned value) 
{
  ASSERT (sema != NULL);
  sema->value = value;
  list_init (&sema->waiters);
}

/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. */
  
 void
 sema_down (struct semaphore *sema) {
  enum intr_level old_level;
  ASSERT (sema != NULL);
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  if (sema->value == 0) 
    {
      // add current thread in waiters list in order according to thread's priority.
      list_insert_ordered (&sema->waiters, &thread_current ()->elem,compare_priority,NULL);
      // change the status of thread to blocked.
      thread_block ();
    }
  sema->value--;
  intr_set_level (old_level);

  }


/* Down or "P" operation on a semaphore, but only if the
   semaphore is not already 0.  Returns true if the semaphore is
   decremented, false otherwise.

   This function may be called from an interrupt handler. */
bool
sema_try_down (struct semaphore *sema) 
{
  enum intr_level old_level;
  bool success;

  ASSERT (sema != NULL);

  old_level = intr_disable ();
  if (sema->value > 0) 
    {
      sema->value--;
      success = true; 
    }
  else
    success = false;
  intr_set_level (old_level);

  return success;
}

/* Up or "V" operation on a semaphore.  Increments SEMA's value
   and wakes up one thread of those waiting for SEMA, if any.

   This function may be called from an interrupt handler. */
void
sema_up (struct semaphore *sema) 
{
  enum intr_level old_level;
  
  ASSERT (sema != NULL);

  old_level = intr_disable ();
  if (!list_empty (&sema->waiters)) {
    list_sort(&sema->waiters,compare_priority,NULL);
    thread_unblock (list_entry (list_pop_front (&sema->waiters),
                               struct thread, elem)); 
    // change the thread's status to ready and add it to ready list in order according to priority      
   
    thread_yield(); // to add currnt thread to ready list and then call the scheduler.
    }
  sema->value++;
  intr_set_level (old_level);
}

static void sema_test_helper (void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printf() to see
   what's going on. */
void
sema_self_test (void) 
{
  struct semaphore sema[2];
  int i;

  printf ("Testing semaphores...");
  sema_init (&sema[0], 0);
  sema_init (&sema[1], 0);
  thread_create ("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
  for (i = 0; i < 10; i++) 
    {
      sema_up (&sema[0]);
      sema_down (&sema[1]);
    }
  printf ("done.\n");
}

/* Thread function used by sema_self_test(). */
static void
sema_test_helper (void *sema_) 
{
  struct semaphore *sema = sema_;
  int i;

  for (i = 0; i < 10; i++) 
    {
      sema_down (&sema[0]);
      sema_up (&sema[1]);
    }
}

/* Initializes LOCK.  A lock can be held by at most a single
   thread at any given time.  Our locks are not "recursive", that
   is, it is an error for the thread currently holding a lock to
   try to acquire that lock.

   A lock is a specialization of a semaphore with an initial
   value of 1.  The difference between a lock and such a
   semaphore is twofold.  First, a semaphore can have a value
   greater than 1, but a lock can only be owned by a single
   thread at a time.  Second, a semaphore does not have an owner,
   meaning that one thread can "down" the semaphore and then
   another one "up" it, but with a lock the same thread must both
   acquire and release it.  When these restrictions prove
   onerous, it's a good sign that a semaphore should be used,
   instead of a lock. */
void
lock_init (struct lock *lock)
{
  ASSERT (lock != NULL);
  lock->holder = NULL;
  sema_init (&lock->semaphore, 1);
}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
lock_acquire (struct lock *lock)
{
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (!lock_held_by_current_thread (lock));
  // check if the lock is acquired in the first time or not 
  if(lock->holder == NULL){   
   /* this lock should be added to the locks list that contains all the locks held by the current thread as this lock not be acquired before 
   */
   list_push_back(&thread_current()->locks,&lock->elem);
  }
  else{
   /* if the lock was acquired before so the wait_lock that the thread is waiting for is this lock.
      so the thread will be blocked untill the lock is avilable and update the priority if*/
        thread_current()->wait_lock=lock;
  	if(thread_current()->priority > lock->holder->priority){
                update_priority(thread_current());
 	 }
  }
  sema_down (&lock->semaphore);
/* when the thread returns from sema_down it aquires the current lock and the wait_lock returns to NULL as it isn't blocked on any lock currently*/
  thread_current()->wait_lock=NULL;
  lock->holder = thread_current ();

}

// this function is responsible for calculating the priority when happen donation.
void update_priority(struct thread  *t){
// the base case is when the thread is not blocked on any lock(it isn't waiting any lock to be available)  
   if(t->wait_lock==NULL){
 	return;
   }
/* checks weither the priority of the lock holder is less than that of the blocked thread if it is the blocked thread donates its priority to the lock holder*/
   if(t->wait_lock->holder->priority < t->priority){
	t->wait_lock->holder->priority = t->priority;
	update_priority(t->wait_lock->holder);
   }
}

/* Tries to acquires LOCK and returns true if successful or false
   on failure.  The lock must not already be held by the current
   thread.

   This function will not sleep, so it may be called within an
   interrupt handler. */
bool
lock_try_acquire (struct lock *lock)
{
  bool success;

  ASSERT (lock != NULL);
  ASSERT (!lock_held_by_current_thread (lock));

  success = sema_try_down (&lock->semaphore);
  if (success)
    lock->holder = thread_current ();
  return success;
}

/* Releases LOCK, which must be owned by the current thread.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */
void
lock_release (struct lock *lock) 
{
  ASSERT (lock != NULL);
  ASSERT (lock_held_by_current_thread (lock));
  size_t size=list_size(&thread_current()->locks);
  //removes the current lock from the lock list of the holder
  while(size>0){
    struct lock* lock_1 = list_entry(list_pop_front(&thread_current()->locks),struct lock,elem);
    if(lock_1==lock){
     break;
    }else{
     list_push_back (&thread_current()->locks, &lock_1->elem);
    }
    size--;
    }
  // checkes if the thread doesn't hold any lock change its priority to initial priority.
    if(list_empty(&thread_current()->locks)){
        thread_current()->priority=thread_current()->initial_priority; 
    }
    else{
  /*when the thread releases a lock and is still holding other locks its priority becomes the highest priority of the waiting theads on the locks that the thread holds */
	thread_current()->priority=thread_current()->initial_priority;
	size_t size=list_size(&thread_current()->locks);
	while(size>0){
      		struct lock* lock_1=list_entry(list_front(&thread_current()->locks),struct lock,elem);
		if(!list_empty(&lock_1->semaphore.waiters)){
  		if(list_entry(list_front(&lock_1->semaphore.waiters),struct thread,elem)->priority > thread_current()->priority){
			thread_current()->priority=list_entry(list_front(&lock_1->semaphore.waiters),struct thread,elem)->priority;
                        
    		}
		}
    			list_push_back (&thread_current()->locks, list_pop_front(&thread_current()->locks));
    	size--;
 	}
    }
  lock->holder = NULL;
  sema_up (&lock->semaphore);
}

/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be racy.) */
bool
lock_held_by_current_thread (const struct lock *lock) 
{
  ASSERT (lock != NULL);

  return lock->holder == thread_current ();
}


/* One semaphore in a list. */
struct semaphore_elem 
  {
    struct list_elem elem;              /* List element. */
    struct semaphore semaphore;         /* This semaphore. */
    int priority;
  };
/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it. */
void
cond_init (struct condition *cond)
{
  ASSERT (cond != NULL);

  list_init (&cond->waiters);
}

/* Atomically releases LOCK and waits for COND to be signaled by
   some other piece of code.  After COND is signaled, LOCK is
   reacquired before returning.  LOCK must be held before calling
   this function.

   The monitor implemented by this function is "Mesa" style, not
   "Hoare" style, that is, sending and receiving a signal are not
   an atomic operation.  Thus, typically the caller must recheck
   the condition after the wait completes and, if necessary, wait
   again.

   A given condition variable is associated with only a single
   lock, but one lock may be associated with any number of
   condition variables.  That is, there is a one-to-many mapping
   from locks to condition variables.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
cond_wait (struct condition *cond, struct lock *lock) 
{
  struct semaphore_elem waiter;

  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));
  
  sema_init (&waiter.semaphore, 0);
  waiter.priority=thread_current()->priority;
  list_insert_ordered(&cond->waiters, &waiter.elem,compare_priority_sema,NULL);
  // add semaphore in waiters list according to the thread's priority which waiting on this semaphore.
  lock_release (lock);
  sema_down (&waiter.semaphore);
  
  lock_acquire (lock);
}

/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_signal (struct condition *cond, struct lock *lock UNUSED) 
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));

  if (!list_empty (&cond->waiters)){ 
    sema_up (&list_entry (list_pop_front (&cond->waiters),
                                struct semaphore_elem, elem)->semaphore);
  }
}
/*this function is responsible for comapre the priority of thread which waiting on semaphore.*/
bool compare_priority_sema(const struct list_elem *e1, const struct list_elem *e2)
{
  struct semaphore_elem* first = list_entry (e1, struct semaphore_elem, elem);
  struct semaphore_elem* second = list_entry (e2, struct semaphore_elem, elem);
  if(first->priority > second->priority){
	return true;
  }
        return false;
  
}

/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_broadcast (struct condition *cond, struct lock *lock) 
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);

  while (!list_empty (&cond->waiters))
    cond_signal (cond, lock);
}
