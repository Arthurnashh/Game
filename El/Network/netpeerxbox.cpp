/**
@file   netpeerxbox.cpp
@brief  Network peer object (implementation for Xbox)

Copyright &copy; 2003 by BIStudio (www.bistudio.com)
@author PE
@since  19.2.2003
@date   18.11.2003
*/

#include "El/Network/netpch.hpp"

#if _XBOX_SECURE

#include "El/Network/netpeerxbox.hpp"
#include "El/Network/netchannel.hpp"

#if _ENABLE_REPORT
#  include "El/Debugging/stackMeter.hpp"
#endif

//------------------------------------------------------------
//  NetPeerXbox: time & statistics constants

/// Listener/sender thread timeout in microseconds.
const long TIMEOUT = 5000;

/// Number of udpListen() loop passes before connectivity check is performed (adjusted for 2 seconds for channels absolutely w/o traffic).
const unsigned CHECK_COUNTER = 2000000 / TIMEOUT;

/// Number of udpListenSend() loop passes before NetChannel::tick is performaed.
const unsigned TICK_COUNTER = (unsigned)( (4*NetChannelBasic::RUN_INTERVAL) / TIMEOUT );

/// How many packets are received/transmitted in one batch. For udpListenSend only.
const unsigned PACKET_BATCH = 3;

#ifdef NET_STRESS
/// NetStress config file check interval in microseconds.
const unsigned64 NET_STRESS_INTERVAL = 10000000;
#endif

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
  XNADDR xna;
  DWORD status;
  do
  status = XNetGetTitleXnAddr(&xna);
  while ( status == XNET_GET_XNADDR_PENDING );

  if ( status == XNET_GET_XNADDR_NONE )
  {
#ifdef NET_LOG_GET_LOCAL_ADDRESS
    NetLog("getLocalAddress: XNetGetTitleXnAddr failed!");
#endif
    return false;
  }

  me.sin_addr = xna.ina;
  me.sin_port = htons(port);
  return true;
}

/**
Retrieves name of the local host.
@param  name Buffer to hold the result.
@param  len Buffer length.
@return <code>true</code> if succeeded.
*/
bool getLocalName ( char *name, unsigned len )
{
  return false;
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
    if ( host.sin_addr.s_addr == INADDR_NONE )
      return false;
  }
  return true;
}

//------------------------------------------------------------
//  Common asynchronous listener/send thread:

/*!
\patch_internal 2.01 Date 2/12/2003 by Pepca
- Improved: udpListen and udpSend threads were merged into one (udpListenSend).
*/

