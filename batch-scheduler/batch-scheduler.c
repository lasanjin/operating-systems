/* Tests cetegorical mutual exclusion with different numbers of threads.
 * Automatic checks only catch severe problems like crashes.
 */
#include <stdio.h>
#include "lib/random.h"  //generate random numbers
#include "tests/threads/tests.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"

#define BUS_CAPACITY 3
#define SENDER 0
#define RECEIVER 1
#define NORMAL 0
#define HIGH 1

/*
 *	initialize task with direction and priority
 *	call o
 * */
typedef struct {
  int direction;
  int priority;
} task_t;

/*Task requires to use the bus and executes methods below*/
void oneTask(task_t task);
/* task tries to use slot on the bus */
void getSlot(task_t task);
/* task processes data on the bus either sending or receiving based on the
 * direction*/
void transferData(task_t task);
/* task release the slot */
void leaveSlot(task_t task);

void init_bus(void);
void senderTask(void *);
void receiverTask(void *);
void senderPriorityTask(void *);
void receiverPriorityTask(void *);
void batchScheduler(unsigned int num_tasks_send, unsigned int num_task_receive,
                    unsigned int num_priority_send,
                    unsigned int num_priority_receive);

/* - - - - - - - - - - START OUR CODE - - - - - - - - - - */

void create_task_thread(unsigned num, char *name, void(task_function)(void *));
int no_prio_task(task_t task);
int opposite_direction(task_t task);
int slots(void);
int bus_empty(void);
int bus_full(void);
int prio_send_queue(void);
int prio_recv_queue(void);
int send_queue(void);
int recv_queue(void);
void print(task_t task);

int bus_slots;
int bus_direction;
struct lock lock;
struct condition cond_task_send;
struct condition cond_task_recv;
struct condition cond_task_send_prio;
struct condition cond_task_recv_prio;

/* initializes semaphores */
void init_bus(void) {
  random_init((unsigned int)123456789);

  bus_slots = BUS_CAPACITY;
  bus_direction = SENDER;

  lock_init(&lock);

  cond_init(&cond_task_send);
  cond_init(&cond_task_recv);

  cond_init(&cond_task_send_prio);
  cond_init(&cond_task_recv_prio);
}

/*
 *  Creates a memory bus sub-system  with num_tasks_send + num_priority_send
 *  sending data to the accelerator and num_task_receive +
 *  num_priority_receive tasks reading data/results from the accelerator.
 *
 *  Every task is represented by its own thread.
 *  Task requires and gets slot on bus system (1)
 *  process data and the bus (2)
 *  Leave the bus (3).
 */
void batchScheduler(unsigned int num_tasks_send, unsigned int num_task_receive,
                    unsigned int num_priority_send,
                    unsigned int num_priority_receive) {
  // printf("\nTEST: (%d, %d, %d, %d)\n", num_tasks_send, num_task_receive,
  //        num_priority_send, num_priority_receive);

  create_task_thread(num_tasks_send, "send", senderTask);
  create_task_thread(num_task_receive, "recv", receiverTask);
  create_task_thread(num_priority_send, "send_prio", senderPriorityTask);
  create_task_thread(num_priority_receive, "recv_prio", receiverPriorityTask);
}

void create_task_thread(unsigned num, char *name, void(task_function)(void *)) {
  unsigned i;
  for (i = 0; i < num; i++) {
    thread_create(name, 0, task_function, NULL);
  }
}

/* - - - - - - - - - - END OUR CODE - - - - - - - - - - */

/* Normal task,  sending data to the accelerator */
void senderTask(void *aux UNUSED) {
  task_t task = {SENDER, NORMAL};
  oneTask(task);
}

/* Normal task, reading data from the accelerator */
void receiverTask(void *aux UNUSED) {
  task_t task = {RECEIVER, NORMAL};
  oneTask(task);
}

/* prio task, sending data to the accelerator */
void senderPriorityTask(void *aux UNUSED) {
  task_t task = {SENDER, HIGH};
  oneTask(task);
}

/* prio task, reading data from the accelerator */
void receiverPriorityTask(void *aux UNUSED) {
  task_t task = {RECEIVER, HIGH};
  oneTask(task);
}

/* abstract task execution*/
void oneTask(task_t task) {
  getSlot(task);
  transferData(task);
  leaveSlot(task);
}

/* - - - - - - - - - - START OUR CODE - - - - - - - - - - */

