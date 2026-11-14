/*
  @file   netapi.cpp
  @brief  API for abstract network transport layer.
  
  It should use unreliable underlying network protocol.
  <p>Copyright &copy; 2001-2003 by BIStudio (www.bistudio.com)
  @author PE
  @since  27.11.2001
  @date   18.11.2003
*/

#include "El/Network/netpch.hpp"
#include "El/Network/netpeer.hpp"
#include "El/Network/netchannel.hpp"

//------------------------------------------------------------
//  Support (ImplicitMap routines):

unsigned64 channelKey (const RefD<NetChannel> &ch)
{
    struct sockaddr_in addr;
    ch->getDistantAddress(addr);
    return sockaddrKey(addr);
}

RefD<NetChannel> ImplicitMapTraits< RefD<NetChannel> >::zombie = (NetChannel*)1;
RefD<NetChannel> ImplicitMapTraits< RefD<NetChannel> >::null;

unsigned16 netPeerToPort (const RefD<NetPeer> &peer)
{
    if ( peer.IsNull() ) return 0;
    return peer->getPort();
}

RefD<NetPeer> ImplicitMapTraits< RefD<NetPeer> >::zombie = (NetPeer*)1;
RefD<NetPeer> ImplicitMapTraits< RefD<NetPeer> >::null;

//------------------------------------------------------------
//  Network parameters:

#ifdef NET_LOG

void NetworkParams::printParams1 ( char *buf )
  // Prints out actual network parameters (part 1). Buffer must hold at least 256 chars.
{
  Assert( buf );
  sprintf(buf,"%.1f,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%.3f,%.1f,%.3f,%u,%u,%u",
          (double)winsockVersion,rcvBufSize,maxPacketSize,dropGap,ackTimeoutA,ackTimeoutB,
          ackRedundancy,initBandwidth,minBandwidth,maxBandwidth,minActivity,initLatency,
          minLatencyUpdate,(double)minLatencyMul,(double)minLatencyAdd,(double)goodAckBandFade,
          outWindow,ackWindow,maxChannelBitMask
          );
}

void NetworkParams::printParams2 ( char *buf )
  // Prints out actual network parameters (part 2). Buffer must hold at least 256 chars.
{
  Assert( buf );
  sprintf(buf,"%.2f,%.1f,%u,%u,%.4f,%.4f,%.4f,%.4f,%.2f,%.1f,%.2f,%.1f,%.2f,%.1f,%.2f,%.1f"
          ",%.2f,%.2f",
          (double)lostLatencyMul,(double)lostLatencyAdd,maxOutputAckMask,minAckHistory,
          (double)maxDropouts,(double)midDropouts,(double)okDropouts,(double)minDropouts,
          (double)latencyOverMul,(double)latencyOverAdd,(double)latencyWorseMul,(double)latencyWorseAdd,
          (double)latencyOkMul,(double)latencyOkAdd,(double)latencyBestMul,(double)latencyBestAdd,
          (double)maxBandOverGood,(double)safeMaxBandOverGood
          );
}

void NetworkParams::printParams3 ( char *buf )
  // Prints out actual network parameters (part 3). Buffer must hold at least 256 chars.
{
  Assert( buf );
  int i;
  for ( i = 0; i < sizeof(grow)/sizeof(GrowCoefs); i++ )
    buf += sprintf(buf," [%.3f,%.1f]",(double)grow[i].mul,(double)grow[i].add);
}

#endif

//------------------------------------------------------------
//  NetChannel: abstract network communication channel

#ifdef NET_LOG
unsigned NetChannel::nextId = 0;
#endif

bool NetChannel::UseAlternativeBandwidthControlAlgorithm = false;

NetChannel::NetChannel ()
{
    processRoutine = NULL;
    data = NULL;
    counter = 0;
#ifdef NET_LOG
    id = nextId++;
#endif
#if _ENABLE_REPORT
    _counter = 0;
    _dropped = 0;
    _dropRatio = 0;
#endif
}

#if defined(NET_LOG) || DEBUG_VON

char *NetChannel::getChannelInfo ( char *buf ) const
{
    Assert( buf );
    sprintf(buf,"local channel (DirectChannel)");
    return buf;
}

unsigned NetChannel::getChannelId () const
{
    return id;
}

#endif

#if _ENABLE_REPORT || defined(NET_LOG)

char *NetChannel::channelDump ( char *buf ) const
{
    Assert( buf );
    sprintf(buf,"local channel (DirectChannel)");
    return buf;
}

#endif

unsigned NetChannel::maxMessageData () const
{
    return( 1490 - IP_UDP_HEADER - MSG_HEADER_LEN );
}

bool NetChannel::dropPacketToSend ()
{
#if _ENABLE_REPORT
  if (_counter>10000)
  {
    _counter = 0;
    _dropped = 0;
  }
  _counter++;
  if (_dropped<_counter*_dropRatio)
  {
    _dropped++;
    return true;
  }
#endif
  return false;
}

