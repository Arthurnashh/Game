#include <El/elementpch.hpp>
#include <El/FileServer/fileServerAsync.hpp>
#include <El/ActiveObject/activeObject.hpp>
#include <Es/Files/fileContainer.hpp>
#include <El/FileServer/fileServerMT.hpp>


/// normal arguments are passed to native function as is
template <class Type>
const Type &ArgToCall(const Type &src){return src;}

// futures need to be passed as their result instead
template <class Type>
const Type &ArgToCall(const Future<Type> &src){return src.GetResult();}

#define CALLARGS0 
#define CALLARGS1 ArgToCall(args.arg1)
#define CALLARGS2 CALLARGS1,ArgToCall(args.arg2)
#define CALLARGS3 CALLARGS2,ArgToCall(args.arg3)
#define CALLARGS4 CALLARGS3,ArgToCall(args.arg4)
#define CALLARGS5 CALLARGS4,ArgToCall(args.arg5)
#define CALLARGS6 CALLARGS5,ArgToCall(args.arg6)
#define CALLARGS7 CALLARGS6,ArgToCall(args.arg7)
#define CALLARGS8 CALLARGS7,ArgToCall(args.arg8)
#define CALLARGS9 CALLARGS8,ArgToCall(args.arg9)
#define CALLARGS10 CALLARGS9,ArgToCall(args.arg10)



