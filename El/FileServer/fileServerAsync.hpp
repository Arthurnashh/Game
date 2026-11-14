#ifdef _MSC_VER
#pragma once
#endif

#ifndef _FILE_SERVER_ASYNC_HPP
#define _FILE_SERVER_ASYNC_HPP

#define _HELISIM_OFFTREEREGION_64   1

#ifdef _WIN32

#include <El/elementpch.hpp>
#include <El/FileServer/fileServer.hpp>
#include <El/FileServer/fileServerMT.hpp>
#include <El/ActiveObject/activeObject.hpp>
#include <Es/Strings/bString.hpp>
#include "Es/Containers/offTree.hpp"

struct RequestCompletionKey
{
  FileServerHandle _handle;
  QFileSize _start;
  QFileSize _size;
  LLink<RequestableObject> _obj;
  RequestableObject::RequestContext *_ctx;
};
struct RequestCompletion: public FileRequestCompletionRefCount, public RequestCompletionKey, private NoCopy
{
  // TODO: move to ActObj
  struct StoredRequest: public SLIST_ENTRY
  {
    FileRequestHandle request;
    
    StoredRequest(const FileRequestHandle &req):request(req){}
  };
  
  FileRequestHandle request;
  FileRequestPriority priority;
  bool completed;
  int frameId;

  
  RequestCompletion():completed(false){}
  ~RequestCompletion();
  
  bool RequestContains(RequestableObject * obj, RequestableObject::RequestContext * ctx, FileRequestPriority prio, FileServerHandle handle, QFileSize start, QFileSize size) 
  {
    return (
      (!obj && !ctx || obj==_obj && ctx==_ctx) && prio>=priority &&
      handle==_handle && start>=_start && start+size<=_start+_size
    );
  }
  void Refresh();
};

typedef BString<512> StorePath;


/// list of misc. objects waiting to be destroyed by the worker thread
extern StoredRefContainer FileServerAsyncRefToDestroyWorkerThread;

/// list of items waiting to be destroyed by the main thread
extern StoredRefContainer FileServerAsyncRefToDestroyMainThread;

template <class Type>
class StoredRefMainThread: public StoredRef<Type,&FileServerAsyncRefToDestroyMainThread>
{
  typedef StoredRef<Type,&FileServerAsyncRefToDestroyMainThread> base;
  
  public:
  StoredRefMainThread(){}
  StoredRefMainThread(Type *src):base(src){}
};

/// list of items waiting to be destroyed by the worker thread
template <class Type>
class StoredRefWorkerThread: public StoredRef<Type,&FileServerAsyncRefToDestroyWorkerThread>
{
  typedef StoredRef<Type,&FileServerAsyncRefToDestroyWorkerThread> base;
  
  public:
  StoredRefWorkerThread(){}
  StoredRefWorkerThread(Type *src):base(src){}
};

#define FILE_SERVER_MESSAGES_STD(CTX,MSG) \
  MSG(CTX,FileServerHandle,OpenReadHandle,(const char *arg1),1,StorePath arg1) \
  MSG(CTX,void,MakeReadHandlePermanent,(FileServerHandle arg1),1,FileServerHandle arg1) \
  /*MSG(CTX,StorePath,GetHandleName,(FileServerHandle arg1),1,FileServerHandle arg1)*/ \
  MSG(CTX,void,CloseReadHandle,(FileServerHandle arg1),1,FileServerHandle arg1) \
  MSG(CTX,bool,CheckReadHandle,(FileServerHandle arg1),1,FileServerHandle arg1) \
  MSG(CTX,RString,GetHandleName,(FileServerHandle arg1),1,FileServerHandle arg1) \
  MSG(CTX,QFileTime,TimeStamp,(FileServerHandle arg1),1,FileServerHandle arg1) \
  MSG(CTX,QFileSize,GetFileSize,(FileServerHandle arg1),1,FileServerHandle arg1) \
  MSG(CTX,void *,GetWinHandle,(FileServerHandle arg1),1,FileServerHandle arg1) \



