#ifdef _MSC_VER
#  pragma once
#endif

/**
  @file   netglobal.hpp
  @brief  Global declarations for the "network" subproject

  Copyright &copy; 2001-2003 by BIStudio (www.bistudio.com)
  @author PE
  @since  21.11.2001
  @date   18.11.2003
*/

#ifndef _NETGLOBAL_H
#define _NETGLOBAL_H

#ifndef __GNUC__
#  pragma pack (push,netGlobal,1)
#endif

/// Message serial number (assigned chronologically starting at <code>MSG_SERIAL_NULL+1</code>)
typedef unsigned32 MsgSerial;
/// Dummy message serial number
const MsgSerial MSG_SERIAL_NULL = 0;

#ifdef _WIN32

/// IP address from sockaddr_in
#  define ADDR(addr) ntohl((addr).sin_addr.S_un.S_addr)

/// IP4 byte from sockaddr_in
#  define IP4(addr)  ((addr).sin_addr.S_un.S_un_b.s_b1)
/// IP3 byte from sockaddr_in
#  define IP3(addr)  ((addr).sin_addr.S_un.S_un_b.s_b2)
/// IP2 byte from sockaddr_in
#  define IP2(addr)  ((addr).sin_addr.S_un.S_un_b.s_b3)
/// IP1 byte from sockaddr_in
#  define IP1(addr)  ((addr).sin_addr.S_un.S_un_b.s_b4)

#else

/// IP address from sockaddr_in
#  define ADDR(addr) ntohl((addr).sin_addr.s_addr)

/// IP4 byte from sockaddr_in
#  define IP4(addr)  ((addr).sin_addr.s_addr & 0xff)
/// IP3 byte from sockaddr_in
#  define IP3(addr)  (((addr).sin_addr.s_addr>>8) & 0xff)
/// IP2 byte from sockaddr_in
#  define IP2(addr)  (((addr).sin_addr.s_addr>>16) & 0xff)
/// IP1 byte from sockaddr_in
#  define IP1(addr)  (((addr).sin_addr.s_addr>>24) & 0xff)

typedef int SOCKET;

/// Return value in case of error (general usage).
#  define SOCKET_ERROR   -1

/// Return value of socket() in case of error.
#  define INVALID_SOCKET -1

/// Closing of socket handle is like closing anything else in POSIX..
#  define closesocket(s) ::close(s)

#endif

#define PORT(addr)  ntohs((addr).sin_port)

#if !_SUPER_RELEASE

/// message-ID is included in MsgHeader (for debugging purposes only).
//#  define MSG_ID

#endif

/// Maximum size of incoming datagram. For static allocation only (can be larger than real packets - not smaller).
const int MAX_IN_DATA = 2048;

/**
    Message header.
    <p>Fixed-size message header (designed to be used above the UDP protocol).
    Underlying protocol needs not to be reliable (16-bit checksum in UDP is not trustable?).
    <p>Network-byte order is used in all items.
    @todo   Some security mechanism? (against message-faking)
*/
struct MsgHeader {
    /// length of the message data (<strong>including</strong> this header). Can be <code>MSG_HEADER_LEN</code> (for ack).
    unsigned16 length;
    /// message flags
    unsigned16 flags;
      /// 32-bit CRC check-sum (including the header). ??? alternative: 16-bit CRC ???
      /** no need for CRC when packes are secured */
      unsigned32 crc;
    /// serial number of the message (unique within one directed communication channel)
    MsgSerial serial;
    /// origin of the acknowledge-bitmask (serial number of the newest acknowledgement being transmitted)
    unsigned32 ackOrigin;
    union {
        /// acknowledge bit-mask (newer messages are acknowledged in low-significant bits, includes <code>MSG_NULL</code>)
        unsigned64 ackBitmask;
        struct {
            /// 1st part of an acknowledgement, any other control data
            unsigned32 control1;
            /// previous VIM message for VIM ordering, any other control data
            unsigned32 control2;
            } c;
        };
#ifdef MSG_ID
    /// Copy of NetMessage::id (for debugging purposes only)
    MsgSerial id;
#endif
    } PACKED;

