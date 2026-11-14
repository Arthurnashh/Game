// ArchiveStreamTCP.h: interface for the ArchiveStreamTCP class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ARCHIVESTREAMTCP_H__58E062DE_7BB5_4F4A_98CD_73604BA7A862__INCLUDED_)
#define AFX_ARCHIVESTREAMTCP_H__58E062DE_7BB5_4F4A_98CD_73604BA7A862__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <winsock.h>
#include "ArchiveStream.h"

#define ARCHTCP_NOTSUPPORTED -1001
#define ARCHTCP_CONNECTIONLOST -1002
#define ARCHTCP_TIMEOUT -1003
#define ARCHTCP_SOCKETWAITERROR -1004
#define ARCHTCP_CANCELED -1005

#ifdef AFX_SOCKET_H__2B60EC2B_12BB_49EB_B182_1E352C09B6B3__INCLUDED_
#ifndef USESOCKETCLASS
#pragma message (" warning :To use ArchiveStreamTCP with Socket class, define USESOCKETCLASS symbol in project settings")
#endif
#endif

#ifdef USESOCKETCLASS
#define SOCKETTYPE Socket
#else
#define SOCKETTYPE SOCKET
#endif

class ArchiveStreamTCP;


///Wait function, it is called when waiting failed with timeout. 
/**Application may handle timeout by own or for watch state of connection. If timeout is sets
to zero, it may simulate polling or idle processing. Finnaly, application may terminate waiting, 
anytime.
@param caller pointer to object, that calling this function. It is usefull, to identify connection 
while watching more than one.
@param context pointer to application defined context data, that is passed to the object during construction.
@param counter counter of fails. Everytime is function called, this counter is incerased.
@return Function can return one of this results. 
            (positive_value) = Waits specified miliseconds and called function again, if no data arrived.
                 Function can used caller->GetTimeout() to get preset timeout value
            0 = check state of connection and call function again, if no data arrived.
           -1 = terminates waiting, and reports error ARCHTCP_CANCELED         
*/        
        
typedef long (*ASTCP_WaitFunction)(ArchiveStreamTCP *caller, void *context, int counter);



class ArchiveStreamTCP : public ArchiveStream  
{
protected:
  SOCKETTYPE _link;
  struct timeval _timeout;
  bool _autoclose;
  int _err;
  ASTCP_WaitFunction _wait_funct;
  void *_waitcontext;
    virtual bool HandleTimeout(struct timeval &_timeout, int counter);

public:
	ArchiveStreamTCP(SOCKETTYPE link, unsigned long timeout, bool autoclose,ASTCP_WaitFunction wait_funct=NULL,void *context=NULL);
	virtual ~ArchiveStreamTCP();
    virtual int IsError() {return _err;}
    virtual void Reset() {_err=0;}
    virtual void SetError(int err) {_err=err;}
    virtual __int64 Tell();
    virtual void Seek(__int64 lOff, SeekOp seekop);
    DWORD GetTimeout() {return _timeout.tv_sec*1000+_timeout.tv_usec/1000;}
    
};

class ArchiveStreamTCPIn: public ArchiveStreamTCP
  {
  public:
    ArchiveStreamTCPIn(SOCKETTYPE link, unsigned long timeout, bool autoclose,ASTCP_WaitFunction wait_funct=NULL,void *context=NULL):
		ArchiveStreamTCP(link,timeout,autoclose,wait_funct,context) {}
    virtual bool IsStoring() {return false;}
    virtual int DataExchange(void *buffer, int maxsize);

  };

class ArchiveStreamTCPOut: public ArchiveStreamTCP
  {
  public:
	ArchiveStreamTCPOut(SOCKETTYPE link, unsigned long timeout, bool autoclose,ASTCP_WaitFunction wait_funct=NULL,void *context=NULL):
		ArchiveStreamTCP(link,timeout,autoclose,wait_funct,context) {}
    virtual bool IsStoring() {return true;}
    virtual int DataExchange(void *buffer, int maxsize);

  };



#endif // !defined(AFX_ARCHIVESTREAMTCP_H__58E062DE_7BB5_4F4A_98CD_73604BA7A862__INCLUDED_)
