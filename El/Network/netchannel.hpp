#ifdef _MSC_VER
#  pragma once
#endif

/*
  @file   netchannel.hpp
  @brief  Network channel object.

  Copyright &copy; 2001-2004 by BIStudio (www.bistudio.com)
  @author PE
  @since  21.11.2001
  @date   15.4.2004
*/

#ifndef _NETCHANNEL_H
#define _NETCHANNEL_H

#include "El/Network/netpeer.hpp"


//------------------------------------------------------------
//  NetChannelBasic: basic implementation of network communication channel

extern MsgSerial netMessageToSerial (const RefD<NetMessage> &msg);
extern MsgSerial netMessageDepend (const RefD<NetMessage> &msg);

#define MAX_ACK_ARRAY        1024           ///< Size of acknowledgement array (incoming traffic).

#define MAX_OLD_ACKS            8           ///< Size of high-priority acknowledgement queue.

/**
    Struct to collect status of all active message dispatchers (on all channels).
    Data collected here will help to individual NetChannelBasic instances to decide
    whether to send a message (in getPreparedMessage()) or not..
*/
struct DispatcherStatusBasic : public DispatcherStatus {

    unsigned channels;                      ///< number of active channels.

    unsigned channelsWithUrgentMessages;    ///< channels with at least one urgent message prepared to send.

    unsigned channelsWithCommonMessages;

    unsigned totalUrgentMessages;           ///< total number of urgent messages prepared in all channels.

    unsigned channelsWithVIMMessages;       ///< channels with at least one VIM (but not urgent) message prepared.

    unsigned totalVIMMessages;              ///< total number of VIM (but not urgent) messages prepared in all channels

    unsigned totalCommonMessages;

    };

#include <Es/Memory/normalNew.hpp>

/**
    NetChannelBasic class. Each net-channel is responsible for individual
    [localIP:port]-[distantIP:port] communication.
    @since  27.11.2001
    @date   24.9.2003
*/
class NetChannelBasic : public NetChannel {

protected:

    /// Associated net-peer instance.
    NetPeer *peer;

    /// Actual acknowledgement timeout in microseconds (for VIM only).
    unsigned timeout;

    /// Is this control channel?
    bool control;

public:

    /**
        Default constructor. Does no initialization, open() should be used
        to setup a connection.
        @param  _control Is this control channel?
    */
    NetChannelBasic ( bool _control );

    /**
        Establishes connection for this net-channel.
        <p>Could do some negotiation between local and distant peers. A small
        amound of data can be exchanged. Blocks program flow.
        @param  _peer Net-peer to be associated with
        @param  distant Regularly filled <code>sockaddr_in</code> structure containing
                        address of distant peer
        @return Status code (<code>nsError</code> or <code>nsOK</code>)
    */
    virtual NetStatus open ( NetPeer *_peer, struct sockaddr_in &distant );

    /**
        Reconnects this channel to the given address.
        <p>Keeps the whole channel state.
        @param  distant Regularly filled <code>sockaddr_in</code> structure containing
                        address of distant peer
        @return Status code (<code>nsError</code> or <code>nsOK</code>)
    */
    virtual NetStatus reconnect ( struct sockaddr_in &distant );

    /// Returns maximum data length for communication through this net-channel.
	virtual unsigned maxMessageData () const;

    /**
        Is this control channel?
    */
    virtual bool isControl () const;

#ifdef NET_LOG

	virtual char *getChannelInfo ( char *buf ) const;

#endif

#if _ENABLE_REPORT || defined(NET_LOG)

    /// Prints out dump of all important status variables of the channel
    virtual char *channelDump ( char *buf ) const;

#endif

    /**
        Sets network tuning parameters.
        @param  p Structure holding network params.
    */
    virtual void setNetworkParams ( const NetworkParams &p );

    /**
        Sets network tuning parameters.
        @param  p Structure holding network params.
    */
    static void setGlobalNetworkParams ( const NetworkParams &p );

    /**
        Actual channel latency in microseconds.
        @return The actual channel-latency in microseconds (<code>0</code> if not determined yet).
        @param  actLat return actual latency
        @param  minLat return minimal (best possible) latency
    */
    virtual unsigned getLatency ( unsigned *actLat =NULL, unsigned *minLat =NULL );

