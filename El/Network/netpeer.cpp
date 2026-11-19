/**
  @file   netpeer.cpp
  @brief  Network peer object (pilot implementation using UDP)

  Copyright &copy; 2001-2003 by BIStudio (www.bistudio.com)
  @author PE
  @since  18.11.2001
  @date   6.11.2003
*/

#include "El/Network/netpch.hpp"
#include "El/Network/netpeer.hpp"
#include "El/Speech/debugVoN.hpp"
#include "El/MultiThread2/multithread/ThreadBase.h"
#ifndef _WIN32
#include <sys/ioctl.h>
#endif

  //		#pragma comment(lib, "ws2_32")
  #pragma comment(lib, "wsock32")

// Desc: Associate existing socket with object. Object will close the socket
//       when it is destroyed.
PPSocket::PPSocket( SOCKET sock )
:_socket( sock )
{
}


// Desc: Create a socket of the given type
PPSocket::PPSocket( SocketType type )
:
_socket( INVALID_SOCKET )
{
  bool bSuccess = Open( type );
  Assert( bSuccess );
  (void)bSuccess;
}

// Desc: Create a socket of the given type/protocol
PPSocket::PPSocket( int iType, int iProtocol )
:
_socket( INVALID_SOCKET )
{
  bool bSuccess = Open( iType, iProtocol );
  Assert( bSuccess );
  (void)bSuccess;
}

// Desc: Close and release socket
PPSocket::~PPSocket()
{
  Close();
}

// Desc: Open a socket of the given type
bool PPSocket::Open( SocketType type )
{
  switch( type )
  {
  case Type_UDP:
    return Open( SOCK_DGRAM, IPPROTO_UDP );

  case Type_TCP:
    return Open( SOCK_STREAM, IPPROTO_TCP );

  default:
    Assert( type == Type_VDP );
    return Open( SOCK_DGRAM, IPPROTO_VDP );
  }
}

// Desc: Open a socket of the given type/protocol
bool PPSocket::Open( int iType, int iProtocol )
{
  Close();
  _socket = socket( AF_INET, iType, iProtocol );

#if defined(_WIN32) 
  if (_socket)
  {
    // disable the reporting of WSAECONNRESET to avoid calling of NetPeerUDP::reconnect()
    DWORD enableReporting = FALSE;
    DWORD returned = 0;
    // IOC_IN = 0x80000000;
    // IOC_VENDOR = 0x18000000;
    // SIO_UDP_CONNRESET = IOC_IN | IOC_VENDOR | 12;
    const DWORD  SIO_UDP_CONNRESET = 0x9800000C;
    if (WSAIoctl(_socket, SIO_UDP_CONNRESET, &enableReporting, sizeof(enableReporting), NULL, 0, &returned, NULL, NULL) == SOCKET_ERROR)
    {
      DWORD err = WSAGetLastError();
      if (err != WSAEWOULDBLOCK)
      {
# ifdef NET_LOG_CREATE_PEER
#   ifdef NET_LOG_BRIEF
        NetLog("Fac:cp: WSAIoctl(SIO_UDP_CONNRESET) failed with error %d", err);
#   else
        NetLog("PeerChannelFactoryUDP::createPeer: WSAIoctl(SIO_UDP_CONNRESET) failed with error %d", err);
#   endif
# endif
        closesocket(_socket);
        return NULL;
      }
    }
  }
#endif

  DisableMTUDiscovery(_socket);

  return( _socket != INVALID_SOCKET );
}

void DisableMTUDiscovery(int socket)
{
#if defined IP_DONTFRAGMENT // Windows version
  {
    int val = 0;
    setsockopt(socket, IPPROTO_IP, IP_DONTFRAGMENT, &val, sizeof(val));
  }
#endif

#if defined IP_DONTFRAG // BSD version
  {
    int val = 0;
    setsockopt(socket, IPPROTO_IP, IP_DONTFRAG, &val, sizeof(val));
  }
#endif

#if defined IP_MTU_DISCOVER && defined IP_PMTUDISC_DONT // Linux version
  {
    int val = IP_PMTUDISC_DONT;
    setsockopt(socket, IPPROTO_IP, IP_MTU_DISCOVER, &val, sizeof(val));
  }
#endif

}

// Desc: TRUE if socket is open
bool PPSocket::IsOpen() const
{
  return( _socket != INVALID_SOCKET );
}

// Desc: Close socket
int PPSocket::Close()
{
  int result = 0;
  if( _socket != INVALID_SOCKET )
  {
    result = closesocket( _socket );
    _socket = INVALID_SOCKET;
  }
  return result;
}

// Desc: Permit incoming connection attempt
SOCKET PPSocket::Accept( sockaddr_in* pSockAddr )
{
  Assert( _socket != INVALID_SOCKET );
  int iSize = sizeof( sockaddr_in );

  SOCKET sockResult = accept( _socket, (sockaddr*)(pSockAddr), (socklen_t*)&iSize );

#if _DEBUG
  if( sockResult != INVALID_SOCKET && pSockAddr != NULL )
    Assert( iSize == sizeof( sockaddr_in ) );
#endif
  return sockResult;
}

// Desc: Associate local address with socket
int PPSocket::Bind( const sockaddr_in* pSockAddr )
{
  Assert( _socket != INVALID_SOCKET );
  Assert( pSockAddr != NULL );
  Assert( pSockAddr->sin_family == AF_INET );

  int result = bind( _socket, (const sockaddr*)(pSockAddr), 
    sizeof( sockaddr_in ) );
  return result;
}

// Desc: Connect socket
int PPSocket::Connect( const sockaddr_in* pSockAddr )
{
  Assert( _socket != INVALID_SOCKET );
  Assert( pSockAddr != NULL );
  Assert( pSockAddr->sin_family == AF_INET );

  int result = connect( _socket, (const sockaddr*)(pSockAddr), 
    sizeof( sockaddr_in ) );
  return result;
}

// Desc: Returns the socket handle
SOCKET PPSocket::GetSocket() const
{
  return _socket;
}

// Desc: Get socket "name"
int PPSocket::GetSockName( sockaddr_in* pSockAddr ) const
{
  Assert( _socket != INVALID_SOCKET );
  Assert( pSockAddr != NULL );
  int iSize = sizeof( sockaddr_in );

  int result = getsockname( _socket, (sockaddr*)(pSockAddr), (socklen_t*)&iSize );

#if _DEBUG
  if( result != SOCKET_ERROR )
    Assert( iSize == sizeof( sockaddr_in ) );
#endif
  return result;
}

