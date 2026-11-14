// Socket.h: interface for the Socket class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SOCKET_H__2B60EC2B_12BB_49EB_B182_1E352C09B6B3__INCLUDED_)
#define AFX_SOCKET_H__2B60EC2B_12BB_49EB_B182_1E352C09B6B3__INCLUDED_

#ifdef _MSC_VER
#pragma once
#endif

#ifdef _WIN32
#include <winsock.h>
#endif
#include "SharedResource.h"

#pragma comment (lib,"Ws2_32.lib")

///This class envelops classic socket implementation
class Socket_Traits 
{ public : static void Release(SOCKET _s)
{
  if (_s!=INVALID_SOCKET) 
    closesocket(_s);
}
};

class Socket
{
  SharedResource<SOCKET,Socket_Traits> _s;  ///<Internal socket handle
public:

  ///Creates Socket from SOCKET
  /** @params s source socket handle. Do not close source socket. This will not duplicate
  handle, only share.
  */
  Socket(SOCKET s):_s(s) 
  {}

  ///Creates Socket from other Socket
  /** @params s source Socket object. Do not close source socket. This will not duplicate
  handle, only share.
  */
  Socket(const Socket &other):_s(other._s) 
  {}

  ///Creates empty object with no socket. 
  /** To prepare object, call Create function, to create socket. */      
  Socket(): _s(INVALID_SOCKET) 
  {}


  Socket(int type):_s(socket(AF_INET,type,0)) 
  {}

  ///Creates new socket. 
  /** Function creates new unbinded, unconnected socket. If socket already exists, closes it before.
  You shuld bind, or connect socket before any other use.
  @param type SOCK_STREAM - Creates TCP socket, SOCK_DGRAM - Creates UDP socket
  @return true, if socket were created, otherwise false. Call WSAGetLastError to determine error
  */
  bool Create(int type)
  {Close();_s=socket(AF_INET,type,0);return IsValid();}

  ///Attach socket handle to object. 
  /** Attached socket can be controled by Socket class same way as socket created using
  Create function. There is little diference: Attached socket must be detached before 
  object is destroyed, otherwise destructor may close source socked without any notify to
  the owner. Call Detach before destroing object.
  @param s socket handle to attach.
  */  
  SOCKET Attach(SOCKET s) 
  {_s=s;}

  ///Detach socket handle from object
  /** Detach socket handle, so destructor will not close the socket.
  @return function return handle of socket that was detached.
  */
  SOCKET Detach() 
  {SOCKET out=_s;_s.SetResource(INVALID_SOCKET); return out;}

  ///Single assigment.
  /** This works similar as Attach. Do not call close source socket. This will not duplicate
  handle, only share.
  @param s socket to attach using operator
  */
  Socket& operator=(SOCKET s) 
  {_s=s;return *this;}

  ///Single assigment.
  /** This works similar as Attach. Do not call close source socket. This will not duplicate
  handle, only share.
  @param other socket to attach using operator
  */
  Socket& operator=(const Socket &other) 
  {_s=other._s;return *this;}

  ///Operator provide convert, so object Socket can be used in functions, that excepting SOCKET.
  operator SOCKET() 
  {return _s;}

  ///Binds socket to address and port. 
  /** Function binds socket address and port. This is often used to bind port number to the
  listening sockets. To bind remote address, use Connect instead Bind. 
  @param sin Internet Address in SOCKADDR_IN type. 
  @return true, if socket were successfully binded. otherwise return false. Use WSAGetLastError to
  determine error.
  */
  bool Bind(const SOCKADDR_IN &sin)
  {return bind(_s,(SOCKADDR *)&sin,sizeof(sin))!=SOCKET_ERROR;}
  ///Sets Socket to the listen mode.
  /** Listening socket can accept incomming connections. This only works with SOCK_STREAM sockets
  (for SOCK_DGRAM, socket don't need to be listen mode).
  @param backlog maximum number connection in aqueue. Each new connection is placed into a
  queue, and waits until they are accepted. Call Accept to extract connection first in queue.
  If the queue is not long enough, every next connection is droped and reset. Maximum lenght
  of the queue is limited to SOMAXCONN constant and may depend on implementation of service provider.
  @return true, if socket is in listen mode, or false if an error occured. Use WSAGetLastError
  to determine the error*/
  bool Listen(int backlog=SOMAXCONN)
  {return listen(_s,backlog)!=SOCKET_ERROR;}

