#ifdef _MSC_VER
#  pragma once
#endif

/*
@file   peerfactoryxbox.hpp
@brief  Net-peer factory creating NetPeerXbox instances.

Copyright &copy; 2003 by BIStudio (www.bistudio.com)
@author PE
@since  19.2.2003
@date   2.4.2003
*/

#ifndef _PEERFACTORYXBOX_H
#define _PEERFACTORYXBOX_H

#if _XBOX_SECURE

void initNetworkXbox();
void doneNetworkXbox();

//------------------------------------------------------------
//  PeerChannelFactoryXbox class:

/**
Factory for creating new NetPeer instances.
*/
class PeerChannelFactoryXbox : public PeerChannelFactory {

  friend void initNetworkXbox();
  friend void doneNetworkXbox();

protected:

  static int instances;                   ///< number of instances (for WSAStartup)

public:

  PeerChannelFactoryXbox ();

  /**
  Creates a new net-peer.
  @param  pool Network pool to work with.
  @param  tryPorts Local ports to try (if <code>NULL</code>, use <code>pool->getLocalPorts()</code>).
  @param  useVDP Use VDP protocol (instead of UDP).
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

  //virtual NetPeerToPeerChannel *createPeerToPeer();
  
  virtual ~PeerChannelFactoryXbox ();

};


#endif
#endif