// Desc: Get socket option
int PPSocket::GetSockOpt( int iLevel, int iName, void* pValue, int* piSize ) const
{
  Assert( _socket != INVALID_SOCKET );
  Assert( pValue != NULL );
  Assert( piSize != NULL );

  int result = getsockopt( _socket, iLevel, iName, (char*)(pValue), (socklen_t*)piSize );
  return result;
}

// Desc: Configure socket I/O mode
int PPSocket::IoCtlSocket( long nCmd, DWORD* pArg )
{
  Assert( _socket != INVALID_SOCKET );
  Assert( pArg != NULL );

  int result = ioctlsocket( _socket, nCmd, pArg );
  return result;
}

// Desc: Listen for incoming connection
int PPSocket::Listen( int iBacklog )
{
  Assert( _socket != INVALID_SOCKET );

  int result = listen( _socket, iBacklog );
  return result;
}

// Desc: Receive data on socket
int PPSocket::Recv( void* pBuffer, int iBytes )
{
  Assert( _socket != INVALID_SOCKET );
  Assert( pBuffer != NULL );
  Assert( iBytes >= 0 );

  int result = recv( _socket, (char*)(pBuffer), iBytes, 0 );
#if DEBUG_VON
  if (result>=0) 
  {
    char diagTxt[1024];
    sprintf(diagTxt, "PeerToPeer::Recv ... %d Bytes", result);
    //DIAG_MESSAGE(500,diagTxt); //MT safety broken
    LogF(diagTxt);
  }
#endif
  return result;
}

#if DEBUG_VON_DIAGS
PoCriticalSection lockRecvDiagMessage;
char RecvDiagMessage[1024] = "";
PoCriticalSection lockSendDiagMessage;
char SendDiagMessage[1024] = "";
DWORD SendDiagTimeStamp;
int SendDiagSize = 0;
int SendDiagCnt = 1;
int SendDiagBps = 0;
#endif

// Desc: Receive data on socket and report source address
int PPSocket::RecvFrom( void* pBuffer, int iBytes, sockaddr_in* pSockAddr )
{
  Assert( _socket != INVALID_SOCKET );
  Assert( pBuffer != NULL );
  Assert( iBytes >= 0 );
  int iSize = sizeof( sockaddr_in );

  int result = recvfrom( _socket, (char*)(pBuffer), iBytes, 0, 
    (sockaddr*)(pSockAddr), (socklen_t*)&iSize );
#if _DEBUG
  if( result != SOCKET_ERROR && pSockAddr != NULL )
    Assert( iSize == sizeof( sockaddr_in ) );
#endif
#if DEBUG_VON_DIAGS
  if (result>=0) 
  {
    char diagTxt[1024]; 
    sprintf(diagTxt,"PeerToPeer::RecvFrom %d Bytes ... %d.%d.%d.%d", result, pSockAddr->sin_addr.S_un.S_un_b.s_b1, pSockAddr->sin_addr.S_un.S_un_b.s_b2, pSockAddr->sin_addr.S_un.S_un_b.s_b3, pSockAddr->sin_addr.S_un.S_un_b.s_b4);
    //DIAG_MESSAGE(500,diagTxt); //MT safety broken
#if DEBUG_VON
    LogF(diagTxt);
#endif
    lockRecvDiagMessage.enter();
    strcpy(RecvDiagMessage,diagTxt);
    lockRecvDiagMessage.leave();
  }
#endif
  return result;
}

// Desc: Does a select call to check status of socket - returns separate 
//       BOOLs for read, write, and error
int PPSocket::Select( bool* pbRead, bool* pbWrite, bool* pbError )
{
  Assert( _socket != INVALID_SOCKET );

  int iResultTotal = 0;

  timeval tv = {0};
  if( pbRead )
  {
    fd_set fdsRead = {0};
    FD_SET( _socket, &fdsRead );

    int result = select( 0, &fdsRead, NULL, NULL, &tv );
    Assert( result != SOCKET_ERROR );

    *pbRead = ( result == 1 );
    iResultTotal += result;
  }

  if( pbWrite )
  {
    fd_set fdsWrite = {0};
    FD_SET( _socket, &fdsWrite );

    int result = select( 0, NULL, &fdsWrite, NULL, &tv );
    Assert( result != SOCKET_ERROR );

    *pbWrite = ( result == 1 );
    iResultTotal += result;
  }

  if( pbError )
  {
    fd_set fdsError = {0};
    FD_SET( _socket, &fdsError );

    int result = select( 0, NULL, NULL, &fdsError, &tv );
    Assert( result != SOCKET_ERROR );

    *pbError = ( result == 1 );
    iResultTotal += result;
  }

  return iResultTotal;
}

// Desc: Send data on socket
int PPSocket::Send( const void* pBuffer, int iBytes )
{
  Assert( _socket != INVALID_SOCKET );
  Assert( pBuffer != NULL );
  Assert( iBytes >= 0 );

  int result = send( _socket, (const char*)(pBuffer), iBytes, 0 );
  return result;
}

// Desc: Send data on socket to specific destination
int PPSocket::SendTo( const void* pBuffer, int iBytes, const sockaddr_in* pSockAddr )
{
  Assert( _socket != INVALID_SOCKET );
  Assert( pBuffer != NULL );
  Assert( iBytes >= 0 );

  int result = sendto( _socket, (const char*)(pBuffer), iBytes, 0, 
    (const sockaddr*)(pSockAddr), sizeof( sockaddr_in ) );
  return result;
}

// Desc: Set socket option
int PPSocket::SetSockOpt( int iLevel, int iName, const void* pValue, int iBytes )
{
  Assert( _socket != INVALID_SOCKET );
  Assert( pValue != NULL );

  int result = setsockopt( _socket, iLevel, iName, (const char*)(pValue), 
    iBytes );
  return result;
}

// Desc: Disabled sending and/or receiving on socket
int PPSocket::Shutdown( int iHow )
{
  Assert( _socket != INVALID_SOCKET );

  int result = shutdown( _socket, iHow );
  return result;
}

#include "El/Network/netchannel.hpp"

#if _ENABLE_REPORT
#  include "El/Debugging/stackMeter.hpp"
#endif

//------------------------------------------------------------
//  NetPeerUDP: time & statistics constants

/// Listener/sender thread timeout in microseconds.
const long TIMEOUT = 5000;

