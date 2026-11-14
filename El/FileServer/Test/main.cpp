// MakefileGen.cpp : Defines the entry point for the console application.
//

#include <El/elementpch.hpp>
#include <Es/Common/win.h>
#include <Es/Strings/bString.hpp>
#include <El/Stringtable/stringtable.hpp>
#include <El/FileServer/fileServer.hpp>
#include <El/FileServer/fileServerAsync.hpp>
#include <El/MemoryStore/memStore.hpp>
#include <El/Multicore/multicore.hpp>
#include <El/ActiveObject/activeObject.hpp>
#include <direct.h>
#include <Intel/include/ittnotify.h>

#pragma comment(lib,"ittnotify_static")

extern bool IsAnotherEmpty();

static void InitStringtable()
{
  switch (PRIMARYLANGID (GetUserDefaultLangID ()))
  {
  case LANG_FRENCH:
    GLanguage = "French";
    break;
  case LANG_SPANISH:
    GLanguage = "Spanish";
    break;
  case LANG_ITALIAN:
    GLanguage = "Italian";
    break;
  case LANG_GERMAN:
    GLanguage = "German";
    break;
  case LANG_POLISH:
    GLanguage = "Polish";
    break;
  case LANG_CZECH:
    GLanguage = "Czech";
    break;
  case LANG_RUSSIAN:
    GLanguage = "Russian";
    break;
  default:
    GLanguage = "English";
    break;
  }

  // String table
  FILE *testf = fopen("fileServerTest.csv","rb");
  if (testf!=NULL)
  {
    LoadStringtable ("global", "fileServerTest.csv");
  }
}

void PrintVMMap(int mode,char const *name)
{
}

void ReportMemoryStatus()
{
}

#include <El/FileServer/multithread.hpp>

/// Test thread
class MyTestThread : public MultiThread::ThreadBase
{
  MultiThread::EventBlocker *_terminateEvent;
public:
  MyTestThread(MultiThread::EventBlocker *terminateEvent) : _terminateEvent(terminateEvent) {}
  unsigned long Run()
  {
    LogF("This is TestThread!");
    int count = 0;
    while (true)
    {
#if 1
      BlockerArItem bar[] = {_terminateEvent};
      if ( WaitForMultiple(bar, 1, false, 1000) == 1 )
        break;
#else
      if (_terminateEvent->Acquire(1000)) //one second
        break;
#endif
      LogF("Timeout inside thread elapsed, count = %d", ++count);
    }
    Sleep(1000); //wait a little
    return 0;
  }
};


#define QPC_START() QueryPerformanceCounter(&start)
#define QPC_END() QueryPerformanceCounter(&end);LogF("%g",1e6*double(end.QuadPart-start.QuadPart)/double(freq.QuadPart))


static double QPCTime()
{
  LARGE_INTEGER qpcFreq;
  QueryPerformanceFrequency(&qpcFreq);
  LARGE_INTEGER time;
  QueryPerformanceCounter(&time);
  return double(time.QuadPart)/qpcFreq.QuadPart;
}
struct TimeScope
{
  double start;
  const char *name;
  
  TimeScope(const char *n) {start=QPCTime(),name=n;}
  void Progress(const char *title){LogF("%s:%s %.4f ms",name,title,(QPCTime()-start)*1000);}
  ~TimeScope()
  {
    double duration = QPCTime()-start;
    LogF("%s%s took %.4f ms",duration>0.0001 ? "#" : "",name,(duration)*1000);
  }
  double GetTime() const {return QPCTime()-start;}
};

void CreateLongFile(const char *name, int size)
{
  TimeScope scope(__FUNCTION__);
  
  QOFStream out(name);
  while (--size>=0)
  {
    out.put(rand()&0xff);
  }
  out.close();
}
void ReadLongFile(const char *name)
{
  
  QIFStream in;
  in.open(name);
  // do not measure file opening, only file reading
  TimeScope scope(__FUNCTION__);
  struct ProcessBuffer
  {
    int &totalSize;
    ProcessBuffer(int &res):totalSize(res){}
    void operator () (const char *buf, int size) const
    {
      totalSize += size;
    }
  };
  int totalSize = 0;
  in.Process(ProcessBuffer(totalSize));
  LogF("Read %s: %d B, avg throughput: %.2f MB/s",name,totalSize,totalSize/scope.GetTime()/(1024*1024));
}