  ///Accepts incomming connection
  /** Function "pops" first connection peding in queue and accepts it. New connection
  is created as new Socket. New object is returned by calling this function.
  Origin Socket object may stay alive, and wait to next connections. Only new Socket object
  can be used for data exchange with remote computer. 
  @param sin reference to address type SOCKADDR_IN that receives remote address and port 
  of connection.
  @return New Socket object, that presents accepted connection. Returned object can be used
  for data exchange with the remote computer. If an error occured, invalid socket is returned.<br>
  NOTE: If no connection awaits, normally function blocks until new connection arrives. It 
  socket was set into nonblocking mode (SetNonBlocking), function will not wait, and return invalid socket.      
  */
  Socket Accept(SOCKADDR_IN &sin)
  {
    int sinlen=sizeof(sin);
    return Socket(accept(_s,(SOCKADDR *)&sin,&sinlen));
  }

  ///Function tries to connect to remote computer. 
  /**Network addres should exists, and remote computer has to have specified port open.
  Function will blocks until connect will successful, or until internal timeout will ellapse.
  if socket is set into noblocking mode (SetNonBlocking), function returns immediately.
  In this case, use Wait function to check that connection is complette.
  @param sin Internet Address and port number of computer to connect.
  @return true if connection is successful, or false if an error occured, or socket is
  in nonblocking mode.
  */
  bool Connect(SOCKADDR_IN &sin)
  {
    return connect(_s,(SOCKADDR *)&sin,sizeof(sin))!=SOCKET_ERROR;
  }
  ///Function returns true, if socket is valid
  /**@return true, if socket is valud */
  bool IsValid() const 
  {return _s!=INVALID_SOCKET && !(_s.IsClosed());}

  ///Operator returns true, if socket is NOT valid
  /**@return true, if socket is NOT valud */

  bool operator!() const  
  {return !IsValid();}

  ///Can compare two object, if they share the same socket handle.
  bool operator==(const Socket &other) const 
  {return _s==other._s;}
  ///Can compare two object, if they not share the same socket handle.
  bool operator!=(const Socket &other) const 
  {return _s!=other._s;}
  ///Function close connection and socket
  /** Function closes sockets, closes connection and frees any resource attached to
  the socket. If socket is shared, each object that share this socket will be closed.
  */
  void Close() 
  {_s.CloseResource();_s=INVALID_SOCKET;}
  ///Function read input socket stream
  /** 
  @param buff input buffer, that receives data
  @param len maximum length of of buffer in bytes
  @param flags flags defined in MSDN (recv), normally is set to zero
  @return number of bytes received. Function will wait until at least one character is
  received. Then returns number bytes received. If function returns zero, connection
  has been reset or closed by remote computer. If function returns -1, an error has occured.
  If socket is in nonblocking mode, function returns -1 when no characters is received (it
  will not wait). You should call WSAGetLastError to determine error reason.
  */


  int Read(char *buff, int len, int flags=0) 
  {return recv(_s,buff,len,flags);}

  ///Function writes into output socket stream
  /** 
  @param buff output buffer, that contain the data
  @param len maximum length of of buffer in bytes
  @param flags flags defined in MSDN (recv), normally is set to zero
  @return number of bytes were sent. Function will wait until at least one character is
  sent. Then returns number bytes sent. If function returns zero, connection
  has been reset or closed by remote computer. If function returns -1, an error has occured.
  If socket is in nonblocking mode, function returns -1 when no characters were sent (it
  will not wait). You should call WSAGetLastError to determine error reason.
  */
  int Write(const char *buff, int len, int flags=0) 
  {return send(_s,buff,len,flags);}

