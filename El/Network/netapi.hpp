#ifdef _MSC_VER
#  pragma once
#endif

/**
  @file   netapi.hpp
  @brief  API for abstract network transport layer.

  It should use unreliable underlying network protocol).
  <p>Copyright &copy; 2001-2003 by BIStudio (www.bistudio.com)
  @author PE
  @since  27.11.2001
  @date   7.9.2003
*/

#ifndef _NETAPI_H
#define _NETAPI_H

#include "El/Speech/debugVoN.hpp"

/**
    Parts of the network API can be found in "netglobal.hpp" and "netmessage.hpp".
*/

class NetPeer;
class NetChannel;
class NetPool;
class NetMessage;

//------------------------------------------------------------
//  NetChannel: abstract network communication channel

/// Enhanced info returned by NetChannel::getOutputBandWidth()
struct EnhancedBWInfo {

    unsigned actBW;                         ///< Actual (acknowledged) band-width in bytes per second.

    unsigned goodBW;                        ///< goodAckBandwidth.

    unsigned sentBW;                        ///< Output band-width in bytes per second.
    
    unsigned outRB;                         ///< Output band-width based on received packet-pairs (averaged value sent from the receiver).

    unsigned inRB;                          ///< Input band-width based on received packet-pairs (actual value).

    int growMode;                           ///< grow-mode of the NetChannel (0 .. I feel good, -2 .. slow down!, 2 .. growing fast).

    int growModeLost;                       ///< grow-mode based on lost-packet ratio.

    int growModePing;                       ///< grow mode based on actual ping.

    };

/// Data for collecting dispatcher statistics..
struct DispatcherStatus {

    unsigned structLen;                     ///< Length of the structure

    };

/// NetChannel internal statistics..
struct ChannelStatistics {

    unsigned revisitedNo;                   ///< Number of revisited messages.

    unsigned revisitedAveAge;               ///< Average age of revisited messages.

    unsigned revisitedMaxAge;               ///< Maximum age of revisited messages.

    unsigned ackTotal;                      ///< Total number of acknowledged messages (approx. window size: 400).

    unsigned ackLost;                       ///< Number of messages which are considered "lost".

    };

/// Structure holding constants for maxBandwidth generator. Recomputed approximately each 1000ms.
struct GrowCoefs {

    float mul;                              ///< Addition (in bytes per second).

    float add;                              ///< Multiplication.

    };

/// Grow states: -MAX_GROW_STATE .. 0 .. MAX_GROW_STATE+1
#define MAX_GROW_STATE      2

/// Structure holding tuning parameters for Sockets.
struct NetworkParams {

    float winsockVersion;                   ///< WSAStartup WinSock required version.

    unsigned rcvBufSize;                    ///< SO_RCVBUF parameter of socket.

    unsigned maxPacketSize;                 ///< Maximum (BRUTTO) packet size transferred over the net.

    unsigned dropGap;                       ///< Channel-drop interval in seconds.

    unsigned ackTimeoutA;                   ///< Multiplicative coefficient for actual ack-timeout computation.

    unsigned ackTimeoutB;                   ///< Additive coefficient for actual ack-timeout computation (in microseconds).

    unsigned ackRedundancy;                 ///< acknowledge redundancy (repeat count for each VIM acknowledgement)

    unsigned initBandwidth;                 ///< Initial bandwidth in bytes/sec.

    unsigned minBandwidth;                  ///< Minimal bandwidth in bytes/sec. maxBandwidth should never drop below this value..

    unsigned maxBandwidth;                  ///< Maximal bandwidth in bytes/sec. maxBandwidth should never raise over this value..

    unsigned minActivity;                   ///< Minimum channel traffic for maxBandwidth growing..

    unsigned initLatency;                   ///< Initial latency in microseconds.

    unsigned minLatencyUpdate;              ///< Update interval for minLatency.

    float minLatencyMul;                    ///< Multiplicative coefficient for periodic minLatency update.

    float minLatencyAdd;                    ///< Additive coefficient for periodic minLatency update (in microseconds).

    float goodAckBandFade;                  ///< Fading coefficient for goodAckBandwidth.

    unsigned outWindow;                     ///< Sent-bandwidth computation window (for getPreparedMessage, in microseconds).

