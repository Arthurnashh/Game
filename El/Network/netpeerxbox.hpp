#ifdef _MSC_VER
#  pragma once
#endif

/**
    @file   netpeerxbox.hpp
    @brief  Network peer object (implementation for Xbox)

    Copyright &copy; 2003 by BIStudio (www.bistudio.com)
    @author PE
    @since  19.2.2003
    @date   2.4.2003
*/

#ifndef _NETPEERXBOX_H
#define _NETPEERXBOX_H

#include "netpeer.hpp"

#if _XBOX_SECURE

//------------------------------------------------------------
//  Support:

/**
    Retrieves the local network address.
    @param  me <code>sockaddr_in</code> structure to be filled.
    @param  port Port number to be used.
    @return <code>true</code> if succeeded.
*/
bool getLocalAddress ( struct sockaddr_in &me, unsigned16 port );

/**
    Retrieves name of the local host.
    @param  name Buffer to hold the result.
    @param  len Buffer length.
    @return <code>true</code> if succeeded.
*/
bool getLocalName ( char *name, unsigned len );

/**
    Fills-in the <code>sockaddr_in</code> address from string.
    @param  host <code>sockaddr_in</code> structure to be filled.
    @param  ip String representation of network address.
    @param  port Port number.
    @return <code>true</code> if succeeded.
*/
bool getHostAddress ( struct sockaddr_in &host, const char *ip, unsigned16 port );

//------------------------------------------------------------
//  NetPeerXbox class:

/// Xbox implementation of Peer to Peer communication
class XboxPeerToPeerChannel: public NetPeerToPeerChannel
{
  PPSocket _direct;
  int _port;
  ThreadId _listener;
  bool _endListener;
  
  ProcessF *_process;
  void *_ctx;
  
  public:
  
  XboxPeerToPeerChannel();
  ~XboxPeerToPeerChannel();
  
  /// initialize - open sockets as needed
  virtual bool Init(int port);

  virtual void RegisterCallback(ProcessF *process, void *ctx);
  
  virtual void SendMessage(
    const sockaddr_in &ia, const void *data, int size, int sizeEncrypted
  );

  void Thread();

  virtual int GetPort() const {return _port;}
  virtual SOCKET GetSocket() const {return _direct.GetSocket();}
};
/**
    Net-peer class. If opened it is bound to the specific
    UDP port and does all (duplex) communication through that port
    (the whole family of [localIP:port]-[*:*] requests).
    <p>Individual I/O requests are processed by net-channels.
    Each net-channel object is responsible for respective
    [localIP:port]-[distantIP:port] communication.
    @since  19.2.2003
    @see    NetChannel
*/
class NetPeerXbox : public NetPeer {

protected:

    /// channel routing table - mapping:  [distantIP:port] -> NetChannel
    ImplicitMap<unsigned64,RefD<NetChannel>,channelKey,true,MemAllocSafe> chMap;

    /// shared SOCKET handle
    SOCKET sock;

    /// Socket is in VDP mode.
    bool vdp;

    /// listener thread
    ThreadId listener;

    /// is this port in 'listening' state?
    bool listen;

    /// 'suspended' socket state (time between temporary and regular state).
    bool suspended;

    friend THREAD_PROC_RETURN THREAD_PROC_MODE udpListenSend ( void *param );

public:

    /**
        Default constructor. For service use only.
        @param  _pool Net-pool to work with
    */
    NetPeerXbox ( NetPool *_pool );

    /**
        Default constructor - does initialization.
        @param  _sock Socket handle (initialized for 'broadcast' mode).
        @param  _pool Net-pool to work with
        @param  _vdp The peer is using VDP protocol (instead of UDP).
    */
    NetPeerXbox ( SOCKET _sock, unsigned16 _port, NetPool *_pool, bool _vdp );

    /**
        Retrieves local network address (peer).
        @param  local Buffer the result will be filled in
    */
    virtual void getLocalAddress ( struct sockaddr_in &local ) const;

    /// Returns the used socket (needed for GameSpy SDK - NAT Negotiation)
    virtual SOCKET GetSocket() const {return sock;}

    /// Returns maximum data length for communication through this net-peer
    virtual unsigned maxMessageData () const;

    /**
        Registers a new net-channel into the net-peer.
        <p>There shouldn't be two channels sharing the same distant network address and port number.
        @param  distant Regularly filled <code>sockaddr_in</code> structure containing
                        address of distant peer
        @param  ch Net-channel bound to the given address
        @return <code>true</code> if succeeded
    */
    virtual bool registerChannel ( struct sockaddr_in &distant, NetChannel *ch );

    /**
        Unregisters the given net-channel.
        @param  ch Previously registered net-channel
    */
    virtual void unregisterChannel ( NetChannel *ch );

    /**
        Looks for a net-channel bound to the given network address.
        @param  distant Regularly filled <code>sockaddr_in</code> structure containing
                        address of distant peer
        @return Reference to a suitable net-channel (<code>NULL</code> if failed)
    */
    virtual NetChannel *findChannel ( const struct sockaddr_in &distant );

    /// Closes the net-peer including all associates channels.
    virtual void close ();

    //  Xbox-specific operations:

    /**
        Enters the 'suspended' socket state. Asynchronous thread(s) should
        not be active in this state.
    */
    virtual void suspendSocket ( bool susp =true );

    /**
        Replaces the broadcast socket with the new one.
        Cancels 'suspended' state automatically.
    */
    virtual void replaceSocket ( SOCKET _sock );

    //  network I/O:

    /**
        Process the given incoming data.
        Asynchronously called by the listener thread. Must not call channel-processing,
        used for peer statistics only.
        @param  hdr Header that was received (includes message length,
                    continues with message data itself).
        @param  distant IP address data came from.
    */
    virtual void processData ( MsgHeader *hdr, const struct sockaddr_in &distant );

    /**
        Sends the given data to the given network address.
        @param  hdr Prepared message header (includes message length,
                    continues with message data itself).
        @param  distant IP destination address (can be <code>Zero()</code> for
                        broadcast).
        @param  encrypted Size of encrypted message part.
    */
    virtual NetStatus sendData ( MsgHeader *hdr, struct sockaddr_in distant, unsigned16 encrypted =0 );

    /**
        Cancels all pending messages (already dispatched for delivery).
    */
    virtual void cancelAllMessages ();

    /**
        Initializes dispatcher status. Dispatcher takes care of all messages waiting for departure.
        This method is used for status initializing.
        @param  data Buffer to receive dispatcher statistics. If it is <code>NULL</code>, only the required data-length is returned.
        @return Required length of <code>data</code> struct.
    */
    virtual unsigned initDispatcherStatus ( DispatcherStatus *data );

    virtual ~NetPeerXbox ();

    };

#endif
#endif