    /**
        Actual output channel band-width in bytes per second.
        @param  data Additional data record (non-mandatory).
        @return The actual output band-width (throughput) in bytes/sec.
    */
    virtual unsigned getOutputBandWidth ( EnhancedBWInfo *data =NULL );

    /**
        Retrieve some special internal NetChannel statistics. Not all items should be meaningfull in all implementations..
        @return <code>true</code> if at least some items were filled.
    */
    virtual bool getInternalStatistics ( ChannelStatistics &stat );

    /**
        Last time any message was received (in getSystemTime() format).
        All messages are taken into account (even control ones).
    */
    virtual unsigned64 getLastMessageArrival () const;

    /**
        Retrieves information about output message queue.
        Only not-yet-sent messages are relevant.
        @param  msgs Number of comomon messages in the queue.
        @param  bytes Total number of bytes waiting to be sent (for common messages).
        @param  vimMsgs Number of VIM messages in the queue.
        @param  vimBytes Total number of bytes waiting to be sent (for VIM messages).
    */
    virtual void getOutputQueueStatistics ( int &msgs, int &bytes, int &vimMsgs, int &vimBytes );

    /**
        Retrieves associated distant network address (peer).
        @param  distant Buffer the result will be filled in
    */
    virtual void getDistantAddress ( struct sockaddr_in &distant ) const;

    /**
        Retrieves associated local network address (peer).
        @param  distant Buffer the result will be filled in
    */
    virtual void getLocalAddress ( struct sockaddr_in &local ) const;

    virtual void sendRaw( const sockaddr_in &ia, const void *data, int size, int sizeEncrypted );

    /**
        Process the given incoming data.
        Asynchronously called by the listener thread.
        @param  hdr Header that was received (includes message length,
                    continues with message data itself).
        @param  distant network address data came from.
    */
    virtual void processData ( MsgHeader *hdr, const struct sockaddr_in &distant );

    virtual void dataSent(size_t size, unsigned64 refTime);

    virtual void dataSentAck(size_t size);

    /**
        Dispatches the given message for output.
        @param  msg Message to be sent.
        @param  urgent Is this message urgent?
    */
    virtual void dispatchMessage ( NetMessage *msg, bool urgent =false );

    /**
        Retrieves the last VIM message dispatched so far..
    */
    virtual NetMessage *getLastVIM ( bool urgent );

    /**
        Collects dispatcher status for this channel.
        @param  data Pre-allocated buffer to receive dispatcher statistics.
    */
    virtual void nextDispatcherStatus ( DispatcherStatus *data );

    /**
        Prepares next message from the send-queue.
        Sets prepared member variable.
        @param  data Buffer with collected dispatcher status of all active channels (even broadcast ones).
        @return <code>true</code> if prepared contains (fully prepared) message to send.
    */
    virtual bool getPreparedMessage ( void *data );

    /**
        Called immediately before 'prepared' message is sent.
        Sets timing variables, checks the packet-bunch mechanism, etc.
        @param  bunchStart Time of actual send-bunch start (or <code>0</code> if this message is just starting a new bunch).
        @return Actual system time in microseconds.
    */
    virtual unsigned64 preSend ( unsigned64 bunchStart );

    /**
        Called immediately after 'prepared' message has been sent.
        Store the message for future access (acknowledgements, re-send, etc.).
    */
    virtual void postSend ();

    /**
        Finds the given (sent) message and returns its reference time (or 0 if not found).
    */
    virtual unsigned64 getMessageTime ( MsgSerial ser );

    /// Maximum time interval of runRevisited.
    static const unsigned64 RUN_INTERVAL;

    /// Maximum time interval of adjustChannel.
    static const unsigned64 ADJUST_INTERVAL;

    /**
        Cancels all messages waiting for send..
    */
    virtual void cancelAllMessages ();

    /**
        Checks channel connectivity.
        Should be called time-to-time to assure continuing channel-connectivity.
        <p>Sends a new "ping" request if necessary..
        @param  now Actual system time (can be <code>0</code>).
    */
    virtual void checkConnectivity ( unsigned64 now );