    unsigned ackWindow;                     ///< Window for ack-based bandwidth estimation (in microseconds).

    unsigned maxChannelBitMask;             ///< Maximum size of internal NetChannel bit-masks.

    float lostLatencyMul;                   ///< Time at which a message is considered lost (multiplicative coef. for aveLatency).
    
    float lostLatencyAdd;                   ///< Time at which a message is considered lost (additive coef. in microseconds).
    
    unsigned maxOutputAckMask;              ///< Maximum size of outputAckMask. Another restriction is based on time (outWindow).
    
    unsigned minAckHistory;                 ///< Minimum acknowledgement-history size (in number of messages) for dropout restrictions.

    float maxDropouts;                      ///< Maximum ratio of packet dropouts (switches the most radical slow-down mode on).
    
    float midDropouts;                      ///< Packet drop-out ratio for slow-down.
    
    float okDropouts;                       ///< Early warning => leave the optimistic grow-mode.
    
    float minDropouts;                      ///< This drop-out ratio is considered as "noise" (doesn't matter).

    float latencyOverMul;                   ///< Multiplicative upper bound of actual latency (over minLatency) for output restrictions.
    
    float latencyOverAdd;                   ///< Additive upper bound of actual latency (over minLatency) for output restrictions.
    
    float latencyWorseMul;                  ///< Upper bound for "small restrictions" state (multiplicative coef.).

    float latencyWorseAdd;                  ///< Upper bound for "small restrictions" state (additive coef.).

    float latencyOkMul;                     ///< Upper bound for "OK" state (multiplicative coef.).
    
    float latencyOkAdd;                     ///< Upper bound for "OK" state (additive coef.).
    
    float latencyBestMul;                   ///< Upper bound for the best grow-state (multiplicative coef.).

    float latencyBestAdd;                   ///< Upper bound for the best grow-state (additive coef.).

    float maxBandOverGood;                  ///< Maximum maxBandwidth / goodAckBandwidth ratio.
    
    float safeMaxBandOverGood;              ///< Safe over-estimation of maxBandwidth over goodAckBandwidth (used in case of drop-out alert).

    GrowCoefs grow[ 2 * MAX_GROW_STATE + 2 ]; ///< Grow-states for periodic maxBandwidth update (0 .. constant, <0 .. slow-down, >0 .. speed-up).

#ifdef NET_LOG

    void printParams1 ( char *buf );        ///< Prints out actual network parameters (part 1). Buffer must hold at least 256 chars.
    void printParams2 ( char *buf );        ///< Prints out actual network parameters (part 2). Buffer must hold at least 256 chars.
    void printParams3 ( char *buf );        ///< Prints out actual network parameters (part 3). Buffer must hold at least 256 chars.

#endif

    };

/// Default network parameters..
extern const NetworkParams defaultNetworkParams;

/// Item of a message-routing table
struct RoutingItem {

    /// "MsgHeader.flags && andFlag" must be equal to "eqFlag".
    unsigned16 andFlag;

    /// "MsgHeader.flags && andFlag" must be equal to "eqFlag".
    unsigned16 eqFlag;

    /// Routine for incoming message processing.
    NetCallBack *processRoutine;

    /// Data passed to processRoutine.
    void *data;

    };
TypeIsSimple(RoutingItem);

/**
    Net-channel class.
    <p>Each net-channel instance is responsible for individual
    (peer-to-peer) communication /example: [localIP:port]-[distantIP:port]/.
    @since  27.11.2001
    @date   14.3.2003
    @see    NetPeer
*/
class NetChannel : public RefCountSafe {

protected:
  /// Critical section to lock object. RefCountSafe lock cannot be used due to possible DeadLock. 
  PoCriticalSection lock;

  /// Enter the object's critical section.
  inline void enter() const
  { lock.enter(); }

  /// Leave the object's critical section.
  inline void leave() const
  { lock.leave(); }

    /**
        Routine for incoming message processing.
        Must be set (by an external API user) before any data are received.
    */
    NetCallBack *processRoutine;

    /// Data passed to processRoutine.
    void *data;

    /// Explicit routing table. Subset routing is done before common processing.
    AutoArray<RoutingItem,MemAllocSafe> subsets;
    