/// Number of udpListen() loop passes before connectivity check is performed (adjusted for 2 seconds for channels absolutely w/o traffic).
const unsigned CHECK_COUNTER = 2000000 / TIMEOUT;

/// Number of udpSend() loop passes before NetChannel::tick is performaed.
const unsigned TICK_COUNTER = (unsigned)( (4*NetChannelBasic::RUN_INTERVAL) / TIMEOUT );

/// How many packets are received/transmitted in one batch. For udpListenSend only.
const unsigned PACKET_BATCH = 3;

/// Needs to be defined externally.
//#define NET_BREAK

#ifdef NET_BREAK

const unsigned64 BreakCheckInterval = 2000000;

const char *BreakFileName = "break.txt";

#endif

//------------------------------------------------------------

//------------------------------------------------------------
//  support (network):

/**
    Retrieves the local network address.
    @param  me <code>sockaddr_in</code> structure to be filled (network endian).
    @param  port Port number to be used (host endian).
    @return <code>true</code> if succeeded.
*/
bool getLocalAddress ( struct sockaddr_in &me, unsigned16 port )
{
#if !_GAMES_FOR_WINDOWS
    me.sin_family = AF_INET;
    char meName[128];
    if ( gethostname(meName,128) == SOCKET_ERROR ) {
#ifdef NET_LOG_GET_LOCAL_ADDRESS
#  ifdef _WIN32
        NetLog("getLocalAddress: gethostname() failed with error: %d",WSAGetLastError());
#  else
        NetLog("getLocalAddress: gethostname() failed!");
#  endif
#endif
        return false;
        }
#ifdef NET_LOG_GET_LOCAL_ADDRESS
    NetLog("getLocalAddress: local hostname: %s",meName);
#endif
/*
    if ( isalpha(meName[0]) ) {
        struct hostent *h = gethostbyname(meName);
        if ( !h ) {
#ifdef NET_LOG_GET_LOCAL_ADDRESS
#  ifdef _WIN32
            NetLog("getLocalAddress: gethostbyname(%s) failed with error: %d",meName,WSAGetLastError());
#  else
            NetLog("getLocalAddress: gethostbyname(%s) failed!",meName);
#  endif
#endif
            return false;
            }
        if ( h->h_length < 4 ) {
#ifdef NET_LOG_GET_LOCAL_ADDRESS
            NetLog("getLocalAddress: invalid network address type!");
#endif
            return false;
            }
        memcpy(&(me.sin_addr.s_addr),h->h_addr,4);
#ifdef NET_LOG_GET_LOCAL_ADDRESS
        NetLog("getLocalAddress: resolved local hostname: %s (%s)",h->h_name,inet_ntoa(me.sin_addr));
#endif
        }
    else
        me.sin_addr.s_addr = inet_addr(meName);
*/
    struct hostent *h = gethostbyname(meName);
    if (h)
    {
      if ( h->h_length < 4 )
      {
#ifdef NET_LOG_GET_LOCAL_ADDRESS
        NetLog("getLocalAddress: invalid network address type!");
#endif
        return false;
      }
      memcpy(&(me.sin_addr.s_addr),h->h_addr,4);
#ifdef NET_LOG_GET_LOCAL_ADDRESS
      NetLog("getLocalAddress: resolved local hostname: %s (%s)",h->h_name,inet_ntoa(me.sin_addr));
#endif
    }
    else
    {
      me.sin_addr.s_addr = inet_addr(meName);
      if (me.sin_addr.s_addr == INADDR_NONE)
        return false;
    }

    me.sin_port = htons(port);
#ifdef NET_LOG_GET_LOCAL_ADDRESS
    NetLog("getLocalAddress: local port: %u",(unsigned)port);
#endif
    return true;
#else // _GAMES_FOR_WINDOWS
    XNADDR xna;
    DWORD status;
    do
    {
        status = XNetGetTitleXnAddr(&xna);	
    } while (status == XNET_GET_XNADDR_PENDING);

    if (status == XNET_GET_XNADDR_NONE)
    {
#ifdef NET_LOG_GET_LOCAL_ADDRESS
        NetLog("getLocalAddress: XNetGetTitleXnAddr failed!");
#endif
        return false;
    }
// #if _ENABLE_XBOX_PC_MP
    me.sin_addr = xna.ina;
// #endif
    me.sin_port = htons(port);
    return true;
#endif
}

/**
    Retrieves name of the local host.
    @param  name Buffer to hold the result.
    @param  len Buffer length.
    @return <code>true</code> if succeeded.
*/
bool getLocalName ( char *name, unsigned len )
{
    if ( !name ) return false;
    if ( gethostname(name,len) == SOCKET_ERROR ) {
#ifdef NET_LOG_GET_LOCAL_NAME
#  ifdef _WIN32
        NetLog("getLocalName: gethostname() failed with error: %d",WSAGetLastError());
#  else
        NetLog("getLocalName: gethostname() failed!");
#  endif
#endif
        return false;
        }
#ifdef NET_LOG_GET_LOCAL_NAME
    NetLog("getLocalName: local hostname: %s",name);
#endif
/*
    if ( isalpha(name[0]) ) {
        struct hostent *h = gethostbyname(name);
        if ( !h ) {
#ifdef NET_LOG_GET_LOCAL_NAME
#  ifdef _WIN32
            NetLog("getLocalName: gethostbyname(%s) failed with error: %d",name,WSAGetLastError());
#  else
            NetLog("getLocalName: gethostbyname(%s) failed!",name);
#  endif
#endif
            return false;
            }
        strncpy(name,h->h_name,len);
        name[len-1] = (char)0;
#ifdef NET_LOG_GET_LOCAL_NAME
        NetLog("getLocalName: resolved local hostname: %s",name);
#endif
        }
    return true;
*/
    struct hostent *h = gethostbyname(name);
    if (h)
    {
      strncpy(name,h->h_name,len);
      name[len-1] = (char)0;
#ifdef NET_LOG_GET_LOCAL_NAME
      NetLog("getLocalName: resolved local hostname: %s",name);
#endif
    }
    return true;
}

/**
    Fills-in the <code>sockaddr_in</code> address from string.
    @param  host <code>sockaddr_in</code> structure to be filled.
    @param  ip String representation of network address.
    @param  port Port number.
    @return <code>true</code> if succeeded.
*/
bool getHostAddress ( struct sockaddr_in &host, const char *ip, unsigned16 port )
{
    host.sin_family = AF_INET;
    host.sin_port = htons(port);
    if ( !ip || !ip[0] )
        host.sin_addr.s_addr = INADDR_BROADCAST;
    else {
        host.sin_addr.s_addr = inet_addr(ip);
        if ( host.sin_addr.s_addr == INADDR_NONE ) {
            struct hostent *h = gethostbyname(ip);
            if ( !h ) return false;
            host.sin_addr.s_addr = *(unsigned32*)h->h_addr_list[0];
            }
        }
    return true;
}

