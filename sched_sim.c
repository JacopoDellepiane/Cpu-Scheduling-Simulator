#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "fake_os.h"

FakeOS os;

typedef struct {
  float best;                                                                                   // best prediction value in the ready list
  ListItem* shortest;                                                                           // process with the shortest prediction
} SchedSJFArgs;  
                                                                            
void schedSJF(FakeOS* os, void* args_, int i) {
  SchedSJFArgs* args = (SchedSJFArgs*) args_;

  if (os->ready.first) {                                                            
    ListItem* b = os->ready.first;                                                              // take the first process in ready
    FakePCB* pcb = (FakePCB*)(b);   
    args->best = pcb->prediction;                                                               // use the first process to initialize the best prediction
    //printf("pid: %d, prediction: %f\n", pcb->pid, pcb->prediction);
    args->shortest = b;
    b = b->next;
    while(b) {                                                                                  // cycle through the ready list searching for a shorter prediction
      FakePCB* pcb1 = (FakePCB*)(b);   
      //printf("pid: %d, prediction: %f\n", pcb1->pid, pcb1->prediction);
      if (pcb1->prediction < args->best) {
          args->best = pcb1->prediction;
          args->shortest = b;
      }
      b = b->next;
    }
    if (!os->running[i]) {                                                                      // if the os isn't running anything, run the process with the shortest prediction
      FakePCB* pcb2 = (FakePCB*) List_detach(&os->ready, args->shortest);                                   
      assert(pcb2->events.first);                                                           
      ProcessEvent* e2 = (ProcessEvent*)pcb2->events.first;                                  
      assert(e2->type==CPU); 
      os->running[i] = pcb2;                                              
    }
    else {                                                                                      // if all the cpus are running a process, check if there's a process with a shortest prediction and do a preemption                                                    
      if (os->running[i]->prediction > args->best) {                                            
        FakePCB* pcb2 = (FakePCB*) List_detach(&os->ready, args->shortest);
        List_pushBack(&os->ready, (ListItem*) os->running[i]); 
        assert(pcb2->events.first);                                
        ProcessEvent* e2 = (ProcessEvent*)pcb2->events.first;                     
        assert(e2->type==CPU); 
        os->running[i] = pcb2;                   
      }
    }
  }

  if (os->running[i]) {                                                                         // if a process is running, increment process's time quantum and update the prediction
    os->running[i]->q_current += 1;                                                             
    os->running[i]->prediction = 0.5 * os->running[i]->q_current + 0.5 * os->running[i]->prediction;    
    //printf("pid: %d, new prediction: %f\n", os->running[i]->pid, os->running[i]->prediction);  
  }        
}

typedef struct {
  int quantum;
} SchedRRArgs;

void schedRR(FakeOS* os, void* args_, int i){                                                   // passing the index of the i-th cpu

  if (os->running[i])
    return;

  SchedRRArgs* args=(SchedRRArgs*)args_;
  
  // look for the first process in ready
  // if none, return
  if (! os->ready.first)
    return;

  FakePCB* pcb=(FakePCB*) List_popFront(&os->ready);
  os->running[i]=pcb;                                                                           // assign a process to the i-th cpu
  
  assert(pcb->events.first);
  ProcessEvent* e = (ProcessEvent*)pcb->events.first;
  assert(e->type==CPU);

  // look at the first event
  // if duration>quantum
  // push front in the list of event a CPU event of duration quantum
  // alter the duration of the old event subtracting quantum
  if (e->duration>args->quantum) {
    ProcessEvent* qe=(ProcessEvent*)malloc(sizeof(ProcessEvent));
    qe->list.prev=qe->list.next=0;
    qe->type=CPU;
    qe->duration=args->quantum;
    e->duration-=args->quantum;
    List_pushFront(&pcb->events, (ListItem*)qe);
  }
};

int main(int argc, char** argv) {
  int N;                                                                                        // variable to store the number of cpus to use
  char schedule[9];                                                                             // stores the scheduling algorithm to use
  printf("Enter the number of cpus to use in this simulation: ");
  scanf("%d", &N);
  printf("Choose the scheduling algorithm to use between schedRR and schedSJF: ");
  scanf("%s", schedule);
  FakeOS_init(&os, N);
  SchedRRArgs srr_args;
  SchedSJFArgs ssjf_args;
  if (strcmp(schedule, "schedRR") == 0) {
    srr_args.quantum = 5;
    os.schedule_args = &srr_args;
    os.schedule_fn = schedRR;
  }
  if (strcmp(schedule, "schedSJF") == 0) {
    ssjf_args.best = 0.0;
    ssjf_args.shortest = NULL;
    os.schedule_args = &ssjf_args;
    os.schedule_fn = schedSJF;
  }
  
  for (int i=1; i<argc; ++i){
    FakeProcess new_process;
    int num_events=FakeProcess_load(&new_process, argv[i]);
    printf("loading [%s], pid: %d, events:%d",
           argv[i], new_process.pid, num_events);
    if (num_events) {
      FakeProcess* new_process_ptr=(FakeProcess*)malloc(sizeof(FakeProcess));
      *new_process_ptr=new_process;
      List_pushBack(&os.processes, (ListItem*)new_process_ptr);
    }
  }
  printf("num processes in queue %d\n", os.processes.size);
  for (int i = 0; i < os.N; i++) {                                                              // check on all the cpus
    while(os.running[i]
          || os.ready.first
          || os.waiting.first
          || os.processes.first){
      FakeOS_simStep(&os);
    }
  }
  FakeOS_destroy(&os);
}