    /**
        Non-public default constructor. Does no initialization, <code>open()</code> should be used
        to setup a connection.
    */
    NetChannel ();

#if defined(NET_LOG) || DEBUG_VON

    /// Next assigned NetChannel id#.
    static unsigned nextId;

    /// Actual NetChannel id#.
    unsigned id;

#endif

#if _ENABLE_REPORT
    int _counter;
    int _dropped;
    float _dropRatio;
#endif

public:


    /**
        Establishes connection for this net-channel.
        <p>Could do some negotiation between local and distant peers. A small
        amound of data can be exchanged. Blocks program flow.
        <p>Very limited (strictly defined) usage on client computers!
        @param  _peer Net-peer to be associated with
        @param  distant Regularly filled <code>sockaddr_in</code> structure containing
                        address of distant peer (network endian)
        @return Status code (<code>nsError</code> or <code>nsOK</code>)
    */
    virtual NetStatus open ( NetPeer *_peer, struct sockaddr_in &distant ) =0;

    /**
        Reconnects this channel to the given address.
        <p>Keeps the whole channel state.
        @param  distant Regularly filled <code>sockaddr_in</code> structure containing
                        address of distant peer (network endian)
        @return Status code (<code>nsError</code> or <code>nsOK</code>)
    */
    virtual NetStatus reconnect ( struct sockaddr_in &distant ) =0;

    /// Returns maximum data length for communication through this net-channel.
    virtual unsigned maxMessageData () const;

    /**
        Retrieves associated distant network address (peer). Uses network endian.
        @param  distant Buffer the result will be filled in
    */
    virtual void getDistantAddress ( struct sockaddr_in &distant ) const =0;

    /**
        Retrieves associated local network address (peer). Uses network endian.
        @param  distant Buffer the result will be filled in
    */
    virtual void getLocalAddress ( struct sockaddr_in &local ) const =0;

#if defined(NET_LOG) || DEBUG_VON
    /**
        Prints out verbose channel info (e.g. "localIP:port<->distantIP:port").
        @param  buf Buffer to print to.
    */
    virtual char *getChannelInfo ( char *buf ) const;

    /// Returns unique peer# (for easy NetPeer identification in Net-logs).
    unsigned getChannelId () const;

#endif

#if _ENABLE_REPORT || defined(NET_LOG)

    /// Prints out dump of all important status variables of the channel
    virtual char *channelDump ( char *buf ) const;

#endif

    virtual bool dropPacketToSend();

    /**
        Is this control channel?
    */
    virtual bool isControl () const;

    /**
        Sets network tuning parameters.
        @param  p Structure holding network params.
    */
    virtual void setNetworkParams ( const NetworkParams &p )
    {}

    /**
        Actual channel latency in microseconds.
        @return The actual channel-latency in microseconds (<code>0</code> if not determined yet).
        @param  actLat return actual latency
        @param  minLat return minimal (best possible) latency
    */
    virtual unsigned getLatency ( unsigned *actLat =NULL, unsigned *minLat =NULL ) =0;

    /**
        Actual output channel band-width in bytes per second.
        @param  data Additional data record (non-mandatory).
        @return The actual output band-width (throughput) in bytes/sec.
    */
    virtual unsigned getOutputBandWidth ( EnhancedBWInfo *data =NULL ) =0;

    /**
        Retrieve some special internal NetChannel statistics. Not all items should be meaningfull in all implementations..
        @return <code>true</code> if at least some items were filled.
    */
    virtual bool getInternalStatistics ( ChannelStatistics &stat );

    /**
        Last time any message was received (in getSystemTime() format).
    */
    virtual unsigned64 getLastMessageArrival () const =0;

    /**
        Retrieves information about output message queue.
        Only not-yet-sent messages are relevant.
        @param  msgs Number of comomon messages in the queue.
        @param  bytes Total number of bytes waiting to be sent (for common messages).
        @param  vimMsgs Number of VIM messages in the queue.
        @param  vimBytes Total number of bytes waiting to be sent (for VIM messages).
    */
    virtual void getOutputQueueStatistics ( int &msgs, int &bytes, int &vimMsgs, int &vimBytes ) =0;