#define SCRAMBLE_MSG 1

#if SCRAMBLE_MSG

template <int size>
struct RandomTable8b
{
  // all clients need to always have the same value
  char values[size];

  // source adapted from http://en.wikipedia.org/wiki/Linear_feedback_shift_register#Galois_LFSRs
  // 0xb4 from http://www.piclist.com/techref/microchip/rand8bit.htm
  unsigned short NextValue(unsigned short lfsr)
  {
    /* taps: 16 14 13 11; characteristic polynomial: x^16 + x^14 + x^13 + x^11 + 1 */
    return (lfsr >> 1) ^ (-(signed short)(lfsr & 1u) & 0xB400u);
//     unsigned lsb = lfsr & 1;  /* Get lsb (i.e., the output bit). */
//     lfsr >>= 1;               /* Shift register */
//     if (lsb == 1)             /* Only apply toggle mask if output bit is 1. */
//       lfsr ^= 0xB4;
//     return lfsr;
  }
  RandomTable8b()
  {
    int val = 0xACE1u;
    for (int i=0; i<sizeof(values); i++)
    {
      val = NextValue(val);
      values[i] = val;
    }
  }
  char operator ()(int i) const {return values[i%sizeof(values)];}
};

static RandomTable8b<MAX_IN_DATA> RandMsg;

static int Hash15b(int seed)
{
  // fast hash function
	int val = (seed ^ 61) ^ (seed >> 16);
	val = val + (val << 3);
	val = val ^ (val >> 4);
	val = val * 0x27d4eb2d;
	val = val ^ (val >> 15);
	return (val&0x7fff);
}

#endif

//------------------------------------------------------------
//  Asynchronous listener/send threads:

/*!
\patch 1.51 Date 4/18/2002 by Pepca
- Improved: Connectivity-check is done more frequently (in some cases the yellow square
was irelevant). [Sockets]
\patch 1.52 Date 4/20/2002 by Pepca
- Fixed: Latency (and packet drop-ratio) climbs to terrible heights on Win9x some times. [Sockets]
\patch_internal 1.52 Date 4/21/2002 by Pepca
- Improved: Both "listener" and "sender" threads have now higher priority. [Sockets]
\patch 1.56 Date 5/16/2002 by Pepca
- Fixed: Rare crashes in case of sudden socket-close (crc32). [Sockets]
\patch_internal 2.01 Date 2/12/2003 by Pepca
- Improved: udpListen and udpSend threads were merged into one (udpListenSend).
*/

