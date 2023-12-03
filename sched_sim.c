#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "fake_os.h"

FakeOS os;

typedef struct {
  float prediction;
  float best;                                                                                 // best/shortest prediction value
  ListItem* shortest;                                                                         // process with the shortest prediction
} SchedSJFArgs;                                                                                

void schedSJF(FakeOS* os, void* args_, int i) {
  SchedSJFArgs* args = (SchedSJFArgs*) args_;

  if (! os->ready.first)                                                                
    return;

  ListItem* b = os->ready.first;                                                              // take the first process in ready
  FakePCB* pcb=(FakePCB*)(b);   
  ProcessEvent* e = (ProcessEvent*)pcb->events.first;
  args->prediction = 0.5 * e->duration + 0.5 * args->prediction;
  args->best = args->prediction;                                                              // use the first process to initialize the best prediction
  args->shortest = b;
  b = b->next;
  while(b) {                                                                                  // cycle through the ready list searching for a shorter process's burst
    FakePCB* pcb2 =(FakePCB*)(b);   
    ProcessEvent* f = (ProcessEvent*)pcb2->events.first;
    args->prediction = 0.5 * f->duration + 0.5 * args->prediction;
    if (args->prediction < args->best) {
        args->best = args->prediction;
        args->shortest = b;
    }
    b = b->next;
  }
 
  if (!os->running[i]) {                                                                      // if the os isn't running anything, run the process with the shortest burst
    FakePCB* pcb1=(FakePCB*) List_detach(&os->ready, List_find(&os->ready, args->shortest));                                   
    os->running[i]=pcb1;
    assert(pcb1->events.first);                                                           
    ProcessEvent* e3 = (ProcessEvent*)pcb1->events.first;                                  
    assert(e3->type==CPU); 
    args->prediction = 0.0;
  }
  else {                                                                                      // if all the cpus are running a process, check if there's a process with a shortest burst and do a preemption
    ProcessEvent* running_burst = (ProcessEvent*) os->running[i]->events.first;
    if (running_burst->duration > args->prediction) {
      FakePCB* pcb1=(FakePCB*) List_detach(&os->ready, List_find(&os->ready, args->shortest));
      List_pushBack(&os->ready, (ListItem*) os->running[i]); 
      os->running[i]=pcb1;
      assert(pcb1->events.first);                                
      ProcessEvent* e3 = (ProcessEvent*)pcb1->events.first;                     
      assert(e3->type==CPU); 
      args->prediction = 0.0;
    }
  }
}

typedef struct {
  int quantum;
} SchedRRArgs;

void schedRR(FakeOS* os, void* args_, int i){                                                 // passing the index of the i-th cpu
  SchedRRArgs* args=(SchedRRArgs*)args_;

  // look for the first process in ready
  // if none, return
  if (! os->ready.first)
    return;

  FakePCB* pcb=(FakePCB*) List_popFront(&os->ready);
  os->running[i]=pcb;                                                                         // assign a process to the i-th cpu
  
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
  int N;                                                                                      // variable to store the number of cpus to use
  printf("Enter the number of cpus to use in this simulation: ");
  scanf("%d", &N);
  FakeOS_init(&os, N);
  SchedRRArgs srr_args;
  srr_args.quantum=5;
  os.schedule_args=&srr_args;

  SchedSJFArgs ssjf_args;
  ssjf_args.prediction = 0.0;
  ssjf_args.best = 5000.0;
  ssjf_args.shortest = NULL;
  os.schedule_args = &ssjf_args;
  os.schedule_fn=schedSJF;
  
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
  for (int i = 0; i < os.N; i++) {                                                            // check on all the cpus
    while(os.running[i]
          || os.ready.first
          || os.waiting.first
          || os.processes.first){
      FakeOS_simStep(&os);
    }
  }
  FakeOS_destroy(&os);
}
