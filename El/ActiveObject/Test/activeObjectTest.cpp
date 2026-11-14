#include <El/elementpch.hpp>
#include <El/ActiveObject/activeObject.hpp>
#include <io.h>
#include <fcntl.h>


#define FILE_OPS_MESSAGES(CTX,MSG) \
  MSG(CTX,int,GetFileSize,(const char *arg1),1,const char *arg1) \
  MSG(CTX,int,Add,(int arg1, int arg2),2,int arg1; int arg2) \
  MSG(CTX,Void,Nop,(),0,) \
  MSG(CTX,double,Factorial,(int arg1),1,int arg1) \


FILE_OPS_MESSAGES(FileOps,ACTOBJ_DECLARE_ARGS)

class FileOps
{
  public:
  DECLARE_ACTIVE_OBJECT_SERVANT(FileOps,FILE_OPS_MESSAGES)
};

class FileOpsActObj: public ActiveObject<FileOps,0>
{
  public:
  FILE_OPS_MESSAGES(FileOps,ACTOBJ_WRAP_FUNC)
  
};

int FileOps::GetFileSize(const GetFileSizeAllArgs &args)
{
  LogF("GetFileSize %s",args.arg1);
  int handle = open(args.arg1,O_RDONLY|O_BINARY);
  if (handle<0) return 0;
  int size = filelength(handle);
  close(handle);
  return size;
}

int FileOps::Add(const AddAllArgs &args)
{
  LogF("Add %d,%d",args.arg1,args.arg2);
  return args.arg1+args.arg2;
}

Void FileOps::Nop(const NopAllArgs &args)
{
  LogF("Nop");
  return Void();
}

double FileOps::Factorial(const FactorialAllArgs &args)
{
  double factorial = 1;
  for (int i=1; i<=args.arg1; i++)
  {
    factorial *= i;
  }
  LogF("Factorial %d = %g",args.arg1,factorial);
  return factorial;
}

struct TestFutureItem: public SLIST_ENTRY, public RefCount
{
  Future<bool> value;
};

bool TestFutureDone;
SListMT<TestFutureItem> TestFutureQueue;

DWORD WINAPI TestFutureServant(void *context)
{
  for(;;)
  {
    if (TestFutureDone) break;
    if (TestFutureItem *item = TestFutureQueue.Pop())
    {
      item->value.SetResult(true);
      item->Release();
    }
  }
  return 0;
}

void FutureTest()
{
  HANDLE t = CreateThreadOnCPU(16*1024,TestFutureServant,NULL,0,THREAD_PRIORITY_NORMAL,"FutureTest");
  // Future stress test
  for (int i=0; i<100000; i++)
  {
    Ref<TestFutureItem> item = new TestFutureItem();
    item->AddRef();
    TestFutureQueue.Push(item);
    item->value.GetResult();
  }
  TestFutureDone = true;
  WaitForSingleObject(t,INFINITE);
  CloseHandle(t);
}


int main()
{
  FutureTest();
    
  const char *name = __FILE__;
//  FileOps ops;
//   int sizeSync = ops.GetFileSize(name);
//   int sizeSync2 = ops.Add(sizeSync,sizeSync);
  
  FileOpsActObj actOps;
  
  int err = 0;
  for (int i=0; i<1000; i++)
  {
    {
      Future<int> sizeAsync = actOps.GetFileSize(name);

      Future<double> factI = actOps.Factorial(i%50+19);
      
      actOps.Nop();
      actOps.Nop();
      actOps.Nop();
      actOps.Nop();
      actOps.Nop();
      
      actOps.KickOff();
      while (actOps.Update<true>()){}
      
      int sizeAsyncVal = sizeAsync.GetResult();
      Future<int> sizeAsync2 = actOps.Add(sizeAsyncVal,sizeAsyncVal);
      
      actOps.KickOff();
      while (actOps.Update<true>()){}
    }
    
  }
  return err;
}