THREAD_PROC_RETURN THREAD_PROC_MODE udpListenSend ( void *param )
{
  // receiver/common variables:
#pragma pack(push)
#pragma pack(1)
  union {
    char data[MAX_IN_DATA+2];           // fixed buffer to receive message data
    struct {
      unsigned16 encryptedLength;
      MsgHeader header;
    };
  };
#pragma pack(pop)
  NetPeerXbox *peer = (NetPeerXbox*)param;
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

#ifdef NET_STRESS
#  ifdef NET_LOG
  NetStress netStress(NET_STRESS_INTERVAL,peer->getPeerId());
#  else
  NetStress netStress(NET_STRESS_INTERVAL,0);
#  endif
#endif

#if _ENABLE_REPORT
  STACK_INIT(58*1024);
#endif

#ifdef NET_LOG_UDP_LISTEN
#  ifdef NET_LOG_BRIEF
  NetLog("Pe(%u):list(%u)",peer->getPeerId(),peer->getLocalPort());
#  else
  NetLog("Peer(%u)::udpListenSend: start listening/sending at local port %u",peer->getPeerId(),peer->getLocalPort());
#  endif
#endif

  while ( peer->listen ) {                // check it at least each 50ms (according to "TIMEOUT")

    // 0. suspended mode check
    peer->enter();
    if ( peer->suspended ) {
      previousBatch = false;
      peer->leave();
      SLEEP_MS(TIMEOUT/1000);
      continue;
    }
    else
      peer->leave();

    // 1. receiver batch
    for ( batchIt = 0; batchIt++ < PACKET_BATCH; )
    {
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
      error = recvfrom(peer->sock, peer->vdp ? data : data+2 ,MAX_IN_DATA-1,0,(struct sockaddr*)&from,&fromLen);
      if ( error != SOCKET_ERROR && peer->vdp ) error -= 2;
      if ( error != SOCKET_ERROR && error == (int)header.length &&
        error >= MSG_HEADER_LEN && error <= MAX_IN_DATA )
#ifdef NET_STRESS
        if ( netStress.valid() )
          netStress.insertData(data+2,error,from,getSystemTime());
        else
#endif
        {
          // CRC check:
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
#if 0
          LogF("udpListenSend: received len=%3d, serial=%4u, flags=%04x, wait=%u ms",
            (int)header.length-MSG_HEADER_LEN,(unsigned)header.serial,(unsigned)header.flags,waitTime);
#endif
          unsigned32 crc = header.crc;
          header.crc = 0;
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
#ifdef NET_LOG_UDP_RECEIVE
            NetLog("Peer(%u)::udpListenSend: message processed (serial=%4u)",
              peer->getPeerId(),(unsigned)header.serial);
#endif
          }
#if defined NET_LOG_UDP_LISTEN
          else
#  ifdef NET_LOG_BRIEF
            NetLog("Pe(%u):err-crc",peer->getPeerId());
#  else
            NetLog("Peer(%u)::udpListenSend: received message has bad CRC!",peer->getPeerId());
#  endif
#endif
        }
      else
      {                               // receiver error
        int werror = WSAGetLastError();
        WSASetLastError(0);         // to be sure!
#ifdef NET_LOG_UDP_LISTEN
#  ifdef NET_LOG_BRIEF
        NetLog("Pe(%u):err-l(%x,%d)",
          peer->getPeerId(),(unsigned)peer->sock,werror);
#  else
        NetLog("Peer(%u)::udpListen: error reading data (socket=%x, error=%d)",
          peer->getPeerId(),(unsigned)peer->sock,werror);
#  endif
#endif
      }

      waitTime = 0;
      previousBatch = true;
    }                                   // receiver batch

#ifdef NET_STRESS
    // 1b. net-stress receiver batch
    if ( netStress.valid() )
    {
      char *defData;
      int defLen;
      now = getSystemTime();
      while ( (defData = netStress.getData(now,defLen,from)) )
      {                                 // I have one deferred message to process
        memcpy(data+2,defData,defLen);
        netStress.removeData(defData);
#ifdef NET_LOG_UDP_RECEIVE
        NetLog("Peer(%u)::udpListenSend: received deferred message (from=%u.%u.%u.%u:%u,len=%3d,serial=%4u,flags=%04x)",
          peer->getPeerId(),
          (unsigned)IP4(from),(unsigned)IP3(from),(unsigned)IP2(from),(unsigned)IP1(from),(unsigned)PORT(from),
          (int)header.length-MSG_HEADER_LEN,(unsigned)header.serial,(unsigned)header.flags);
#endif

        // CRC check:
        unsigned32 crc = header.crc;
        header.crc = 0;
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
#ifdef NET_LOG_UDP_RECEIVE
          NetLog("Peer(%u)::udpListenSend: message processed (serial=%4u)",
            peer->getPeerId(),(unsigned)header.serial);
#endif
        }
#ifdef NET_LOG_UDP_LISTEN
        else
#  ifdef NET_LOG_BRIEF
          NetLog("Pe(%u):err-crc",peer->getPeerId());
#  else
          NetLog("Peer(%u)::udpListenSend: received message has bad CRC!",peer->getPeerId());
#  endif
#endif
      }
    }
#endif  // NET_STRESS

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
      if ( peer->sendData(channel->prepared->header,dist,channel->prepared->encryptedLength) != nsError )
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
#ifdef NET_STRESS
      netStress.tick(getSystemTime());
#endif
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

#ifdef NET_LOG_UDP_SENDING
    NetLog("Peer(%u)::udpListenSend: looping",peer->getPeerId());
#endif
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
  unsigned sUsage = STACK_METER(58*1024);
  if ( sUsage )
  {
#ifdef NET_LOG
#  ifdef NET_LOG_BRIEF
    NetLog("SM(udp,64K): %u",sUsage);
#  else
    NetLog("StackMeter(udpListenSend,64KB): %u bytes",sUsage);
#  endif
#endif
    LogF("StackMeter(udpListenSend,64KB): %u bytes",sUsage);
  }
  else
  {
#ifdef NET_LOG
#  ifdef NET_LOG_BRIEF
    NetLog("SM(udp,64K) overflow");
#  else
    NetLog("StackMeter(udpListenSend,64KB) overflow");
#  endif
#endif
    LogF("StackMeter(udpListenSend,64KB) overflow");
  }