THREAD_PROC_RETURN THREAD_PROC_MODE udpListenSend ( void *param )
{
    // Register for MultiThread2 library purposes (set TLS to use GetCurrentThread on Linux)
    MultiThread::RunningThread<MultiThread::ThreadBase> mtRegistration;

    // receiver/common variables:
    union {
        MsgHeader header;
        char data[MAX_IN_DATA];             // fixed buffer to receive message data
        };
    NetPeerUDP *peer = (NetPeerUDP*)param;
    Assert( peer );
    fd_set set;                             // list of receiving sockets (we're using only the 1st item)
    struct timeval timeout;                 // select() timeout (for receiver).
    struct sockaddr_in from;                // IP address the message came from
    socklen_t fromLen;
    int checkCounter = CHECK_COUNTER;       // periodic NetChannel::checkConnectivity revocation

        // sender variables:
    IteratorState origin = ITERATOR_NULL;   // origin for cyclic pass through the channels
    IteratorState robin = ITERATOR_NULL;    // general purpose iterator
    RefD<NetChannel> channel;               // channel sending the actual message
    unsigned64 bunchStart = 0;              // "packet-bunch" start time
    int tickCounter = TICK_COUNTER;         // periodic NetChannel::tick revocation
    IteratorState it;                       // general-purpose channel-iterator
    DispatcherStatus *ddata = NULL;         // struct for dispatcher-state collecting
    unsigned waitTime = 1;                  // 0 inside the "packet-bunch"

    bool previousBatch = true;              // previous receiver/transmitter batch was full..
    unsigned batchIt;
    unsigned64 now;

#if _ENABLE_REPORT
    STACK_INIT(30*1024);
#endif

#ifdef NET_LOG_UDP_LISTEN
#  ifdef NET_LOG_BRIEF
    NetLog("Pe(%u):list(%u)",peer->getPeerId(),peer->getLocalPort());
#  else
    NetLog("Peer(%u)::udpListenSend: start listening/sending at local port %u",peer->getPeerId(),peer->getLocalPort());
#  endif
#endif

    while ( peer->listen ) 
    {                // check it at least each 50ms (according to "TIMEOUT")

        // 1. receiver batch
        for ( batchIt = 0; batchIt++ < PACKET_BATCH; ) {

            FD_ZERO ( &set );
            FD_SET ( peer->sock, &set );
            timeout.tv_sec  = 0;
            timeout.tv_usec = previousBatch ? 0 : TIMEOUT;

            int error = select(FD_SETSIZE,&set,NULL,NULL,&timeout);

            if ( error != 1 ) {             // no data are ready => go directly to transmitter batch
                if ( !previousBatch )
                    waitTime += TIMEOUT/1000;
                else {
                    waitTime = 0;
                    previousBatch = false;
                    }
                break;
                }

            // data are ready => read it
            fromLen = sizeof(from);
            memset(&from,0,sizeof(from)); // provide zeros - in case of error
            error = recvfrom(peer->sock,data,MAX_IN_DATA-1,0,(struct sockaddr*)&from,&fromLen);
            if ( error != SOCKET_ERROR && error == (int)header.length && error >= MSG_HEADER_LEN && error <= MAX_IN_DATA) 
            {
                #if SCRAMBLE_MSG
                  {
                    MsgHeader *hdr = reinterpret_cast<MsgHeader *>(data);
                    int offset = Hash15b(hdr->serial);
                    for (int s=sizeof(MsgHeader); s<=hdr->length; s++) data[s] ^= RandMsg(s+offset);
                  }
                #endif
                // CRC check:
                unsigned32 crc = header.crc;
                header.crc = 0;
#ifdef NET_LOG_UDP_RECEIVE
#  ifdef MSG_ID
                NetLog("Peer(%u)::udpListenSend: received message (from=%u.%u.%u.%u:%u,len=%3d,serial=%4u,flags=%04x,ID=%x), wait=%u ms",
                       peer->getPeerId(),
                       (unsigned)IP4(from),(unsigned)IP3(from),(unsigned)IP2(from),(unsigned)IP1(from),(unsigned)PORT(from),
                       (int)header.length-MSG_HEADER_LEN,(unsigned)header.serial,(unsigned)header.flags,
                       header.id,waitTime);
#  else
                NetLog("Peer(%u)::udpListenSend: received message (from=%u.%u.%u.%u:%u,len=%3d,serial=%4u,flags=%04x), wait=%u ms",
                       peer->getPeerId(),
                       (unsigned)IP4(from),(unsigned)IP3(from),(unsigned)IP2(from),(unsigned)IP1(from),(unsigned)PORT(from),
                       (int)header.length-MSG_HEADER_LEN,(unsigned)header.serial,(unsigned)header.flags,
                       waitTime);
#  endif
#endif
                if ( crc32(0,(const unsigned8*)&header,header.length) == crc ) 
                {
                    header.crc = crc;
                    peer->enter();
                    peer->processData(&header,from);    // only NetPeer-related processing (statistics)!
                    RefD<NetChannel> ch;
                    peer->chMap.get(sockaddrKey(from),ch);
                    if ( (header.flags & MSG_TO_BCAST_FLAG) || !ch )
                        ch = peer->getBroadcastChannel();
                    peer->leave();
                    if ( ch )
                        ch->processData(&header,from);  // main data-processing routine
                }
#ifdef NET_LOG_UDP_RECEIVE
                else
#  ifdef NET_LOG_BRIEF
                    NetLog("Pe(%u):err-crc",peer->getPeerId());
#  else
                    NetLog("Peer(%u)::udpListenSend: received message has bad CRC!",peer->getPeerId());
#  endif
#endif
            }
            else if (error != SOCKET_ERROR && peer->CheckMessageHook(data, error, &from))
            {
              // processed by the hook
            }
            else if (error>=offsetof(MsgHeader,length)+sizeof(header.length) && header.length=='B'+0x100*'E')
            {
              // BattlEye packet - do not report as an error
            }
            else 
            {                          // receiver error
#ifdef _WIN32
                int werror = WSAGetLastError();
#else
                int werror = 0;
#endif
                int serror = 0;
                socklen_t errLen = sizeof(serror);
                getsockopt(peer->sock,SOL_SOCKET,SO_ERROR,(char*)&serror,&errLen);
#ifdef _WIN32
                WSASetLastError(0);         // to be sure!
#endif
#ifdef NET_LOG_UDP_LISTEN
                {
                  RefD<NetChannel> ch;
                  peer->enter();
                  peer->chMap.get(sockaddrKey(from),ch);
                  if (header.flags & MSG_TO_BCAST_FLAG)
                      ch = peer->getBroadcastChannel();
                  peer->leave();
                  int chnum = ch ? ch->getChannelId() : -1;
                  int msgSize = -1;
                  if (error>=offsetof(MsgHeader,length)+sizeof(header.length)) msgSize = header.length;

#ifdef NET_LOG_BRIEF
# define PEER_STR "Pe"
# define CHANNEL_STR "Ch"
# define MSG_STR "err-l"
#else
# define PEER_STR "Peer"
# define CHANNEL_STR "Channel"
# define MSG_STR ":udpListen: error reading data"
#endif

                  NetLog(
                    PEER_STR "(%u):" MSG_STR "(%x,%d,%d,%d~%d) - " CHANNEL_STR "(%d)-%d.%d.%d.%d:%d",
                    peer->getPeerId(),(unsigned)peer->sock,serror,werror,error,msgSize,chnum,
                    (unsigned)IP4(from),(unsigned)IP3(from),(unsigned)IP2(from),(unsigned)IP1(from),(unsigned)PORT(from)
                  );
                }
#endif
#ifdef _WIN32
                if ( werror == WSAECONNRESET ) {
                    peer->reconnect();
#  ifdef NET_LOG_UDP_LISTEN
#    ifdef NET_LOG_BRIEF
                    NetLog("Pe(%u):rec(%x)",
                           peer->getPeerId(),(unsigned)peer->sock);
#    else
                    NetLog("Peer(%u)::udpListenSend: reconnect() after WSAECONNRESET (socket=%x)",
                           peer->getPeerId(),(unsigned)peer->sock);
#    endif
#  endif
                }
#endif
            }

            waitTime = 0;
            previousBatch = true;
            }                               // receiver batch

        // 2. transmitter batch
        peer->enter();
        for ( batchIt = 0; batchIt++ < PACKET_BATCH; ) {

            if ( !ddata )                   // 1st-time run => allocate the collection-struct
                ddata = (DispatcherStatus*)safeNew(peer->initDispatcherStatus(NULL));

            if ( ddata ) {     
                peer->initDispatcherStatus(ddata);
                    // collection-struct is initialized => do the collection job:
                if ( peer->chMap.getFirst(it,channel) )
                    do
                        channel->nextDispatcherStatus(ddata);
                    while ( peer->chMap.getNext(it,channel) );
                channel = peer->getBroadcastChannel();
                if ( channel )
                    channel->nextDispatcherStatus(ddata);
                }

            channel = peer->getBroadcastChannel(); // control channel has the highest priority..
            if ( !channel || !channel->getPreparedMessage(ddata) ) {
                robin = origin;             // round-robin strategy
                if ( peer->chMap.getFirstCyclic(robin,origin,channel) )
                                            // go through all channels:
                    while ( !channel->getPreparedMessage(ddata) &&
                            peer->chMap.getNextCyclic(robin,origin,channel) ) ;
                }

            if ( !channel || !channel->prepared ) {
                previousBatch = false;      // no data are prepared => go directly to receiver batch
                break;
            }

            // the message (channel->prepared) is ready to send
            origin = robin;                 // I want to start here next time
            struct sockaddr_in dist;
            if ( channel->isControl() )     // control channel => get distant address from the message
                channel->prepared->getDistant(dist);
            else                            // common channel => use distant address associated with the channel
                channel->getDistantAddress(dist);

#ifdef NET_LOG_UDP_SENDING
            NetLog("Peer(%u)::udpListenSend: sending message (to=%u.%u.%u.%u:%u,len=%3d,serial=%4u,flags=%04x,ID=%x), wait=%u ms",
                   peer->getPeerId(),
                   (unsigned)IP4(dist),(unsigned)IP3(dist),(unsigned)IP2(dist),(unsigned)IP1(dist),(unsigned)PORT(dist),
                   (int)channel->prepared->getLength(),channel->prepared->getSerial(),
                   (unsigned)channel->prepared->getFlags(),channel->prepared->id,waitTime);
#endif
            if ( waitTime )                 // a new bunch is starting
                now = bunchStart = channel->preSend(0);
            else
                now = channel->preSend(bunchStart);

            waitTime = 0;

            #if _ENABLE_REPORT
              bool drop = channel->dropPacketToSend();
            #else
              bool drop = false;
            #endif

            if ( drop || peer->sendData(channel->prepared->header,dist) != nsError )
                channel->prepared->status = // sent OK
                (channel->prepared->status == nsOutputPending) ? nsOutputSent : nsOutputTimeout;
            else                            // send error
                channel->prepared->status = nsError;

            channel->postSend();            // remember the message for some time..
            // pass over the message (call-back, acknowledgements, resent etc.)

            previousBatch = true;
        }                               // transmitter batch

        if ( tickCounter-- <= 0 ) {         // tick() call on all channels:
            if ( peer->chMap.getFirst(it,channel) )
                do                          // check one channel
                    channel->tick();
                while ( peer->chMap.getNext(it,channel) );
            channel = peer->getBroadcastChannel();
            if ( channel ) channel->tick();
            tickCounter = TICK_COUNTER;
        }

        if ( checkCounter-- <= 0 ) {        // connectivity checks on all channels:
            now = getSystemTime();
            RefD<NetChannel> channel;
            if ( peer->chMap.getFirst(it,channel) )
                do                          // check one channel
                    channel->checkConnectivity(now);
                while ( peer->chMap.getNext(it,channel) );
            checkCounter = CHECK_COUNTER;
        }
        peer->leave();

    }                                   // while ( peer->listen )

    if ( ddata )
        safeDelete(ddata);

#ifdef NET_LOG_UDP_LISTEN
#  ifdef NET_LOG_BRIEF
    NetLog("Pe(%u):stopl(%u)",
           peer->getPeerId(),(unsigned)peer->getLocalPort());
#  else
    NetLog("Peer(%u)::udpListenSend: stop listening/sending at local port %u",
           peer->getPeerId(),(unsigned)peer->getLocalPort());
#  endif
#endif

#if _ENABLE_REPORT
    unsigned sUsage = STACK_METER(30*1024);
    if ( sUsage )
      LogF("StackMeter(udpListenSend,32KB): %u bytes",sUsage);
    else
      LogF("StackMeter(udpListenSend,32KB) overflow");
#endif

    return (THREAD_PROC_RETURN)0;
}


