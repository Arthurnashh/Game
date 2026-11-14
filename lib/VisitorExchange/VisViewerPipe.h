#ifdef _MSC_VER
#pragma once
#endif

#ifndef __VIS_VIEVER_PIPE
#define __VIS_VIEVER_PIPE

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <El/TCPIPBasics/SharedResource.h>
#include "visviewerexchange.h"



///Implements interface using the pipes
class VisViewerPipe : public VisViewerExchange
{  
  class PipeHandleTraits
  {
  public:
    static void Release(HANDLE h)
    {
      if (h!=0 && h!=INVALID_HANDLE_VALUE) CloseHandle(h);
    }
  };

  SharedResource<HANDLE,PipeHandleTraits> _send,_receive;

  char *_outbuff;
  size_t _bufsize;
  int _bufused;

  char *_inbuffer;
  int _inbufsz;
  int _inbufptr;
  int _inbufmax;

  Status FlushBuffer();

  Status ReadPipe(void *ptr, int size);
  Status PeekPipe();
  Status FillUpBuffer();

public:
  VisViewerPipe(IVisViewerExchangeDocView &event);
  virtual ~VisViewerPipe(void);

  void Create(HANDLE send, HANDLE receive);
  bool Create(WPARAM wParam, LPARAM lParam);
  ///
  /*
  @params params 0x0HWND,0x0MESSAGE
  */
  bool Create(const char *params);


  Status SendMessage(const SPosMessage &sMsg);
  Status ReceiveMessage(PSPosMessage &pMsg);
  Status WaitMessage(const Array<int> &msgIds, PSPosMessage& msg, unsigned long timeout);
  Status CheckInterfaceStatus();

  Status BlockAndWait();
  virtual Status DispatchMessage(const SPosMessage &msg);

  void SetInBuffer(int size);

  virtual Status StartBuffer(size_t bufferSize);
  virtual Status EndBuffer();

};

#endif