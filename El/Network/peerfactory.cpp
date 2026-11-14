/**
  @file   peerfactory.cpp
  @brief  Net-peer factory creating NetPeerUDP and NetChannelUDP instances

  Copyright &copy; 2001-2004 by BIStudio (www.bistudio.com)
  @author PE
  @since  3.12.2001
  @date   24.3.2004
*/

#include "El/Network/netpch.hpp"

#include "El/Network/netpeer.hpp"
#include "El/Network/netchannel.hpp"
#include "El/Network/peerfactory.hpp"
#ifdef _WIN32
#include "El/TCPIPBasics/Socket.h"
#endif

//------------------------------------------------------------
//  PeerChannelFactoryUDP class:

int PeerChannelFactoryUDP::instances = 0;

void initNetworkUDP()
{
  if ( PeerChannelFactoryUDP::instances++ ) return;

  // XNetStartup
#if _GAMES_FOR_WINDOWS
  XNetStartupParams xnsp;
  memset( &xnsp, 0, sizeof( xnsp ) );
  xnsp.cfgSizeOfStruct = sizeof( XNetStartupParams );
  xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;
  INT err = XNetStartup(&xnsp);
  if (err != 0)
  {
#  ifdef NET_LOG_PEER_CHANNEL_FACTORY
#    ifdef NET_LOG_BRIEF
    NetLog("Fac: start failed with error %d",WSAGetLastError());
#    else
    NetLog("PeerChannelFactoryUDP: WSAStartup failed with error %d",WSAGetLastError());
#    endif
#  endif
    PeerChannelFactoryUDP::instances--;
    return;
  }
#endif

  // WSAStartup
#ifdef _WIN32
  WSADATA wsaData;
  BYTE major = (BYTE)floor(NetChannelBasic::par.winsockVersion);
  BYTE minor = (BYTE)floor(10.0 * (NetChannelBasic::par.winsockVersion - floor(NetChannelBasic::par.winsockVersion)) + 0.5);
  if ( WSAStartup(MAKEWORD(major,minor),&wsaData) == SOCKET_ERROR ) {
#  ifdef NET_LOG_PEER_CHANNEL_FACTORY
#    ifdef NET_LOG_BRIEF
    NetLog("Fac: start failed with error %d",WSAGetLastError());
#    else
    NetLog("PeerChannelFactoryUDP: WSAStartup failed with error %d",WSAGetLastError());
#    endif
#  endif
    WSACleanup(); // ??? why call cleanup when startup failed ???
    PeerChannelFactoryUDP::instances--; // FIX - Cleanup was already called
    return;
  }
#  ifdef NET_LOG_CREATE_PEER
#    ifdef NET_LOG_BRIEF
  NetLog("Fac:start(%u.%u,%u.%u,'%s','%s',%u,%u)",
    (unsigned)LOBYTE(wsaData.wVersion),(unsigned)HIBYTE(wsaData.wVersion),
    (unsigned)LOBYTE(wsaData.wHighVersion),(unsigned)HIBYTE(wsaData.wHighVersion),
    wsaData.szDescription,wsaData.szSystemStatus,(unsigned)wsaData.iMaxSockets,(unsigned)wsaData.iMaxUdpDg);
#    else
  NetLog("PeerChannelFactoryUDP: WSAStartup succeeded - version=%u.%u, highest=%u.%u, descr='%s'",
    (unsigned)LOBYTE(wsaData.wVersion),(unsigned)HIBYTE(wsaData.wVersion),
    (unsigned)LOBYTE(wsaData.wHighVersion),(unsigned)HIBYTE(wsaData.wHighVersion),
    wsaData.szDescription);
  NetLog("PeerChannelFactoryUDP: WSAStartup - status='%s', maxSockets=%u, maxUdpDg=%u",
    wsaData.szSystemStatus,(unsigned)wsaData.iMaxSockets,(unsigned)wsaData.iMaxUdpDg);
#    endif
#  endif
#endif

  // XOnlineStartup
#if _GAMES_FOR_WINDOWS
  HRESULT result = XOnlineStartup(NULL);
  if (result != S_OK)
  {
#  ifdef NET_LOG_PEER_CHANNEL_FACTORY
#    ifdef NET_LOG_BRIEF
    NetLog("Fac: online-start failed with error 0x%x", result);
#    else
    NetLog("PeerFactoryUDP: XOnlineStartup failed with error 0x%x", result);
#    endif
#  endif
  }
#endif
}