    /**
        Process the given incoming data.
        Asynchronously called by the listener thread.
        @param  hdr Header that was received (includes message length,
                    continues with message data itself).
        @param  distant network address data came from (network endian).
    */
    virtual void processData ( MsgHeader *hdr, const struct sockaddr_in &distant ) =0;

    /**
        Sets call-back routine for incoming data.
        @param  processR Call-back routine to process incoming data.
                         Can be <code>NULL</code> for dummy input channel.
        @param  _data Context data passed to call-back routine.
    */
    virtual void setProcessRoutine ( NetCallBack *processR, void *_data =NULL );

    /// shader bandwidth notification for external sources
    virtual void dataSentAck(size_t size) {}
    /**
        Sets call-back routine for message subset.
        @param  item Subset definition and callback parameters.
    */
    virtual void setSubsetRoutine ( const RoutingItem &item );

    /**
        Removec call-back routine for message subset.
        @param  item Subset definition and callback parameters.
        @return <code>true</code> if this subset was used.
    */
    virtual bool removeSubsetRoutine ( const RoutingItem &item );

    /**
        Dispatches the given message for output.
        @param  msg Message to be sent.
        @param  urgent Is this message urgent?
    */
    virtual void dispatchMessage ( NetMessage *msg, bool urgent =false ) =0;

    /// Message prepared to send. Set by getUrgentMessage() and getCommonMessage().
    Ref<NetMessage> prepared;

    /// Counter used for balancing depending on dispatcher state
    int counter;


    /**
        Retrieves the last VIM message dispatched so far..
    */
    virtual NetMessage *getLastVIM ( bool urgent ) =0;

    /**
        Collects dispatcher status for this channel.
        @param  data Pre-allocated buffer to receive dispatcher statistics.
    */
    virtual void nextDispatcherStatus ( DispatcherStatus *data ) =0;

    /**
        Prepares next message from the send-queue.
        Sets prepared member variable.
        @param  data Buffer with collected dispatcher status of all active channels (even broadcast ones).
        @return <code>true</code> if prepared contains (fully prepared) message to send.
    */
    virtual bool getPreparedMessage ( void *data ) =0;


    virtual void sendRaw(const sockaddr_in &ia, const void *data, int size, int sizeEncrypted) = 0;
    /**
        Called immediately before 'prepared' message is sent.
        Sets timing variables, checks the packet-bunch mechanism, etc.
        @param  bunchStart Time of actual send-bunch start (or <code>0</code> if this message is just starting a new bunch).
        @return Actual system time in microseconds.
    */
    virtual unsigned64 preSend ( unsigned64 bunchStart ) =0;

    /**
        Called immediately after 'prepared' message has been sent.
        Store the message for future access (acknowledgements, re-send, etc.).
    */
    virtual void postSend () =0;

    /**
        Finds the given (sent) message and returns its reference time (or 0 if not found).
    */
    virtual unsigned64 getMessageTime ( MsgSerial ser ) =0;

    /**
        Cancels all messages waiting for send..
    */
    virtual void cancelAllMessages () =0;

    /**
        Checks channel connectivity.
        Should be called time-to-time to assure continuing channel-connectivity.
        <p>Sends a new "ping" request if necessary..
        @param  now Actual system time (can be <code>0</code>).
    */
    virtual void checkConnectivity ( unsigned64 now ) =0;

    /**
        Checks whether the connection has dropped.
        Should use tolerant time constants to enable player re-connection..
    */
    virtual bool dropped () =0;

    /**
        Periodic channel-update function.
    */
    virtual void tick () =0;

    /**
        Closes the net-channel.
        <p>Cancels all pending operations! Discards all associations and registrations.
    */
    virtual void close () =0;

    /**
        Destructor.
        <p>Cancels all pending operations!
    */
    virtual ~NetChannel ();

    // testing opportunity
    static bool UseAlternativeBandwidthControlAlgorithm;
    };

/// This function enable to process some messages in raw format directly after received (return true if processed)
typedef bool (*RawMessageCallback)(char *data, int len, struct sockaddr_in *from);

//------------------------------------------------------------
//  NetPeer: abstract network communication peer