//------------------------------------------------------------
//  NetPeerUDP class:

/*!
\patch_internal 1.51 Date 4/17/2002 by Pepca
- Fixed: NetPeerUDP::sock and chMap are now protected by the lock. [Sockets]
*/

NetPeerUDP::NetPeerUDP ( NetPool *_pool ) : NetPeer(_pool), chMap(1)
{
    LockRegister(lock,"NetPeerUDP");
    sock = INVALID_SOCKET;
    port = 0;
    listen = sending = reconnecting = false;
}

NetPeerUDP::NetPeerUDP ( SOCKET _sock, unsigned16 _port, NetPool *_pool, RawMessageCallback callback )
    : NetPeer(_pool), chMap(2), _callback(callback)              // space for at least two channels..
{
    LockRegister(lock,"NetPeerUDP");
    lock.enterNoDeadLockDetector();
    sock = _sock;
    port = _port;
    listen = sending = reconnecting = false;
    broadcastCh = NULL;
    if ( pool && pool->getFactory() ) {     // create a broadcast channel
        broadcastCh = pool->getFactory()->createChannel(pool,true);
        if ( broadcastCh ) {
            struct sockaddr_in distant;
            Zero(distant);
            distant.sin_addr.s_addr = INADDR_BROADCAST;
            broadcastCh->open(this,distant);
            }
        }
    if ( sock != INVALID_SOCKET ) {         // prepare asynchronous listener/send threads
#ifdef NET_LOG_PEER
        char buf[256];
#  ifdef NET_LOG_BRIEF
        NetLog("Pe(%u):succ(%s)",getPeerId(),getPeerInfo(buf));
#  else
        NetLog("Peer(%u)::NetPeerUDP succeeded: %s",getPeerId(),getPeerInfo(buf));
#  endif
#endif
            // start asynchronous I/O thread(s):
        listen = true;
        if ( poThreadCreate(&listener,32*1024,&udpListenSend,this) ) {
            Verify( poSetPriority(listener,2) ); // the highest priority
            }
        else
            listen = false;                 // only "listen" flag is used
        }
    lock.leaveNoDeadLockDetector();
}

void NetPeerUDP::getLocalAddress ( struct sockaddr_in &local ) const
{
    ::getLocalAddress(local,port);
}

unsigned NetPeerUDP::maxMessageData () const
{
    return( NetChannelBasic::par.maxPacketSize - IP_UDP_HEADER - MSG_HEADER_LEN );
}

bool NetPeerUDP::registerChannel ( struct sockaddr_in &distant, NetChannel *ch )
{
    if ( !ch ) return false;
    enter();
    bool result = !chMap.presentKey(sockaddrKey(distant));
    if ( result ) chMap.put(ch);
    leave();
        // the new net-channel will receive incoming data automatically!
    return result;
}

void NetPeerUDP::unregisterChannel ( NetChannel *ch )
{
    if ( !ch ) return;
    chMap.removeValue(ch);
}

NetChannel *NetPeerUDP::findChannel ( const struct sockaddr_in &distant )
{
    RefD<NetChannel> result;
    chMap.get(sockaddrKey(distant),result);
    return result;
}