void StressTestFile(const char *name, QIStream &in, TimeScope &scope)
{
  int totalSize = 0;
  int pageSize = GetPageRecommendedSize();
  int pages = in.rest()/pageSize;
  Temp<char> buf(pageSize);
  #if _DEBUG
  const int iters = 1000;
  #else
  const int iters = 25000;
  #endif
  for (int i=0; i<iters; i++)
  {
    int randPage = rand()%pages;
    in.seekg(randPage*pageSize,QIOS::beg);
    #if 1
    while (!in.PreRead(FileRequestPriority(1),randPage*pageSize,pageSize*16))
    {
      // TODO: better synchronization mechanism
      Sleep(0);
    }
    #endif
    in.read(buf.Data(),buf.Size());
    totalSize += buf.Size();
    if ((i+1)%1000==0) scope.Progress(Format("Iters %d",i+1));
    FreeOnDemandFrame();
  }
  LogF("Random access (cached) %s: %d B, avg throughput: %.2f MB/s",name,totalSize,totalSize/scope.GetTime()/(1024*1024));
}

#define ASYNC_FILES 1

#if ASYNC_FILES


DWORD WINAPI ReadAsyncThread(void *ctx)
{
  TimeScope scope(__FUNCTION__);
  const char *name = reinterpret_cast<const char *>(ctx);
  // prevent creating too many requests
  FileServerHandle handle = GAsyncFileServer.OpenReadHandle(name);
  QFileSize size = GAsyncFileServer.GetFileSize(handle);
  //int totalSize = size->GetResult();
  int step = 4*1024;
  #if _DEBUG
    if (size>1024*1024) size = 1024*1024;
  #endif
  for (int i=0; i<(int)size; i+=step)
  {
    PosQIStreamBuffer posF = GAsyncFileServer.Read(handle,i,step);
    //posF.GetResult();
  }
  GAsyncFileServer.CloseReadHandle(handle);
  // we have submitted async instructions - now we should wait for them to complete
  return 0;
}


void ReadAsync(const char *name)
{
  // create a worker thread and read from it
  HANDLE threadHandle = CreateThreadOnCPU(64*1024,ReadAsyncThread,unconst_cast(name),0,THREAD_PRIORITY_NORMAL,"ReadAsync");
  #if ASYNC_FILES==2
  for(;;)
  {
    if (::WaitForSingleObject(threadHandle,10)==WAIT_OBJECT_0)
      break;
    //for (int i=0; i<10; i++)
    {
      GAsyncFileServer.DoWork(0);
    }
  }
  #else
  ::WaitForSingleObject(threadHandle,INFINITE);
  #endif
}


void StressTestFileAsync(const char *name)
{
  QIFStream in;
  in.open(name);
  TimeScope scope(__FUNCTION__);
  StressTestFile(name,in,scope);
    }

DWORD WINAPI StressTestFileThreadCallback(void *ctx)
{
  const char *name = reinterpret_cast<const char *>(ctx);

  QIFStream in;
  in.open(name);
  TimeScope scope(__FUNCTION__);
  StressTestFile(name,in,scope);
  return 0;
}
void StressTestFileThread(const char *name)
{
  HANDLE threadHandle = CreateThreadOnCPU(64*1024,StressTestFileThreadCallback,unconst_cast(name),0,THREAD_PRIORITY_NORMAL,"StressTestFileThread");
  
  for(;;)
  {
    if (::WaitForSingleObject(threadHandle,10)==WAIT_OBJECT_0)
      break;
    //for (int i=0; i<1000; i++)
    {
      GAsyncFileServer.DoWork(0);
    }
  }
  
}

#endif

/************************************************************************/

void ReadSync(const char *name)
{
  TimeScope scope(__FUNCTION__);
  // prevent creating too many requests
  FileServerHandle handle = GFileServerFunctions->OpenReadHandle(name);
  QFileSize size = GFileServerFunctions->GetFileSize(handle);
  int totalSize = size;
  int step = 4*1024;
  for (int i=0; i<totalSize; i+=step)
  {
    PosQIStreamBuffer req = GFileServerFunctions->Read(handle,i,step);
  }
  GFileServerFunctions->CloseReadHandle(handle);
}


void StressTestFile(const char *name)
{
  QIFStream in;
  in.open(name);
  // do not measure file opening, only file reading
  TimeScope scope(__FUNCTION__);
  StressTestFile(name,in,scope);
  }