/**
    Abstract net-peer class.
    <p>A net-peer instance is opened and thus bound to the specific
    port. Then it does all (duplex) communication through that port
    (the whole family of [localIP:port]-[*:*] requests).
    <p>Individual I/O requests are processed by net-channels.
    Each net-channel object is responsible for respective
    [localIP:port]-[distantIP:port] communication.
    @since  27.11.2001
    @see    NetChannel
*/
class NetPeer : public RefCountSafe {

protected:
  /// Critical section to lock object. RefCountSafe lock cannot be used due to possible DeadLock. 
  PoCriticalSection lock;

  /// Enter the object's critical section.
  inline void enter() const
  { lock.enter(); }

  /// Leave the object's critical section.
  inline void leave() const
  { lock.leave(); }

    /// Associated NetPool instance.
    NetPool *pool;

    /// Local port number associated with this peer (host endian).
    unsigned16 port;

    /// Broadcast channel. It is responsible for broadcast (control) message processing.
    Ref<NetChannel> broadcastCh;

    /**
        Non-public default constructor.
        Does no initialization, descendants have to do it!
        @param  _pool Associated NetPool object.
    */
    NetPeer ( NetPool * _pool );

#ifdef NET_LOG

    /// Next assigned NetPeer id#.
    static unsigned nextId;

    /// Actual NetPeer id#.
    unsigned id;

#endif

public:

    /// Returns associated NetPool instance.
    virtual NetPool *getPool () const;

    /**
        Returns local port number (host endian).
        @return Local port number or <code>0</code> if failed.
    */
    virtual unsigned16 getPort () const;

    /**
        Retrieves local network address (peer). Uses network endian.
        @param  local Buffer the result will be filled in
    */
    virtual void getLocalAddress ( struct sockaddr_in &local ) const =0;

#ifdef NET_LOG

    /**
        Prints out verbose peer info (e.g. "localIP:port").
        @param  buf Buffer to print to.
    */
    virtual char *getPeerInfo ( char *buf ) const;

    /// Returns unique peer# (for easy NetPeer identification in Net-logs).
    virtual unsigned getPeerId () const;

#endif

    /// Returns the used socket (needed for GameSpy SDK - NAT Negotiation)
    virtual SOCKET GetSocket() const = 0;

    /**
        Returns broadcast channel (responsible for broadcast message processing).
        @return Reference to the broadcast channel.
    */
    virtual NetChannel *getBroadcastChannel () const;

    /**
        Registers a new net-channel into the net-peer.
        <p>There shouldn't be two channels sharing the same distant network address and port number.
        <p>Client computers can use this service only by a very limited way!
        @param  distant Regularly filled <code>sockaddr_in</code> structure containing
                        address of distant peer (if distant port is <code>0</code>,
                        a control-port will be contacted and distant port number will be assigned dynamically)
        @param  ch Net-channel bound to the given address
        @return <code>true</code> if succeeded
    */
    virtual bool registerChannel ( struct sockaddr_in &distant, NetChannel *ch ) =0;

    /**
        Unregisters the given net-channel.
        <p>Client computers can use this service only by a very limited way!
        @param  ch Previously registered net-channel
    */
    virtual void unregisterChannel ( NetChannel *ch ) =0;

    /**
        Looks for a net-channel bound to the given network address.
        @param  distant Regularly filled <code>sockaddr_in</code> structure containing
                        address of distant peer
        @return Reference to a suitable net-channel (<code>NULL</code> if failed)
    */
    virtual NetChannel *findChannel ( const struct sockaddr_in &distant ) =0;

    /// Returns maximum data length for communication through this net-peer
    virtual unsigned maxMessageData () const;
        // maximum data size of an UDP datagram minus the wole overhead.

    /// Informs about local port number associated with this net-peer (host endian).
    virtual unsigned16 getLocalPort () const;

    /// Closes the net-peer including all associates channels.
    virtual void close () =0;

    virtual void stopThreads () {};

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
    virtual void processData ( MsgHeader *hdr, const struct sockaddr_in &distant ) =0;


    /**
        Sends the given data to the given network address.
        @param  hdr Prepared message header (includes message length,
                    continues with message data itself).
        @param  distant IP destination address (can be <code>Zero()</code> for
                        broadcast).
    */
    virtual NetStatus sendData ( MsgHeader *hdr, struct sockaddr_in distant ) =0;

