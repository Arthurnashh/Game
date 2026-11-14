#include "wpch.hpp"
#include <El/FileServer/fileServer.hpp>

#if 0
//////////////////////////////////////////////////////////////////////////
/// example - requestable object using single callback
class SimpleRequestableObject: public RequestableObject
{
  RString _filename;

  public:
  void RequestDone(RequestContext *context, RequestResult reason);
  void Request(FileRequestPriority priority);
};

void SimpleRequestableObject::Request(FileRequestPriority priority)
{
  // using QStream
  QIStream in;
  // ask for pre-read
  in.PreReadObject(this, NULL, FileRequestPriority(300));

  // or directly, using low-level interface
  Ref<RefCount> request;
  GFileServerFunctions->Preload(request,this,NULL,priority,fileHandle,fileStart,fileOffset)
}


void SimpleRequestableObject::RequestDone(RequestContext *context, RequestResult reason)
{
  // data are ready to be loaded - load them using normal I/O
}


//////////////////////////////////////////////////////////////////////////
/// example - requestable object using multiple callbacks
class ComplexRequestableObject: public RequestableObject
{
  RString _filename1;
  RString _filename2;

  typedef void (ComplexRequestableObject::*Callback)(RequestResult reason);
  class ComplexRequestContext: public RequestContext
  {
    public:
    Callback _callback;
    ComplexRequestContext(Callback callback):_callback(callback){}
  };
  public:
  void RequestDone(RequestContext *context, RequestResult reason);
  void RequestDone1(RequestResult reason);
  void RequestDone2(RequestResult reason);
  void Request(FileRequestPriority priority);
};

void ComplexRequestableObject::Request(FileRequestPriority priority)
{
  FileRequestHandle req1 = GFileServer->Request(priority, _filename1);
  FileRequestHandle req2 = GFileServer->Request(priority, _filename2);
  GFileServer->AddHandler(
    req1,this,new ComplexRequestContext(&ComplexRequestableObject::RequestDone1)
  );
  GFileServer->AddHandler(
    req2,this,new ComplexRequestContext(&ComplexRequestableObject::RequestDone2)
  );
}


void ComplexRequestableObject::RequestDone(RequestContext *context, RequestResult reason)
{
  // handler is stored in the context
  ComplexRequestContext *ctx = static_cast<ComplexRequestContext *>(context);
  (this->*ctx->_callback)(reason);
}

void ComplexRequestableObject::RequestDone1(RequestResult reason)
{
  // data loaded - part 1
}

void ComplexRequestableObject::RequestDone2(RequestResult reason)
{
  // data loaded - part 2
}
#endif
