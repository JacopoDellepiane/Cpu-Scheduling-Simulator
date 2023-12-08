#include "fake_process.h"
#include "linked_list.h"
#pragma once

typedef struct {
  ListItem list;
  int pid;
  int q_current;                                                          // process's time quantum
  float prediction;                                                       // process's quantum prediction
  ListHead events;
} FakePCB;

struct FakeOS;
typedef void (*ScheduleFn)(struct FakeOS* os, void* args, int i);

typedef struct FakeOS{
  int N;                                                                  // N cpus
  FakePCB** running;                                                      // pointer to switch between the N FakePCB pointers
  ListHead ready;
  ListHead waiting;
  int timer;
  ScheduleFn schedule_fn;
  void* schedule_args;

  ListHead processes;
} FakeOS;

void FakeOS_init(FakeOS* os, int N);
void FakeOS_simStep(FakeOS* os);
void FakeOS_destroy(FakeOS* os);