/// Length of message header in bytes (constant) .. 24 bytes.
#define MSG_HEADER_LEN      sizeof(MsgHeader)
/// VIM (Very Important Message = guaranteed) flag.
#define MSG_VIM_FLAG        0x8000
/// Urgent message flag.
#define MSG_URGENT_FLAG     0x4000
/// VIM message ordering flag (<code>control2</code> contains serial number of previous VIM message).
#define MSG_ORDERED_FLAG    0x2000
/// From-broadcast flag (acknowledgement mask etc. should be ignored).
#define MSG_FROM_BCAST_FLAG 0x1000
/// To-broadcast flag (message will be processed by special /control/ channel).
#define MSG_TO_BCAST_FLAG   0x0800
/// Time-delay flag (<code>control2</code> contains time-delay value in micro-seconds ... refers to the newest acknowledged message).
#define MSG_DELAY_FLAG      0x0400
/// Instant-reply flag (instant message reply is required - with <code>MSG_DELAY_FLAG</code> set).
#define MSG_INSTANT_FLAG    0x0200
/// Message header contains bandwidth value computed on the receiver (<code>control2</code>).
#define MSG_BANDWIDTH_FLAG  0x0100
/// Packet-bunch flag (this message was sent immediately after the previous one).
#define MSG_BUNCH_FLAG      0x0080
/// Dummy = internal packet (must be ignored by upper layers).
#define MSG_DUMMY_FLAG      0x0040
/// Part of a bigger message.
#define MSG_PART_FLAG       0x0020
/// Closing part of bigger message.
#define MSG_CLOSING_FLAG    0x0010
/// Voice packet.
#define MSG_VOICE_FLAG      0x0008
/// User flags
#define MSG_USER_FLAGS      0x0007
/// All flags
#define MSG_ALL_FLAGS       0xffff

/// Maximum value for MsgHeader:length.
#define MSG_MAX_LENGTH      0xffff

/// Short (32-bit) acknowledgement bit-mask predicate.
#define SHORT_ACK(flags)    (((flags)&(MSG_ORDERED_FLAG|MSG_DELAY_FLAG|MSG_BANDWIDTH_FLAG))!=0)

/// Total headers' size of superior protocols (IP & UDP).
#define IP_UDP_HEADER       28


/**
    Fraction message header. Not imlemented!
    <p>This fixed-size header immediately follows the <code>MsgHeader</code> in <code>MSG_FRACTION_FLAG</code>-tagged
    messages.
    <p>Network-byte order is used in all items.
*/
struct MsgFractionHeader {

    /// Total message length in bytes
    unsigned32 totalLen;

    /// Offset of this partial message (fraction) in bytes
    unsigned32 offset;

    } PACKED;

/**
    Status codes for getStatus() routines (asynchronous receive* and send* result codes).
    <p>INPUT:
    <ul>
      <li><code>nsInvalidMessage</code> - I/O operation has not been initiated yet
      <li><code>nsInputPending</code> - Input operation was initiated but has not been finished yet
      <li><code>nsInputReceived</code> - Input (receive) operation was finished (but no acknowledgements has been sent yet)
      <li><code>nsInputPartialAck</code> - Small number of acknowledgements (but yet not enough) has been sent
      <li><code>nsInputAck</code> - Complete set of acknowledgements has been sent
      <li><code>nsInvalidMessage</code> - message was recycled (returned to the <code>NetMessagePool</code>)
    </ul>
    <p>Memo: for <b>non-VIM</b> messages there is no guarantee of <code>nsInputPartialAck</code> and
       <code>nsInputAck</code> states!
    <p>OUTPUT: 
    <ul>
      <li><code>nsInvalidMessage</code> - I/O operation has not been initiated yet (it is possible to cancel the <b>send</b> operation)
      <li><code>nsOutputPending</code> - Output operation was initiated but has not been finished yet
      <li><code>nsOutputSent</code> - Output (send) operation was finished (but no acknowledgement has been received yet)
      <li>[<code>nsOutputTimeout</code>] - Message has been re-sent (at least once) after acknowledgement timeout
      <li><code>nsOutputAck</code> - Message acknowledgement has been received
      <li><code>nsInvalidMessage</code> - message was recycled (returned to the <code>NetMessagePool</code>)
    </ul>
    <p>Memo: for <b>non-VIM</b> messages there is no guarantee of <code>nsOutputTimeout</code> and
       <code>nsOutputAck</code> states!
*/
enum NetStatus {
    nsError,                    ///< undetermined error has been occurred
    nsOK,                       ///< operation finished successfully

    nsInvalidSharing,           ///< invalid port/address sharing (while establishing a connection)
    nsInvalidMessage,           ///< invalid message header/serial number (before I/O initiation call)