#define FILE_SERVER_MESSAGES_SPEC(CTX,MSG) \
  MSG(CTX,bool,FlushReadHandle,(const char *arg1),1,StorePath arg1) \
  MSG(CTX,void,PreloadAsync, \
    (RequestCompletion *arg1, RequestableObject *arg2, RequestableObject::RequestContext *arg3, FileRequestPriority arg4, FileServerHandle arg5, QFileSize arg6, QFileSize arg7), \
    7, \
    Ref<RequestCompletion> arg1; StoredRefMainThread<RequestableObject> arg2; RequestableObject::RequestContext *arg3; FileRequestPriority arg4; FileServerHandle arg5; QFileSize arg6; QFileSize arg7 \
  ) \
  MSG(CTX,bool,PreloadSync, \
    (RequestableObject *arg1, RequestableObject::RequestContext *arg2, FileRequestPriority arg3, FileServerHandle arg4, QFileSize arg5, QFileSize arg6), \
    6, \
    StoredRefMainThread<RequestableObject> arg1; RequestableObject::RequestContext *arg2; FileRequestPriority arg3; FileServerHandle arg4; QFileSize arg5; QFileSize arg6 \
  ) \
  MSG(CTX,PosQIStreamBuffer,ReadAsync,(FileServerHandle arg1, QFileSize arg2, QFileSize arg3),3,FileServerHandle arg1; QFileSize arg2; QFileSize arg3) \
  MSG(CTX,WaitableVoid,WaitForOneRequestDone,(int arg1),1,int arg1;) \
  MSG(CTX,WaitableVoid,SubmitRequestsMinimizeSeek,(),0,;) \
  MSG(CTX,WaitableVoid,FlushServer,(),0,;) \


#define FILE_SERVER_MESSAGES(CTX,MSG) FILE_SERVER_MESSAGES_STD(CTX,MSG) FILE_SERVER_MESSAGES_SPEC(CTX,MSG)

FILE_SERVER_MESSAGES(FileServerAsync,ACTOBJ_DECLARE_ARGS)

/// async. file server servant implementation
class FileServerAsync
{
  protected:

  SListMT<RequestCompletion::StoredRequest> _toDestroy;
  
  public:
  
  DECLARE_ACTIVE_OBJECT_SERVANT(FileServerAsync,FILE_SERVER_MESSAGES)


  protected:
  void FreeStoredArgs();
  void AddToDestroy(RequestCompletion::StoredRequest *store) {_toDestroy.Push(store);}

  virtual void DoFlush() = 0;
};

extern QIFileServerFunctions *GFileServerFunctionsSync;

template <>
struct OffTreeRegionTraits< Ref<RequestCompletion> >
{
#if _HELISIM_OFFTREEREGION_64
  /**
  we need to be able to interpret negative numbers, and we need to handle 2 GB and more => hence 64b int
  */
  typedef OffTreeRegion<long long> Region;
#else
  // <2 GB seems enough for us now
  typedef OffTreeRegion<int> Region;
#endif

  /// get corresponding region  
  static Region GetRegion(const RequestCompletion *item)
  {
    return Region(item->_start,0,item->_start+item->_size,1);
  }

  static bool IsIntersection(const RequestCompletion *item, const Region &q)
  {
    return Region::NumericTraits::Max(item->_start,q._begX)<Region::NumericTraits::Min(item->_start+item->_size,q._endX);
  }
  
  static bool IsEqual(const RequestCompletion *item1, const RequestCompletion *item2)
  {
    return item1->_start==item2->_start && item1->_size==item2->_size && item1->_obj==item2->_obj && item1->_ctx==item2->_ctx;
  }

};

/// store all requests connected with a single file
class FileRequests: public RefCount
{
  FileServerHandle _file;

  struct RequestCompletionBinFindArrayKeyTraits
{
  typedef const RequestCompletionKey &KeyType;