void NetPeerUDP::close ()
{
    enter();
        // stop sender & listener threads as early as possible:
    bool wasSending = sending;
    sending = false;
    bool wasListen = listen;
    listen = false;
        // close all associated (point-to-point) channels:
    IteratorState iter;
    RefD<NetChannel> ch;
    if ( chMap.getFirst(iter,ch) )
        do
            ch->close();
        while ( chMap.getNext(iter,ch) );
    chMap.reset();
        // close the broadcast channel:
    if ( broadcastCh ) {
        broadcastCh->close();
        broadcastCh = NULL;
        }
    leave();
        // destroy the sender thread:
    if ( wasSending )
        Verify( poThreadJoin(sender,NULL) );
        // destroy the listener thread:
    if ( wasListen )
        Verify( poThreadJoin(listener,NULL) );
    enter();
        // close the socket:
    if ( sock != INVALID_SOCKET ) {
        closesocket(sock);
        sock = INVALID_SOCKET;
        }
    leave();
}

void NetPeerUDP::stopThreads ()
{
    enter();
        // stop sender & listener threads
    bool wasSending = sending;
    sending = false;
    bool wasListen = listen;
    listen = false;
    leave();
        // destroy the sender thread:
    if ( wasSending )
        Verify( poThreadJoin(sender,NULL) );
        // destroy the listener thread:
    if ( wasListen )
        Verify( poThreadJoin(listener,NULL) );
}

void NetPeerUDP::processData ( MsgHeader *hdr, const struct sockaddr_in &distant )
{
    // !!! TODO: peer statistics !!!
}

void NetPeerUDP::reconnect ()
{
    enter();
    if ( sock == INVALID_SOCKET ) {
        leave();
        return;
        }
    reconnecting = true;
    closesocket(sock);
    sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if ( sock == INVALID_SOCKET ) {
#ifdef NET_LOG_PEER
#  ifdef _WIN32
#    ifdef NET_LOG_BRIEF
        NetLog("Pe(%u):rec: socket() failed with error %d",getPeerId(),WSAGetLastError());
#    else
        NetLog("Peer(%u)::reconnect: socket() failed with error %d",getPeerId(),WSAGetLastError());
#    endif
#  else
#    ifdef NET_LOG_BRIEF
        NetLog("Pe(%u):rec: socket() failed",getPeerId());
#    else
        NetLog("Peer(%u)::reconnect: socket() failed!",getPeerId());
#    endif
#  endif
#endif
        reconnecting = false;
        leave();
        return;
        }
	int tmp = 1;
    if ( setsockopt(sock,SOL_SOCKET,SO_BROADCAST,(char*)&tmp,sizeof(tmp)) == SOCKET_ERROR ) {
#ifdef NET_LOG_PEER
#  ifdef _WIN32
#    ifdef NET_LOG_BRIEF
        NetLog("Pe(%u):rec: setsockopt(SO_BROADCAST) failed with error %d",getPeerId(),WSAGetLastError());
#    else
        NetLog("Peer(%u)::reconnect: setsockopt(SO_BROADCAST) failed with error %d",getPeerId(),WSAGetLastError());
#    endif
#  else
#    ifdef NET_LOG_BRIEF
        NetLog("Pe(%u):rec: setsockopt(SO_BROADCAST) failed!",getPeerId());
#    else
        NetLog("Peer(%u)::reconnect: setsockopt(SO_BROADCAST) failed!",getPeerId());
#    endif
#  endif
#endif
        closesocket(sock);
        sock = INVALID_SOCKET;
        reconnecting = false;
        leave();
        return;
        }
    tmp = NetChannelBasic::par.rcvBufSize;
    setsockopt(sock,SOL_SOCKET,SO_RCVBUF,(char*)&tmp,sizeof(tmp));
    //setsockopt(sock,SOL_SOCKET,SO_SNDBUF,(char*)&tmp,sizeof(tmp));
#ifdef _WIN32
    const BOOL exclusive = TRUE;
    if (setsockopt(sock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
      reinterpret_cast<const char *>(&exclusive), sizeof(exclusive)) == SOCKET_ERROR)
    {
      closesocket(sock);
      sock = INVALID_SOCKET;
      reconnecting = false;
      leave();
      return;
    }
#endif
    struct sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = bindIPAddress;
    local.sin_port = htons(port);
    if ( bind(sock,(struct sockaddr*)&local,sizeof(local)) == SOCKET_ERROR ) {
#ifdef NET_LOG_PEER
#  ifdef _WIN32
#    ifdef NET_LOG_BRIEF
        NetLog("Pe(%u):rec: bind() failed with error %d",getPeerId(),WSAGetLastError());
#    else
        NetLog("Peer(%u)::reconnect: bind() failed with error %d",getPeerId(),WSAGetLastError());
#    endif
#  else
#    ifdef NET_LOG_BRIEF
        NetLog("Pe(%u):rec: bind() failed!",getPeerId());
#    else
        NetLog("Peer(%u)::reconnect: bind() failed!",getPeerId());
#    endif
#  endif
#endif
        closesocket(sock);
        sock = INVALID_SOCKET;
        }
    reconnecting = false;
    leave();
}

void NetPeerUDP::sendRaw( const sockaddr_in &ia, const void *data, int size, int sizeEncrypted )
{
  enter(); // note: send thread safety is not certain
  sendto(sock,reinterpret_cast<const char *>(data),size,0,reinterpret_cast<const sockaddr *>(&ia),sizeof(ia));
  leave();
}

