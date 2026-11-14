// MultiThreading.cpp : Defines the entry point for the console application.
//
#include <stdio.h>
#include <stdlib.h>
#include "../MTCommon.h"
#include "../Thread.h"
#include "../CriticalSection.h"
///Example of thread that derives ThreadBase class
using namespace MultiThread;

static MiniCriticalSection miniSection[4];

class TestThread: public ThreadBase
{
  int threadId;
public:
  TestThread():threadId(0) {}
  void SetID(int id) {threadId=id;}
  unsigned long Run()
  {
    for (int i=0;i<100;i++)
    {
      printf("Thread: %d/%d - locking critical section...\n",threadId,i);
      miniSection[threadId & 3].Lock();
      printf("Thread: %d - working...\n",threadId);
      Sleep(rand()%50);
      miniSection[threadId & 3].Unlock();
      printf("Thread: %d - work done, unlocking, sleeping...\n",threadId);
//      Sleep(rand()%5000);
    }
  printf("Thread: %d - exit\n",threadId);
  return 0;
  }

};

#define TESTTHREADCOUNT 100

int main(int argc, const char * argv[])
{
  TestThread threads[TESTTHREADCOUNT];
  for (int i=0;i<TESTTHREADCOUNT;i++) {threads[i].SetID(i);threads[i].Start();}
  for (int i=0;i<TESTTHREADCOUNT;i++) {threads[i].Join();}
}  