    /**
        Checks whether the connection has dropped.
        Should use tolerant time constants to enable player re-connection..
    */
    virtual bool dropped ();

    /**
        Periodic channel-update function.
    */
    virtual void tick ();

    /**
        Closes the net-channel.
        <p>Cancels all pending operations! Discards all associations and registrations.
    */
    virtual void close ();

    static NetworkParams par;               ///< actual network params.

    /**
        MT-safe new operator.
    */
    static void* operator new ( size_t size );

    static void* operator new ( size_t size, const char *file, int line );

    /**
        MT-safe delete operator.
    */
    static void operator delete ( void *mem );

#ifdef __INTEL_COMPILER

    static void operator delete ( void *mem, const char *file, int line );
    
#endif

    /**
        Destructor.
        <p>Cancels all pending operations!
    */
    virtual ~NetChannelBasic ();

protected:

    bool opened;                            ///< is this net-channel opened?

    struct sockaddr_in dist;                ///< distant network address (dist.sin_addr.s_addr == INADDR_BROADCAST iff in broadcast mode).

    //-----------------------------------------------------
    //  output:

    MsgSerial serial;                       ///< next outgoing-message serial number (must not be <code>MSG_NULL</code>)
    MsgSerial lastSerialSent;               ///< serial of last message really sent

    Ref<NetMessage> vimToSend;              ///< linked-list of VIM (but not-urgent!) messages to send.
    NetMessage *vimToSendEnd;               ///< end of the VIM queue.
    Ref<NetMessage> lastVIM;                ///< last VIM message thas has been dispatched.

    Ref<NetMessage> urgentToSend;           ///< linked-list of urgent messages to send.
    NetMessage *urgentToSendEnd;            ///< end of the urgent queue.
    Ref<NetMessage> lastUrgent;             ///< last urgent VIM message thas has been dispatched.

    Ref<NetMessage> commonToSend;           ///< linked list of common messages to send.
    NetMessage *commonToSendEnd;            ///< end of the common queue.

    BitMaskMTS outputAckMask;               ///< ack bit-mask = negative mask of received acknowledgements. 1 if sent but not acked (yet).

    MsgSerial ackMax;                       ///< actual maximum of incoming-acknowledgement.

    MsgSerial ackMin;                       ///< actual minimum of incoming-acknowledgement (incremented by batch).

    BitMaskMTS newOutputAckMask;            ///< fresh messages being sent in recent time. Time to time it is copied into 'outputAckMask'.

    /// Queue of sent messages chunks. [2k] .. time, [2k+1] .. summary in bytes.
    unsigned64 sendQueue[SLIDING_WINDOW_SEND];

    /// Index into sendQueue. Must be even.
    int sendIndex;
    
    
#define NET_USE_SEND_QUEUE 0

#if !NET_USE_SEND_QUEUE
    //@{
    /// \name sending buffer state prediction
    /// time when last send was done
    unsigned64 _sendTime;
    /// amount of data which was sent as of sendTime
    float _sendAmount;

    /// update _sendAmount based on time expired and bandwidth estimation
    void SimulateSending(unsigned64 now);
    //@}
#endif

    void InitSendQueue(unsigned64 time);

    /// Set of NetMessage-s waiting for acknowledgement/resend etc.
    typedef ImplicitMap<MsgSerial,RefD<NetMessage>,netMessageToSerial,true,MemAllocSafe> RevisitedMap;
    RevisitedMap revisited;

    /// Last time runRevisited() was called.
    unsigned64 runTime;

    /// Check all revisited NetMessage-s for timeout.
    void runRevisited ();

    /// Sets MsgHeader data for the given output message (acknowledgements etc.).
    void setOutputData ( NetMessage *msg );

    /// inserts the re-sended message to the correct place..
    void insertResend ( NetMessage *msg );

    /// Inserts the given message into urgent-message queue.
    void insertUrgent ( NetMessage *msg );

    /// Inserts the given message into urgent-message queue just after the second given message.
    void insertUrgentAfter ( NetMessage *msg, NetMessage *after );

    /// Inserts the given message into VIM-message queue.
    void insertVIM ( NetMessage *msg );