NetStatus NetPeerUDP::sendData ( MsgHeader *hdr, struct sockaddr_in distant )
{
    enter();
    if ( sock == INVALID_SOCKET ) {
        leave();
        return nsError;
        }
    Assert( hdr );
    Assert( hdr->length );
      // crc is calculated from CRC field as well - we need to initialize it
      hdr->crc = 0;
      hdr->crc = crc32(0,(const unsigned8*)hdr,hdr->length);
#ifdef _WIN32
    int retryCounter = 12;
  retry:
#endif
    #if !_SUPER_RELEASE
    static int simulateMTU = -1; // 8 KB is large enough to be never activated
    if (simulateMTU>=0 && hdr->length>simulateMTU)
    {
      // simulate a packet loss - do not send a packet, but pretend it was sent
#ifdef NET_LOG
      NetLog("Pe(%u):simLoss(%d,%d,%x)",getPeerId(),hdr->length,hdr->serial,hdr->flags);
#endif
      leave();
      return nsOutputSent;
    }
    #endif
    char copyToSend[MAX_IN_DATA];
    memcpy(copyToSend,hdr,hdr->length);
    // scramble the message content (leave the header alone)
    #if SCRAMBLE_MSG
      {
        int offset = Hash15b(hdr->serial);
        for (int s=sizeof(MsgHeader); s<=hdr->length; s++) copyToSend[s] ^= RandMsg(s+offset);
      }
    #endif
    int result = sendto(sock,copyToSend,hdr->length,0,(const sockaddr*)&distant,sizeof(distant));
    if ( result == SOCKET_ERROR ) {
#ifdef _WIN32
        int werror = WSAGetLastError();
#endif
        int error = 0;
        socklen_t errLen = sizeof(error);
        getsockopt(sock,SOL_SOCKET,SO_ERROR,(char*)&error,&errLen);
#ifdef _WIN32
        WSASetLastError(0);                 // to be sure!
#  ifdef NET_LOG_UDP_SEND
#    ifdef NET_LOG_BRIEF
        NetLog("Pe(%u):err-s(%d,%d)",getPeerId(),error,werror);
#    else
        NetLog("Peer(%u)::sendData: sendto() failed with error: %d, %d",getPeerId(),error,werror);
#    endif
#  endif
        if ( werror == WSAECONNRESET ) {    // connection reset by peer => reconnect it!
            reconnect();
            if ( sock == INVALID_SOCKET ) {
#  ifdef NET_LOG_UDP_SEND
#    ifdef NET_LOG_BRIEF
                NetLog("Pe(%u):rec-giveup",getPeerId());
#    else
                NetLog("Peer(%u)::sendData: reconnect() after WSAECONNRESET failed .. giving up",getPeerId());
#    endif
#  endif
                leave();
                return nsError;
                }
            else {
#  ifdef NET_LOG_UDP_SEND
#    ifdef NET_LOG_BRIEF
                NetLog("Pe(%u):rec-retry",getPeerId());
#    else
                NetLog("Peer(%u)::sendData: reconnect() after WSAECONNRESET succeeded .. retrying",getPeerId());
#    endif
#  endif
                if ( retryCounter-- )
                    goto retry;
                else {
                    leave();
                    return nsError;
                    }
                }
            }
#else
#  ifdef NET_LOG_UDP_SEND
#    ifdef NET_LOG_BRIEF
        NetLog("Pe(%u):err-s(%d)",getPeerId(),error);
#    else
        NetLog("Peer(%u)::sendData: sendto() failed with error: %d",getPeerId(),error);
#    endif
#  endif
#endif
        leave();
        return nsError;
        }
#ifdef NET_LOG_SEND_DATA
    else
        NetLog("Peer(%u)::sendData: OK sending data (socket=%x, result=%d)",getPeerId(),(unsigned)sock,result);
#endif
    leave();
    return nsOutputSent;
}

void NetPeerUDP::cancelAllMessages ()
{
    IteratorState iter;
    enter();
    RefD<NetChannel> ch;
    if ( chMap.getFirst(iter,ch) )
        do
            ch->cancelAllMessages();
        while ( chMap.getNext(iter,ch) );
    leave();
}

unsigned NetPeerUDP::initDispatcherStatus ( DispatcherStatus *data )
{
    if ( data ) {
        DispatcherStatusBasic *ds = (DispatcherStatusBasic*)data;
        memset(ds,0,sizeof(*ds));           // fast solution
        ds->structLen = sizeof(DispatcherStatusBasic);
        }
    return sizeof(DispatcherStatusBasic);
}

bool NetPeerUDP::CheckMessageHook(char *data, int len, struct sockaddr_in *from)
{
  if (!_callback) return false; // not processed
  return (*_callback)(data, len, from); // offer to callback
}

NetPeerUDP::~NetPeerUDP ()
{
#if defined(NET_LOG_DESTRUCTOR) || defined(NET_LOG_PEER)
#  ifdef NET_LOG_BRIEF
    NetLog("Pe(%u):~(%d,%d,%x,%u,%u)",
           getPeerId(),(int)listen,(int)sending,(unsigned)sock,(unsigned)port,chMap.card());
#  else
    NetLog("Peer(%u)::~NetPeerUDP: listening=%d, sending=%d, socket=%x, port=%u, |chMap|=%u",
           getPeerId(),(int)listen,(int)sending,(unsigned)sock,(unsigned)port,chMap.card());
#  endif
#endif
    close();
}



PcPeerToPeerChannel::PcPeerToPeerChannel()
{
}

THREAD_PROC_RETURN THREAD_PROC_MODE voiceListenSend(void*context)
{
  PcPeerToPeerChannel *chan = (PcPeerToPeerChannel *)context;
  chan->Thread();
  return (THREAD_PROC_RETURN)0;
}

/*!
\patch 5140 Date 3/14/2007 by Jirka
- Fixed: Voice over net was not working when DS and client was running on a single PC
*/

bool PcPeerToPeerChannel::Init(int port)
{
  _keepAliveProc = NULL;

  _endListener = false;
  // create a listener thread
  if ( poThreadCreate(&_listener,64*1024,&voiceListenSend,this) )
  {
    Verify( poSetPriority(_listener,2) ); // the higher priority
  }
  return true;
}

void PcPeerToPeerChannel::RegisterKeepAliveCallback(ProcessKeepAlive *process, void *ctx)
{
  _keepAliveProc = process;
  _ctx = ctx;
}

PcPeerToPeerChannel::~PcPeerToPeerChannel()
{
  if (_keepAliveProc)
  {
    // bool write is atomic
    _endListener = true;
    Verify (poThreadJoin(_listener, NULL));
    _keepAliveProc = NULL;
  }
}

void PcPeerToPeerChannel::StopListener()
{
  if (_keepAliveProc)
  {
    // bool write is atomic
    _endListener = true;
    Verify (poThreadJoin(_listener, NULL));
    _keepAliveProc = NULL;
  }
}

// send in open
void PcPeerToPeerChannel::SendMessage(
  const sockaddr_in &sa, const void *data, int size, int sizeEncrypted
)
{
  Fail("No longer supported");
}

void PcPeerToPeerChannel::Thread()
{
  for(;;)
  {
    if (_endListener)
    {
      return;
    }

    // if no data are ready, sleep
    //Assert( WSAGetLastError() == WSAEWOULDBLOCK );
    SLEEP_MS(20);
    /// Send encoded Voice (not send directly from Encode due to possible deadlock)
    //  [Deadlock was caused by critical sections EventToCallback.handleLock and VoNClient.lock])
    // NOTE: this send is processed from the _keepAliveProc

    /// KeepAlive system
    // traversed NAT should be maintained open by sending KeepAlive packet from time to time
    if (_keepAliveProc)
    {
      (*_keepAliveProc)(_ctx);
    }
  }
}

// bind to specific IP address or use IN_ADDR_ANY otherwise
unsigned long bindIPAddress = INADDR_ANY;