/* task tries to get slot on the bus subsystem */
void getSlot(task_t task) {
  lock_acquire(&lock);

  /*
   *  Add task to appropriate queue to wait for signal:
   *
   *  IF bus full <OR>
   *    {IF slots < 3 <AND>
   *      ([IF not prio task <AND> no prio tasks in queue] <OR> IF wrong direction)}
   */
  while (bus_full() ||
         ((slots() && !bus_empty()) &&
          ((!prio_task(task) && prio_queue()) || opposite_direction(task)))) {

    if (task.priority && task.direction == SENDER) {
      cond_wait(&cond_task_send_prio, &lock);
    }

    else if (task.priority && task.direction == RECEIVER) {
      cond_wait(&cond_task_recv_prio, &lock);
    }

    else if (!task.priority && task.direction == SENDER) {
      cond_wait(&cond_task_send, &lock);
    }

    else {
      cond_wait(&cond_task_recv, &lock);
    }
  }

  bus_slots--;
  bus_direction = task.direction;

  lock_release(&lock);
}

/* task processes data on the bus send/receive */
void transferData(task_t task) {
  /* Sleep for random ticks between 0-10 */
  timer_sleep((int64_t)random_ulong() % 10);
}

/* task releases the slot */
void leaveSlot(task_t task) {
  lock_acquire(&lock);
  bus_slots++;

  /* 
   * Prioritize high prio tasks, then check if any 
   * norm prio task is waiting for bus in same direction
   */

  if (bus_direction == SENDER) {
    /* IF prio send is waiting then signal one prio send*/
    if (prio_send_queue()) {
      cond_signal(&cond_task_send_prio, &lock);
    }

    /* ELIF prio recv is waiting AND bus empty then signal all prio recv */
    else if (prio_recv_queue() && bus_empty()) {
      cond_broadcast(&cond_task_recv_prio, &lock);
    }

    /* 
     * ELIF prio recv is waiting then don't signal, 
     * i.e wait for empty bus and then signal high prio recv
     */
    else if (prio_recv_queue()) {
      goto NO_SIGNAL;
    }

    /* ELIF send is waiting then signal one send */
    else if (send_queue()) {
      cond_signal(&cond_task_send, &lock);
    }
  }

  else if (bus_direction == RECEIVER) {
    /* IF prio recv is waiting then signal one prio recv*/
    if (prio_recv_queue()) {
      cond_signal(&cond_task_recv_prio, &lock);
    }

    /* ELIF prio send is waiting AND bus empty then signal all prio send */
    else if (prio_send_queue() && bus_empty()) {
      cond_broadcast(&cond_task_send_prio, &lock);
    }

    /* 
     * ELIF prio recv is waiting then don't signal, 
     * i.e wait for empty bus and then signal high prio send
     */
    else if (prio_send_queue()) {
      goto NO_SIGNAL;
    }

    /* ELIF recv is waiting then signal one recv */
    else if (recv_queue()) {
      cond_signal(&cond_task_recv, &lock);
    }
  }

NO_SIGNAL:

  lock_release(&lock);
}

int prio_task(task_t task) { return task.priority == HIGH; }

int opposite_direction(task_t task) { return task.direction != bus_direction; }

int bus_empty(void) { return bus_slots == BUS_CAPACITY; }

int bus_full(void) { return bus_slots == 0; }

int slots(void) { return bus_slots > 0 && bus_slots <= BUS_CAPACITY; }

int prio_send_queue(void) { return !list_empty(&cond_task_send_prio.waiters); }

int prio_recv_queue(void) { return !list_empty(&cond_task_recv_prio.waiters); }

int send_queue(void) { return !list_empty(&cond_task_send.waiters); }

int recv_queue(void) { return !list_empty(&cond_task_recv.waiters); }

int prio_queue(void) { return (prio_send_queue() || prio_recv_queue()); }

void print(task_t task) {
  if (task.direction == SENDER && task.priority == HIGH) {
    printf("send prio");
  }

  else if (task.direction == SENDER && task.priority == NORMAL) {
    printf("send norm");
  }

  else if (task.direction == RECEIVER && task.priority == HIGH) {
    printf("recv prio");
  }

  else if (task.direction == RECEIVER && task.priority == NORMAL) {
    printf("recv norm");
  }
  printf("\t direction: %d \t bus: %d\n", bus_direction, bus_slots);
}

/* - - - - - - - - - - END OUR CODE - - - - - - - - - - */
