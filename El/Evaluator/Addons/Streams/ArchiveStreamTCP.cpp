// ArchiveStreamTCP.cpp: implementation of the ArchiveStreamTCP class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <Es/Common/win.h>
#ifdef USESOCKETCLASS
#include <el/TCPIPBasics/Socket.h>
#endif
#include "ArchiveStreamTCP.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ArchiveStreamTCP::ArchiveStreamTCP(SOCKETTYPE link, unsigned long timeout, bool autoclose,ASTCP_WaitFunction wait_funct,void *context):
_wait_funct(wait_funct),_waitcontext(context)
{
  u_long nonblock=1;
  _link=link;
  _autoclose=autoclose;
  _timeout.tv_sec=timeout/1000;
  _timeout.tv_usec=(timeout%1000)*1000;
  Reset();
  ioctlsocket(_link,FIONBIO,&nonblock); //set nonblocking mode
}

ArchiveStreamTCP::~ArchiveStreamTCP()
{
  if (_autoclose) closesocket(_link);
}

__int64 ArchiveStreamTCP::Tell()
{
  if (_err) return -1;
  _err=ARCHTCP_NOTSUPPORTED;
  return -1;
}

void ArchiveStreamTCP::Seek(__int64 lOff, SeekOp seekop)
{
  if (_err) return;
  _err=ARCHTCP_NOTSUPPORTED;
  return;
}


int ArchiveStreamTCPIn::DataExchange(void *buffer, int maxsize)
{
  struct timeval localtm=_timeout;
  int save=maxsize;
  int counter=0;
  if (_err) return _err;
  char *ptr=(char *)buffer;
  while (maxsize)
  {
    int rcv=recv(_link,ptr,maxsize,0);
    if (rcv==0) 
    {
      _err=ASTRERR_EOF;      //connection lost, notify end of file
      return _err;
    }
    else if (rcv==-1)
    {
      if (WSAGetLastError()==WSAEWOULDBLOCK)
      {
        struct fd_set rdset;
        FD_ZERO(&rdset);
        FD_SET(_link,&rdset);
        int res=select(1,&rdset,NULL,NULL,&localtm);
        if (res==0)  
        {
          if (!HandleTimeout(localtm,counter++)) {_err=ARCHTCP_TIMEOUT; return _err;}
          if (_err) return _err;
        }        
        if (res==SOCKET_ERROR) {_err=ARCHTCP_SOCKETWAITERROR;return _err;}
      }
      else
      {
        _err=WSAGetLastError();
        return _err;
      }
    }
    else 
    {
      maxsize-=rcv;
      ptr+=rcv;
    }
  }
  return _err;
}

int ArchiveStreamTCPOut::DataExchange(void *buffer, int maxsize)
{
  struct timeval localtm=_timeout;
  int counter=0;
  if (_err) return _err;
  char *ptr=(char *)buffer;
  while (maxsize)
  {
    int rcv=send(_link,ptr,maxsize,0);
    if (rcv==0) 
    {
      _err=ARCHTCP_CONNECTIONLOST;
      return _err;
    }
    else if (rcv==-1)
    {
      if (WSAGetLastError()==WSAEWOULDBLOCK)
      {
        struct fd_set wrset;
        FD_ZERO(&wrset);
        FD_SET(_link,&wrset);
        int res=select(1,NULL,&wrset,NULL,&localtm);
        if (res==0)  
        {
          if (!HandleTimeout(localtm,counter++)) {_err=ARCHTCP_TIMEOUT; return _err;}
          if (_err) return _err;
        }        
        if (res==SOCKET_ERROR) {_err=ARCHTCP_SOCKETWAITERROR;return _err;}
      }
      else
      {
        _err=WSAGetLastError();
        return _err;
      }
    }
    else 	  
    {
      maxsize-=rcv;
      ptr+=rcv;
    }
  }
  return _err;
}

bool ArchiveStreamTCP::HandleTimeout(struct timeval &timeout, int counter)
{
  if (_wait_funct==NULL) return false;
  long nwtm=_wait_funct(this,_waitcontext,counter);
  if (nwtm==-1) {SetError(ARCHTCP_CANCELED);return true;}
  timeout.tv_sec=nwtm/1000;
  timeout.tv_usec=nwtm*1000;
  return true;
}