    nsInputPending,             ///< operation is in input-pending state
    nsInputReceived,            ///< message was received but no acknowledgement has been sent yet
    nsInputPartialAck,          ///< message was received and a small number of acknowledgements has been sent
    nsInputAck,                 ///< message was received and a complete set of acknowledgements has been sent

    nsOutputPending,            ///< operation is in output-pending state
    nsOutputSent,               ///< message has been sent (but no acknowledgement was received yet)
    nsOutputObsolete,           ///< send-timeout of the message had been expired; the message was not sent
    nsOutputTimeout,            ///< message has been re-sent after acknowledgement timeout
    nsOutputAck,                ///< message acknowledgement has been received

    nsCancel,                   ///< message has been cancelled
    nsNoMoreCallbacks,          ///< no more callbacks are needed for this message (for nextEvent only)
    };

class NetMessage;

/**
    Call-back routine for asynchronous network I/O.
    @param  msg NetMessage to be processed.
    @param  event Type of call-back event.
	@param  data Context data (non-mandatory).
    @return Next required call-back event type (with the same call-back routine).
*/
typedef NetStatus NetCallBack ( NetMessage *msg, NetStatus event, void *data );

#ifndef __GNUC__
#  pragma pack (pop,netGlobal)
#endif

//-------------------------------------------------------------------------
//  NET_STRESS:

#if !_SUPER_RELEASE
    // enable NET_STRESS only in internal build

/// Net-stress switch.
#  define NET_STRESS
/// Net-stress: control file (examined every couple of seconds).
#  define NET_STRESS_FILE     "netstress.txt"

#endif

//-------------------------------------------------------------------------

#ifdef NET_STRESS

#include "El/Common/randomJames.h"

#include <Es/Memory/normalNew.hpp>

/**
 * Incoming network data is stored in this structure until they are
 * processed (artificial latency).
 */
class DeferredMessage : public RefCountSafe
{

public:

  /// Data is going to be processed at this time.
  unsigned64 m_time;

  /// Source IP address.
  struct sockaddr_in m_from;

  /// Actual data length.
  int m_len;

  /// Actual data packet.
  union
  {
    MsgHeader m_header;
    char m_data[MAX_IN_DATA];           ///< fixed buffer to receive message data
  };

  RefD<DeferredMessage> m_next;         ///< linked list sorted by "m_time"

  /// MT-safe new operator.
  static void* operator new ( size_t size );

  static void* operator new ( size_t size, const char *file, int line );

  /// MT-safe delete operator.
  static void operator delete ( void *mem );

};

#include <Es/Memory/debugNew.hpp>

//-------------------------------------------------------------------------
//  NetStress manager:

class NetStress : public RefCountSafe
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

  /// netstress.txt file exists?
  bool m_valid;

  /// Peer-id (for logging purposes).
  unsigned m_peerId;

  /// Private random generator
  RandomJames m_rnd;

  /// Linked-list of deferred messages.
  RefD<DeferredMessage> m_def;

  /// Time interval to check NET_STRESS_FILE for changes in configuration.
  unsigned64 m_interval;

  /// Next check-time.
  unsigned64 m_nextCheck;

  /// Minimal artificial latency in microseconds.
  unsigned64 m_minLatency;

  /// Variation of artificial latency in microseconds.
  unsigned64 m_varLatency;

  /// Drop probability for each packet.
  double m_dropProbability;

  /// Statistics: number of undropped packets.
  unsigned m_packets;

  /// Statistics: number of packet drops.
  unsigned m_drops;

  /// Statistics: total latency in us.
  unsigned64 m_sumLat;

  /// Statistics: minimal latency in us.
  unsigned64 m_minLat;

  /// Statistics: maximal latency in us.
  unsigned64 m_maxLat;

  /// [re-]reads config file (NET_STRESS_FILE).
  void readConfig ();

public:
  
  NetStress ( unsigned64 interval, unsigned peerId );

  /// Returns true if it is properly configured and can be used..
  bool valid ()
  {
    return m_valid;
  }

  /// Returns next deferred message to be processed (its time should be <= now).
  char *getData ( unsigned64 now, int &len, struct sockaddr_in &from );

  /**
   * Remove previously processed data from the list.
   * Must be called for every packet received by "getDeferredData()"
   */
  void removeData ( char *data );

  /**
   * Receive new data.
   * NetStress manager decides whether it can be dropped or delayed..
   */
  void insertData ( const char *data, int len, const struct sockaddr_in &from, unsigned64 now );

  /// Periodical maintenance (config-file re-read, etc.).
  void tick ( unsigned64 now );

};

#endif

#endif
