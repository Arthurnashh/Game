#ifdef _MSC_VER
#  pragma once
#endif

/*
    @file   directchannel.hpp
    @brief  NetChannel for direct (local) communication.

    Copyright &copy; 2003 by BIStudio (www.bistudio.com)
    @author PE
    @since  21.4.2003
    @date   23.4.2003
*/

#ifndef _DIRECTCHANNEL_H
#define _DIRECTCHANNEL_H

//------------------------------------------------------------
//  DirectChannel: basic implementation of local communication channel

#include <Es/Memory/normalNew.hpp>

/**
    DirectChannel class.
    @since  21.4.2003
    @date   23.4.2003
*/
class DirectChannel : public NetChannel {

public:

    /**
        Default constructor. Does no initialization, open() should be used
        to setup a connection.
    */
    DirectChannel ();

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
        Connects this channel locally to the given <code>NetChannel</code> instance.
    */
    virtual NetStatus connect ( RefD<NetChannel> dist );

    /**
        Reconnects this channel to the given address.
        <p>Keeps the whole channel state.
        @param  distant Regularly filled <code>sockaddr_in</code> structure containing
                        address of distant peer
        @return Status code (<code>nsError</code> or <code>nsOK</code>)
    */
    virtual NetStatus reconnect ( struct sockaddr_in &distant );

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

    /**
        Process the given incoming data.
        Asynchronously called by the listener thread.
        @param  hdr Header that was received (includes message length,
                    continues with message data itself).
        @param  distant network address data came from.
    */
    virtual void processData ( MsgHeader *hdr, const struct sockaddr_in &distant );

    virtual void sendRaw( const sockaddr_in &ia, const void *data, int size, int sizeEncrypted );
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
    virtual ~DirectChannel ();

protected:

    bool opened;                            ///< is this net-channel opened?

    RefD<NetChannel> distant;               ///< Communication partner.

    MsgSerial serial;                       ///< next outgoing-message serial number (must not be <code>MSG_NULL</code>)

    };

#include <Es/Memory/debugNew.hpp>

#endif
