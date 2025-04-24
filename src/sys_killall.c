/*
 * Copyright (C) 2025 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Sierra release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 * 
 * Pham Tran Duc Hanh && Nguyen Chi Thien
 */

#include "common.h"
#include "syscall.h"
#include "stdio.h"
#include "libmem.h"
#include "queue.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

struct queue_t tmp_run_queue;
struct queue_t tmp_ready_queue;
struct queue_t tmp_mlq_ready_queue[MAX_PRIO];

pthread_mutex_t queue_lock;

int has_allocated_memory(struct pcb_t *proc) {
    if (proc == NULL || proc->mm == NULL)
        return 0;

    struct vm_area_struct *area = proc->mm->mmap;

    while (area) {
        if (area->vm_end > area->vm_start)
            return 1;
        area = area->vm_next;
    }

    return 0;
}


int __sys_killall(struct pcb_t *caller, struct sc_regs* regs)
{
    pthread_mutex_init(&queue_lock, NULL);

    char proc_name[100];
    uint32_t data;
    printf("~~~~~~~~~~~~~~~~~Syscall killall~~~~~~~~~~~~~~~~~\n");
    //hardcode for demo only
    uint32_t memrg = regs->a1;      //cái memrg này hiện tại là bao nhiêu cũng được vì khi mảng các region mới (toàn 0) thì thành ra nó lại luôn truy cập được vào đầu
    /* TODO: Get name of the target proc */
    //proc_name = libread..

    if(!has_allocated_memory(caller)){
        fprintf(stderr, "[Sycall fail] No allocated from caller.\n");
        return -1;
    }
    if(!(caller->mm->symrgtbl[memrg].rg_start - caller->mm->symrgtbl[memrg].rg_end)){
        fprintf(stderr, "[Sycall fail] Null region.\n");
        return -1;
    }
    int i = 0;
    data = 0;
    while(data != -1){
        libread(caller, memrg, i, &data);
        proc_name[i]= data;
        if(data == -1) proc_name[i]='\0';
        i++;
        if (i >= 100) {                 //catch lỗi không có ký tự kết thúc trong region.
            fprintf(stderr, "[Syscall fail] Could not find terminating byte (-1) in region name.\n");
            return -1;
        }
    }
    printf("[Caller PID: %d]The procname retrieved from memregionid %d is \"%s\"\n", caller->pid, memrg, proc_name);
    // /* TODO: Traverse proclist to terminate the proc
    //  *       stcmp to check the process match proc_name
    //  */
    // //caller->running_list
    // //caller->mlq_ready_queu

    pthread_mutex_lock(&queue_lock);
    struct pcb_t * victim = dequeue(&caller->running_list);
    pthread_mutex_unlock(&queue_lock);
    while (victim != NULL){
        int err = 0;
        char victim_name[100];
        uint32_t byte_data = 0;
        int idx = 0;
        while(byte_data != -1){
            if(!has_allocated_memory(victim)){
                fprintf(stderr, "[Victim PID: %d] No allocated.\n", victim->pid);
                err = 1;
                break;
            }
            if(!(victim->mm->symrgtbl[memrg].rg_end - victim->mm->symrgtbl[memrg].rg_start)){
                fprintf(stderr, "[Victim PID: %d] Null region.\n", victim->pid);
                err = 1;
                break;
            }
            libread(victim, memrg, idx, &byte_data);
            victim_name[idx]= byte_data;
            if(byte_data == -1) victim_name[idx]='\0';
            idx++;
            if (idx >= 100) {                 //catch lỗi không có ký tự kết thúc trong region.
                fprintf(stderr, "[Victim PID: %d] Could not find terminating byte (-1) in region name.\n", victim->pid);
                err  = 1;
                break;
            }
        }

        if (!err) printf("[Victim PID: %d]The procname retrieved from memregionid %d is \"%s\"\n", victim->pid, memrg, victim_name);
        if (strcmp(proc_name, victim_name) == 0 && !err) {
            printf("Killing process with pid: %d \n", victim->pid);
        
            libfree(victim, memrg);   //free the allocate memory
            free(victim);             //free the all structure of victim process
        
        } 
        else{
            pthread_mutex_lock(&queue_lock);
            enqueue(&tmp_run_queue, victim);
            pthread_mutex_unlock(&queue_lock);
        }
        pthread_mutex_lock(&queue_lock);   
        victim = dequeue(&caller->running_list);  // tiếp tục vòng lặp
        pthread_mutex_unlock(&queue_lock);
    }

    pthread_mutex_lock(&queue_lock);
    while(!empty(&tmp_run_queue)){
        struct pcb_t * put_back1 = dequeue(&tmp_run_queue);
        enqueue(&caller->running_list, put_back1);
    }
    pthread_mutex_unlock(&queue_lock);

#ifdef MLQ_SCHED
for(int prio = 0; prio < MAX_PRIO; prio++){                 //traverse mlq_queues
    pthread_mutex_lock(&queue_lock);
    struct pcb_t * mlq_victim = dequeue(&caller->mlq_ready_queue[prio]);
    pthread_mutex_unlock(&queue_lock);
    while (mlq_victim != NULL){
        int err = 0;
        char victim_name[100];
        uint32_t byte_data = 0;
        int idx = 0;
        while(byte_data != -1){
            if(!has_allocated_memory(mlq_victim)){
                fprintf(stderr, "[Victim PID: %d] No allocated.\n", mlq_victim->pid);
                err = 1;
                break;
            }
            if(!(mlq_victim->mm->symrgtbl[memrg].rg_end - mlq_victim->mm->symrgtbl[memrg].rg_start)){
                fprintf(stderr, "[Victim PID: %d] Null region.\n", mlq_victim->pid);
                err = 1;
                break;
            }
            libread(mlq_victim, memrg, idx, &byte_data);        //read the mem context (like how we do with proc_name)
            victim_name[idx]= byte_data;
            if(byte_data == -1) victim_name[idx]='\0';
            idx++;
            if (idx >= 100) {                 //catch lỗi không có ký tự kết thúc trong region.
                fprintf(stderr, "[Victim PID: %d] Could not find terminating byte (-1) in region name.\n", mlq_victim->pid);
                err = 1;
                break;
            }
        }

        if (!err) printf("[Victim PID: %d]The procname retrieved from memregionid %d is \"%s\"\n", mlq_victim->pid, memrg, victim_name);

        if(strcmp(proc_name, victim_name) == 0 && !err){
            printf("Killing process with pid: %d \n", mlq_victim->pid);  //print to debug 
            libfree(mlq_victim, memrg);             //free the allocate memory
            free(mlq_victim);                       //free the all structure of victim process
        }
        else{
            pthread_mutex_lock(&queue_lock);
            enqueue(&tmp_mlq_ready_queue[prio], mlq_victim);
            pthread_mutex_unlock(&queue_lock);
        }
        pthread_mutex_lock(&queue_lock);
        mlq_victim = dequeue(&caller->mlq_ready_queue[prio]);  //next
        pthread_mutex_unlock(&queue_lock);
    }
}
pthread_mutex_lock(&queue_lock);
for(int prio = 0; prio < MAX_PRIO; prio++){
    while(!empty(&tmp_mlq_ready_queue[prio])){
        struct pcb_t * put_back2 = dequeue(&tmp_mlq_ready_queue[prio]);
        enqueue(&caller->mlq_ready_queue[prio], put_back2);
    }
}
pthread_mutex_unlock(&queue_lock);
#else
//Tương tự như trường hợp running queue thôi
    struct pcb_t * ready_victim = dequeue(&caller->ready_queue);
    while (ready_victim != NULL){
        char victim_name[100];
        uint32_t byte_data = 0;
        int idx = 0;
        while(byte_data != -1){
            if(!has_allocated_memory(ready_victim)){
                fprintf(stderr, "[Victim] No allocated.\n");
                break;
            }
            libread(ready_victim, memrg, idx, &byte_data);
            victim_name[idx]= byte_data;
            if(byte_data == -1) victim_name[idx]='\0';
            idx++;
            if (idx >= 100) {                 //catch lỗi không có ký tự kết thúc trong region.
                fprintf(stderr, "[Victim] Could not find terminating byte (-1) in region name.\n");
            }
        }

        printf("[Victim PID: %d]The procname retrieved from memregionid %d is \"%s\"\n", ready_victim->pid, memrg, victim_name);

        if(strcmp(proc_name, victim_name) == 0){
            printf("Killing process with pid: %d \n", ready_victim->pid);  //print to debug 
            libfree(ready_victim, memrg);             //free the allocate memory
            free(ready_victim);                       //free the all structure of victim process
            printf("###############======DONE KILLING======###############\n");
        }
        else{
            enqueue(&tmp_ready_queue, ready_victim);
        }

        victim = dequeue(&caller->ready_queue);  // tiếp tục vòng lặp
    }
    while(!empty(&tmp_ready_queue)){
        struct pcb_t * put_back3 = dequeue(&tmp_ready_queue);
        enqueue(&caller->ready_queue, put_back3);
    }
#endif
    
printf("~~~~~~~~~~~~~~~End killall sucessfully~~~~~~~~~~~~~~~\n");
    return 0; 
}