bool NetChannel::isControl () const
{
    return false;
}

bool NetChannel::getInternalStatistics ( ChannelStatistics &stat )
{
    return false;
}

void NetChannel::setProcessRoutine ( NetCallBack *processR, void *_data )
{
    processRoutine = processR;
    data = _data;
}


void NetChannel::setSubsetRoutine ( const RoutingItem &item )
{
    enter();
    removeSubsetRoutine(item);
    RoutingItem &newItem = subsets.Append();
    newItem = item;
    leave();
}

bool NetChannel::removeSubsetRoutine ( const RoutingItem &item )
{
    Assert( (item.andFlag & ~(MSG_USER_FLAGS | MSG_VOICE_FLAG)) == 0 );
    Assert( (item.andFlag & item.eqFlag) == item.eqFlag );
    enter();
    int i;
    for ( i = 0; i < subsets.Size(); i++ )
        if ( subsets[i].andFlag == item.andFlag &&
             subsets[i].eqFlag  == item.eqFlag ) {
            subsets.Delete(i);
            leave();
            return true;
            }
    leave();
    return false;
}

NetChannel::~NetChannel ()
{
#ifdef NET_LOG_DESTRUCTOR
    NetLog("Channel(%u)::~NetChannel",getChannelId());
#endif
}

//------------------------------------------------------------
//  NetPeer: abstract network communication peer

#ifdef NET_LOG
unsigned NetPeer::nextId = 0;
#endif

NetPeer::NetPeer ( NetPool * _pool )
{
    pool = _pool;
    port = 0;
    broadcastCh = NULL;
#ifdef NET_LOG
    id = nextId++;
#endif
}

#ifdef NET_LOG

char *NetPeer::getPeerInfo ( char *buf ) const
{
    Assert( buf );
    struct sockaddr_in local;
    getLocalAddress(local);
    sprintf(buf,"%u.%u.%u.%u:%u",
            (unsigned)IP4(local),(unsigned)IP3(local),(unsigned)IP2(local),(unsigned)IP1(local),(unsigned)PORT(local));
    return buf;
}

unsigned NetPeer::getPeerId () const
{
    return id;
}

#endif

NetPool *NetPeer::getPool () const
{
    return pool;
}

unsigned16 NetPeer::getPort () const
{
    return port;
}

NetChannel *NetPeer::getBroadcastChannel () const
{
    return broadcastCh;
}

unsigned NetPeer::maxMessageData () const
{
        // maximum data size of an UDP datagram minus the wole overhead.
    return( 1490 - IP_UDP_HEADER - MSG_HEADER_LEN );
}

unsigned16 NetPeer::getLocalPort () const
{
    return port;
}

NetPeer::~NetPeer ()
{
    port = 0;
}

void NetPeer::suspendSocket ( bool susp )
{
}

void NetPeer::replaceSocket ( SOCKET _sock )
{
}

//-------------------------------------------------------------------------
//  NetStress manager:

#ifdef NET_STRESS

#include <Es/Memory/normalNew.hpp>

void* DeferredMessage::operator new ( size_t size )
{
  return safeNew(size);
}

void* DeferredMessage::operator new ( size_t size, const char *file, int line )
{
  return safeNew(size);
}

void DeferredMessage::operator delete ( void *mem )
{
  safeDelete(mem);
}

#include <Es/Memory/debugNew.hpp>

void NetStress::readConfig ()
{
  enter();
  FILE *f;
  f = fopen(NET_STRESS_FILE,"rt");
  if ( f )
  {
    int minL, maxL;
    if ( fscanf(f,"%d %d %lf",&minL,&maxL,&m_dropProbability) != 3 )
      LogF("NetStress file '" NET_STRESS_FILE "' has bad format (expecting 'int int double')");
    else
    {
      if ( minL < 0 ) minL = 0;
      if ( minL > 10000 ) minL = 10000;
      if ( maxL > 10000 ) maxL = 10000;
      if ( maxL < minL ) maxL = minL;
      m_minLatency = 1000 * (unsigned64)minL;
      m_varLatency = 1000 * (unsigned64)(maxL - minL);
      if ( m_dropProbability < 0.0  ) m_dropProbability = 0.0;
      if ( m_dropProbability > 0.99 ) m_dropProbability = 0.99;
    }
    fclose(f);
#ifdef NET_LOG_STRESS
#  ifdef NET_LOG_BRIEF
    NetLog("Pe(%u):NS(%u,%u,%.3f,%u,%u,%u,%u,%u)",
           m_peerId,(unsigned)(m_minLatency/1000),(unsigned)((m_minLatency+m_varLatency)/1000),m_dropProbability,
           m_packets+m_drops,m_drops,(unsigned)(m_packets ? m_minLat/1000 : 0),
           (unsigned)(m_packets ? (m_sumLat/m_packets)/1000 : 0),(unsigned)(m_maxLat/1000));
#  else
    NetLog("Peer(%u): NetStress - minLat = %u, maxLat = %u, drop = %.3f, packets = %u(%u), lat = %u,%u,%u",
           m_peerId,(unsigned)(m_minLatency/1000),(unsigned)((m_minLatency+m_varLatency)/1000),m_dropProbability,
           m_packets+m_drops,m_drops,(unsigned)(m_packets ? m_minLat/1000 : 0),
           (unsigned)(m_packets ? (m_sumLat/m_packets)/1000 : 0),(unsigned)(m_maxLat/1000));
#  endif
#endif
    m_packets = m_drops = 0;
    m_sumLat = m_maxLat = 0;
    m_minLat = m_minLatency + m_varLatency;
    m_valid = true;
  }
  else
  {
#ifdef NET_LOG_STRESS
#  ifdef NET_LOG_BRIEF
    NetLog("Pe(%u):NSmissing('" NET_STRESS_FILE "')",m_peerId);
#  else
    NetLog("Peer(%u): NetStress - missing config file '" NET_STRESS_FILE "'",m_peerId);
#  endif
#endif
    m_valid = false;
  }
  leave();
}