    virtual void sendRaw( const sockaddr_in &ia, const void *data, int size, int sizeEncrypted ) = 0;
    /**
        Cancels all pending messages (already dispatched for delivery).
    */
    virtual void cancelAllMessages () =0;

    /**
        Initializes dispatcher status. Dispatcher takes care of all messages waiting for departure.
        This method is used for status initializing.
        @param  data Buffer to receive dispatcher statistics. If it is <code>NULL</code>, only the required data-length is returned.
        @return Required length of <code>data</code> struct.
    */
    virtual unsigned initDispatcherStatus ( DispatcherStatus *data ) =0;

    /// Destroys all communication going through this net-peer.
    virtual ~NetPeer ();

    // inside NetClient::Init we need to lock both enterSnd and enterPeer in order to prevent possible DeadLock
    // but we want typical usage (enter/leave left protected)
    void enterPeer() { enter(); }
    void leavePeer() { leave(); }

    };

/// Interface to peer to peer communication
/**
Currently used for voice data
*/
#ifndef _WIN32
#include <sys/types.h>
#include <sys/socket.h>
typedef struct in_addr IN_ADDR;
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1300)
#define DECLSPEC_DEPRECATED __declspec(deprecated)
#elif defined(__GNUC__) && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1))
#define DECLSPEC_DEPRECATED	__attribute__((__deprecated__))
#else
#define DECLSPEC_DEPRECATED
#endif

class NetPeerToPeerChannel : public RefCountSafe
{
protected:
  /// Critical section to lock object. RefCountSafe lock cannot be used due to possible DeadLock. 
  PoCriticalSection lock;

  /// Enter the object's critical section.
  inline void enter() const
  { lock.enter(); }

  /// Leave the object's critical section.
  inline void leave() const
  { lock.leave(); }

  public:  
  typedef void ProcessKeepAlive(void *ctx);
  
  virtual bool Init(int port) = 0;
  
  virtual void RegisterKeepAliveCallback(ProcessKeepAlive *process, void *ctx) {};

  virtual void StopListener() {};


  /// port where socket is bind to
  virtual int GetPort() const = 0;
  /// used socket
  virtual SOCKET GetSocket() const = 0;

  /// send a message
  virtual DECLSPEC_DEPRECATED void SendMessage(
    const sockaddr_in &ia, const void *data, int size, int sizeEncrypted
  ) = 0;
};

//------------------------------------------------------------
//  NetNegotiator: interface for negotiation objects
//      1. creation of a new game instance
//      2. joining an existing game

class NetNegotiator : public RefCountSafe {

public:

    /**
        Initial network negotiation.
        <p>Aditional information (local ports, ..) will be obtained from the given
        NetPool instance.
        @param  pool NetPool instance to work with.
        @return <code>true</code> if successful 
    */
    virtual bool negotiate ( NetPool *pool ) =0;

    virtual ~NetNegotiator () {}

    };

//------------------------------------------------------------
//  PeerChannelFactory: creation of NetPeer and NetChannel instances

/**
    Factory for creating new NetPeer and NetChannel instances.
*/
class PeerChannelFactory : public RefCountSafe
{
  public:

  /**
  Creates a new net-peer.
  @param  pool Network pool to work with.
  @param  tryPorts Local ports to try.
  @param  useVDP Use VDP protocol (instead of UDP) - only for Xbox.
  @return Reference to the new peer (<code>NULL</code> if failed)
  */
  virtual NetPeer *createPeer ( NetPool *pool, BitMask *tryPorts, bool useVDP, RawMessageCallback callback) =0;

  /**
  Creates a new net-channel.
  @param  pool Network pool to work with.
  @param  broadcast Create broadcast channel?
  @return Reference to the new channel (<code>NULL</code> if failed).
  */
  virtual NetChannel *createChannel ( NetPool *pool, bool broadcast =false ) =0;

  //virtual NetPeerToPeerChannel *createPeerToPeer() {return NULL;}

  virtual ~PeerChannelFactory () {}

};

//------------------------------------------------------------
//  Support (ImplicitMap routines):


inline unsigned64 sockaddrKey ( const struct sockaddr_in &addr )
{
    return( (unsigned64)ADDR(addr) | ((unsigned64)PORT(addr) << 32) );
}