    /// Inserts the given message into common-message queue.
    void insertCommon ( NetMessage *msg );

    /// Assembles the big-acknowledgement-message. Accepts predefined message length.
    void setBigAckMessage ( NetMessage *msg );

    /// Prepares the 1st urgent message to 'prepared'.
    bool getUrgentMessage ();

    /// Prepares the 1st VIM message to 'prepared'.
    bool getVIMMessage ();

    /// Prepares the 1st common message to 'prepared'.
    bool getCommonMessage ();

    //-----------------------------------------------------
    //  input:

    /// Ordered messages waiting for their predecessors. Maps "predecessor serial number" to linked list of its successors.
    ImplicitMap<MsgSerial,RefD<NetMessage>,netMessageDepend,true,MemAllocSafe> deferred;

    /**
        Update input NetChannel statistics according to new NetMessage.
        @param  msg A new received message.
    */
    void inputStatistics ( NetMessage *msg );

    /**
        Processing of duplicite incoming message.
        @param  msg Resent message.
    */
    void inputResent ( NetMessage *msg );

    /**
        Process incoming VIM message.
        Takes into account everything but message ordering. Should process pending messages
        as well. It is called in "enter()" state, it calls "leave()".
        @param  msg Message to be processed.
    */
    void processVIM ( NetMessage *msg );

    /**
        Sets/creates instant-message (heart-beat) as response to the given message.
        Works in the middle of "enter()" mode.
        @param  request NetMessage which had requested instant-response.
    */
    void setDelayMessage ( NetMessage *request );

    /**
        Incoming-messages which should be acknowledged.
        <code>0xff</code> .. not received yet, <code>0</code> .. needs not be acknowledged.
        <code>0 &lt; N &lt; 0xff</code> .. needs to be acknowledged
        at least <code>N</code> times.
    */
    unsigned8 ack[MAX_ACK_ARRAY];

    /// Short queue for old messages to be acknowledged. Their acknowledgement will have the highest priority.
    MsgSerial oldAckQueue[MAX_OLD_ACKS];

    /// Index of the next oldAckQueue item to be processed.
    int oldAckFirst;

    /// Insertion point of oldAckQueue.
    int oldAckLast;

    /**
        Reference times of all incoming messages
        (lower 32 bits .. doesn't matter because I need only 32-bit differences).
        Only for packet-pairs!
        Indices are the same as for 'ack'.
    */
    unsigned ackTime[MAX_ACK_ARRAY];

    int ackPtr;                             ///< index into 'ack' corresponding to the 'inputMax' message.

    MsgSerial inputMax;                     ///< actual maximum of incoming-message serial number (incremented by 1).

    MsgSerial inputMin;                     ///< actual minimum serial number stored in 'ack'.

    BitMaskMTS ackMask;                     ///< ack bit-mask = mask of received messages (very large - for safe).

    MsgSerial ackMaskMin;                   ///< the oldest serial number stored in ackMask.

    bool starvation;                        ///< acknowledgement starvation (too many incoming messages but a little outgoing ones).

    unsigned recentVIMs;                    ///< number of received VIM messages which arrive after the last message departure.

    inline bool received ( MsgSerial ser )
    {
        if ( ser < ackMaskMin ) return true;
        return ackMask.get(ser);
    }

    BitMaskMTS procMask;                    ///< BitMask of really processed messages (on application side). Only for VIM messages!

    unsigned newAcks;                       ///< Number of newly acnowledged messages (temporary).

    unsigned newBytes;                      ///< Total number of newly acknowledged bytes (temporary).

    /// Process message acknowledgement. Must be called inside of enter().
    void newAcknowledgement ( MsgSerial s, NetMessage *msg );

    //--- timings, reliability ----------------------------

    unsigned64 lastMsgArrival;              ///< Time of last message (of any type) arrival.

    unsigned64 lastMsgDeparture;            ///< Time of last message (of any type) departure (send time).

    unsigned64 lastPingArrival;             ///< Time of last "ping" (MSG_DELAY) message arrival.

    unsigned64 lastPingDeparture;           ///< Time of last "ping" (MSG_INSTANT) message departure (request).

