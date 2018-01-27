#include "devices/timer.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include "devices/pit.h"
#include "threads/interrupt.h"
#include "threads/synch.h"
#include "threads/thread.h"

#if TIMER_FREQ < 19
#error 8254 timer requires TIMER_FREQ >= 19
#endif
#if TIMER_FREQ > 1000
#error TIMER_FREQ <= 1000 recommended
#endif


static int64_t ticks;
int size_ready_list =-1;
static unsigned loops_per_tick;
static intr_handler_func timer_interrupt;
static bool too_many_loops (unsigned loops);
static void busy_wait (int64_t loops);
static void real_time_sleep (int64_t num, int32_t denom);
static void real_time_delay (int64_t num, int32_t denom);

void
timer_init (void) 
{
  pit_configure_channel (0, 2, TIMER_FREQ);
  intr_register_ext (0x20, timer_interrupt, "8254 Timer");
}

void
timer_calibrate (void) 
{
  unsigned high_bit, test_bit;

  ASSERT (intr_get_level () == INTR_ON);
  printf ("Calibrating timer...  ");

  loops_per_tick = 1u << 10;
  while (!too_many_loops (loops_per_tick << 1)) 
    {
      loops_per_tick <<= 1;
      ASSERT (loops_per_tick != 0);
    }

  high_bit = loops_per_tick;
  for (test_bit = high_bit >> 1; test_bit != high_bit >> 10; test_bit >>= 1)
    if (!too_many_loops (high_bit | test_bit))
      loops_per_tick |= test_bit;

  printf ("%'"PRIu64" loops/s.\n", (uint64_t) loops_per_tick * TIMER_FREQ);
}

int64_t
timer_ticks (void) 
{
  enum intr_level old_level = intr_disable ();
  int64_t t = ticks;
  intr_set_level (old_level);
  return t;
}

int64_t
timer_elapsed (int64_t then) 
{
  return timer_ticks () - then;
}

void
timer_sleep (int64_t ticks) 
{

     struct thread *cur = thread_current();
      if (ticks > 0) {
	int64_t start = timer_ticks();
        // calculate the thread's time to wake up
	cur->sleep_time = start + ticks;
	ASSERT(intr_get_level() == INTR_ON);
	if (timer_elapsed(start) < ticks) {
                thread_sleep();  //this function is responsible for sleep the threads in sleep_list
	}
   }
}


void
timer_msleep (int64_t ms) 
{
  real_time_sleep (ms, 1000);
}

void
timer_usleep (int64_t us) 
{
  real_time_sleep (us, 1000 * 1000);
}


void
timer_nsleep (int64_t ns) 
{
  real_time_sleep (ns, 1000 * 1000 * 1000);
}

void
timer_mdelay (int64_t ms) 
{
  real_time_delay (ms, 1000);
}

void
timer_udelay (int64_t us) 
{
  real_time_delay (us, 1000 * 1000);
}

void
timer_ndelay (int64_t ns) 
{
  real_time_delay (ns, 1000 * 1000 * 1000);
}


void
timer_print_stats (void) 
{
  printf ("Timer: %"PRId64" ticks\n", timer_ticks ());
}

static void
timer_interrupt (struct intr_frame *args UNUSED)
{
  ticks++;
  thread_tick ();
  struct thread* index_thread;
  if(thread_current()!=idle_thread){
    // increase the time that the thread is taking from the cpu each tick
    thread_current()->recent_cpu = add_mix(thread_current()->recent_cpu,1);
  }
  //update the load_avg and recent_cpu when the system tick counter reaches a multiple of a second
  if(ticks%TIMER_FREQ==0&& thread_mlfqs){
    //recalculate the load_avg of the system
    calculate_load_avg();   
     //update recent_cpu() of the current thread; 
    calculate_recent_cpu(thread_current());
    if(!list_empty(&ready_list)){
      // update the recent_cpu of all the threads in the ready queue.
      size_t size=list_size(&ready_list);
      while(size>0){
      	struct thread* thread=list_entry(list_front(&ready_list),struct thread,elem);
	calculate_recent_cpu(thread);
    	list_push_back (&ready_list, list_pop_front(&ready_list));
    	size--;
      }
    }
    if(!list_empty(&sleep_list)){
      // update the recent_cpu of all the threads in the sleep list.
      size_t size=list_size(&sleep_list);
      while(size>0){
      	struct thread* thread=list_entry(list_front(&sleep_list),struct thread,elem);
	calculate_recent_cpu(thread);
    	list_push_back (&sleep_list, list_pop_front(&sleep_list));
    	size--;
      }
    }
  }
  // update the priority once every fourth clock tick .
  if(ticks % 4 == 0 && thread_mlfqs){
    //update the priority of the running thread and all ready threads .
  calculate_nice(thread_current());
  if(!list_empty(&ready_list)){
      size_t size=list_size(&ready_list);
      while(size>0){
      	struct thread* thread=list_entry(list_front(&ready_list),struct thread,elem);
	calculate_nice(thread);
    	list_push_back (&ready_list, list_pop_front(&ready_list));
    	size--;
 	}
// sort the threads in order according to the priority in the ready list.
   list_sort(&ready_list,compare_priority,NULL);
  }
  if(!list_empty(&sleep_list)){
      //update the priority of all the sleeping threads.
      size_t size=list_size(&sleep_list);
      while(size>0){
      	struct thread* thread=list_entry(list_front(&sleep_list),struct thread,elem);
	calculate_nice(thread);
    	list_push_back (&sleep_list, list_pop_front(&sleep_list));
    	size--;
 	}
   // sort the threads in order according to the priority in the sleep list.
   list_sort(&sleep_list,compare_priority,NULL);
  }
}
  /*after each tick check all the sleeping threads if the sleeping time of any thread ended we will wake up this thread
   and add it to the ready queue*/
  if(!list_empty(&sleep_list)){
  enum intr_level old_level= intr_disable ();
  size_t size=list_size(&sleep_list);
   while(size>0){
  index_thread=list_entry(list_front(&sleep_list),struct thread,elem);
  if(index_thread->sleep_time<=timer_ticks ()){
    index_thread->status=THREAD_READY;
    index_thread->sleep_time=0;
    list_push_back (&ready_list, list_pop_front(&sleep_list)) ;
   }
  else{
    list_push_back (&sleep_list, list_pop_front(&sleep_list)) ;
      }
    size--;
 }
 // sort the threads in order according to the priority in the ready list.
   list_sort(&ready_list,compare_priority,NULL); 
   intr_set_level (old_level);
}
}

static bool
too_many_loops (unsigned loops) 
{
 
  int64_t start = ticks;
  while (ticks == start)
    barrier ();
  start = ticks;
  busy_wait (loops);

  barrier ();
  return start != ticks;
}


static void NO_INLINE
busy_wait (int64_t loops) 
{
  while (loops-- > 0)
    barrier ();
}


static void
real_time_sleep (int64_t num, int32_t denom) 
{
  int64_t ticks = num * TIMER_FREQ / denom;

  ASSERT (intr_get_level () == INTR_ON);
  if (ticks > 0)
    {               
      timer_sleep (ticks); 
    }
  else 
    {
      real_time_delay (num, denom); 
    }
}


static void
real_time_delay (int64_t num, int32_t denom)
{
  ASSERT (denom % 1000 == 0);
  busy_wait (loops_per_tick * num / 1000 * TIMER_FREQ / (denom / 1000)); 
}