NetStress::NetStress ( unsigned64 interval, unsigned peerId )
{
  LockRegister(lock,"NetStress");
  m_valid = false;
  m_peerId = peerId;
  m_interval = interval;
  m_nextCheck = getSystemTime() + m_interval;
  m_minLatency = 0;
  m_varLatency = 0;
  m_dropProbability = 0.0;
  m_packets = m_drops = 0;
  m_sumLat = m_minLat = m_maxLat = 0;
  readConfig();
}

char *NetStress::getData ( unsigned64 now, int &len, struct sockaddr_in &from )
{
  char *result = NULL;
  enter();
  if ( m_def.NotNull() && m_def->m_time <= now )
  {
    result = m_def->m_data;
    len = m_def->m_len;
    from = m_def->m_from;
  }
  leave();
  return result;
}

void NetStress::removeData ( char *data )
{
  enter();
  RefD<DeferredMessage> *ptr = &m_def;
  while ( (*ptr).NotNull() && (*ptr)->m_data != data )
    ptr = &((*ptr)->m_next);
  if ( (*ptr).NotNull() )
    *ptr = (*ptr)->m_next;
  leave();
}

void NetStress::insertData ( const char *data, int len, const struct sockaddr_in &from, unsigned64 now )
{
  enter();
  if ( m_dropProbability > 0.0 &&
       m_rnd.uniformNumber() < m_dropProbability )
  {                                         // throw away this packet ..
    m_drops++;
    leave();
#ifdef NET_LOG_STRESS_VERBOSE
#  ifdef NET_LOG_BRIEF
    NetLog("Pe(%u):NSdrop(%u)",m_peerId,((MsgHeader*)data)->serial);
#  else
    NetLog("Peer(%u): NetStress is dropping message (serial=%u)",m_peerId,((MsgHeader*)data)->serial);
#  endif
#endif
    return;
  }
    // prepare DeferredMessage:
  m_packets++;
  RefD<DeferredMessage> msg = new DeferredMessage;
  msg->m_time = m_minLatency;
  if ( m_varLatency )
    msg->m_time += (unsigned64)(m_varLatency * m_rnd.uniformNumber());
  m_sumLat += msg->m_time;
  if ( msg->m_time > m_maxLat ) m_maxLat = msg->m_time;
  if ( msg->m_time < m_minLat ) m_minLat = msg->m_time;
#ifdef NET_LOG_STRESS_VERBOSE
#  ifdef NET_LOG_BRIEF
  NetLog("Pe(%u):NSdelay(%u,%u)",
         m_peerId,((MsgHeader*)data)->serial,(unsigned)(msg->m_time/1000));
#  else
  NetLog("Peer(%u): NetStress is delaying message (serial=%u, delay=%u)",
         m_peerId,((MsgHeader*)data)->serial,(unsigned)(msg->m_time/1000));
#  endif
#endif
  msg->m_time += now;
  msg->m_len = len;
  memcpy(msg->m_data,data,len);
  msg->m_from = from;
    // insert the message into our list:
  RefD<DeferredMessage> *ptr = &m_def;
  while ( (*ptr).NotNull() && (*ptr)->m_time < msg->m_time )
    ptr = &((*ptr)->m_next);
  msg->m_next = *ptr;
  *ptr = msg;
  leave();
}

void NetStress::tick ( unsigned64 now )
{
  enter();
  if ( now > m_nextCheck )
  {
    m_nextCheck = now + m_interval;
    readConfig();
    if ( !m_valid )
      m_nextCheck += (m_interval << 3);
  }
  leave();
}

#endif
