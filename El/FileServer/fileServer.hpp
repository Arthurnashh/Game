#ifdef _MSC_VER
#pragma once
#endif

#ifndef _FILE_SERVER_HPP
#define _FILE_SERVER_HPP

#include <El/QStream/qStream.hpp>
#include <Es/Types/lLinks.hpp>
#include <Es/Threads/multisync.hpp>
#include <El/ActiveObject/activeObject.hpp>

#if defined _WIN32
#include <Es/Common/win.h>
#endif

// file buffering class, suitable for background (multithreaded) usage

class QFBank;


struct _SECURITY_ATTRIBUTES;

/// Interface for platform specific functions
class FileServerFunctions
{
public:
  virtual void ProcessFCPHManager(const char *name) = 0;
  virtual RString GetDamagedDiscErrorString() = 0;
#ifdef _WIN32
  virtual void ReportFileReadError(HRESULT err, const char *name, bool terminate=true) = 0;
  virtual void ProgressRefresh() = 0;
#endif
};

/// abstract class - single/multi threaded file loading
class FileServer: public RefCount
{
	public:
  static FileServerFunctions *GFileServerFunctions;

  //! preload into the cache, request handle is owned by the server
	virtual bool Preload
  (
    FileRequestPriority priority, const char *name, QFileSize start=0, QFileSize size=QFileSizeMax
  ) = 0;
  //! preload into the cache, request handle is owned by the server
	virtual bool Preload
  (
    FileRequestPriority priority, FileServerHandle handle, QFileSize start=0, QFileSize size=QFileSizeMax
  ) = 0;

  //! request
	virtual FileRequestHandle Request
  (
    FileRequestPriority priority, const char *name, QFileSize start=0, QFileSize size=QFileSizeMax
  ) = 0;
  //! request
	virtual FileRequestHandle Request
  (
    RequestableObject *obj, RequestableObject::RequestContext *ctx,
    FileRequestPriority priority, FileServerHandle handle, QFileSize start=0, QFileSize size=QFileSizeMax
  ) = 0;
  //! cancel request
	virtual void CancelRequest(FileRequestHandle handle) = 0;

  //! check if given request is done
  virtual bool WaitUntilDone(FileRequestHandle handle, int timeout) = 0;
  //! check if given request is done
  /** version to be used from other threads */
  virtual bool RequestIsDone(FileRequestHandle handle) = 0;
  
  //! get part of the request result
  /** version to be used from other threads */
  virtual bool RequestIsDoneAsync(FileRequestHandle handle) = 0;

  /// wait until one request is done
  /** Can be handy during batch preloads, where we wait for multiple files simultaneously.
  Unless there was a request satisfied, we know there is nothing to process.
  */
  virtual void WaitForOneRequestDone(int timeout) = 0;
  /// start processing preload requests, even non-urgent ones
  virtual void SubmitRequestsMinimizeSeek() = 0;

  virtual SignaledObject &GetUpdateEvent() = 0;


  /// add a handler to be called once the request is finished
  virtual void AddHandler(
    FileRequestHandle handle,
    RequestableObject *object,
    RequestableObject::RequestContext *context
  ) = 0;

  //! open file - this can be used as a hint to preread it
  //virtual void Open(QIFStream &in, const char *name, QFileSize from=0, QFileSize to=QFileSizeMax) = 0;

	virtual void Init()= 0; //!< init (may include data preloading)
  virtual void OpenPreloadLog(const char *name) = 0; //!< start preload monitoring
  virtual void SavePreloadLog() = 0; //!< called when preload monitoring is finished
  virtual void OpenDVDLog(const char *name) = 0; //!< start preload monitoring
  virtual void SaveDVDLog() = 0; //!< called when preload monitoring is finished
  virtual void PreloadDVD(RString name) = 0;
  virtual void WaitForRequestsDone() = 0;
	virtual void Start()= 0; //!< start/stop background activity
	virtual void Stop()= 0;

	virtual void Update()= 0;
	
  virtual void UpdateByThread(int threadName) = 0;
	
	/// debugging - log content of the request waiting before a given one
	virtual void LogWaitingRequests(FileRequestHandle req) = 0;

  /// diagnostics - relative status of the preload queue
  /** 1 means queue is very full */
  virtual float PreloadQueueStatus() const {return 0;}
  /// how many requests are there in the preload queue
  virtual int PreloadQueueRequests(int *size=NULL) const {if (size) *size = 0;return 0;}
  //! set max. allowed size of cached buffers
  virtual void SetMaxSize(size_t maxCacheSize) = 0;
  //! release all
  virtual void Clear() = 0;
  //! called when manual destructor is necessary (when FileServer is static variable, we need to have the control over order of destruction)
  virtual void Finalize() {};


	virtual void FlushBank(QFBank *bank) = 0;
  
  /// verify internal structures are not corrupted - introduced when debugging nasty memory corruption on Xbox
  virtual bool CheckIntegrity() const {return true;}

  virtual Future<QFileTime> RequestTimeStamp(const char *name) = 0;

#ifdef _WIN32
  virtual HANDLE CreateFileLongPath(
    const char *lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    _SECURITY_ATTRIBUTES *lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    FileServerHandle hTemplateFile) = 0;
#endif
#if POSIX_FILES_COMMON
  virtual int CreateFileLongPath(const char *lpFileName, int oflag, int pmode) = 0;
#endif
};

extern FileServer *GFileServer;

// Global functions
void CreatePath(RString path);

#endif