    unsigned64 nextBandwidthDeparture;      ///< Time to send the next RB-estimate of bandwidth back to the sender.

    unsigned64 lastPairDeparture;           ///< Time of last "packet-pair" (MSG_BUNCH) message departure.

    /**
        Checks channel connectivity.
        Internal routine - must be called inside the enter() .. leave().
        @param  now Actual system time (can be <code>0</code>).
    */
    void checkConnectivityInternal ( unsigned64 now );

    //-----------------------------------------------------

    /// Actual heart-beat gap (computed dynamically - based on "timeout", in microseconds).
    unsigned heartBeatGap;

    /// Average latency in micro-seconds (<code>0</code> if not computed yet).
    unsigned aveLatency;

    /// Actual latency in micro-seconds (<code>0</code> if not computed yet).
    unsigned actLatency;

    /// Minimum latency in micro-seconds (<code>INT_MAX</code> if not computed yet).
    unsigned minLatency;

    /// Time when minimum latency was set
    /// When is it getting older, it should be replaced by new value even if it is higher
    unsigned64 minLatencyTime;

    /// Long-term estimation of ack-bandwidth (will fade slowly to 0 if no ougoing traffic is acknowledged).
    unsigned goodAckBandwidth;

    /// Sliding estimation (upper-bound) of output-bandwidth based on actually transferred data (in bytes/sec).
    unsigned maxBandwidth;

    /// Receiver-based packet-bunch estimation of bandwidth (in bytes/sec).
    unsigned rbeBandwidth;

    /// Artificially limited channel bandwidth (used for debugging)
    unsigned limitBandwidth;
    /// Queue of acknowledged message chunks. [2k] .. time, [2k+1] .. summary in bytes.
    unsigned64 ackStatQueue[SLIDING_WINDOW];

    /// Index into ackStatQueue. Must be even.
    int ackStatIndex;

    /// Computes actual ack-bandwidth.
    float getAckBandwidth ( unsigned64 windowSize );

    float getSentBandwidth ( unsigned64 windowSize, unsigned64 now =0 );
    
    /// Computes actual sent-bandwidth.
    bool allowSendingMore (unsigned64 windowSize, unsigned maxBandwidth, unsigned64 now =0);

#if NET_LOG_DISPATCHER>0
    void LogSendBandwidth(unsigned64 windowSize, unsigned64 now = 0);
#endif

    //-----------------------------------------------------
    //  adjustChannel stuff:

    /// Last time adjustChannel() was called.
    unsigned64 adjustTime;

    /// Periodic channel adjustment (maxBandwidth etc.).
    void adjustChannel ();

    /// Grow state determined by actual ping.
    int growStatePing;

    /// Grow state determinated by actual packet loss ratio.
    int growStateLost;

    /// For stat-output only.
    int growState;

    /// Cummulative packet-separation for RB-estimation of incoming bandwidth (in micro-seconds).
    unsigned inDelay;

    /// Cummulative packet size (in bytes).
    unsigned inSize;

    /// Actual (averaged) RB-estimation for the receiver.
    unsigned inRbe;

#ifdef NET_LOG_BANDWIDTH

    /// Number of packet-pairs used for the estimation.
    unsigned inCounter;

#endif

#ifdef NET_LOG_CH_STATE

    /// Maximum time interval of channel-state-log (CSL).
    static const unsigned64 CH_INFO_INTERVAL;

    /// Time of the last CSL record.
    unsigned64 lastChannelStateLog;

    /// Number of packets sent from the last CSL.
    unsigned packetsOut;

    /// Number of VIM packets sent from the last CSL.
    unsigned packetsOutVIM;

    /// number of bytes sent from the last CSL.
    unsigned bytesOut;

    /// Number of packets received from the last CSL.
    unsigned packetsIn;

    /// Number of VIM packets received from the last CSL.
    unsigned packetsInVIM;

    /// number of bytes received from the last CSL.
    unsigned bytesIn;

#endif

    //-----------------------------------------------------
    //  synchronization:

    // Enter channel's critical section. Must be used in message queue access methods.
    // Moved to RefCountSafe!

    };

#include <Es/Memory/debugNew.hpp>

#endif
