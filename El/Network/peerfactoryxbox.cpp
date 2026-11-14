/**
  @file   peerfactoryxbox.cpp
  @brief  Net-peer factory creating NetPeerXbox instances

  Copyright &copy; 2001-2004 by BIStudio (www.bistudio.com)
  @author PE
  @since  3.12.2001
  @date   24.3.2004
*/

#include "El/Network/netpch.hpp"

#if _XBOX_SECURE

#include "El/Network/netpeerxbox.hpp"
#include "El/Network/netchannel.hpp"
#include "El/Network/peerfactoryxbox.hpp"

//------------------------------------------------------------
//  PeerChannelFactoryXbox class:

int PeerChannelFactoryXbox::instances = 0;

void initNetworkXbox()
{
  if ( PeerChannelFactoryXbox::instances++ ) return;

  XNetStartupParams xnsp;
  memset( &xnsp, 0, sizeof( xnsp ) );
  xnsp.cfgSizeOfStruct = sizeof( XNetStartupParams );
  INT err = XNetStartup(&xnsp);
  if (err != 0)
  {
#ifdef NET_LOG_PEER_CHANNEL_FACTORY
#  ifdef NET_LOG_BRIEF
    NetLog("Fac: start failed with error %d",WSAGetLastError());
#  else
    NetLog("PeerFactoryXbox: XNetStartup failed with error %d", err);
#  endif
#endif
    PeerChannelFactoryXbox::instances--;
    return;
  }

  // WSAStartup
  WSADATA wsaData;
  BYTE major = (BYTE)floor(NetChannelBasic::par.winsockVersion);
  BYTE minor = (BYTE)floor(10.0 * (NetChannelBasic::par.winsockVersion - floor(NetChannelBasic::par.winsockVersion)) + 0.5);
  if ( WSAStartup(MAKEWORD(major,minor),&wsaData) == SOCKET_ERROR ) {
#ifdef NET_LOG_PEER_CHANNEL_FACTORY
#  ifdef NET_LOG_BRIEF
    NetLog("Fac: start failed with error %d",WSAGetLastError());
#  else
    NetLog("PeerFactoryXbox: WSAStartup failed with error %d",WSAGetLastError());
#  endif
#endif
    WSACleanup(); // ??? why call cleanup when startup failed ???
    PeerChannelFactoryXbox::instances--; // FIX - Cleanup was already called
    return;
  }

#ifdef NET_LOG_CREATE_PEER
#  ifdef NET_LOG_BRIEF
  NetLog("Fac:start(%u.%u,%u.%u,'%s','%s',%u,%u)",
    (unsigned)LOBYTE(wsaData.wVersion),(unsigned)HIBYTE(wsaData.wVersion),
    (unsigned)LOBYTE(wsaData.wHighVersion),(unsigned)HIBYTE(wsaData.wHighVersion),
    wsaData.szDescription,wsaData.szSystemStatus,(unsigned)wsaData.iMaxSockets,(unsigned)wsaData.iMaxUdpDg);
#  else
  NetLog("PeerChannelFactoryXbox: WSAStartup succeeded - version=%u.%u, highest=%u.%u, descr='%s'",
    (unsigned)LOBYTE(wsaData.wVersion),(unsigned)HIBYTE(wsaData.wVersion),
    (unsigned)LOBYTE(wsaData.wHighVersion),(unsigned)HIBYTE(wsaData.wHighVersion),
    wsaData.szDescription);
  NetLog("PeerChannelFactoryXbox: WSAStartup - status='%s', maxSockets=%u, maxUdpDg=%u",
    wsaData.szSystemStatus,(unsigned)wsaData.iMaxSockets,(unsigned)wsaData.iMaxUdpDg);
#  endif
#endif

  // XOnlineStartup
  HRESULT result = XOnlineStartup();
  if ( result != S_OK )
  {
#ifdef NET_LOG_PEER_CHANNEL_FACTORY
#  ifdef NET_LOG_BRIEF
    NetLog("Fac: online-start failed with error 0x%x",result);
#  else
    NetLog("PeerFactoryXbox: XOnlineStartup failed with error 0x%x", result);
#  endif
#endif
  }
}

void doneNetworkXbox()
{
  if ( --PeerChannelFactoryXbox::instances <= 0 ) {
    PeerChannelFactoryXbox::instances = 0;
    XOnlineCleanup();
    WSACleanup();
    XNetCleanup();
  }
}

PeerChannelFactoryXbox::PeerChannelFactoryXbox ()
{
  LockRegister(lock,"PeerChannelFactoryXbox");
  initNetworkXbox();
}

