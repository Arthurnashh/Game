#ifdef _MSC_VER
#  pragma once
#endif

/*
  @file   logflags.hpp
  @brief  Logging flags (for NetLog).

  Copyright &copy; 2002-2003 by BIStudio (www.bistudio.com)
  @author PE
  @since  13.3.2002
  @date   20.11.2003
*/

#ifndef _LOGFLAGS_H
#define _LOGFLAGS_H

//-------------------------------------------------------------------------
//  NET_LOG (NetLog):

#if _SUPER_RELEASE

/// Enable (network) logger.
//#  define NET_LOG
/// Brief form of net-logging.
#  define NET_LOG_BRIEF
/// Logging for external testers!
#  define NET_EXTERN_TEST
#  define NET_LOG_PERIOD  0x7

#else

/// Enable (network) logger.
// #  define NET_LOG
/// Brief form of net-logging.
#  define NET_LOG_BRIEF
/// Logging for external testers!
#  define NET_EXTERN_TEST
#  define NET_LOG_PERIOD  0x1f

#endif

// no netlog in Xbox retail
# ifndef NO_NET_LOG // by default we want the netlogging (controlled by a command line)
#   define NET_LOG
# endif

#ifdef NET_TEST
#  define NET_LOG
#  define NET_LOG_BRIEF
#endif

//-------------------------------------------------------------------------

#ifdef NET_LOG

//#  define LOCK_TRACING
#  ifndef EXTERN_NET_LOG
#    define EXTERN_NET_LOG
#  endif
//#  define NET_BREAK
//#  define TEST_NET_FREE_MEMORY

//#  define NET_STAT_TUNING                   // NetTransp* statistics only
#  define NET_DEBUG

//-------------------------------------------------------------------------

#  ifdef NET_TEST

	// NetTest option set:

#    define NET_LOG_SAFE_HEAP
#    define NET_LOG_CREATE_PEER
#    define NET_LOG_CLIENT
#    define NET_LOG_SERVER
#    define NET_LOG_LATENCY
#    define NET_LOG_GARBAGE_COLLECT
#    define NET_LOG_TRANSP_STAT
#    define NET_LOG_BANDWIDTH

//-------------------------------------------------------------------------

#  elif defined(NET_EXTERN_TEST)

  // logging for external testers:

#    define NET_LOG_CREATE_PEER    // *
#    define NET_LOG_PEER           // *
#    define NET_LOG_PEER_PARAMS    // *
#    define NET_LOG_UDP_LISTEN     // *
#    define NET_LOG_UDP_SEND       // *

#    define NET_LOG_CHANNEL        // *
#    define NET_LOG_LATENCY        // *
#    define NET_LOG_CH_STATE       // *
#    define NET_LOG_CONNECTIVITY   // *

#    define NET_LOG_CLIENT         // *
#    define NET_LOG_MERGE          // *

#    define NET_LOG_SERVER         // *
#    define NET_LOG_CTRL_RECEIVE   // *

#    define NET_LOG_HTTP           // *
#    define NET_LOG_STRESS         // *

//#    define NET_LOG_VOICE          // *
//#    define NET_LOG_VOICE_TUNING   // *

//-------------------------------------------------------------------------

#  elif defined(NET_STAT_TUNING)

  // tuning of network statistics:

#    define DEDICATED_STAT_LOG
#    define NET_LOG_TRANSP_STAT

//-------------------------------------------------------------------------

#  elif defined(NET_DEBUG)

  // common set of logging options:

//#    define NET_LOG_CREATE_PEER
//#    define NET_LOG_PEER_PARAMS
//#    define NET_LOG_CHANNEL
//#    define NET_LOG_PEER
//#    define NET_LOG_SESSION_ENUM
//#    define NET_LOG_CLIENT
//#    define NET_LOG_SERVER
//#    define NET_LOG_START_ENUM

//#    define NET_LOG_TRANSP_STAT
//#    define NET_LOG_LATENCY
//#    define NET_LOG_LATENCY1

//#    define NET_LOG_UDP_LISTEN
//#    define NET_LOG_UDP_RECEIVE
//#    define NET_LOG_UDP_SEND
//#    define NET_LOG_UDP_SENDING

//#    define NET_LOG_PROCESS_DATA
//#    define NET_LOG_CLIENT_SEND
//#    define NET_LOG_CLIENT_PROCESS
//#    define NET_LOG_SERVER_SEND
//#    define NET_LOG_SERVER_PROCESS

//#    define NET_LOG_MAPS
//#    define NET_LOG_SAFE_HEAP
//#    define NET_LOG_GARBAGE_COLLECT
//#    define NET_LOG_RUN_REVISITED
//#    define NET_LOG_ADJUST_CHANNEL
//#    define NET_LOG_OUTPUT_ACK_OPTIMIZE
//#    define NET_LOG_ACK_OUT
//#    define NET_LOG_ACK_IN

//#    define NET_LOG_CH_STATE

//#    define NET_LOG_VOICE
//#    define NET_LOG_VOICE_VERBOSE
//#    define NET_LOG_VOICE_VERBOSE2
//#    define NET_LOG_VOICE_P2P 20 // peer to peer voice diags level
//#    define NET_LOG_PCM
//#    define NET_LOG_DPCM
//#    define NET_LOG_ETC
//#    define NET_LOG_ETC_VERBOSE
//#    define NET_LOG_STRESS
//#    define NET_LOG_STRESS_VERBOSE

//#  define NET_LOG_DISPATCHER 1

//-------------------------------------------------------------------------

#  else

  // all settings:

/// NetChannelBasic global messages
#  define NET_LOG_CHANNEL
/// NetChannelBasic::processData()
#  define NET_LOG_PROCESS_DATA
/// NetChannelBasic::inputStatistics()
#  define NET_LOG_LATENCY
/// NetChannelBasic::inputStatistics()
#  define NET_LOG_LATENCY1
/// NetChannelBasic::inputStatistics()
#  define NET_LOG_ACK_IN
/// NetChannelBasic::inputStatistics()
#  define NET_LOG_BANDWIDTH
/// Periodical logging of channel state
#  define NET_LOG_CH_STATE
/// NetChannelBasic::checkConnectivity()
#  define NET_LOG_CONNECTIVITY
/// NetChannelBasic::dispatchMessage()
#  define NET_LOG_DISPATCH_MESSAGE
/// NetChannelBasic::getPreparedMessage()
#  define NET_LOG_DISPATCHER 1
/// Update of maxBandwidth due to high actLatency (over minLatency)
#  define NET_LOG_LATENCY_OVER
/// NetChannelBasic::setOutputData()
#  define NET_LOG_ACK_OUT
/// NetChannelBasic::runRevisited()
#  define NET_LOG_RUN_REVISITED
/// NetChannelBasic::adjustChannel()
#  define NET_LOG_ADJUST_CHANNEL
/// NetMessagePool::newMessage()
#  define NET_LOG_NEW_MESSAGE
/// NetMessagePool::recycleMessage()
#  define NET_LOG_RECYCLE_MESSAGE
/// NetMessagePool::garbageCollect()
#  define NET_LOG_GARBAGE_COLLECT
/// getLocalAddress()
#  define NET_LOG_GET_LOCAL_ADDRESS
/// getLocalName()
#  define NET_LOG_GET_LOCAL_NAME
/// udpListen() global messages
#  define NET_LOG_UDP_LISTEN
/// udpListen()
#  define NET_LOG_UDP_RECEIVE
/// udpListen()
#  define NET_LOG_STRESS
/// udpListen()
#  define NET_LOG_STRESS_VERBOSE
/// udpSend() global messages
#  define NET_LOG_UDP_SEND
/// udpSend()
#  define NET_LOG_UDP_SENDING
/// NetPeerUDP global messages
#  define NET_LOG_PEER
/// NetPeerUDP::sendData()
#  define NET_LOG_SEND_DATA
/// PeerChannelFactoryUDP::PeerChannelFactoryUDP()
//#  define NET_LOG_PEER_CHANNEL_FACTORY
/// PeerChannelFactoryUDP::createPeer()
#  define NET_LOG_CREATE_PEER
/// PeerChannelFactoryUDP::createPeer() details
#  define NET_LOG_PEER_PARAMS
/// MT-safe heap global messages
#  define NET_LOG_SAFE_HEAP
/// serverReceive()
#  define NET_LOG_SERVER_RECEIVE
/// ctrlReceive()
#  define NET_LOG_CTRL_RECEIVE
/// enumReceive()
#  define NET_LOG_ENUM_RECEIVE
/// NetSessionEnum global messages
#  define NET_LOG_SESSION_ENUM
/// NetSessionEnum::StartEnumHosts()
#  define NET_LOG_START_ENUM
/// NetSessionEnum::StopEnumHosts()
#  define NET_LOG_STOP_ENUM
/// NetSessionEnum::NSessions()
#  define NET_LOG_NSESSIONS
/// Merging large splitted messages
#  define NET_LOG_MERGE
/// NetClient global messages
#  define NET_LOG_CLIENT
/// clientReceive()
#  define NET_LOG_CLIENT_RECEIVE
/// NetClient::SendMsg()
#  define NET_LOG_CLIENT_SEND
/// NetClient::GetConnectionInfo(), NetServer::GetConnectionInfo()
#  define NET_LOG_INFO
/// NetClient::ProcessUserMessages()
#  define NET_LOG_CLIENT_PROCESS
/// NetClient::ProcessSendComplete
#  define NET_LOG_CLIENT_COMPLETE
/// NetServer global messages
#  define NET_LOG_SERVER
/// NetServer::KickOff()
#  define NET_LOG_SERVER_KICKOFF
/// NetServer::SendMsg()
#  define NET_LOG_SERVER_SEND
/// NetServer::ProcessUserMessages()
#  define NET_LOG_SERVER_PROCESS
/// NetServer::ProcessSendComplete
#  define NET_LOG_SERVER_COMPLETE
/// NetTransp* - GetStatistics
#  define NET_LOG_TRANSP_STAT
/// All destructors
#  define NET_LOG_DESTRUCTOR
/// ImplicitMap & ExplicitMap
#  define NET_LOG_MAPS
/// ImplicitMap: zombie reports
#  define NET_LOG_ZOMBIE
/// Global voice events
#  define NET_LOG_VOICE
/// Detailed voice events
#  define NET_LOG_VOICE_VERBOSE
/// Super-detailed voice events
#  define NET_LOG_VOICE_VERBOSE2
/// VoN tuning (VoNReplayer::decode)
#  define NET_LOG_VOICE_TUNING
/// PCM codec statistics
#  define NET_LOG_PCM
/// PCM codec statistics (old version)
#  define NET_LOG_PCM_OLD
/// DPCM codec statistics
#  define NET_LOG_DPCM
/// Event-to-callback logging
#  define NET_LOG_ETC

#  endif

#endif

//-------------------------------------------------------------------------
//  NetLogger object:

extern bool netLogValid;                    ///< for safer destruction etc.

extern double getLogTime ();

#if !defined(_WIN32) || defined NET_LOG

  #define NetLog netLog

  extern void netLog ( const char *format, ... );

  extern void netLogFlush ();

#else   // !defined(NET_LOG)

  #define NetLog (void)

#endif  // NET_LOG

#endif