  ///Function determines number of bytes awaits in input queue. 
  /** Also can return number connection awaits in queue of listening socket  
  returns -1 if there were an error */
  long GetCountToRead() 
  {
    unsigned long cnt=0;
    ioctlsocket(_s,FIONREAD,&cnt);
    return cnt;
  }

  ///Function sets socket into nonblocking mode. 
  /** After setting nonblocking mode, everywhere function should wait to operation complette
  returns immediatelly with error WSAEWOULDBLOCK. Then use Wait function to check completeness of
  operation.
  @param enable true to enable nonblocking mode
  */
  void SetNonBlocking(bool enable)
  {
    unsigned long arg=enable;
    ioctlsocket(_s,FIONBIO,&arg);
  }
  ///Shutdowns the socket
  /**The shutdown function is used on all types of sockets 
  to disable reception, transmission, or both.
  @param how If the how parameter is SD_RECEIVE, subsequent calls 
  to the recv function on the socket will be disallowed. 
  This has no effect on the lower protocol layers. 
  For TCP sockets, if there is still data queued on the 
  socket waiting to be received, or data arrives subsequently, 
  the connection is reset, since the data cannot be delivered to the user. 
  For UDP sockets, incoming datagrams are accepted and queued. In no case will 
  an ICMP error packet be generated.<br><br>
  If the how parameter is SD_SEND, subsequent calls to the send function are 
  disallowed. For TCP sockets, a FIN will be sent after all data is sent and 
  acknowledged by the receiver.<br><br>
  Setting how to SD_BOTH disables both sends and receives as described above.<br><br>
  The shutdown function does not close the socket. Any resources attached to the 
  socket will not be freed until Close is invoked.
  */



  int Shutdown(int how)
  {
    return shutdown(_s,how);
  }
  int SendTo(char *buff, int len,int flags, const SOCKADDR_IN &sin) 
  {return sendto(_s,buff,len,flags,(SOCKADDR *)&sin,sizeof(sin));}
  int RecvFrom(char *buff, int len,int flags, SOCKADDR_IN &sin) 
  {
    int sinlen=sizeof(sin);
    return recvfrom(_s,buff,len,flags,(SOCKADDR *)&sin,&sinlen);
  }    
  enum WaitMode 
  {
    WaitRead=1, ///<Wait until socket is ready to reading / socket is ready to reading
    WaitWrite=2, ///<Wait until socket is ready to writting / socket is ready to writting
    WaitExcept=4, ///<Wait until socket has an exception / socket has an exception
    WaitError=-1, ///<Wait function failed
    WaitTimeout=0 ///<Wait function returned because timeout ellapsed
  };

  ///Function stops thread, until wait contition on socket is executed
  /**
  @param timeout maximum time that function will wait in miliseconds. if timeout is INFINITE, the function's time-out interval never elapses.
  @param mode combination of flags (combined by | - or - operator)<br>
  Socket::WaitRead - wait until socket is ready to reading.<br>
  Socket::WaitWrite - wait until socket is ready to writting.<br>
  Socket::WaitExcept - wait until exception is occured on the socket.
  @return function returns combination of flags, each flag for contition, that executed.<br>
  Socket::WaitRead - socket is ready to reading<br>
  Socket::WaitWrite - socket is ready to writting<br>
  Socket::WaitExcept- an exception on socket has occured<br>
  Function can return one of next values:
  Socket::Error - an error has occured during waitin (e.g. someone closes waiting socket)
  Socket::Timeout - timeout elapsed<p>
  NOTE: Waiting for reading can be used for listening socket. If listening socket is ready to
  reading, a connection is peding in its queue. Waiting for writting can be used for connecting
  socket. If connecting socket (called connect on nonblocking socket) is ready for writting,
  it was successfully connected. An exception can occured, while socket is trying connect, 
  an error is reported (e.g. port is not open, no route to host, and other errors reports,
  that connect cannot be completted).
  */