extern unsigned64 channelKey (const RefD<NetChannel> &ch);

template <>
struct ImplicitMapTraits< RefD<NetChannel> >
{
    static RefD<NetChannel> zombie;
    static RefD<NetChannel> null;
};

template <>
struct ImplicitMapTraits< RefD<NetPeer> >
{
    static RefD<NetPeer> zombie;
    static RefD<NetPeer> null;
};


//------------------------------------------------------------
//  NetPool: global network manager

extern unsigned16 netPeerToPort (const RefD<NetPeer> &peer);

/**
    Global network pool routines.
    <p>Client and server implementations vary a little. Default implementation
    is for client computers.
*/
class NetPool : public RefCountSafe {

protected:

    /**
        The only one factory to be asked for new peers and channels.
        <p>There might be more factories to work with in the future!
    */
    Ref<PeerChannelFactory> factory;

    /// Default set of local ports to try
    Ref<BitMask> localPorts;

    /// Set of actual peers. Port -> NetPeer mapping.
    ImplicitMap<unsigned16,RefD<NetPeer>,netPeerToPort,true,MemAllocSafe> peers;

    /// Channel routing table - mapping:  [distantIP:port] -> NetChannel
    ImplicitMap<unsigned64,RefD<NetChannel>,channelKey,true,MemAllocSafe> channels;

public:

    /**
        Default initializing constructor.
        <p>Establishes network connections..
        @param  fact Peer-Channel-factory.
        @param  neg Negotiator object instance.
        @param  ports Default set of local ports to try.
    */
    NetPool ( PeerChannelFactory *fact, NetNegotiator *neg, BitMask *ports );

    /**
        Retrieve a set of available local port numbers.
        @return Bit-mask representing available local port numbers.
    */
    virtual BitMask *getLocalPorts () const;

    /**
        Retrieves the working Peer-Channel-factory.
        @return Peer-Channel-factory used for creating Peer and Channel instances..
    */
    virtual PeerChannelFactory *getFactory () const;

    /**
        Creates a new net-peer.
        <p>There shouldn't be two peers sharing the same local port number!
        @param  tryPorts Local ports to try (if <code>NULL</code>, use localPorts).
        @param  useVDP Use VDP protocol (instead of UDP) - for Xbox only.
        @return Reference to the new peer (<code>NULL</code> if failed)
    */
    virtual NetPeer *createPeer ( BitMask *tryPorts, bool useVDP, RawMessageCallback callback );

    /**
        Finds a net-peer associated with the given local port number.
        @param  localPort Local port number (host endian).
        @return Reference to a suitable net-peer (<code>NULL</code> if not found)
    */
    virtual NetPeer *findPeer ( unsigned16 localPort );

    /**
        Destroys the given net-peer instance.
        <p>All pending operations, channels etc. will be cancelled/destroyed, too.
        @param  peer Net-peer to be destroyed.
    */
    virtual void deletePeer ( NetPeer *peer );

    /**
        Creates a new net-channel.
        @param  distant Regularly filled <code>sockaddr_in</code> structure containing
                        address of distant peer (network endian).
        @param  peer Peer to use (or <code>NULL</code> if doesn't matter)
        @return A new channel or <code>NULL</code> if failed.
    */
    virtual NetChannel *createChannel ( struct sockaddr_in &distant, NetPeer *peer =NULL );

    /**
        Finds a net-channel connected to the given distant IP address.
        @param  distant Regularly filled <code>sockaddr_in</code> structure containing
                        address of distant peer (network endian).
        @return Existing channel or <code>NULL</code> if no such channel exists.
    */
    virtual NetChannel *findChannel ( struct sockaddr_in &distant );

    /**
        Returns a net-channel which is able to process broadcast messages.
        @return Reference to the broadcast (control) NetChannel.
    */
    virtual NetChannel *getBroadcastChannel ();

    /**
        Destroys the given net-channel instance. Cannot be used for broadcast channels.
        <p>All pending operations will be cancelled too.
        @param  channel Net-channel reference to be destroyed.
    */
    virtual void deleteChannel ( NetChannel *channel );

    /**
        Destroys the whole network communication.
        Should be called in program shutdown progress only.
    */
    virtual ~NetPool ();

    };

#endif