#endif

  return (THREAD_PROC_RETURN)0;
}

//------------------------------------------------------------
//  NetPeerXbox class:

NetPeerXbox::NetPeerXbox ( NetPool *_pool ) : NetPeer(_pool), chMap(1)
{
  LockRegister(lock,"NetPeerXbox");
  sock = INVALID_SOCKET;
  port = 0;
  listen = suspended = false;
}

NetPeerXbox::NetPeerXbox ( SOCKET _sock, unsigned16 _port, NetPool *_pool, bool _vdp )
: NetPeer(_pool), chMap(2)              // space for at least two channels..
{
  LockRegister(lock,"NetPeerXbox");
  enter();
  sock = _sock;
  vdp = _vdp;
  port = _port;                           // broadcast mode
  listen = suspended = false;
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
    NetLog("Peer(%u)::NetPeerXbox succeeded: %s",getPeerId(),getPeerInfo(buf));
#  endif
#endif
    // start asynchronous I/O thread(s):
    listen = true;
    if ( poThreadCreate(&listener,64*1024,&udpListenSend,this) ) {
      Verify( poSetPriority(listener,2) ); // the higher priority
    }
    else
      listen = false;                 // only "listen" flag is used
  }
  leave();
}

void NetPeerXbox::getLocalAddress ( struct sockaddr_in &local ) const
{
  ::getLocalAddress(local,port);
}

unsigned NetPeerXbox::maxMessageData () const
{
  return( MAX_XBOX_PAYLOAD - MSG_HEADER_LEN );
}

bool NetPeerXbox::registerChannel ( struct sockaddr_in &distant, NetChannel *ch )
{
  if ( !ch ) return false;
  enter();
  bool result = !chMap.presentKey(sockaddrKey(distant));
  if ( result ) chMap.put(ch);
  leave();
  // the new net-channel will receive incoming data automatically!
  return result;
}

void NetPeerXbox::unregisterChannel ( NetChannel *ch )
{
  if ( !ch ) return;
  chMap.removeValue(ch);
}

NetChannel *NetPeerXbox::findChannel ( const struct sockaddr_in &distant )
{
  RefD<NetChannel> result;
  chMap.get(sockaddrKey(distant),result);
  return result;
}