NetPeer *PeerChannelFactoryXbox::createPeer ( NetPool *pool, BitMask *tryPorts, bool useVDP, RawMessageCallback callback )
{
    Assert( pool );
    if ( !tryPorts ) tryPorts = pool->getLocalPorts();
    if ( !tryPorts ) return NULL;
    int port = tryPorts->getFirst();
    if ( port == BitMask::END ) return NULL;
    SOCKET s = socket(AF_INET,SOCK_DGRAM, useVDP ? IPPROTO_VDP : IPPROTO_UDP );
    if ( s == INVALID_SOCKET ) {
#ifdef NET_LOG_CREATE_PEER
#  ifdef NET_LOG_BRIEF
        NetLog("Fac:cp: socket() failed with error %d",WSAGetLastError());
#  else
        NetLog("PeerChannelFactoryXbox::createPeer: socket() failed with error %d",WSAGetLastError());
#  endif
#endif
        return NULL;
        }
    int tmp = 1;
    if ( !useVDP && setsockopt(s,SOL_SOCKET,SO_BROADCAST,(char*)&tmp,sizeof(tmp)) == SOCKET_ERROR ) {
#ifdef NET_LOG_CREATE_PEER
#  ifdef NET_LOG_BRIEF
        NetLog("Fac:cp: setsockopt(SO_BROADCAST) failed with error %d",WSAGetLastError());
#  else
        NetLog("PeerChannelFactoryXbox::createPeer: setsockopt(SO_BROADCAST) failed with error %d",WSAGetLastError());
#  endif
#endif
        closesocket(s);
        return NULL;
        }
    tmp = NetChannelBasic::par.rcvBufSize;
    setsockopt(s,SOL_SOCKET,SO_RCVBUF,(char*)&tmp,sizeof(tmp));
    //setsockopt(s,SOL_SOCKET,SO_SNDBUF,(char*)&tmp,sizeof(tmp));
#ifdef NET_LOG_CREATE_PEER
    unsigned maxMsgSize = 0;
    socklen_t dummy = 4;
#  ifdef SO_MAX_MSG_SIZE
    if ( getsockopt(s,SOL_SOCKET,SO_MAX_MSG_SIZE,(char*)&maxMsgSize,&dummy) ) {
#    ifdef NET_LOG_BRIEF
        NetLog("Fac:cp: getsockopt(SO_MAX_MSG_SIZE) failed with error %d",WSAGetLastError());
#    else
        NetLog("PeerChannelFactoryXbox::createPeer: getsockopt(SO_MAX_MSG_SIZE) failed with error %d",WSAGetLastError());
#    endif
        }
#  endif
    int rcvBuf = 0;
    int sndBuf = 0;
    if ( getsockopt(s,SOL_SOCKET,SO_RCVBUF,(char*)&rcvBuf,&dummy) ) {
#  ifdef NET_LOG_BRIEF
        NetLog("Fac:cp: getsockopt(SO_RCVBUF) failed with error %d",WSAGetLastError());
#  else
        NetLog("PeerChannelFactoryXbox::createPeer: getsockopt(SO_RCVBUF) failed with error %d",WSAGetLastError());
#  endif
        }
    if ( getsockopt(s,SOL_SOCKET,SO_SNDBUF,(char*)&sndBuf,&dummy) ) {
#  ifdef NET_LOG_BRIEF
        NetLog("Fac:cp: getsockopt(SO_SNDBUF) failed with error %d",WSAGetLastError());
#  else
        NetLog("PeerChannelFactoryXbox::createPeer: getsockopt(SO_SNDBUF) failed with error %d",WSAGetLastError());
#  endif
        }
#endif
#if defined(SO_PROTOCOL_INFO) && (defined(NET_LOG_CREATE_PEER) || defined(NET_LOG_PEER_PARAMS))
    WSAPROTOCOL_INFO info;
#endif
#if defined(NET_LOG_PEER_PARAMS) && defined(SO_PROTOCOL_INFO)
    dummy = sizeof(info);
    if ( getsockopt(s,SOL_SOCKET,SO_PROTOCOL_INFO,(char*)&info,&dummy) ) {
#  ifdef NET_LOG_BRIEF
        NetLog("Fac:cp: getsockopt(SO_PROTOCOL_INFO) failed with error %d",WSAGetLastError());
#  else
        NetLog("PeerChannelFactoryXbox::createPeer: getsockopt(SO_PROTOCOL_INFO) failed with error %d",WSAGetLastError());
#  endif
        }
#endif
    struct sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
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
        NetLog("PeerChannelFactoryXbox::createPeer: no free ports are available!");
#  endif
#endif
        return NULL;
        }
#if defined(NET_LOG_PEER_PARAMS) && defined(SO_MAX_MSG_SIZE)
    if ( info.dwMessageSize == 1 ) {
        dummy = 4;
        if ( getsockopt(s,SOL_SOCKET,SO_MAX_MSG_SIZE,(char*)&maxMsgSize,&dummy) ) {
#  ifdef NET_LOG_BRIEF
            NetLog("Fac:cp: getsockopt(SO_MAX_MSG_SIZE) failed with error %d",WSAGetLastError());
#  else
            NetLog("PeerChannelFactoryXbox::createPeer: getsockopt(SO_MAX_MSG_SIZE) failed with error %d",WSAGetLastError());
#  endif
            }
        }
#endif
#ifdef NET_LOG_CREATE_PEER
#  ifdef NET_LOG_BRIEF
    NetLog("Fac:cp(%d,%u,%d,%d)",
           port,maxMsgSize,rcvBuf,sndBuf);
#  else
    NetLog("PeerChannelFactoryXbox::createPeer: using local port=%d, MAX_MSG_SIZE=%u, RCVBUF=%d, SNDBUF=%d",
           port,maxMsgSize,rcvBuf,sndBuf);
#  endif
#  ifdef XP1_SUPPORT_BROADCAST
#    ifdef NET_LOG_BRIEF
    NetLog("Fac:cp(%d,%d,%u,'%s')",
           (info.dwServiceFlags1 & XP1_SUPPORT_BROADCAST) > 0 ? 1 : 0,
           info.iVersion,(unsigned)info.dwMessageSize,info.szProtocol);
#    else
    NetLog("PeerChannelFactoryXbox::createPeer: bcast=%d, version=%d, msgSize=%u, protocol='%s'",
           (info.dwServiceFlags1 & XP1_SUPPORT_BROADCAST) > 0 ? 1 : 0,
           info.iVersion,(unsigned)info.dwMessageSize,info.szProtocol);
#    endif
#  endif
#endif
    return new NetPeerXbox(s,port,pool,useVDP);
}

NetChannel *PeerChannelFactoryXbox::createChannel ( NetPool *pool, bool control )
{
    return new NetChannelBasic(control);
}

PeerChannelFactoryXbox::~PeerChannelFactoryXbox ()
{
  doneNetworkXbox();
}

#endif
