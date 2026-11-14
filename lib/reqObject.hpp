#ifdef _MSC_VER
#pragma once
#endif

#ifndef _REQ_OBJECT_HPP
#define _REQ_OBJECT_HPP

// determine which kind of links is used to link to objects
#include <Es/Types/lLinks.hpp>
#include "fileServer.hpp"

/// object which can be requested to preload
class RequestableObject: public RemoveLLinks
{
  public:
  class RequestContext {};

  /// result of file loading request
  enum RequestResult
  {
    RRSuccess,
    RRFailed,
    RRCanceled
  };

  virtual void RequestDone(
    FileRequestHandle req, RequestContext *context, RequestResult result
  ) = NULL;
};

/// contains information needed to call the request done handler
struct RequestableObjectRequestInfo
{
  LLink<RequestableObject> _object;
  SRef<RequestableObject::RequestContext> _context;
};

TypeIsMovableZeroed(RequestableObjectRequestInfo)


/*
class FileRequest // ...
{
  //...

  AutoArray<RequestableObjectRequestInfo> _callbacks;

  public:
  void AddCallback(
    RequestableObject *object,
    RequestableObject::RequestContext *context
  );
  void ProcessCallbacks(RequestableObject::RequestResult reason);
};


void FileRequest::AddCallback(
  RequestableObject *object,
  RequestableObject::RequestContext *context
)
{
  RequestableObjectRequestInfo &item = _callbacks.Append();
  item._object = object;
  item._context = context;
}

void FileRequest::ProcessCallbacks(RequestableObject::RequestResult reason)
{
  // if the object still exists, report the result
  if (item._object)
  {
    for (int i=0; i<_callbacks.Size(); i++)
    {
      const RequestableObject::CallbackInfo &item = _callbacks[i];
      item._object->RequestDone(item._context,reason);
    }
  }
  _callbacks.Clear();
}

*/

#endif