void NetPeerXbox::close ()
{
  enter();
  // stop sender & listener thread as early as possible:
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

void NetPeerXbox::suspendSocket ( bool susp )
{
  enter();
  suspended = susp;
  leave();
}

void NetPeerXbox::replaceSocket ( SOCKET _sock )
{
  enter();
  suspended = true;
  if ( sock != INVALID_SOCKET )
    closesocket(sock);
  sock = _sock;
  suspended = false;
  leave();
}

void NetPeerXbox::processData ( MsgHeader *hdr, const struct sockaddr_in &distant )
{
  // !!! TODO: peer statistics !!!
}

NetStatus NetPeerXbox::sendData ( MsgHeader *hdr, struct sockaddr_in distant, unsigned16 encrypted )
{
  enter();
  if ( sock == INVALID_SOCKET ) {
    leave();
    return nsError;
  }
  Assert( hdr );
  Assert( hdr->length );
  hdr->crc = 0;
  hdr->crc = crc32(0,(const unsigned8*)hdr,hdr->length);
  int result;
  if ( vdp ) {
    union {
      char data[MAX_IN_DATA+2];
      unsigned16 encryptedLength;
    };
    encryptedLength = encrypted;
    memcpy(data+2,hdr,hdr->length);
    result = sendto(sock,data,hdr->length+2,0,(const sockaddr*)&distant,sizeof(distant));
  }
  else
    result = sendto(sock,(const char*)hdr,hdr->length,0,(const sockaddr*)&distant,sizeof(distant));
  if ( result == SOCKET_ERROR ) {
    int werror = WSAGetLastError();
    WSASetLastError(0);                 // to be sure!
#ifdef NET_LOG_UDP_SEND
#  ifdef NET_LOG_BRIEF
    NetLog("Pe(%u):err-s(%d)",getPeerId(),werror);
#  else
    NetLog("Peer(%u)::sendData: sendto() failed with error: %d",getPeerId(),werror);
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

void NetPeerXbox::cancelAllMessages ()
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

unsigned NetPeerXbox::initDispatcherStatus ( DispatcherStatus *data )
{
  if ( data ) {
    DispatcherStatusBasic *ds = (DispatcherStatusBasic*)data;
    memset(ds,0,sizeof(*ds));           // fast solution
    ds->structLen = sizeof(DispatcherStatusBasic);
  }
  return sizeof(DispatcherStatusBasic);
}

NetPeerXbox::~NetPeerXbox ()
{
#if defined(NET_LOG_DESTRUCTOR) || defined(NET_LOG_PEER)
#  ifdef NET_LOG_BRIEF
  NetLog("Pe(%u):~(%d,%x,%u,%u)",
    getPeerId(),(int)listen,(unsigned)sock,(unsigned)port,chMap.card());
#  else
  NetLog("Peer(%u)::~NetPeerXbox: listening/sending=%d, socket=%x, port=%u, |chMap|=%u",
    getPeerId(),(int)listen,(unsigned)sock,(unsigned)port,chMap.card());
#  endif
#endif
  close();
}

XboxPeerToPeerChannel::XboxPeerToPeerChannel()
{
  _port = 0;
}

THREAD_PROC_RETURN THREAD_PROC_MODE voiceListenSend(void*context)
{
  XboxPeerToPeerChannel *chan = (XboxPeerToPeerChannel *)context;
  chan->Thread();
  return (THREAD_PROC_RETURN)0;
}


bool XboxPeerToPeerChannel::Init(int port)
{

  // The direct socket is a non-blocking socket on port DIRECT_PORT.
  // Sockets are encrypted by default, but can have encryption disabled
  // as an optimization for non-secure messaging
  bool bSuccess = _direct.Open( PPSocket::Type_VDP );
  if( !bSuccess )
  {
    LogF( "Peer to peer socket open failed", WSAGetLastError() );
    return false;
  }

  SOCKADDR_IN directAddr;
  directAddr.sin_family      = AF_INET;
  directAddr.sin_addr.s_addr = INADDR_ANY;
  directAddr.sin_port        = htons( port );
  int result = _direct.Bind( &directAddr );
  if( result == SOCKET_ERROR )
  {
    LogF( "Peer to peer socket bind failed", WSAGetLastError() );
    return false;
  }
  DWORD nonBlocking = 1;
  result = _direct.IoCtlSocket( FIONBIO, &nonBlocking );
  if (result == SOCKET_ERROR )
  {
    LogF( "Peer to peer socket ioCtl failed", WSAGetLastError() );
  }
  
  _port = port;
  
  _process = NULL;

  _endListener = false;
  // create a listener thread
  if ( poThreadCreate(&_listener,64*1024,&voiceListenSend,this) )
  {
    Verify( poSetPriority(_listener,2) ); // the higher priority
  }
  return true;
}

void XboxPeerToPeerChannel::RegisterCallback(ProcessF *process, void *ctx)
{
  _process = process;
  _ctx = ctx;
}

XboxPeerToPeerChannel::~XboxPeerToPeerChannel()
{
  // bool write is atomic
  _endListener = true;
  Verify (poThreadJoin(_listener, NULL));
  _process = NULL;
}

  // send in open
void XboxPeerToPeerChannel::SendMessage(
  const sockaddr_in &sa, const void *data, int size, int sizeEncrypted
)
{
  // convert to VDP format
  union
  {
    char vdpData[MAX_IN_DATA+2];
    unsigned16 encryptedLength;
  };
  encryptedLength = sizeEncrypted;
  memcpy(vdpData+2,data,size);

  // avoid using the same socket twice
  enter();
  _direct.SendTo( vdpData, size+2, &sa );
  leave();
}

void XboxPeerToPeerChannel::Thread()
{
  for(;;)
  {
    if (_endListener)
    {
      return;
    }

    union
    {
      char vdpData[MAX_IN_DATA+2];
      unsigned16 encryptedLength;
    };
    
    SOCKADDR_IN saFromIn;
    enter();
    int result = _direct.RecvFrom( vdpData, sizeof(vdpData), &saFromIn );
    leave();
    //SOCKADDR_IN saFrom = saFromIn;

    // If message waiting, process it
    if( result != SOCKET_ERROR && result > 0)
    {
      if (_process && result>2)
      {
        (*_process)(vdpData+2,result-2,&saFromIn,_ctx);
      }
    }
    else
    {
      // if no data are ready, sleep
      //Assert( WSAGetLastError() == WSAEWOULDBLOCK );
      SLEEP_MS(20);
    }
  }
}


#endif
