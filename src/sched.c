#include "queue.h"
#include "sched.h"
#include <pthread.h>

#include <stdlib.h>
#include <stdio.h>
static struct queue_t ready_queue;
static struct queue_t run_queue;
static pthread_mutex_t queue_lock;

static struct queue_t running_list;
#ifdef MLQ_SCHED
static struct queue_t mlq_ready_queue[MAX_PRIO];
static int slot[MAX_PRIO];
#endif

int queue_empty(void) {
#ifdef MLQ_SCHED
	unsigned long prio;
	for (prio = 0; prio < MAX_PRIO; prio++)
		if(!empty(&mlq_ready_queue[prio])) 
			return -1;
#endif
	return (empty(&ready_queue) && empty(&run_queue));
}

void init_scheduler(void) {
#ifdef MLQ_SCHED
    int i ;

	for (i = 0; i < MAX_PRIO; i ++) {
		mlq_ready_queue[i].size = 0;
		slot[i] = MAX_PRIO - i; 
      
	}
#endif
	ready_queue.size = 0;
	run_queue.size = 0;
	pthread_mutex_init(&queue_lock, NULL);
}


#ifdef MLQ_SCHED
/* 
 *  Stateful design for routine calling
 *  based on the priority and our MLQ policy
 *  We implement stateful here using transition technique
 *  State representation   prio = 0 .. MAX_PRIO, curr_slot = 0..(MAX_PRIO - prio)
 */
struct pcb_t * get_mlq_proc(void) {
	struct pcb_t * proc = NULL;
	/*TODO: get a process from PRIORITY [ready_queue].
	 * Remember to use lock to protect the queue.
	 * */
	unsigned long curr_prio = 0;
        while (curr_prio < MAX_PRIO) {
            // Check if the queue for the current priority is not empty
            if (!empty(&mlq_ready_queue[curr_prio])) {
                pthread_mutex_lock(&queue_lock);
                // Dequeue the process from the ready queue of the current priority
                proc = dequeue(&mlq_ready_queue[curr_prio]);
                if (proc != NULL) {
                    // Reset slot when empty
                    if (empty(&mlq_ready_queue[curr_prio])) {
                        mlq_ready_queue[curr_prio].slot = slot[curr_prio];
                    }
                }
                pthread_mutex_unlock(&queue_lock);
                if (proc != NULL) break;
            }
            curr_prio++;  // Move to the next lower priority if the current one is empty
        }
        return proc;	
}

void put_mlq_proc(struct pcb_t * proc) {
	pthread_mutex_lock(&queue_lock);
	enqueue(&mlq_ready_queue[proc->prio], proc);
	pthread_mutex_unlock(&queue_lock);
}

void add_mlq_proc(struct pcb_t * proc) {
	pthread_mutex_lock(&queue_lock);
	enqueue(&mlq_ready_queue[proc->prio], proc);
	pthread_mutex_unlock(&queue_lock);	
}

struct pcb_t * get_proc(void) {
	return get_mlq_proc();
}

void put_proc(struct pcb_t * proc) {
	proc->ready_queue = &ready_queue;
	proc->mlq_ready_queue = mlq_ready_queue;
	proc->running_list = & running_list;

	/* TODO: put running proc to running_list */
    	pthread_mutex_lock(&queue_lock);
	if (running_list.size < MAX_QUEUE_SIZE) {  // Assuming MAX_QUEUE_SIZE is defined
		enqueue(&running_list, proc);
	}
	pthread_mutex_unlock(&queue_lock);
	return put_mlq_proc(proc);
}

void add_proc(struct pcb_t * proc) {
	proc->ready_queue = &ready_queue;
	proc->mlq_ready_queue = mlq_ready_queue;
	proc->running_list = &running_list;

	/* TODO: put running proc to running_list */
    	pthread_mutex_lock(&queue_lock);
	if (running_list.size < MAX_QUEUE_SIZE) {  // Assuming MAX_QUEUE_SIZE is defined
		enqueue(&running_list, proc);
	}
	pthread_mutex_unlock(&queue_lock);
	return add_mlq_proc(proc);
}
#else
struct pcb_t * get_proc(void) {
	struct pcb_t * proc = NULL;
	/*TODO: get a process from [ready_queue].
	 * Remember to use lock to protect the queue.
	 * */
	if (empty(&ready_queue)) return NULL;
	pthread_mutex_lock(&queue_lock);
        if (!empty(&ready_queue)) {
            proc = dequeue(&ready_queue);
        }
        pthread_mutex_unlock(&queue_lock);
	return proc;
}

void put_proc(struct pcb_t * proc) {
	proc->ready_queue = &ready_queue;
	proc->running_list = & running_list;

	/* TODO: put running proc to running_list */

	pthread_mutex_lock(&queue_lock);
	if (running_list.size < MAX_QUEUE_SIZE) {  // Assuming MAX_QUEUE_SIZE is defined
		enqueue(&running_list, proc);
	}
	pthread_mutex_unlock(&queue_lock);

	pthread_mutex_lock(&queue_lock);
	enqueue(&run_queue, proc);
	pthread_mutex_unlock(&queue_lock);
}

void add_proc(struct pcb_t * proc) {
	proc->ready_queue = &ready_queue;
	proc->running_list = & running_list;

	/* TODO: put running proc to running_list */

	pthread_mutex_lock(&queue_lock);
	if (running_list.size < MAX_QUEUE_SIZE) {  // Assuming MAX_QUEUE_SIZE is defined
		enqueue(&running_list, proc);
	}
	pthread_mutex_unlock(&queue_lock);

	pthread_mutex_lock(&queue_lock);
	enqueue(&ready_queue, proc);
	pthread_mutex_unlock(&queue_lock);	
}
#endif


