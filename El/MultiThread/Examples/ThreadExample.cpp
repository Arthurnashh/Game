// MultiThreading.cpp : Defines the entry point for the console application.
//
#define __PLACEMENT_NEW_INLINE
#include <stdio.h>
#include "../MTCommon.h"
#include "../Thread.h"
///Example of thread that derives ThreadBase class
using namespace MultiThread;

class TestThread1: public ThreadBase
{
public:
  unsigned long Run()
  {
    for (int i=0;i<10;i++)
    { 
      printf("Thread1: %d\n",i);
      Sleep(1000);
    }
    return 22;
  }
};



int main(int argc, const char * argv[])
{
  ///Example of thread that derives IRunnable interface (as local class)
  
  class TestThread2: public IRunnable
  {
  public:
    unsigned long Run()
    {
      for (int i=0;i<15;i++)
      {
        printf("Thread2: %c\n",i+65);
        ThreadBase::Sleep(1200);
      }
      return 11;
    }  
  };

  RunningThread<> myself;   // Instance of current thread (not necessary)
  TestThread1 thr1;       // Instance of first thread (derived from ThreadBase)
  TestThread2 thr2Runnable; // Instance of second thread derived from IRunnable
  Thread thr2(&thr2Runnable); //Instance of thread that starting runnable class
  thr1.Start();         //Start first thread
  thr2.Start();         //Start second thread

  //wait for finish all threads
  myself.JoinMulti(true,-1,2,&thr1,&thr2); //ThreadBase::JoinMulti is also acceptable
      
  //print exit codes
  printf("ExitCode1: %d\n",thr1.GetExitCode());
  printf("ExitCode2: %d\n",thr2.GetExitCode());

	return 0;
}