  int Wait(DWORD timeout, int mode)
  {
    struct fd_set rdset,wrset,exset;    
    struct timeval tout;
    int ret;

    FD_ZERO(&rdset); 
    FD_ZERO(&wrset); 
    FD_ZERO(&exset); 

    if (mode & WaitRead) FD_SET((SOCKET)_s,&rdset);
    if (mode & WaitWrite) FD_SET((SOCKET)_s,&wrset);
    if (mode & WaitExcept) FD_SET((SOCKET)_s,&exset);

    tout.tv_sec=timeout/1000;
    tout.tv_usec=(timeout%1000)*1000;
    if (select(1,&rdset,&wrset,&exset,&tout)==-1) return WaitError;
    ret=WaitTimeout;
    if (FD_ISSET(_s,&rdset)) ret|=WaitRead;
    if (FD_ISSET(_s,&wrset)) ret|=WaitWrite;
    if (FD_ISSET(_s,&exset)) ret|=WaitExcept;
    return ret;
  }

  enum BlockOpRet 
  {
    BlockOk=1,BlockError=0, BlockLost=-1
  };

  BlockOpRet BlockRead(void *data, size_t sz)
  {
    if (sz==0) return BlockOk;
    char *stream=reinterpret_cast<char *>(data);
    int read=Read(stream,(int)sz);  
    while (read<(int)sz)
    {
      if (read==0) return BlockLost;
      if (read<1) return BlockError; //connection error;
      sz-=read;
      stream+=read;
      read=Read(stream,(int)sz);  
    }
    return BlockOk;
  }

  BlockOpRet BlockWrite(const void *data, size_t sz)
  { 
    if (sz==0) return BlockOk;
    const char *stream=reinterpret_cast<const char *>(data);
    int written=Write(stream,(int)sz);  
    while (written<(int)sz)
    {
      if (written==0) return BlockLost; //connection error;
      if (written<1) return BlockError; //connection error;
      sz-=written;
      stream+=written;
      written=Write(stream,(int)sz);  
    }
    return BlockOk;
  }


  bool IsConnected()
  {
    if (!IsValid()) return false;
    SOCKADDR_IN sin;
    int len=sizeof(sin);
    return getsockname(_s,(SOCKADDR *)&sin,&len)!=SOCKET_ERROR;
  }


#if defined(AFX_IPA_H__DA6818E8_23F2_11D4_90D5_00C0DFAE7D0A__INCLUDED_)
  bool Bind(CIPA &ip)
  {
    SOCKADDR saddr;
    ip.GetSockAddres(saddr);
    return bind(_s,&saddr,sizeof(SOCKADDR_IN))!=SOCKET_ERROR;
  }
  Socket Accept(CIPA &ip)
  {
    SOCKADDR saddr;
    int sinlen=sizeof(SOCKADDR_IN);
    Socket out=accept(_s,&saddr,&sinlen);
    ip=CIPA(saddr);
    return out;
  }
  bool Connect(CIPA &ip)
  {
    SOCKADDR saddr;
    ip.GetSockAddres(saddr);
    return connect(_s,&saddr,sizeof(SOCKADDR_IN))!=SOCKET_ERROR;
  }
  int SendTo(char *buff, int len,int flags, const CIPA &ipa) 
  {	
    SOCKADDR saddr;
    ipa.GetSockAddres(saddr);
    return sendto(_s,buff,len,flags,&saddr,sizeof(SOCKADDR_IN));
  }
  int RecvFrom(char *buff, int len,int flags, CIPA &ipa) 
  {
    SOCKADDR saddr;
    int sinlen=sizeof(SOCKADDR_IN);
    int out=recvfrom(_s,buff,len,flags,&saddr,&sinlen);
    ipa=CIPA(saddr);
    return out;
  }    

#endif
};

#endif // !defined(AFX_SOCKET_H__2B60EC2B_12BB_49EB_B182_1E352C09B6B3__INCLUDED_)