void TestMemoryStore()
{
  MEMORYSTATUS memstat;
  memstat.dwLength = sizeof(memstat);
  GlobalMemoryStatus(&memstat);
  
  {
    TimeScope scope(__FUNCTION__);
    
      
    // basic file mapping test (512 MB)
    long long size = 512*1024*1024;
    
    MemoryStore store;
  
    store.Init(size);
  
    // create and destroy temporary views
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    //const int allocSize = sysInfo.dwAllocationGranularity;
    const int pageSize = sysInfo.dwPageSize;
    
    GlobalMemoryStatus(&memstat);
    
    for (long long offset=0; offset<=size-pageSize; offset+=pageSize)  
    {
      store.Alloc();
    }
    for (int i=0; i<1; i++)
    {
      int t = 0;
      for (long long offset=0; offset<=size-pageSize; offset+=pageSize,t++)
      {
        void *map = store.MapView(t,true);
        //void *map = MapViewOfFile(mapping,FILE_MAP_WRITE,(DWORD)(offset>>32),(DWORD)offset,allocSize);
        Assert(map);
        if (map)
        {
          memset(map,(char)(0x11+t),pageSize);
          store.UnmapView(t,map);
        }
      }
      GlobalMemoryStatus(&memstat);
      

      t = 0;
      for (long long offset=0; offset<=size-pageSize; offset+=pageSize,t++)
      {
        void *map = store.MapView(t,false);
        Assert(map);
        if (map)
        {
          for (int p=0; p<pageSize; p++)
          {
            if (((char *)map)[p]!=char(0x11+t))
            {
              Fail("Memory read failed");
            }
          }
          store.UnmapView(t,map);
        }
      }
      GlobalMemoryStatus(&memstat);
    }
    
    GlobalMemoryStatus(&memstat);
  }
  GlobalMemoryStatus(&memstat);
}

void BasicQStreamTest()
{
  TimeScope scope(__FUNCTION__);
  //QStream & FileServer test
  QIFStream in;
  QOFStream out; out.open("fileServerTestOut.txt");
  in.open("fileServerTest.csv");
  Temp<char> buf; buf.Realloc(1024);
  while (in.readLine(buf, buf.Size()))
  {
    printf("%s\n", buf.Data());
    out << buf.Data() << "\r\n";
  }
  out.close();
  in.close();
}

void StringTableTest()
{
  //Stringtable & FileServer test
  InitStringtable();
  RString txt = LocalizeString("FILE_SERVER_TEST");
  puts(txt); LogF("Localized FILE_SERVER_TEST string from stringtable is \"%s\"", cc_cast(txt));
  ClearStringtable();
}

void MultiThreadLibTest()
{
  // Simple Test of MultiThread library
  MultiThread::EventBlocker terminateEvent;
  MyTestThread tstThread(&terminateEvent);
  MultiThread::Thread thread(&tstThread);
  thread.Start();
  Sleep(5500); //5.5 seconds
  terminateEvent.Unblock(); //after unblocking the thread will wait one second to proceed our wait test
  //Sleep(2000); //2 seconds [uncomment this to see, the thread was Joined even when already NOT running]
  bool isRunning = thread.IsRunning();
  LogF("Thread is%s running", (isRunning ? "" : " NOT"));
#if 0
  bool thrExit = thread.Join();
  if (thrExit) LogF("Thread was successfully joined!");
#else
  MultiThread::ThreadBlocker thrBlocker(thread);
  BlockerArItem bar[] = {&thrBlocker};
  int retval = WaitForMultiple(bar, 1, false, INFINITE);
  LogF("WaitForMultiple retval = %d", retval);
#endif
}

size_t GetMemoryUsedSize();

static double TimeAccess(const char *mem)
{
  double start = QPCTime();
  volatile char read = *mem;
  return QPCTime()-start;
}