void doneNetworkUDP()
{
  if ( --PeerChannelFactoryUDP::instances <= 0 ) {
    PeerChannelFactoryUDP::instances = 0;
#if _GAMES_FOR_WINDOWS
    XOnlineCleanup();
#endif
#ifdef _WIN32
    WSACleanup();
#endif
#if _GAMES_FOR_WINDOWS
    XNetCleanup();
#endif
  }
}

PeerChannelFactoryUDP::PeerChannelFactoryUDP ()
{
  LockRegister(lock,"PeerChannelFactoryUDP");
  initNetworkUDP();
}

/*!
\patch 5163 Date 6/4/2007 by Jirka
- Fixed: Connection to GameSpy during dedicated server reporting was sometimes lost (Windows 2000 SP2 or newer are required now)
*/

NetPeer *PeerChannelFactoryUDP::createPeer ( NetPool *pool, BitMask *tryPorts, bool useVDP, RawMessageCallback callback )
{
    Assert( pool );
    if ( !tryPorts ) tryPorts = pool->getLocalPorts();
    if ( !tryPorts ) return NULL;
    int port = tryPorts->getFirst();
    if ( port == BitMask::END ) return NULL;
    SOCKET s = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if ( s == INVALID_SOCKET ) {
#ifdef NET_LOG_CREATE_PEER
#  ifdef _WIN32
#    ifdef NET_LOG_BRIEF
        NetLog("Fac:cp: socket() failed with error %d",WSAGetLastError());
#    else
        NetLog("PeerChannelFactoryUDP::createPeer: socket() failed with error %d",WSAGetLastError());
#    endif
#  else
#    ifdef NET_LOG_BRIEF
        NetLog("Fac:cp: socket() failed!");
#    else
        NetLog("PeerChannelFactoryUDP::createPeer: socket() failed!");
#    endif
#  endif
#endif
        return NULL;
        }

#if defined(_WIN32) 
    // disable the reporting of WSAECONNRESET to avoid calling of NetPeerUDP::reconnect()
    DWORD enableReporting = FALSE;
    DWORD returned = 0;
    // IOC_IN = 0x80000000;
    // IOC_VENDOR = 0x18000000;
    // SIO_UDP_CONNRESET = IOC_IN | IOC_VENDOR | 12;
    const DWORD SIO_UDP_CONNRESET = 0x9800000C;
    if (WSAIoctl(s, SIO_UDP_CONNRESET, &enableReporting, sizeof(enableReporting), NULL, 0, &returned, NULL, NULL) == SOCKET_ERROR)
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
        closesocket(s);
        return NULL;
      }
    }
#endif

    int tmp = 1;
    if ( setsockopt(s,SOL_SOCKET,SO_BROADCAST,(char*)&tmp,sizeof(tmp)) == SOCKET_ERROR ) {
#ifdef NET_LOG_CREATE_PEER
#  ifdef _WIN32
#    ifdef NET_LOG_BRIEF
        NetLog("Fac:cp: setsockopt(SO_BROADCAST) failed with error %d",WSAGetLastError());
#    else
        NetLog("PeerChannelFactoryUDP::createPeer: setsockopt(SO_BROADCAST) failed with error %d",WSAGetLastError());
#    endif
#  else
#    ifdef NET_LOG_BRIEF
        NetLog("Fac:cp: setsockopt(SO_BROADCAST) failed!");
#    else
        NetLog("PeerChannelFactoryUDP::createPeer: setsockopt(SO_BROADCAST) failed!");
#    endif
#  endif
#endif
        closesocket(s);
        return NULL;
        }
    tmp = NetChannelBasic::par.rcvBufSize;
    setsockopt(s,SOL_SOCKET,SO_RCVBUF,(char*)&tmp,sizeof(tmp));
    //setsockopt(s,SOL_SOCKET,SO_SNDBUF,(char*)&tmp,sizeof(tmp));

    DisableMTUDiscovery(s);