#define DEFINE_FUNCTION(ctx,Ret,Name,Args,NumArgs,DefArgs) \
  Ret ctx::Name(const Name##AllArgs &args){return GFileServerFunctionsSync->Name(CALLARGS##NumArgs);}

FILE_SERVER_MESSAGES_STD(FileServerAsync,DEFINE_FUNCTION)


#define COMPLETION_FROM_REQUEST(req) (static_cast<FileRequestCompletionRefCount *>((RefCount *)req.GetRef()))
#define REQUEST_FROM_COMPLETION(req) (static_cast<FileRequest *>((RefCount *)req.GetRef()))

void FileServerAsync::PreloadAsync(const PreloadAsyncAllArgs &args)
{
  RequestCompletion *handle = static_cast_checked<RequestCompletion *>(args.arg1.GetRef());
  // once completed, there is no need to test any more - all data are in the request
  FileRequestCompletionHandle req = COMPLETION_FROM_REQUEST(handle->request);
  bool ret = GFileServerFunctionsSync->Preload(req,args.arg2,args.arg3,args.arg4,args.arg5,args.arg6,args.arg7);
  #if _DEBUG
  FileRequest *request = REQUEST_FROM_COMPLETION(req);
  Assert(request->InNeeded(args.arg6,args.arg7));
  #endif
  handle->request = REQUEST_FROM_COMPLETION(req);
  MemoryPublish();
  handle->completed = ret;

  //GFileServer->Update();
}

PosQIStreamBuffer FileServerAsync::ReadAsync(const ReadAsyncAllArgs &args)
{
  PosQIStreamBuffer ret = GFileServerFunctionsSync->Read(args.arg1,args.arg2,args.arg3);
  //GFileServer->Update();
  return ret;
}

bool FileServerAsync::PreloadSync(const PreloadSyncAllArgs &args)
{
  FileRequestCompletionHandle dummy;
  return GFileServerFunctionsSync->Preload(dummy,args.arg1,args.arg2,args.arg3,args.arg4,args.arg5,args.arg6);
}

bool FileServerAsync::FlushReadHandle(const FlushReadHandleAllArgs &args)
{
  // first of all destroy everything which waits to be destroyed
  // some of that could have the handle still open
  FreeStoredArgs();
  return GFileServerFunctionsSync->FlushReadHandle(args.arg1);
}

WaitableVoid FileServerAsync::WaitForOneRequestDone(const WaitForOneRequestDoneAllArgs &args)
{
  GFileServer->WaitForOneRequestDone(args.arg1);
  return WaitableVoid();
}

WaitableVoid FileServerAsync::SubmitRequestsMinimizeSeek(const SubmitRequestsMinimizeSeekAllArgs &args)
{
  GFileServer->SubmitRequestsMinimizeSeek();
  return WaitableVoid();
}

WaitableVoid FileServerAsync::FlushServer(const FlushServerAllArgs &args)
{
  DoFlush();

  GFileServer->Stop();
  GFileServer->Start();
  return WaitableVoid();
}


RequestCompletion::~RequestCompletion()
{
  if (!request)
    return; // no need to destroy NULL completion
  // TODO: push the RequestCompletion directly to avoid new memory allocation
  StoredRequest *store = new StoredRequest(request);
  // therefore we must release the reference count first, and push later
  request.Free();
  MemoryPublish();
  // otherwise we risk we will still be performing the destruction, as it could be popped before we release
  // we cannot be sure we were the last owner, but we do not need that. We only need to make sure
  // we were not releasing the last ref
  Assert(store->request.IsNull() || store->request->RefCounter()>=1);
  GAsyncFileServer.AddToDestroy(store);
}

void RequestCompletion::Refresh()
{
  frameId = FreeOnDemandFrameID();
}
static void UpdateMainThread()
{
  GFileServer->UpdateByThread(0);
  GAsyncFileServer.MaintainMainThread();
}

void FileServerActObj::DoFlush()
{
  {
    ScopeLockSection lock(_lockReqCache);
    // we are the one responsible for the server cleanup
    _reqCache.Clear();
  }
  #if 0
  _lastBufferHandle = NULL;
  _lastBuffer.Free();
  _lastBuffer._len = 0;
  _lastBuffer._start = 0;
  #endif
  FreeStoredArgs(); // _reqCache.Clear() may have added some stored args
}

void FileServerActObj::Worker()
{
  GFileServer->Start();
  
  // wait until some job is ready or termination
  for(;;)
  {
    if (!DoWork(100))
      break;
  }
  
  Update<false>();
  //FreeStoredArgs();
  DoFlush();

  GFileServer->Stop();
  GFileServer->Clear();
}

//#pragma warning(disable:4355)
FileServerActObj::FileServerActObj()
:_worker(NoInitHandle),_workerRunning(false)  
{
}

void FileServerActObj::StartWorkerThread()
{
  GlobalAtAlive(UpdateMainThread);
  if (!_workerRunning)
  {
    _workerRunning = true;
    _worker.Init( CreateThreadOnCPU(64*1024,WorkerCallback,this,1,THREAD_PRIORITY_NORMAL,"FileServerAsync") );
  }
}

void FileServerActObj::FlushWorkerThread()
{
  Future<WaitableVoid> wait = FlushServer();
  wait.GetResult();
}

void FileServerActObj::StopWorkerThread()
{
  // signal termination and wait for it
  _terminate = true;
  KickOff(); // kick so that terminate is noticed
  _worker.Wait();
  _worker.Done();
  GlobalCancelAtAlive(UpdateMainThread);
  _workerRunning = false;
}


FileServerActObj::~FileServerActObj()
{
  if (_workerRunning)
    StopWorkerThread();
}


bool FileRequests::RequestCheckItegrity() const
{
  // traverse whole offtree, for each item check there is a matching one in the array
  return true;
}

RequestCompletion *FileRequests::Find(const RequestCompletionKey &key) const
{
  OffTree< Ref<RequestCompletion> >::QueryResult res;
  _requestsOffTree.Query(res,key._start,0,key._start+key._size,1);
  RequestCompletion *ret = NULL;
  for (int i=0; i<res.regions.Size(); i++)
  {
    RequestCompletion *item = res.regions[i];
    // prefer exact match
    if (item->_start==key._start && item->_size==key._size)
    {
      if (item->_obj==key._obj && item->_ctx==key._ctx)
      {
        ret = item;
        break;
      }
    }
  }
  #if _DEBUG
    RequestCompletion *verifyRet = NULL;
    int index = _requests.FindKey(key);
    if (index<0) verifyRet = NULL;
    else verifyRet = _requests[index];
    Assert(verifyRet==ret);
  #endif
  
  return ret;
}

RequestCompletion *FileRequests::FindContaining(const RequestCompletionKey &key, FileRequestPriority prio) const
{
  OffTree< Ref<RequestCompletion> >::QueryResult res;
  _requestsOffTree.Query(res,key._start,0,key._start+key._size,1);
  // query returns all which intersect with our query
  // we are interested only about those who contain us
  // TODO: pass filter as a functor into Query
  RequestCompletion *ret = NULL;
  for (int i=0; i<res.regions.Size(); i++)
  {
    RequestCompletion *item = res.regions[i];
    // prefer exact match
    if (item->_start<=key._start && item->_start+item->_size>=key._start+key._size)
    {
      if (item->_obj==key._obj && item->_ctx==key._ctx)
      {
        // absolutely prefer exact match
        if (item->_start==key._start && item->_size==key._size)
        {
          ret = item;
          break;
        }
        // prefer higher priority requests, with same priority prefer shorter ones
        if (!ret || item->priority<ret->priority || item->priority==ret->priority && item->_size<ret->_size)
          ret = item;
      }
    }
  }

  #if 0 // too strict check
    RequestCompletion *verifyRet = NULL;
    for (int i=0; i<_requests.Size(); i++)
    {
      RequestCompletion *item = _requests[i];
      if (item->_start<=key._start && item->_start+item->_size>=key._start+key._size)
      {
        if (item->_obj==key._obj && item->_ctx==key._ctx)
        {
          // absolutely prefer exact match
          if (item->_start==key._start && item->_size==key._size)
          {
            verifyRet = item;
            break;
          }
          // prefer higher priority requests
          if (!verifyRet || item->priority<verifyRet->priority || item->priority==verifyRet->priority && item->_size<verifyRet->_size)
            verifyRet = item;
        }
      }
    }
    Assert(verifyRet==ret);
  #endif
  return ret;
}

RString GetReqDebugName(FileServerHandle handle, QFileSize start, QFileSize size, FileRequestPriority prio);

bool FileServerActObj::Preload(
  FileRequestCompletionHandle &request, RequestableObject *obj, RequestableObject::RequestContext *ctx,
  FileRequestPriority priority, FileServerHandle handle, QFileSize start, QFileSize size
)
{
  PROFILE_SCOPE_EX(fsAPr,file);
  bool newReq = false;
  Ref<RequestCompletion> reqComplete = static_cast_checked<RequestCompletion *>(request.GetRef());
  // check last request first
  if (!reqComplete || !reqComplete->RequestContains(obj,ctx,priority,handle,start,size))
  {
    ScopeLockSection lock(_lockReqCache);

    QFileSize fileSize = GFileServerFunctionsSync->GetFileSize(handle);
    QFileSize pageSize = GetPageSize();

    // request are often not aligned here yet, see QIStream::PreRead / FileBufferLoading::PreloadBuffer
    QFileSize startAligned = start&~(pageSize-1); // align down
    QFileSize endAligned = (start+size+pageSize-1)&~(pageSize-1); // align up
    if (endAligned>fileSize) endAligned = fileSize;

    start = startAligned;
    size = endAligned-startAligned;

    // before issuing a new request check if there is some other matching what we need
    RequestCompletionKey loc;
    loc._handle = handle;
    loc._start = start;
    loc._size = size;
    loc._obj = obj;
    loc._ctx = ctx;
    Ref<FileRequests> cached;
    const Ref<FileRequests> &search = _reqCache.Get(handle);
    if (_reqCache.IsNull(search))
    {
      cached = new FileRequests(handle,fileSize);
      _reqCache.Add(cached);
    }
    else
    {
      cached = search;
    }

    reqComplete = cached->FindContaining(loc,priority);

    if (reqComplete)
    {
      // we have the file, find the handle
      //reqComplete = static_cast_checked<RequestCompletion *>(cached.GetRef());
      // note: obj and ctx must match as well
      // TODO: make this part of a key
      if(!reqComplete->RequestContains(obj,ctx,priority,handle,start,size))
      {
        if(reqComplete->RequestContains(obj,ctx,FileRequestPriority(INT_MAX),handle,start,size))
        {
          // when the request has already completed, we do not care about priority
          if (!reqComplete->completed && reqComplete->request)
          {
            // checking if request is completed is a sync operation
            reqComplete->completed = IsDone(reqComplete->request);
          }
          if (!reqComplete->completed)
          {
            // TODO: consider how to increase priority efficiently. Currently we create a new request
            LogF("Request priority increase required (%s)",cc_cast(GetReqDebugName(handle,start,size,priority)));
            // with exact match the new request will supersede the old one
            if (reqComplete->_start==start && reqComplete->_size==size)
            {
              // when remove only when there is exact match
              cached->Remove(reqComplete);
            }
            goto NewRequest;
          }
        }
        else
        {
          Assert(obj || ctx);
          Assert(!reqComplete->_obj && !reqComplete->_ctx);
          // new request needed - object information does not match
          goto NewRequest;
        }
      }
      request = reqComplete;
      reqComplete->Refresh();
    }
    else
    {
NewRequest:
      newReq = true;
      reqComplete = new RequestCompletion;
      reqComplete->_obj = obj;
      reqComplete->_ctx = ctx;
      reqComplete->priority = priority;
      reqComplete->_handle = handle;
      reqComplete->_start = start;
      reqComplete->_size = size;
      reqComplete->Refresh();

      request = reqComplete;
      cached->Add(reqComplete);
      // TODO: retire old requests

      // TODO: we might unlock _lockReqCache even before submitting PreloadAsync
      PreloadAsync(reqComplete,obj,ctx,priority,handle,start,size);
    }
  }
  // once completed, there is no need to test any more - all data are in the request
  if (!reqComplete->completed)
  {
    // note: if there is already some Preload pending, there is no need at all to create another one
    // however we need to be sure it is the request we are interested in

#if 1
    if (reqComplete->request)
    {
      // checking if request is completed is a sync operation
      reqComplete->completed = IsDone(reqComplete->request);
    }
#endif
  }

  bool ret = false; 
  if (reqComplete->completed)
  {
    MemorySubscribe();
    ret = true;
  }

  if (PROFILE_SCOPE_NAME(fsAPr).IsActive())
  {
    PROFILE_SCOPE_NAME(fsAPr).AddMoreInfo(RString(ret ? "D " : "")+RString(newReq ? "N " : "")+GetReqDebugName(handle,start,size,priority));
  }
  return ret;
}



PosQIStreamBuffer FileServerActObj::Read(FileServerHandle handle, QFileSize start, QFileSize size)
{
  #if 0
  if (_lastBufferHandle==handle && _lastBuffer._start==start && _lastBuffer._len==size)
  {
    return _lastBuffer;
    }
  #endif
  PROFILE_SCOPE_EX(fsARd,file);
  if (PROFILE_SCOPE_NAME(fsARd).IsActive())
  {
    PROFILE_SCOPE_NAME(fsARd).AddMoreInfo(GetReqDebugName(handle,start,size,FileRequestPriority(0)));
  }
  
  PosQIStreamBuffer ret;
  
  {
  ScopeLockSection lock(_lockReqCache);
  RequestCompletionKey loc;
  loc._handle = handle;
  loc._start = start;
  loc._size = size;
  loc._obj = NULL;
  loc._ctx = NULL;
    const Ref<FileRequests> &cached = _reqCache.Get(handle);
  if (_reqCache.NotNull(cached))
  {
      Ref<RequestCompletion> reqComplete = cached->FindContaining(loc,FileRequestPriority(INT_MAX));
      if (reqComplete)
      {
        reqComplete->Refresh();
      if (reqComplete->completed)
    {
        Assert(reqComplete->request);
        if (reqComplete->request)
        {
          FileRequest *r = static_cast_checked<FileRequest *>(reqComplete->request.GetRef());

          ret = r->GetResultBuffer(start,size);
          if (ret._len>0)
          {
            #if 0
            _lastBuffer = ret;
            _lastBufferHandle = handle;
            #endif
            return ret;
          }
        }
      }
        else if (reqComplete->_start==start && reqComplete->_size==size)
      {
          // marking the request as completed allows us to discard them
        // it is not completed yet, but we know it will be completed
        // as this thread is the only real user of the "completed" flag, it will do no harm to mark it before real completion
        // the flag is also tested in FileServerActObj::DiscardOldRequests, but if it will be wrong there it will do no harm
        reqComplete->completed = true;
}
    }
  }
  }
   
  {
    PROFILE_SCOPE_DETAIL_EX(fsARS,file);
    const Future<PosQIStreamBuffer> &retF = ReadAsync(handle,start,size);
    PROFILE_SCOPE_DETAIL_EX(fsARR,file);
      ret = retF.GetResult();
    }

  #if 0
  _lastBuffer = ret;
  _lastBufferHandle = handle;
  #endif
  return ret;
}

bool FileServerActObj::IsDone(RefCount *request) const
{
  Assert(dynamic_cast<FileReqRef *>(request));
  FileReqRef *fileRequest = static_cast<FileReqRef *>(request);
  return GFileServer->RequestIsDoneAsync(fileRequest);
}

StoredRefContainer FileServerAsyncRefToDestroyWorkerThread;
StoredRefContainer FileServerAsyncRefToDestroyMainThread;

void FileServerAsync::FreeStoredArgs()
{
  for (;;)
  {
    RequestCompletion::StoredRequest *stored = _toDestroy.Pop();
    if (!stored) break;
    delete stored;
  }
  struct DestroyRef
  {
    bool operator () (RefCount *ref) const {ref->Release();return false;}
  };
  FileServerAsyncRefToDestroyWorkerThread.ForEach(DestroyRef());
}

bool FileServerActObj::DoWork(int timeout)
{
  SignaledObject *handles[] = {&_kicked, &GFileServer->GetUpdateEvent()};
  int result;
  {
    //PROFILE_SCOPE_EX(fsATW,file);
    result = SignaledObject::WaitForMultiple(handles, lenof(handles),timeout);
  }
  // timeout
  if (result==0)
  {
    {
      PROFILE_SCOPE_EX(fsATU,file);
      Update<false>();
    }
    {
      PROFILE_SCOPE_EX(fsATM,file);
      Maintain();
    }
  }
  // make sure updates are called on a regular basis
  // note: one update may trigger another update 
  // e.g. when OnBufferCompleted completes a request, it may be necessary to call XXXXX ... what?
  PROFILE_SCOPE_EX(fsATT,file);
  GFileServer->Update();
  if (_terminate)
  {
    _terminate = false;
    return false;
  }
  return true;
}

void FileServerActObj::MaintainMainThread()
{
  AssertMainThread();
  struct DestroyRef
  {
    bool operator () (RefCount *ref) const {ref->Release();return false;}
  };
  FileServerAsyncRefToDestroyMainThread.ForEach(DestroyRef());
}

void FileServerActObj::Maintain()
{
  FreeStoredArgs();
  DiscardOldRequests();
}

void FileServerActObj::DiscardOldRequests()
{
  // caution: we cannot lock _lockReqCache, as it would make impossible for async server to complete the requests
  ScopeLockSection lock(_lockReqCache);
  // traverse and discard
  struct DiscardOld
  {
    FileServerActObj *server;
    int now;
    
    DiscardOld(FileServerActObj *server):server(server),now(FreeOnDemandFrameID()) {}
    bool operator () (FileRequests *item, RequestCache *container)
    {
      struct DiscardOldItem
      {
        int now;
        FileServerActObj *server;
        DiscardOldItem(FileServerActObj *server, int now):server(server),now(now){}
        
        bool operator () (RequestCompletion *item) const
        {
      const int maxAge = 8;
      const int maxOrphanAge = 80;
      // update request completion to improve handling orphaned requests
      if (!item->completed && item->request)
      {
        item->completed = server->IsDone(item->request);
      }
      // never discard incomplete requests - completing them is important because of handlers
          return now-item->frameId>maxAge && item->completed || now-item->frameId>maxOrphanAge;
      }
      };
      item->ForEachWithDelete(DiscardOldItem(server,now));
      return false;
    }
  };
  // TODO: make LRU instead
  _reqCache.ForEachF(DiscardOld(this));
  
  _reqCache.CleanUp();
}

bool FileServerActObjImpl::FlushReadHandle(const char *name)
{
  // TODO: move to the servant instead, we are waiting for async ops here anyway
  {
    ScopeLockSection lock(_lockReqCache);
    #if 1
    // while we have the cache locked, we cannot call any server functions, as they could deadlock
    // convert handle to name: local table
    
    struct DiscardName // discard any requests to the file
    {
      const char *name;
      FileServerActObjImpl *server;
      bool first;
      DiscardName(FileServerActObjImpl *server, const char *name):name(name),server(server),first(true){}
      bool operator () (FileRequests *item, RequestCache *container)
      {
        // prevent _handle.GetResult from blocking, we are holding the lock of _lockReqCache
        RString itemName = server->GetHandleName(item->GetKey());
        if (!strcmp(name,itemName))
        {
          container->Remove(item->GetKey());
          // only one slot with given handle may exist
          DoAssert(first);
          first = false;
        }
        return false;
      }
    };
    _reqCache.ForEachF(DiscardName(this,name)); // mark for deletion
    #endif
    _reqCache.CleanUp(); // perform the deletion
    _reqCache.Optimize(); // reduce the hash table size
  }
  return base::FlushReadHandle(name).GetResult();
}
bool FileServerActObjImpl::Preload(
  FileRequestCompletionHandle &request, RequestableObject *obj, RequestableObject::RequestContext *ctx,
  FileRequestPriority priority, FileServerHandle handle, QFileSize start, QFileSize size
)
{
  // TODO: move implementation from base here
  return base::Preload(request,obj,ctx,priority,handle,start,size);
}
PosQIStreamBuffer FileServerActObjImpl::Read(FileServerHandle handle, QFileSize start, QFileSize size)
{
  // TODO: move implementation from base here
  return base::Read(handle,start,size);
}

PosQIStreamBuffer FileServerActObjImpl::RequestResultAsync(FileRequestCompletionHandle &request, FileServerHandle handle, QFileSize pos)
{
  Ref<RequestCompletion> reqComplete = static_cast<RequestCompletion *>(request.GetRef());
  if (reqComplete && reqComplete->completed)
  {
    FileRequestCompletionHandle req = COMPLETION_FROM_REQUEST(reqComplete->request);
    return GFileServerFunctionsSync->RequestResultAsync(req,handle,pos);
}
  return PosQIStreamBuffer();
}

InFileLocation FileServerActObjImpl::RequestLocationAsync(const FileRequestCompletionHandle &req)
{
  Ref<RequestCompletion> reqComplete = static_cast<RequestCompletion *>(req.GetRef());
  if (reqComplete && reqComplete->completed)
  {
    FileRequestCompletionHandle req = COMPLETION_FROM_REQUEST(reqComplete->request);
    return GFileServerFunctionsSync->RequestLocationAsync(req);
}
  InFileLocation loc;
  loc._handle = NULL;
  loc._start = 0;
  loc._size = 0;
  return loc;
}

FileJournalType FileJournal;

FileServerHandle FileServerActObjImpl::OpenReadHandle(const char *name)
{
  PROFILE_SCOPE_EX(fsAOp,file);
  FileJournal.Add(Format("Open %s",cc_cast(name)));
  return base::OpenReadHandle(name).GetResult();
}
void FileServerActObjImpl::MakeReadHandlePermanent(FileServerHandle handle)
{
  GFileServerFunctionsSync->MakeReadHandlePermanent(handle);
}
RString FileServerActObjImpl::GetHandleName(FileServerHandle handle) const
{
  // handle name is thread safe
  return GFileServerFunctionsSync->GetHandleName(handle);
  //return unconst_cast(this)->base::GetHandleName(handle).GetResult();
}
void FileServerActObjImpl::CloseReadHandle(FileServerHandle handle)
{
  PROFILE_SCOPE_EX(fsACl,file);
  FileJournal.Add(Format("Close %s",cc_cast(GetHandleName(handle))));
  base::CloseReadHandle(handle);
}
bool FileServerActObjImpl::CheckReadHandle(FileServerHandle handle) const
{
  PROFILE_SCOPE_EX(fsACh,file);
  return unconst_cast(this)->base::CheckReadHandle(handle).GetResult();
}
int FileServerActObjImpl::GetPageSize()
{
  return GetPageRecommendedSize();
}
QFileTime FileServerActObjImpl::TimeStamp(FileServerHandle handle) const
{
  return unconst_cast(this)->base::TimeStamp(handle).GetResult();
}

Future<QFileTime> FileServerActObjImpl::RequestTimeStamp(const char *name)
{
  FileServerHandle handle = OpenReadHandle(name);
  Future<QFileTime> ret = base::TimeStamp(handle);
  CloseReadHandle(handle);
  return ret;
}

QFileSize FileServerActObjImpl::GetFileSize(FileServerHandle handle) const
{
  return unconst_cast(this)->base::GetFileSize(handle).GetResult();
}
#ifdef _WIN32
void *FileServerActObjImpl::GetWinHandle(FileServerHandle handle) const
{
  return unconst_cast(this)->base::GetWinHandle(handle).GetResult();
}
#endif


bool FileServerActObjImpl::OtherThreadsCanUse() const
{
  return true;
}



FileServerActObjImpl GAsyncFileServer;