	  static KeyType GetKey(const Ref<RequestCompletion> &type) {return *type;}

    static bool IsEqual(KeyType a, KeyType b)
  {
	    if (a._start!=b._start) return false;
	    if (a._size!=b._size) return false;
      int diff = a._obj.CompareTrackers(b._obj);
	    if (diff) return false;
	    if (a._ctx!=b._ctx) return false;
      return true;
  }
    static bool IsLess(KeyType a, KeyType b)
    {
	    if (a._start<b._start) return true;
	    if (a._start>b._start) return false;
	    if (a._size<b._size) return true;
	    if (a._size>b._size) return false;
      int diff = a._obj.CompareTrackers(b._obj);
	    if (diff < 0) return true;
	    if (diff > 0) return false;
	    if (a._ctx<b._ctx) return true;
	    if (a._ctx>b._ctx) return false;
		  return false;
    }
  };

  typedef BinFindArrayKey< Ref<RequestCompletion>, RequestCompletionBinFindArrayKeyTraits> RequestsArray;
  
  #if _DEBUG
  RequestsArray _requests;
  #endif
  
  OffTree< Ref<RequestCompletion> > _requestsOffTree;
  
  public:
  FileRequests(const FileServerHandle &handle, QFileSize size):_file(handle),_requestsOffTree(0,0,size){}
  
  const FileServerHandle &GetKey() const {return _file;}
  
  RequestCompletion *Find(const RequestCompletionKey &key) const;
  RequestCompletion *FindContaining(const RequestCompletionKey &key, FileRequestPriority prio) const;
  
  bool RequestCheckItegrity() const;
  
  void Add(RequestCompletion *item)
  {
    #if _DEBUG
    Assert(RequestCheckItegrity());
    Assert(_requests.Size()==_requestsOffTree.RegionCount());
    Verify(_requests.AddUnique(item)>=0);
    #endif
    _requestsOffTree.Add(item);
    Assert(_requests.Size()==_requestsOffTree.RegionCount());
    Assert(RequestCheckItegrity());
  }
  
  bool IsEmpty() const
  {
    Assert(_requests.Size()==_requestsOffTree.RegionCount());
    //return _requests.Size()==0;
    return _requestsOffTree.IsEmpty();
  }

  void Remove(RequestCompletion *item)
  {
    #if _DEBUG
    Assert(_requests.Size()==_requestsOffTree.RegionCount());
    _requests.Delete(item);
    #endif
    _requestsOffTree.Remove(item);
    Assert(_requests.Size()==_requestsOffTree.RegionCount());
  }
  
  template <class Functor>
  void ForEachWithDelete(const Functor &f)
  {
    #if _DEBUG
    Assert(_requests.Size()==_requestsOffTree.RegionCount());
    _requests.ForEachWithDelete(f);
    #endif
    _requestsOffTree.ForEach(f);
    Assert(_requests.Size()==_requestsOffTree.RegionCount());
  }
};

template <>
struct MapClassTraits< Ref<FileRequests> >: public DefMapClassTraits< Ref<FileRequests> >
{
  typedef const FileServerHandle &KeyType;

  static unsigned int CalculateHashValue(KeyType key)
  {
    // pointers are always a multiple of 4 - discard the two lower bits
    return intptr_t(key)>>2;
  }

  static int CmpKey(KeyType k1, KeyType k2) {return k1!=k2;}
  
  static KeyType GetKey(const FileRequests *item) {return item->GetKey();}

  template <class Container>
  static int CleanUp(Container &array)
  {
    struct DeleteNullRequests
    {
      bool operator () (FileRequests *item) const
      {
        return item->IsEmpty();
      }
    };
    int oldCount = array.Size();
    // remove any targets with idExact = NULL
    array.ForEachWithDelete(DeleteNullRequests());
    // return how many items did we remove
    return oldCount- array.Size();
  }
};