#ifdef NET_LOG_CREATE_PEER
    unsigned maxMsgSize = 0;
    socklen_t dummy = 4;
#  ifdef SO_MAX_MSG_SIZE
    if ( getsockopt(s,SOL_SOCKET,SO_MAX_MSG_SIZE,(char*)&maxMsgSize,&dummy) ) {
#    ifdef _WIN32
#      ifdef NET_LOG_BRIEF
        NetLog("Fac:cp: getsockopt(SO_MAX_MSG_SIZE) failed with error %d",WSAGetLastError());
#      else
        NetLog("PeerChannelFactoryUDP::createPeer: getsockopt(SO_MAX_MSG_SIZE) failed with error %d",WSAGetLastError());
#      endif
#    else
#      ifdef NET_LOG_BRIEF
        NetLog("Fac:cp: getsockopt(SO_MAX_MSG_SIZE) failed!");
#      else
        NetLog("PeerChannelFactoryUDP::createPeer: getsockopt(SO_MAX_MSG_SIZE) failed!");
#      endif
#    endif
        }
#  endif
    int rcvBuf = 0;
    int sndBuf = 0;
    if ( getsockopt(s,SOL_SOCKET,SO_RCVBUF,(char*)&rcvBuf,&dummy) ) {
#  ifdef _WIN32
#    ifdef NET_LOG_BRIEF
        NetLog("Fac:cp: getsockopt(SO_RCVBUF) failed with error %d",WSAGetLastError());
#    else
        NetLog("PeerChannelFactoryUDP::createPeer: getsockopt(SO_RCVBUF) failed with error %d",WSAGetLastError());
#    endif
#  else
#    ifdef NET_LOG_BRIEF
        NetLog("Fac:cp: getsockopt(SO_RCVBUF) failed!");
#    else
        NetLog("PeerChannelFactoryUDP::createPeer: getsockopt(SO_RCVBUF) failed!");
#    endif
#  endif
        }
    if ( getsockopt(s,SOL_SOCKET,SO_SNDBUF,(char*)&sndBuf,&dummy) ) {
#  ifdef _WIN32
#    ifdef NET_LOG_BRIEF
        NetLog("Fac:cp: getsockopt(SO_SNDBUF) failed with error %d",WSAGetLastError());
#    else
        NetLog("PeerChannelFactoryUDP::createPeer: getsockopt(SO_SNDBUF) failed with error %d",WSAGetLastError());
#    endif
#  else
#    ifdef NET_LOG_BRIEF
        NetLog("Fac:cp: getsockopt(SO_SNDBUF) failed!");
#    else
        NetLog("PeerChannelFactoryUDP::createPeer: getsockopt(SO_SNDBUF) failed!");
#    endif
#  endif
        }
#endif
#if defined(SO_PROTOCOL_INFO) && (defined(NET_LOG_CREATE_PEER) || defined(NET_LOG_PEER_PARAMS))
    WSAPROTOCOL_INFO info;
