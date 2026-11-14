#ifdef _MSC_VER
#  pragma once
#endif

/*
    @file   peerfactory.hpp
    @brief  Net-peer factory creating NetPeerUDP instances.
    
    Copyright &copy; 2001-2003 by BIStudio (www.bistudio.com)
    @author PE
    @since  3.12.2001
    @date   2.4.2003
*/

#ifndef _PEERFACTORY_H
#define _PEERFACTORY_H

void initNetworkUDP();
void doneNetworkUDP();

//------------------------------------------------------------
//  PeerChannelFactoryUDP class:

/**
    Factory for creating new NetPeer instances.
*/
class PeerChannelFactoryUDP : public PeerChannelFactory {

  friend void initNetworkUDP();
  friend void doneNetworkUDP();

protected:

    static int instances;                   ///< number of instances (for WSAStartup)

public:

    PeerChannelFactoryUDP ();

    /**
        Creates a new net-peer.
        @param  pool Network pool to work with.
        @param  tryPorts Local ports to try (if <code>NULL</code>, use <code>pool->getLocalPorts()</code>).
        @param  useVDP Use VDP protocol (instead of UDP) - only for Xbox.
        @return Reference to the new peer (<code>NULL</code> if failed)
    */
    virtual NetPeer *createPeer ( NetPool *pool, BitMask *tryPorts, bool useVDP, RawMessageCallback callback );

    /**
        Creates a new net-channel.
        @param  pool Network pool to work with.
        @param  control Create control channel?
        @return Reference to the new channel (<code>NULL</code> if failed).
    */
    virtual NetChannel *createChannel ( NetPool *pool, bool control =false );

    virtual ~PeerChannelFactoryUDP ();

    };


#endif