/// interface similar to QIFileServerFunctions
class FileServerActObj: public ActiveObject<FileServerAsync>
{
  protected:
    
  bool _workerRunning;  
  SignaledObject _worker;
  volatile bool _terminate;
  
  CriticalSection _lockReqCache;
  
  #if 0 // no thread safe - if we want to enable it, thread safety (e.g. per-thread cache) is required
  /// which handle is _lastBuffer for
  FileServerHandle _lastBufferHandle;
  /// cached last buffer used for FileServerActObj::Read, to prevent repeated submission of the same read
  PosQIStreamBuffer _lastBuffer;
  #endif
  
  typedef MapStringToClass< Ref<FileRequests>, RefArray<FileRequests> > RequestCache;
  RequestCache _reqCache;
    
  void Worker();

  void DoFlush();

  
  static DWORD WINAPI WorkerCallback(void *ctx)
  {
    ((FileServerActObj *)ctx)->Worker();
    return 0;
  }
  
  public:
  FILE_SERVER_MESSAGES(FileServerAsync,ACTOBJ_WRAP_FUNC)
  
  int GetPageSize() const {return GetPageRecommendedSize();}
  
  bool IsDone(RefCount *request) const;

  /// wrapper around PreloadAsync, so that it can be used like QIFileServerFunctions::Preload
  bool Preload(
    FileRequestCompletionHandle &request, RequestableObject *obj, RequestableObject::RequestContext *ctx,
    FileRequestPriority priority, FileServerHandle handle, QFileSize start, QFileSize size
  );

  /// wrapper around ReadAsync, so that it can be used like QIFileServerFunctions::Read
  PosQIStreamBuffer Read(FileServerHandle handle, QFileSize start, QFileSize size);

  FileServerActObj();
  ~FileServerActObj();
  
  void StartWorkerThread();
  void StopWorkerThread();
  void FlushWorkerThread();
  
  bool DoWork(int timeout);

  void Maintain();
  void MaintainMainThread();
  void DiscardOldRequests();

  using FileServerAsync::AddToDestroy;
};

/** TODO: check if this could be implemented directly in FileServerActObj */
class FileServerActObjImpl: public FileServerActObj, public QIFileServerFunctions
{
  typedef FileServerActObj base;
  
  public:
  virtual bool FlushReadHandle(const char *name);
  virtual bool Preload(
    FileRequestCompletionHandle &request, RequestableObject *obj, RequestableObject::RequestContext *ctx,
    FileRequestPriority priority, FileServerHandle handle, QFileSize start, QFileSize size
  );
  virtual PosQIStreamBuffer Read(FileServerHandle handle, QFileSize start, QFileSize size);

  virtual PosQIStreamBuffer RequestResultAsync(FileRequestCompletionHandle &request, FileServerHandle handle, QFileSize pos);
  virtual InFileLocation RequestLocationAsync(const FileRequestCompletionHandle &req);
  virtual FileServerHandle OpenReadHandle(const char *name);
  virtual void MakeReadHandlePermanent(FileServerHandle handle);
  virtual RString GetHandleName(FileServerHandle handle) const;
  virtual void CloseReadHandle(FileServerHandle handle);
  virtual bool CheckReadHandle(FileServerHandle handle) const;
  virtual int GetPageSize();
  virtual QFileTime TimeStamp(FileServerHandle handle) const;
  virtual Future<QFileTime> RequestTimeStamp(const char *name);
  virtual QFileSize GetFileSize(FileServerHandle handle) const;
#ifdef _WIN32
  virtual void *GetWinHandle(FileServerHandle handle) const;
#endif
  virtual bool OtherThreadsCanUse() const;
};

extern FileServerActObjImpl GAsyncFileServer;

#if 0 // _PROFILE && USER_Ondra
typedef Journal FileJournalType;
#else
typedef DummyJournal FileJournalType;
#endif

extern FileJournalType FileJournal;


#endif

#endif