#endif
#if defined(NET_LOG_PEER_PARAMS) && defined(SO_PROTOCOL_INFO)
    dummy = sizeof(info);
    if ( getsockopt(s,SOL_SOCKET,SO_PROTOCOL_INFO,(char*)&info,&dummy) ) {
#  ifdef _WIN32
#    ifdef NET_LOG_BRIEF
        NetLog("Fac:cp: getsockopt(SO_PROTOCOL_INFO) failed with error %d",WSAGetLastError());
#    else
        NetLog("PeerChannelFactoryUDP::createPeer: getsockopt(SO_PROTOCOL_INFO) failed with error %d",WSAGetLastError());
#    endif
#  else
#    ifdef NET_LOG_BRIEF
        NetLog("Fac:cp: getsockopt(SO_PROTOCOL_INFO) failed!");
#    else
        NetLog("PeerChannelFactoryUDP::createPeer: getsockopt(SO_PROTOCOL_INFO) failed!");
#    endif
#  endif
        }
#endif
    struct sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = bindIPAddress;
    do {                                    // try one port number
        local.sin_port = htons(port);
        if ( bind(s,(struct sockaddr*)&local,sizeof(local)) != SOCKET_ERROR ) break;
        } while ( (port = tryPorts->getNext(port)) != BitMask::END );
    if ( port == BitMask::END ) {
        closesocket(s);
#ifdef NET_LOG_CREATE_PEER
#  ifdef NET_LOG_BRIEF
        NetLog("Fac:cp: no free ports are available!");
#  else
        NetLog("PeerChannelFactoryUDP::createPeer: no free ports are available!");
#  endif
#endif
        return NULL;
        }
#if defined(NET_LOG_PEER_PARAMS) && defined(SO_MAX_MSG_SIZE)
    if ( info.dwMessageSize == 1 ) {
        dummy = 4;
        if ( getsockopt(s,SOL_SOCKET,SO_MAX_MSG_SIZE,(char*)&maxMsgSize,&dummy) ) {
#  ifdef _WIN32
#    ifdef NET_LOG_BRIEF
            NetLog("Fac:cp: getsockopt(SO_MAX_MSG_SIZE) failed with error %d",WSAGetLastError());
#    else
            NetLog("PeerChannelFactoryUDP::createPeer: getsockopt(SO_MAX_MSG_SIZE) failed with error %d",WSAGetLastError());
#    endif
#  else
#    ifdef NET_LOG_BRIEF
            NetLog("Fac:cp: getsockopt(SO_MAX_MSG_SIZE) failed!");
#    else
            NetLog("PeerChannelFactoryUDP::createPeer: getsockopt(SO_MAX_MSG_SIZE) failed!");
#    endif
#  endif
            }
        }
#endif
#ifdef NET_LOG_CREATE_PEER
#  ifdef NET_LOG_BRIEF
    NetLog("Fac:cp(%d,%u,%d,%d)",
           port,maxMsgSize,rcvBuf,sndBuf);
#  else
    NetLog("PeerChannelFactoryUDP::createPeer: using local port=%d, MAX_MSG_SIZE=%u, RCVBUF=%d, SNDBUF=%d",
           port,maxMsgSize,rcvBuf,sndBuf);
#  endif
#  ifdef XP1_SUPPORT_BROADCAST
#    ifdef NET_LOG_BRIEF
    NetLog("Fac:cp(%d,%d,%u,'%s')",
           (info.dwServiceFlags1 & XP1_SUPPORT_BROADCAST) > 0 ? 1 : 0,
           info.iVersion,(unsigned)info.dwMessageSize,info.szProtocol);
#    else
    NetLog("PeerChannelFactoryUDP::createPeer: bcast=%d, version=%d, msgSize=%u, protocol='%s'",
           (info.dwServiceFlags1 & XP1_SUPPORT_BROADCAST) > 0 ? 1 : 0,
           info.iVersion,(unsigned)info.dwMessageSize,info.szProtocol);
#    endif
#  endif
#endif
    return new NetPeerUDP(s,port,pool,callback);
}

NetChannel *PeerChannelFactoryUDP::createChannel ( NetPool *pool, bool control )
{
    return new NetChannelBasic(control);
}

PeerChannelFactoryUDP::~PeerChannelFactoryUDP ()
{
  doneNetworkUDP();
}