int TestMapping(const char *fname)
{
  HANDLE hfile = CreateFile( fname, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS|FILE_FLAG_NO_BUFFERING, NULL );
  if( hfile == INVALID_HANDLE_VALUE ) return 1;
  
  DWORD size = GetFileSize(hfile,NULL);
  const int block = 20000;
  for (int i=0; i<2; i++)
  {
  
    HANDLE map_handle = CreateFileMapping( hfile, NULL, PAGE_READONLY /*| SEC_RESERVE*/, 0, 0, 0);
    if( map_handle == NULL ) return 1;
    
    char* map_ptr = (char*) MapViewOfFile( map_handle, FILE_MAP_READ, 0, 0, 0 );
    if( map_ptr == NULL ) return 1;

    for( int n = 0 ; n < 10*block ; n++ ) {volatile char read=map_ptr[n*4096];}

    {
      TimeScope scope("Used: Use used page");
      volatile char read=map_ptr[5*block*4096];
    }
    
    {
      TimeScope scope("Used: Use unused page");
      volatile char read=map_ptr[10*block*4096];
    }

    {volatile char read=map_ptr[0*4096];}
    {volatile char read=map_ptr[7*block*4096];}
    {volatile char read=map_ptr[5*block*4096];}
    {volatile char read=map_ptr[8*block*4096];}

    for( int n = 0 ; n < 10*block ; n++ ) {volatile char read=map_ptr[n*4096];}

    VirtualUnlock( map_ptr +7*block*4096, 1*block * 4096 );
    VirtualUnlock( map_ptr, 5*block*4096 );
    
    struct DetectRaisingEdge
    {
      bool last;

      bool Track(int id, int m, bool expected, float duration)
      {
        bool detected = duration>0.0001;
        if (detected==last) return detected;
        last = detected;
        if (detected!=expected)
        {
          LogF("%d: %d: Slow %.4f ms",id,m,duration*1000);
        }
        return detected;
      }
    };

    DetectRaisingEdge c0,c2,c5,c7,c8;
    // map more and more of new data and track when old data start to be discarded
    bool slow = false;
    for (int m=0; m<10*block; m++)
    {
      {volatile char read=map_ptr[(10*block+m)*4096];}
      if (m%block==0) LogF("Block %d",m/block);
      slow |= c0.Track(0,m,slow,TimeAccess(map_ptr+(m)*4096));
      slow |= c2.Track(2,m,slow,TimeAccess(map_ptr+(2*block+m)*4096));
      slow |= c5.Track(5,m,false,TimeAccess(map_ptr+(5*block+m)*4096));
      slow |= c7.Track(7,m,slow,TimeAccess(map_ptr+(7*block+m)*4096));
      slow |= c8.Track(8,m,false,TimeAccess(map_ptr+(8*block+m)*4096));
    }
    {
      TimeScope scope("Unlocked: Use unlocked used page 0b");
      volatile char read=map_ptr[0*4096];
    }
    {
      TimeScope scope("Unlocked: Use unlocked used page 7b");
      volatile char read=map_ptr[7*block*4096];
    }
    {
      TimeScope scope("Unlocked: Use used page 5b");
      volatile char read=map_ptr[5*block*4096];
    }
    {
      TimeScope scope("Unlocked: Use used page 8b");
      volatile char read=map_ptr[8*block*4096];
    }
    {
      TimeScope scope("Unlocked: Use used page 8b");
      volatile char read=map_ptr[8*block];
    }
    
    UnmapViewOfFile(map_ptr);
    
    map_ptr = (char*) MapViewOfFile( map_handle, FILE_MAP_READ, 0, 0, 0 );
    
    {
      TimeScope scope("Unmapped/mapped: Use used page");
      volatile char read=map_ptr[5001*4096];
    }
    
    {
      TimeScope scope("Unmapped/mapped: Use unused page");
      volatile char read=map_ptr[10100*4096];
    }
    
    UnmapViewOfFile(map_ptr);
    
    CloseHandle(map_handle);
    LogF("* Unmapped");
  }
  
  CloseHandle(hfile);
  LogF("** File closed");
  return 0;
}



int main(int argc, char* argv[])
{
  const char *defTestFile = "Test\\test.bin";
  const char *readName = defTestFile;
  if (argc>1)
  {
    readName = argv[1];
  }
  
  GFileServer->SetMaxSize(64*1024*1024);
  
  #if 0
  TestMemoryStore();
  #endif
  
  #if 0
	_mkdir("Test");
  CreateLongFile(defTestFile,1900*1024*1024);
  #endif

  #if 1
  TestMapping(defTestFile);
  TestMapping(defTestFile);
  #else
  
  #if 0
  
  ReadLongFile(readName);
  #endif

  #if ASYNC_FILES==1

  GFileServerFunctions = &GAsyncFileServer;
  
  GAsyncFileServer.StartWorkerThread();
  
  __itt_pause();
  ReadAsync(readName);
  __itt_resume();
  
  StressTestFileAsync(readName);

  GAsyncFileServer.StopWorkerThread();
  
  #elif ASYNC_FILES==2

  GFileServer->Start();
  
  __itt_pause();
  ReadAsync(readName);
  __itt_resume();
  
  StressTestFileThread(readName);
  
  GAsyncFileServer.StopOnMainThread();

  #else
  
  //StressTestFile(readName);
  if (GFileServer) GFileServer->Start();
  
  __itt_pause();
  ReadSync(readName);
  __itt_resume();

  StressTestFile(readName);

  // This must be called in order not to get Asserts in destructor of FileServerST 
  if (GFileServer) GFileServer->Stop(),GFileServer->Clear();
  #endif

  #endif
  
  //BasicQStreamTest();
  
  //StringTableTest();

#if 0

  MultiThreadLibTest();

#endif
  
  return 0;
}

