#ifdef _MSC_VER
#  pragma once
#endif

/**
    @file   netpeer.hpp
    @brief  Network peer object (pilot implementation using UDP)

    Copyright &copy; 2001-2003 by BIStudio (www.bistudio.com)
    @author PE
    @since  18.11.2001
    @date   5.3.2003
*/

#ifndef _NETPEER_H
#define _NETPEER_H

#ifdef _WIN32
# if _GAMES_FOR_WINDOWS // using special Winsock library
# else
#   include <winsock2.h>
# endif
#else
#include <sys/socket.h>
typedef int SOCKET;
#define SOCKET_ERROR   -1
#define INVALID_SOCKET -1
#define closesocket(s) ::close(s)
#endif

extern unsigned long bindIPAddress;

void DisableMTUDiscovery(int socket);

/// Encapsulated socket
/** Adapted from Microsoft XDK LowLevelVoice sample */
//this socket is used for Peer to Peer communication and it is supposed to be joined with class Socket
class PPSocket: private NoCopy
{
  SOCKET _socket;

public:

  enum SocketType
  {
    Type_UDP,
    Type_TCP,
    Type_VDP
  };

  explicit PPSocket( SOCKET = INVALID_SOCKET );
  explicit PPSocket( SocketType );
  PPSocket( int iType, int iProtocol );
  ~PPSocket();

  bool   Open( SocketType );
  bool   Open( int iType, int iProtocol );
  bool   IsOpen() const;
  int    Close();
  SOCKET Accept( sockaddr_in* = NULL );
  int    Bind( const sockaddr_in* );
  int    Connect( const sockaddr_in* );
  SOCKET GetSocket() const;
  int    GetSockName( sockaddr_in* ) const;
  int    GetSockOpt( int iLevel, int iName, void* pValue, int* piSize ) const;
  int    IoCtlSocket( long nCmd, DWORD* pArg );
  int    Listen( int iBacklog = SOMAXCONN );
  int    Recv( void* pBuffer, int iBytes );
  int    RecvFrom( void* pBuffer, int iBytes, sockaddr_in* = NULL );
  int    Select( bool* pbRead, bool* pbWrite, bool* pbError );
  int    Send( const void* pBuffer, int iBytes );
  int    SendTo( const void* pBuffer, int iBytes, const sockaddr_in* = NULL );
  int    SetSockOpt( int iLevel, int iName, const void* pValue, int iBytes );
  int    Shutdown( int iHow );
};

/// Size of sliding window for I/O peer/channel statistics. Number of items = SLIDING_WINDOW / 2.
const int SLIDING_WINDOW = 64;

/// Different sliding-window size for sent messages.
const int SLIDING_WINDOW_SEND = 256;

//------------------------------------------------------------
//  Common support:

#if !_GAMES_FOR_WINDOWS
#define IPPROTO_VDP IPPROTO_UDP
#endif

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
//  NetPeerUDP class:

/**
    Net-peer class. If opened it is bound to the specific
    UDP port and does all (duplex) communication through that port
    (the whole family of [localIP:port]-[*:*] requests).
    <p>Individual I/O requests are processed by net-channels.
    Each net-channel object is responsible for respective
    [localIP:port]-[distantIP:port] communication.
    @since  18.11.2001
    @see    NetChannel
*/
class NetPeerUDP : public NetPeer {

protected:

    /// channel routing table - mapping:  [distantIP:port] -> NetChannel
    ImplicitMap<unsigned64,RefD<NetChannel>,channelKey,true,MemAllocSafe> chMap;

    /// shared SOCKET handle
    SOCKET sock;

    /// listener thread
    ThreadId listener;

    /// sender thread
    ThreadId sender;

    /// is this port in 'listening' state?
    bool listen;

    /// is this port in 'sending' state?
    bool sending;

    friend THREAD_PROC_RETURN THREAD_PROC_MODE udpListen ( void *param );
    friend THREAD_PROC_RETURN THREAD_PROC_MODE udpSend ( void *param );
    friend THREAD_PROC_RETURN THREAD_PROC_MODE udpListenSend ( void *param );

    /// Reconnects the associated port after the WSAECONNRESET error..
    void reconnect ();

    /// The peer is being reconnected.
    bool reconnecting;

    /// Hook for raw messages
    RawMessageCallback _callback;

public:

    /**
        Default constructor. For service use only.
        @param  _pool Net-pool to work with
    */
    NetPeerUDP ( NetPool *_pool );

    /**
        Default constructor - does initialization.
        @param  _sock Socket handle (with successfully finished <code>bind()</code> operation).
        @param  _port Port number.
        @param  _pool Net-pool to work with
    */
    NetPeerUDP ( SOCKET _sock, unsigned16 _port, NetPool *_pool, RawMessageCallback callback );

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

    virtual void stopThreads ();

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
    */
    virtual NetStatus sendData ( MsgHeader *hdr, struct sockaddr_in distant );

    virtual void sendRaw( const sockaddr_in &ia, const void *data, int size, int sizeEncrypted );
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

    /// offer raw message to callback, return true if processed
    bool CheckMessageHook(char *data, int len, struct sockaddr_in *from);

    virtual ~NetPeerUDP ();
};

/// Xbox implementation of Peer to Peer communication
class PcPeerToPeerChannel: public NetPeerToPeerChannel
{
  // TODO: call _keepAliveProc as a part of normal channel handling?
  ThreadId _listener;
  bool _endListener;

  ProcessKeepAlive *_keepAliveProc;
  void *_ctx;

public:

  PcPeerToPeerChannel();
  ~PcPeerToPeerChannel();

  /// initialize - open sockets as needed
  virtual bool Init(int port);

  virtual void RegisterKeepAliveCallback(ProcessKeepAlive *process, void *ctx);

  virtual void StopListener();

  virtual int GetPort() const {Fail("No longer supported");return 0;}
  virtual SOCKET GetSocket() const {Fail("No longer supported");return NULL;}

  virtual void SendMessage(
    const sockaddr_in &ia, const void *data, int size, int sizeEncrypted
    );

  void Thread();
};

#endif
