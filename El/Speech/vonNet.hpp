#ifdef _MSC_VER
#  pragma once
#endif

/**
  @file   vonNet.hpp
  @brief  VoN network interface.

  Communication between VoN system and Net* layer.
  <p>Copyright &copy; 2003 by BIStudio (www.bistudio.com)
  @author PE
  @since  19.1.2003
  @date   15.6.2003
*/

#ifndef _VONNET_H
#define _VONNET_H

#include "vonApp.hpp"

//----------------------------------------------------------------------
//  Network packets:

#define MAGIC_VON_CTRL      0xc1c1a027
#define VON_CMD_CREATE      1
#define VON_CMD_REMOVE      4

/// information about peer to peer communication (sent by a client to server)
#define VON_CMD_PEER_INFO      5
/// information about peer to peer cliques (sent by server to all clients)
#define VON_CMD_PEER_CLIQUES      6
/// VoN data packet sent through game socket
#define VON_CMD_ENCAPSULATE_VOICE  7
/// Connection does not seem to work anymore, server should remove it from Connection Matrix and Cliques, 
/// client should set its peerToPeer to false
#define VON_CMD_RESET_CONNECTION   8

#ifndef __GNUC__
  #pragma warning(disable:4200)
  #pragma pack(push)
  #pragma pack(1)
#endif

/**
    General VoN packet.
*/
struct VoNMagicPacket
{

  /// 32-bit magic number.
  unsigned32 magic;

  /// Voice-channel id (dpid of sender).
  VoNChannelId channel;

} PACKED;

/**
    Packet with voice data.
*/
struct VoNDataPacket
{
  /// Voice-channel id.
  VoNChannelId channel;

  /// Voice chat channel
  VoNChatChannel chatChannel;

  /// Origin of the voice data (in samples).
  unsigned64 origin;

  /// Size of the voice data (in samples).
  unsigned16 samples;

  /// Encoded data size in bytes.
  unsigned16 size;

  /// The data itself.
  unsigned16 data[0];

} PACKED;

const unsigned VON_DATA_OFFSET = sizeof(VoNDataPacket);

/*
  Array 'data':
    Frame code: unsigned16 frameLen;
                unsigned8  data[frameLen]
    Silence:    unsigned16 frameLen = 2;
                unsigned16 silenceSize;
    End-of-msg: unsigned16 frameLen = 0;
*/

/**
    Control (command) packet.
*/
struct VoNControlPacket : public VoNMagicPacket
{

  /// VON_CMD_*
  unsigned8 command;

  /// Private data size in bytes.
  unsigned16 size;

  /// Private data (codec info) itself.
  unsigned8 data[0];

} PACKED;

const unsigned VON_CONTROL_OFFSET = sizeof(VoNControlPacket);

#ifndef __GNUC__
  #pragma pack(pop)
#endif

#endif
