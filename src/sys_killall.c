/*
 * Copyright (C) 2025 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Sierra release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

#include "common.h"
#include "syscall.h"
#include "stdio.h"
#include "libmem.h"
#include "queue.h"

int __sys_killall(struct pcb_t *caller, struct sc_regs* regs)
{
    char proc_name[100];
    uint32_t data;

    //hardcode for demo only
    uint32_t memrg = regs->a1;

    /* TODO: Get name of the target proc */
    //proc_name = libread..
    int i = 0;
    data = 0;
    while(data != -1){
        libread(caller, memrg, i, &data);
        proc_name[i]= data;
        if(data == -1) proc_name[i]='\0';
        i++;
    }
    printf("The procname retrieved from memregionid %d is \"%s\"\n", memrg, proc_name);

    // /* TODO: Traverse proclist to terminate the proc
    //  *       stcmp to check the process match proc_name
    //  */
    // //caller->running_list
    // //caller->mlq_ready_queu
    // struct pcb_t **prev = &(caller->running_list);
    // struct pcb_t *cur = caller->running_list;

    // while (cur != NULL) {
    //     if (strcmp(cur->path, proc_name) == 0) {
    //         *prev = cur->running_list;  
    //         free(cur);  
    //         cur = *prev;  
    //     } 
        
    //     else {
    //         prev = &(cur->running_list);
    //         cur = cur->running_list;
    //     }
    // }

    // /* TODO Maching and terminating 
    //  *       all processes with given
    //  *        name in var proc_name
    //  */
    // for (int prio = 0; prio < MAX_PRIO; prio++) {
    //     struct queue_t *queue = &(caller->mlq_ready_queue[prio]); 

    //     int new_size = 0; 
    //     for (int j = 0; j < queue->size; j++) {
    //         if (strcmp(queue->proc[j]->path, proc_name) == 0) {
    //             free(queue->proc[j]); 
    //         } 
            
    //         else {
    //             queue->proc[new_size++] = queue->proc[j]; 
    //         }
    //     }
    //     queue->size = new_size; 
    // }


    struct pcb_t * victim = dequeue(&caller->running_list);
    while (victim != NULL){
        char victim_name[100];
        uint32_t byte_data = 0;
        int idx = 0;
        while(byte_data != -1){
            libread(victim, memrg, idx, &byte_data);
            victim_name[idx]= byte_data;
            if(byte_data == -1) victim_name[idx]='\0';
            idx++;
        }
        printf("=====VICTIM NAME: %s \n", victim_name);
        printf("=====VICTIM NAME: %s ======\n", victim_name);

        if(strcmp(proc_name, victim_name) == 0){
            printf("Kill process with pid: %d \n", victim->pid);  //print to debug 
            libfree(victim, memrg);             //free the allocate memory
            free(victim);                       //free the all structure of victim process
        }

        victim = dequeue(&caller->running_list);  // tiếp tục vòng lặp
    }

#ifdef MLQ_SCHED
for(int prio = 0; prio < MAX_PRIO; prio++){                 //traverse mlq_queues
    struct pcb_t * mlq_victim = dequeue(&caller->mlq_ready_queue[prio]);
    while (mlq_victim != NULL){
        char victim_name[100];
        uint32_t byte_data = 0;
        int idx = 0;
        while(byte_data != -1){
            libread(mlq_victim, memrg, idx, &byte_data);        //read the mem context (like how we do with proc_name)
            victim_name[idx]= byte_data;
            if(byte_data == -1) victim_name[idx]='\0';
            idx++;
        }
        printf("=====VICTIM NAME: %s ======\n", victim_name);

        if(strcmp(proc_name, victim_name) == 0){
            printf("Kill process with pid: %d \n", mlq_victim->pid);  //print to debug 
            libfree(mlq_victim, memrg);             //free the allocate memory
            free(mlq_victim);                       //free the all structure of victim process
        }

        mlq_victim = dequeue(&caller->mlq_ready_queue[prio]);  //next
    }
}

#else
//chưa hoàn thiện tới
#endif
    
    
    return 0; 
}
