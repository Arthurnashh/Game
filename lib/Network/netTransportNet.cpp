
/**
  @file   netTransportNet.cpp
  @brief  NetTransport implementation using Net* library

  Copyright &copy; 2001-2003 by BIStudio (www.bistudio.com)
  @author PE
  @date   13.10.2003
*/

#include "../wpch.hpp"
#include "netTransport.hpp"
#include "El/ParamFile/paramFile.hpp"
#include "El/QStream/qStream.hpp"
#include "../progress.hpp"
#include "../stringtableExt.hpp"
#include "../Network/networkImpl.hpp"

#include <time.h>

#if _VBS3
  #include "../hla/FileTransfer.hpp"
  #include "../hla/AAR.hpp"
  #include "../Network/networkImpl.hpp"
#endif


#if !_ENABLE_MP

NetTranspSessionEnum *CreateNetSessionEnum ( ParamEntryPar cfg )
{
  return NULL;
}

NetTranspClient *CreateNetClient ( ParamEntryPar cfg )
{
  return NULL;
}

NetTranspServer *CreateNetServer ( ParamEntryPar cfg )
{
  return NULL;
}

void VoiceDataSent(int pid, int len)
{
}

void destroyVoice ()
{
}

void WhatUnitsVoNDirect(AutoArray<int> &units, Vector3Par pos, float maxDist)
{
}

#else


#include "../memHeap.hpp"
#include "El/Network/netpch.hpp"

#  include "El/Network/peerfactory.hpp"
#  include "El/Network/netpeer.hpp"
#include "El/Network/netchannel.hpp"

#if VOICE_OVER_NET
#  include "El/Speech/vonPch.hpp"
#  include "voiceXA2.hpp"
#  if _GAMES_FOR_WINDOWS
#    include "voiceXbox.hpp"
#  endif
#endif

#if _ENABLE_GAMESPY
# include "../GameSpy/qr2/qr2.h"
# include "../GameSpy/natneg/natneg.h"
# ifdef NOMINMAX
#  undef min
#  undef max
# endif
#endif

#define GET_SERVER_PEER   getServerPeer
#define GET_CLIENT_PEER   getClientPeer
#define GET_ENUM_PEER     getClientPeer

//---------------------------------------------------------------------------
//  forward declarations:

class NetSessionEnum;
class NetClient;
class NetServer;

extern int GetNetworkPort();
extern int GetServerPort();
void decodeURLAddress ( RString address, RString &ip, int &port );
void LoadNetworkParams ( NetworkParams &data, ParamEntryPar cfg );
Ref<NetMessage> mergeMessageList ( NetMessage *msg );
NetPool *getPool ();

//---------------------------------------------------------------------------
//  interface:

#ifndef DPNID_ALL_PLAYERS_GROUP
#  define DPNID_ALL_PLAYERS_GROUP   0
#endif

#define MAX_SESSIONS             16
#  define LEN_SESSION_NAME      256
#define LEN_GAMETYPE_NAME         8
#define LEN_MISSION_NAME         40
#define LEN_ISLAND_NAME          16

#define NN_NUM_PORTS_TO_TRY      15
#define PORT_INTERVAL            12
#define UNIQUE_TO_TRY            12

#define RESERVED_IDS              8

#define LEN_PLAYER_NAME          40
#define LEN_PASSWORD_NAME        40

#if !_SUPER_RELEASE
#  undef DEDICATED_STAT_LOG
#endif

#if _ENABLE_DEDICATED_SERVER
#else
#  undef DEDICATED_STAT_LOG
#endif

#ifdef DEDICATED_STAT_LOG
#  define CONSOLE_LOG_INTERVAL 2000000
#endif

#if _ENABLE_DEDICATED_SERVER
  extern RString GetPidFileName ();
  bool IsDedicatedServer ();
  int ConsoleF ( const char *format, ... );
#endif

#ifdef NET_LOG_TRANSP_STAT
#  define STAT_LOG_INTERVAL     80000
#endif

/// Maximum enumeration message age (in milliseconds).
#define MAX_ENUM_AGE          15000
/// Minimum enumeration retry time (in milliseconds).
#define MIN_ENUM_RETRY         4000
/// Maximum time to wait for AckPlayer response (in milliseconds).
#define ACK_PLAYER_TIMEOUT     8000
/// Re-send time for MAGIC_CREATE_PLAYER and MAGIC_RECONNECT_PLAYER (in milliseconds).
#define CREATE_PLAYER_RESEND   2000
/// Time interval to check network response (NetClient::Init(), in milliseconds).
#define NET_CHECK_WAIT          100
/// Time to wait in network destructors (for last message departure, in milliseconds).
#define DESTRUCT_WAIT           500
/// Time to wait for the higher-layer to process player-delete operation (in milliseconds).
#define DESTROY_WAIT            100
/// Send-timeout for common messages (in micro-seconds).
#define SEND_TIMEOUT            800000

/// Tag for messages starting with 32-bit "magic" number.
#define MSG_MAGIC_FLAG          0x0001

/// Define this symbol if both legacy and new enumeration responses will be sent.
//#define ENUM_LEGACY

  // magic numbers (sent in little-endian format /Intel/):

/// Enumeration request (client:broadcast -> server:broadcast).
#define MAGIC_ENUM_REQUEST      0xeee191ae
/// Enumeration response (server:broadcast -> client:broadcast).
#define MAGIC_ENUM_RESPONSE     0xfff1e8ac

#if _VBS3
/// When Enumerating the server, we send back a list of AAR files
/// Avaliable on the server to download.
#define MAGIC_FILELIST_RESPONSE 0xabcdefae
#define MAGIC_SERVERNAME        0xabcdefaa
#define MAGIC_FILE_REQUEST      0xabcdeffa
#define MAGIC_FILE_RESPONSE     0xabcdeffb
#endif

/// RequestPlayer request (client:normal -> server:broadcast). Not VIM. Duplicate messages should be ignored.
#define MAGIC_REQUEST_PLAYER    0xbbba1564
/// server response to MAGIC_REQUEST_PLAYER
#define MAGIC_CHALLENGE_PLAYER  0xa5a48965
/// CreatePlayer request (client:normal -> server:broadcast). Not VIM. Duplicate messages should be ignored.
#define MAGIC_CREATE_W_CHALLENGE  0xccca5e62
/// AckPlayer response (server:both -> client:normal).
#define MAGIC_ACK_PLAYER        0xaaa51a7e
/// ReconnectPlayer request (client:normal -> server:broadcast).
#define MAGIC_RECONNECT_PLAYER  0x111044ec
/// Terminate the session (server:normal -> clients:normal). Not VIM.
#define MAGIC_TERMINATE_SESSION 0x777814a1
/// DestroyPlayer message (client:normal -> server:normal). Not VIM.
#define MAGIC_DESTROY_PLAYER    0xddd15072


#pragma pack (push,netPackets,1)

/// Universal packet starting with 32-bit magic number.
struct MagicPacket
{

  /// Magic number (MAGIC_*).
  unsigned32 magic;

} PACKED;

/// Enumeration request (will be sent through network).
struct EnumPacket : public MagicPacket
{

  /// Application-specific magic number.
  int32 magicApplication;

} PACKED;

/// Session description packet (will be sent through network).
struct SessionPacket : public MagicPacket
{

  /// Name of session.
  char name[LEN_SESSION_NAME];

  /// Current version of application on host.
  int32 actualVersion;

  /// Required version of application on host.
  int32 requiredVersion;

  /// Game type of running mission on host.
  char gameType[LEN_GAMETYPE_NAME];

  /// Localization type for the name of the mission
  char missionLocalizedType;
  /// Name of running mission on host.
  char missionId[LEN_MISSION_NAME-1];

  /// Island where running mission on host.
  char island[LEN_ISLAND_NAME];

  /// State of game on host.
  int32 serverState;

  /// Maximal number of players + 2 (or 0 if not restricted)
  int32 maxPlayers;

  /// Password is required.
  int16 password;

  /// Receiving port.
  int16 port;

  /// Actual number of players.
  int32 numPlayers;

  /// Serial number of enumeration request.
  MsgSerial request;

  /// list of mods (possibly friendly names)
  char mod[MOD_LENGTH];

  /// equal mod are required for clients
  int16 equalModRequired;

  /// selected game language id
  int32 language;
  /// selected mission difficulty
  int32 difficulty;
  //! Estimated time to mission end (in minutes)
  int32 timeleft;
  
  void SetMissionName(const LocalizedString &str);
  RString GetLocalizedMissionName() const;
} PACKED;

const size_t SESSION_PACKET_1 = offsetof(SessionPacket,numPlayers); // LEGACY_SESSION_PACKET
const size_t SESSION_PACKET_2 = offsetof(SessionPacket,mod);
const size_t SESSION_PACKET_3 = sizeof(SessionPacket);
const size_t SESSION_PACKET_SIZE = SESSION_PACKET_3;

/// Description of single session (will be stored within enumeration object).
struct NetSessionDescription : public SessionPacket
{

  /// Full address (string format).
  char address[32];

  /// IP address in network format.
  unsigned32 ip;

  /// Last update time in milliseconds.
  unsigned32 lastTime;

  /// Actual ping (RTT) time.
  unsigned32 pingTime;

} PACKED;

/// request send by the client before attempting CreatePlayerPacket
struct RequestPlayerPacket : public MagicPacket
{
  int32 magicApplication;

  int32 actualVersion;
};

struct ChallengePlayerPacket : public MagicPacket
{
  int32 challenge;
};

/// Create player packet (will be sent from NetTranspClient to NetTranspServer).
struct CreatePlayerPacket : public MagicPacket
{

  /// Application-specific magic number.
  int32 magicApplication;

  char name[LEN_PLAYER_NAME];

  char password[LEN_PASSWORD_NAME];

  char mod[MOD_LENGTH];

  int32 actualVersion;

  int32 requiredVersion;

  int32 actualBuild;


  int16 botClient;                          ///< 1 .. bot-client, 2 .. private-client (Xbox-secure only.

  int32 uniqueID;                           ///< Unique player ID (derived from system time and acknowledged by the server).

} PACKED;

struct CreatePlayerPacketChallenge: public CreatePlayerPacket
{
  int32 challenge;
};
/// Server response to CreatePlayer message (AckPlayer)
struct AckPlayerPacket : public MagicPacket
{

  /// Connection result (enum ConnectResult)
  int32 result;

  /// Player number (for future reconnection).
  int32 playerNo;

} PACKED;

/// Reconnect player packet (sent from NetTranspClient to NetTranspServer).
struct ReconnectPlayerPacket : public MagicPacket
{

  /// Player number (from first player acknowledgement).
  int32 playerNo;

} PACKED;

#pragma pack (pop,netPackets)

//---------------------------------------------------------------------------
//  NetSessionDescriptions:

/// Container for list of sessions.
class NetSessionDescriptions
{

protected:

  /// Current number of contained sessions.
  int _size;

  /// Sessions.
  NetSessionDescription _data[MAX_SESSIONS];

public:

  /// Constructor
  NetSessionDescriptions () { _size = 0; }

  /// Destructor
  ~NetSessionDescriptions () { Clear(); }

  /// Return current number of contained sessions
  int Size () { return _size; }

  /**
    Add new session.
    @return index of added session or -1 if session doesn't fit into container.
  */
  int Add ();

  /// Delete one session.
  void Delete ( int i );

  /// Delete all sessions.
  void Clear ();

  /// R/W access to single session.
  NetSessionDescription &operator [] ( int i )
  { return _data[i]; }

  /// R/O access to single session.
  const NetSessionDescription &operator [] ( int i ) const
  { return _data[i]; }

};

//---------------------------------------------------------------------------
//  NetSessionEnum:

/// Net implementation of NetTranspSessionEnum class
class NetSessionEnum : public NetTranspSessionEnum
{

protected:

  /// List of enumerated sessions.
  NetSessionDescriptions _sessions;

  /// Critical section for _sessions.
  PoCriticalSection _cs;

  /// Enter the critical section.
  inline void enter ()
  { _cs.enter(); }

  /// Leave the critical section.
  inline void leave ()
  { _cs.leave(); }

  /// Enumeration is running.
  bool _running;

  /// Magic number for application separation.
  int32 magicApp;

  /**
    Sends batch of requests to the given host address.
    @param  br Existing (and well-configured) broadcast channel.
    @param  addr IP address of the host (network endian).
    @param  port First port number to try (host endian).
  */
  void sendRequest ( NetChannel *br, struct sockaddr_in &addr, unsigned16 port );

  /**
    Determines if the server needs to be requested (for enumeration retries).
    @param  addr IP address of the host (network endian).
    @param  port First port number to try (host endian).
    @return <code>true</code> if the address needs to be requested again.
  */
  bool needsRequest ( struct sockaddr_in &addr, unsigned16 port );


  friend NetStatus enumReceive ( NetMessage *msg, NetStatus event, void *data );

public:

  /// Constructor
  NetSessionEnum ();

  /// Destructor
  virtual ~NetSessionEnum ();

  /// Initializes enumeration data.
  virtual bool Init ( int magic =0 ) override;

  /// Releases enumeration data.
  virtual void Done ();

  virtual RString IPToGUID ( RString ip, int port ) override;

  /// Session enumeration is running?
  virtual bool RunningEnumHosts () override
  { return _running; }

  /**
    Starts enumeration of remote sessions.
    @param  ip Distant IP address to try (or NULL for broadcast only).
    @param  port Distant port to try.
    @param  hosts Array of distant hosts to try (or NULL if none).
  */
  virtual bool StartEnumHosts ( RString ip, int port, AutoArray<RemoteHostAddress> *hosts ) override;

  /// Cancel session enumeration.
  virtual void StopEnumHosts () override;

  virtual void ClearEnumHosts () override {Done();}

  /// Number of actually enumerated sessions.
  virtual int NSessions () override;

  /// Retrieve all enumerated sessions.
  virtual void GetSessions ( AutoArray<SessionInfo> &sessions ) override;

};

//---------------------------------------------------------------------------
//  NetClient:

static NetPeer *getClientPeer ();
static PoCriticalSection LockDecl(poolLock,"::poolLock"); ///< locking of 'pool', 'clientPeer', 'serverPeer'

/// Interface for network transport client class.
class NetClient : public NetTranspClient
{

protected:

  /// Last reported message time in seconds.
  unsigned64 lastMsgReported;

  /// Associated communication channel (peer-to-peer point).
  Ref<NetChannel> channel;

  // ChallengePlayer result (0 if not finished yet)
  int challengePlayer;
  /// AckPlayer result (CRNone if not finished yet).
  int ackPlayer;

  /// PlayerNo (playerID on the server - for reconnection purposes).
  int playerNo;

  /// Am I bot-client?
  bool amIBot;

  /// The session was terminated (by the server or channel drop)..
  bool sessionTerminated;

  /// The session was terminated (by the server or channel drop)..
  NetTerminationReason whySessionTerminated;

  /// The session was terminated by some specific reason
  char whySessionTerminatedStr[512];

  /// Linked list of received messages (can be NULL). Sorted by serial number.
  Ref<NetMessage> received;

  /// Inserts a new received message into 'received' sorted list..
  void insertReceived ( NetMessage *msg );

  /// Linked list of received message-parts to be merged.
  Ref<NetMessage> split;

  /// -> last received split message-part.
  NetMessage *lastSplit;

  /// Linked list of received message-parts to be merged.
  Ref<NetMessage> splitUrgent;

  /// -> last received split message-part.
  NetMessage *lastSplitUrgent;

  /// Critical section for received.
  PoCriticalSection rcvCs;

  /// Enter the critical section for received messages.
  inline void enterRcv ()
  { rcvCs.enter(); }

  /// Leave the critical section for received messages.
  inline void leaveRcv ()
  { rcvCs.leave(); }

  /// Linked list of sent messages (can be NULL).
  Ref<NetMessage> sent;

  /// Critical section for sent.
  mutable PoCriticalSection sndCs;

  /// Enter the critical section for sent messages.
  inline void enterSnd () const
  { sndCs.enter(); }

  /// Leave the critical section for sent messages.
  inline void leaveSnd () const
  { sndCs.leave(); }

  // inside NetClient::Init we need to lock both enterSnd and enterPeer in order to prevent possible DeadLock
  inline void enterAll() const
  {
    poolLock.enter();
    NetPeer *peer = GET_CLIENT_PEER();
    if (peer) peer->enterPeer();
    enterSnd();
  }
  inline void leaveAll() const
  {
    leaveSnd();
    NetPeer *peer = GET_CLIENT_PEER();
    if (peer) peer->leavePeer();
    poolLock.leave();
  }

  /// Magic number for application separation.
  int32 magicApp;

  friend NetStatus clientReceive ( NetMessage *msg, NetStatus event, void *data );
  friend NetStatus enumReceive ( NetMessage *msg, NetStatus event, void *data );
  friend NetStatus clientSendComplete ( NetMessage *msg, NetStatus event, void *data );

#ifdef NET_LOG_INFO
  int oldLatency;
  int oldThroughput;
  bool printAge;
#endif

#ifdef NET_LOG_TRANSP_STAT
  unsigned64 nextStatLog;
#endif

  bool SendMagicPacket(const void *data, size_t size);
  
public:

  /// Constructor. Does virtually no initialization.
  NetClient ();

  /// Destructor.
  virtual ~NetClient ();

  /**
    Initialize the Client object.
    @param  address IP address:port of the distant server ("server.domain.tld:port").
    @param  password Password used to join the session.
    @param  botClient Is this "bot-client" (default client)?
    @param  port Remote port number! The correct used port number will be returned.
    @param  player Player name.
    @param  versionInfo Version info of the remote session.
    @param  magic Magic number to separate different applications.
  */
  virtual ConnectResult Init (
    RString address, RString password, bool botClient, int &port,
    RString player, MPVersionInfo &versionInfo, int magic =0, CancelNNCallback *cancelNNCallback=NULL
  );

  /// Reconnects the client after IP:port change (re-dial).
  virtual ConnectResult ReInit ();

  /// Create and initialize VoN voice client.
  virtual bool InitVoice ( VoiceMask *voiceMask, RawMessageCallback callback, bool isBotClient);
  /// Return true, if given remote player is speaking and voice is audible on local computer
  virtual bool IsVoicePlaying ( int player );
  
  virtual int CheckVoiceChannel(int dpnid, bool &audible2D, bool &audible3D) const;
  /// Return true, if player on local computer is speaking and voice is recording for Voice Over Net
  virtual bool IsVoiceRecording ();
  
  virtual void IsVoice2D3DPlaying(int dpnid, bool &audible2D, bool &audible3D) const;
#if _ENABLE_CHEATS
  /// VoNSay script command to simulate more speakers without many players necessity
  virtual void VoNSay(int dpnid, int channel, int frequency, int seconds);
#endif
  /// Switches on/off the recorder (microphone).
  virtual void VoiceCapture ( bool on, int quality );
  /// Switches on/off the voice transmition
  virtual void SetVoiceTransmition(bool val);
  /// Sets if voice is toggled on (user is NOT using push to talk button)
  virtual void SetVoiceToggleOn(bool val);
  /// sets threshold for silence analyzer (0.0f - 1.0f)
  virtual void SetVoNRecThreshold(float val);
  /// sets the frequency current CCItemTransmitter operates on
  virtual void SetVoNTransmitterFreq(float f);
  /// frequency current CCItemTransmitter operates on
  virtual float GetVoNTransmitterFreq() const;
  // sets the maximal transmittion range in meters
  virtual void SetVoNTransmitterRange(float range);
  virtual float GetVoNTransmitterRange2() const;
  /// Sets position of remote player (affects sources)
  virtual void SetVoNPosition(int dpnid, Vector3Par pos, Vector3Par speed);
  /// Sets position of player (affects listener)
  virtual void SetVoNPlayerPosition(Vector3Par pos, Vector3Par speed);
  /// Sets obstruction of player (affects source)
  virtual void SetVoNObstruction(int dpnid, float obstruction, float occlusion, float deltaT);
  /// Updates volume and ear accomodation of the player (affects source)
  virtual void UpdateVoNVolumeAndAccomodation(int dpnid, float volume, float accomodation);
  /// Advance all VoN voices (even muted ones)
  virtual void AdvanceAllVoNSounds(float deltaT, bool paused, bool quiet);

  /// Sends the given user (raw) message to server.
  virtual Ref<NetMessage> SendMsg ( BYTE *buffer, int bufferSize, DWORD &msgID, NetMsgFlags flags, const Ref<NetMessage> &dependOn );

  /// Gets actual send-queue statistics.
  virtual void GetSendQueueInfo ( int &nMsg, int &nBytes, int &nMsgG, int &nBytesG );

  /// Gets actual I/O channel statistics.
  virtual bool GetConnectionInfo ( int &latencyMS, int &throughputBPS );
  virtual void GetConnectionLimits(int &maxBandwidthPerClient);
  virtual void SetConnectionLimits(int maxBandwidthPerClient);

  /// Sets internal params for underlying NetChannel object.
  virtual void SetNetworkParams ( ParamEntryPar cfg );

  /// get local (my) address used for communication
  virtual bool GetLocalAddress( in_addr &addr, int &port ) const;

  /// get local (my) IP address
  virtual bool GetLocalAddress( in_addr &addr ) const;

  /// get distant address (address of the server)
  virtual bool GetDistantAddress(in_addr &addr, int &port) const;

  /// get distant address along with port (of the server)
  virtual bool GetServerAddress( sockaddr_in &address ) const;

  /// get port where peer to peer channel is bind to
  virtual int GetVoicePort() const;

  /// get socket used for peer to peer communication
  virtual SOCKET GetVoiceSocket() const;

  /// Last received message's age in seconds.
  virtual float GetLastMsgAge ();

  /// Last received message's age in seconds (used to eliminate "disconnect cheat")
  virtual float GetLastMsgAgeReliable () { return GetLastMsgAge(); }

  /// Last reported message's age in seconds.
  virtual float GetLastMsgAgeReported ();

  /// Remember time of reported message.
  virtual void LastMsgAgeReported ();

  virtual bool IsSessionTerminated ();

  virtual NetTerminationReason GetWhySessionTerminated ();
  virtual RString GetWhySessionTerminatedStr ();

  /// Processes received user messages.
  virtual void ProcessUserMessages ( UserMessageClientCallback *callback, void *context );

  /// Removes all received user messages.
  virtual void RemoveUserMessages ();

  /// Processes sent (and acknowledged) user messages.
  virtual void ProcessSendComplete ( SendCompleteCallback *callback, void *context );

  /// Removes all sent messages.
  virtual void RemoveSendComplete ();

  virtual RString GetStatistics ();

  virtual unsigned FreeMemory ();

  // called when application ends
  virtual void sendDisconnectMsg();
};

/*!
\patch 1.81 Date 8/7/2002 by Ondra
- Fixed: MP: Ghost might be left after user disconnected.
*/

#if 1
/// Empty key value (internal).
template <> int ExplicitMapTraits<int,RefD<NetChannel> >::keyNull = -1;

/// For removed values (internal).
template <> int ExplicitMapTraits<int,RefD<NetChannel> >::zombie  = -2;

#endif

//---------------------------------------------------------------------------
//  NetServer:

class ChannelSupport
{
public:

  /// Player (channel) id.
  int m_id;

  /// Linked list of received message-parts to be merged.
  Ref<NetMessage> m_split;

  /// -> last received split message-part.
  NetMessage *m_lastSplit;

  /// Linked list of received urgent message-parts to be merged.
  Ref<NetMessage> m_splitUrgent;

  /// -> last received urgent split message-part.
  NetMessage *m_lastSplitUrgent;

  ChannelSupport ()
  {
    m_id = 0;
    m_lastSplit = m_lastSplitUrgent = NULL;
  }

  ChannelSupport ( const ChannelSupport &from )
  {
    m_id              = from.m_id;
    m_split           = from.m_split;
    m_lastSplit       = from.m_lastSplit;
    m_splitUrgent     = from.m_splitUrgent;
    m_lastSplitUrgent = from.m_lastSplitUrgent;
  }

  bool operator== ( const ChannelSupport &sec ) const
  { return( m_id == sec.m_id ); }

};

TypeIsMovable(ChannelSupport);

template <> unsigned64 ExplicitMapTraits<unsigned64,ChannelSupport>::keyNull = (unsigned64)-1;
template <> unsigned64 ExplicitMapTraits<unsigned64,ChannelSupport>::zombie  = (unsigned64)-2;
// ExplicitMapTraits<unsigned64,ChannelSupport>::null should be initialized by = because of problems on linux GCC
template <> ChannelSupport ExplicitMapTraits<unsigned64,ChannelSupport>::null = ChannelSupport();

void SessionPacket::SetMissionName(const LocalizedString &str)
{
  missionLocalizedType = ' '+str._type;
  strcpy(missionId,str._id);

}

RString SessionPacket::GetLocalizedMissionName() const
{
  int type = missionLocalizedType-' ';
  saturate(type,LocalizedString::PlainText,LocalizedString::Stringtable);
  LocalizedString str(LocalizedString::Type(type),missionId);
  return str.GetLocalizedValue();
}

/// Socket implementation for network transport server class.
class NetServer : public NetTranspServer
{

protected:

  /// Session password
  RString _password;

  /// Session info in network format.
  SessionPacket session;

  /// Critical section for user map, session info, added & deleted users.
  PoCriticalSection usrCs;

  /// Enter the critical section for user map.
  inline void enterUsr ()
  { usrCs.enter(); }

  /// Leave the critical section for user map.
  inline void leaveUsr ()
  { usrCs.leave(); }

  /// Port on which the session is running.
  unsigned16 sessionPort;

  /// Name of session (contains name of player and IP address of host).
  RString sessionName;

  /// Linked list of received messages (can be NULL). Sorted by serial number.
  Ref<NetMessage> received;

  /// Inserts a new received message into 'received' sorted list..
  void insertReceived ( NetMessage *msg );

  /// Support for actual players (split messages, etc.).
  ExplicitMap<unsigned64,ChannelSupport,true,MemAllocSafe> m_support;

  /// Critical section for received.
  PoCriticalSection rcvCs;

  /// Enter the critical section for received messages.
  inline void enterRcv ()
  { rcvCs.enter(); }

  /// Leave the critical section for received messages.
  inline void leaveRcv ()
  { rcvCs.leave(); }

  /// Linked list of sent messages (can be NULL).
  Ref<NetMessage> sent;

  /// Critical section for sent.
  PoCriticalSection sndCs;

  /// Enter the critical section for sent messages.
  inline void enterSnd ()
  { sndCs.enter(); }

  /// Leave the critical section for sent messages.
  inline void leaveSnd ()
  { sndCs.leave(); }

  /// Mapping from user ID# to NetChannels.
  ExplicitMap<int,RefD<NetChannel>,true,MemAllocSafe> users;

  /// ID of 'bot' client or 0.
  int botId;

  /// Info about new players.
  AutoArray<CreatePlayerInfo,MemAllocSafe> _createPlayers;

  /// Info about removed players.
  AutoArray<DeletePlayerInfo,MemAllocSafe> _deletePlayers;

  /// Magic number for application separation.
  int32 magicApp;

  /// Respond to enumeration request?
  bool m_enumResponse;

  /// player creatiion challenges sent by a server
  struct PlayerChallengeSent
  {
    sockaddr_in addr;
    int challenge;
    unsigned64 time;

    ClassIsSimple(PlayerChallengeSent);
  };

  struct PlayerChallengeSentKeyTraits
  {
    typedef const sockaddr_in &KeyType;
    /// check if two keys are equal
    static bool IsEqual(KeyType a, KeyType b) {return a.sin_port==b.sin_port && ADDR(a)==ADDR(b);}
    /// get a key from an item
    static KeyType GetKey(const PlayerChallengeSent &a) {return a.addr;}
  };

  typedef FindArrayKey<PlayerChallengeSent,PlayerChallengeSentKeyTraits> PlayerChallengeSentList;
  PlayerChallengeSentList _challengesSent;
  PlayerChallengeSentList _oldCreateAttempts;

  static void Expire(PlayerChallengeSentList &list, unsigned64 timeout);

  /**
    Maps NetChannel to player ID#.
    @return Player # or <code>-1</code> if not found.
  */
  int channelToPlayer ( NetChannel *ch );

  friend NetStatus ctrlReceive ( NetMessage *msg, NetStatus event, void *data );
  friend NetStatus serverReceive ( NetMessage *msg, NetStatus event, void *data );
  friend NetStatus destroyPlayerCallback ( NetMessage *msg, NetStatus event, void *data );
  friend NetStatus serverSendComplete ( NetMessage *msg, NetStatus event, void *data );

  /// Complete player-destroy sequence (with notification message to the player).
  void destroyPlayer ( NetChannel *ch, NetTerminationReason reason, const char *reasonStr=NULL );

  /// Final player-destroying (w/o notification message to the player).
  void finishDestroyPlayer ( int player );

  /// diagnostics - log current state of "users" variable
  void logUsers();

#ifdef NET_LOG_INFO
  int oldLatency;
  int oldThroughput;
#endif

#ifdef DEDICATED_STAT_LOG
  unsigned64 nextConsoleLog;
#endif

#ifdef NET_LOG_TRANSP_STAT
  unsigned64 nextStatLog;
  bool forceLog;
  bool inGetConnection;
#endif

public:

  /// Constructor. Does virtually no initialization.
  NetServer ();

  /// Destructor.
  virtual ~NetServer ();

  /**
    Initialize the server object.
    @param  port Local port to work with.
  */
  virtual bool Init ( RString name, RString password, int maxPlayers, int port,
                      int language, int difficulty,
                      MPVersionInfo &versionInfo, bool equalModRequired, int magic =0 );
  void UpdateMaxPlayers(int maxPlayers);
  void UpdateLockedOrPassworded(bool lock, bool passworded);

  /// Create and initialize VoN voice server.
  virtual bool InitVoice ();
  virtual void ProcessVoicePlayers ( CreateVoicePlayerCallback *callback, void *context );
  virtual void RemoveVoicePlayers ();
  virtual void GetTransmitTargets ( int from, AutoArray<int,MemAllocSA> &to );
  virtual void SetTransmitTargets ( int from, AutoArray<int,MemAllocSA> &to );
  virtual void UpdateTransmitTargets();
  virtual void GetVoNTopologyDiag(QOStrStream &out, Array<ConvPair> &convTable);

  virtual int GetSessionPort ()
  { return sessionPort; }

  virtual RString GetSessionName ()
  { return sessionName; }

  virtual Ref<NetMessage> SendMsg ( int to, BYTE *buffer, int bufferSize, DWORD &msgID, NetMsgFlags flags, const Ref<NetMessage> &dependOn );

  virtual void CancelAllMessages ();

  /// called when application ends
  virtual void disconnectAllPlayers();

  virtual void GetSendQueueInfo ( int to, int &nMsg, int &nBytes, int &nMsgG, int &nBytesG );

  virtual bool GetConnectionInfo ( int to, int &latencyMS, int &throughputBPS );
  virtual void GetConnectionLimits(int &maxBandwidthPerClient);
  virtual void SetConnectionLimits(int maxBandwidthPerClient);

  /// Last received message's age in seconds (used to eliminate "disconnect cheat")
  virtual float GetLastMsgAgeReliable ( int player );

  /// Sets internal params for underlying NetChannel object.
  virtual void SetNetworkParams ( ParamEntryPar cfg );

  virtual void UpdateSessionDescription (
    int state, RString gameType, const LocalizedString &mission, RString island,
    int timeleft, int difficulty
  );

  virtual bool GetURL ( char *address, DWORD addressLen );

  virtual bool GetServerAddress(sockaddr_in &address);
  virtual bool GetClientAddress(int client, sockaddr_in &address);

  virtual void KickOff ( int player, NetTerminationReason reason, const char *reasonStr=NULL );

  virtual void ProcessUserMessages ( UserMessageServerCallback *callback, void *context );

  virtual void RemoveUserMessages ();

  virtual void ProcessSendComplete ( SendCompleteCallback *callback, void *context );

  virtual void RemoveSendComplete ();

  virtual void ProcessPlayers ( CreatePlayerCallback *callbackCreate, DeletePlayerCallback *callbackDelete, void *context );

  virtual void RemovePlayers ();

  virtual RString GetStatistics ( int player );

  virtual void NotifyChannelDataSent(int player, size_t size);

  virtual unsigned FreeMemory ();

  virtual RefD<NetChannel> playerToChannel ( int player );

};

//---------------------------------------------------------------------------
//  Global pools and peers etc.:

NetStatus enumReceive ( NetMessage *msg, NetStatus event, void *data );
NetStatus ctrlReceive ( NetMessage *msg, NetStatus event, void *data );
NetStatus clientReceive ( NetMessage *msg, NetStatus event, void *data );
NetStatus serverReceive ( NetMessage *msg, NetStatus event, void *data );

static RefD<NetPool> pool;                  ///< Global network pool.

static RefD<NetPeer> clientPeer;            ///< NetPeer for enumerator and client.

static RefD<NetPeer> serverPeer;            ///< NetPeer for server.

#if VOICE_OVER_NET
#if _GAMES_FOR_WINDOWS

static RefD<VoNSystemXbox> von;             ///< Global VoN factory.

DWORD commPort = (DWORD)-1;                 ///< Actual communicator's port or -1 if communicator is not present.

#else

static RefD<VoNSystem> von;               ///< Global VoN factory.

#endif
#endif

void initNetwork()
{
  initNetworkUDP();
}

void doneNetwork()
{
  doneNetworkUDP();
}

void createPool ()
{
  if ( !pool )
  {
    pool = new NetPool( new PeerChannelFactoryUDP, NULL, NULL );
  }
}

void destroyPool ()
{
  poolLock.enter();
  if ( clientPeer )
  {
    if ( pool )
      pool->deletePeer(clientPeer);
    else
      clientPeer->close();
    clientPeer = NULL;
  }
  if ( serverPeer )
  {
    if ( pool )
      pool->deletePeer(serverPeer);
    else
      serverPeer->close();
    serverPeer = NULL;
  }
#if VOICE_OVER_NET
  if ( von )
  {
#ifdef NET_LOG_VOICE
    NetLog("destroyPool: destroying VoNSystem instance");
#endif
    von = NULL;
  }
#endif
  pool = NULL;
  poolLock.leave();
#ifdef NET_LOG
  netLogFlush();
#endif
}

NetPeerToPeerChannel *VoiceCreatePeerToPeer()
{
  PcPeerToPeerChannel *ch = new PcPeerToPeerChannel();
  if (!ch->Init(GetNetworkPort() + 3))
  {
    // channel init failed
    RptF("Peer to peer channel init failed");
    delete ch;
    return NULL;
  }
  return ch;
}

static void setupPortBitMask ( BitMask &mask, bool server )
{
  mask.empty();
  int port = server ? GetServerPort() : GetNetworkPort() + 2;
  int i;
#if _ENABLE_DEDICATED_SERVER
  if ( server && GetPidFileName().GetLength() )
    mask.on(port);
  else
#endif
  for ( i = 0; i++ < NN_NUM_PORTS_TO_TRY; port += PORT_INTERVAL )
    mask.on(port);
}


#if _ENABLE_GAMESPY
static bool QRInitialized = false;
void SetQRInitialized(bool set)
{
  QRInitialized = set;
}

bool CheckRawMessage(char *data, int len, struct sockaddr_in *from)
{
  if (len >= NATNEG_MAGIC_LEN && memcmp(data, NNMagicData, NATNEG_MAGIC_LEN) == 0)
  {
#if !_SUPER_RELEASE
    int port = ntohs(from->sin_port);
    LogF("*** NAT Message (game) arrived from %s:%d, length %d bytes", inet_ntoa(from->sin_addr), port, len);
#endif
    // NAT Negotiation message - pass to GameSpy SDK
    enterNN();
    NNProcessData(data, len, from);
    leaveNN();
    return true; // processed
  }
  else if (QRInitialized && len >= 2 && (unsigned char)(data[0]) == QR_MAGIC_1 && (unsigned char)(data[1]) == QR_MAGIC_2)
  {
/*
    int port = from->sin_port;
    LogF("*** QR2 Message arrived from %s:%d, length %d bytes", inet_ntoa(from->sin_addr), port, len);
*/
    // Query and Reporting message - pass to GameSpy SDK
    qr2_parse_query(NULL, data, len, reinterpret_cast<sockaddr *>(from));
    return true;
  }
  else if (QRInitialized && len >= 1 && (unsigned char)(data[0]) == 0x3B)
  {
    /*
    int port = from->sin_port;
    LogF("*** CD Key Message arrived from %s:%d, length %d bytes", inet_ntoa(from->sin_addr), port, len);
    */
    // CD Key validation message - pass to GameSpy SDK
    qr2_parse_query(NULL, data, len, reinterpret_cast<sockaddr *>(from));
    return true;
  }

  if (von)
  {
    RefD<VoNClient> client = von->getClient();
    if (client && client->PeerToPeerReceive(data, len, from, true))
    {
      return true;
    }
  }
  return false;
}
#else
bool CheckRawMessage(char *data, int len, struct sockaddr_in *from)
{
  if (von)
  {
    RefD<VoNClient> client = von->getClient();
    if (client && client->PeerToPeerReceive(data, len, from, true))
    {
      return true;
    }
  }
  return false;
}
#endif

static NetPeer *getClientPeer ()
  // should be called inside of poolLock.enter()
{
  if ( !clientPeer )
  {
    BitMask portMask;
    setupPortBitMask(portMask,false);
    clientPeer = getPool()->createPeer(&portMask, false, &CheckRawMessage);
    if ( clientPeer )
    {
      NetChannel *ctrl = clientPeer->getBroadcastChannel();
      if ( ctrl ) ctrl->setProcessRoutine(enumReceive);
    }
  }
  return clientPeer;
}

static NetPeer *getServerPeer (bool create=true)
    // should be called inside of poolLock.enter()
{
  if ( create && !serverPeer )
  {
    BitMask portMask;
    setupPortBitMask(portMask,true);
    serverPeer = getPool()->createPeer(&portMask, false, &CheckRawMessage);
    if ( serverPeer )
    {
      NetChannel *ctrl = serverPeer->getBroadcastChannel();
      if ( ctrl ) ctrl->setProcessRoutine(ctrlReceive);
    }
  }
  return serverPeer;
}

#ifdef NET_LOG_CTRL_RECEIVE
static unsigned GetServerPeerId()
{
  NetPeer *peer = GET_SERVER_PEER();
  return peer ? peer->getPeerId() : 0;
}
#endif 

//---------------------------------------------------------------------------
//  Global enum, server & client instances (NetTransp* objects):

/// Actual enumeration instance (can be NULL).
static NetSessionEnum *_enum = NULL;

/// Actual client instance (can be NULL).
static NetClient *_client = NULL;

/// NetServer instance to process control messages.
static NetServer *_server = NULL;

/*!
\patch 5143 Date 3/21/2007 by Bebul
- Fixed: Possible server crash during NAT negotiation
*/
/// NatNegotiation critical section
static PoCriticalSection _natCriticalSection;

void enterNN() { _natCriticalSection.Lock(); }
void leaveNN() { _natCriticalSection.Unlock(); }

//@{ Helper routines to end the application networking layer properly (no crash + send necessary stuff)
void sendDisconnectMessages()
{
  if ( _server )
  {
    _server->disconnectAllPlayers();
  }
  if ( _client )
  {
    _client->sendDisconnectMsg();
  }
}

void stopUdpListenSend()
{
  poolLock.enter();
  if ( clientPeer )
    clientPeer->stopThreads();
  if ( serverPeer )
    serverPeer->stopThreads();
  clientPeer.Free();
  serverPeer.Free();
  poolLock.leave();
}
//@}

//---------------------------------------------------------------------------
//  Receiving routines:

/*!
\patch_internal 1.51 Date 4/17/2002 by Pepca
- Fixed: Bugs in player registration at the server (multiple registration can occur). [Sockets]
- Fixed: Disconnected user is now accepted by the server immediately when [s]he makes another try.
[Sockets]
\patch_internal 1.51 Date 4/18/2002 by Pepca
- Added: Each player has its "unique identifier" based on current system time (and verified in
initial player<->server negotiation). [Sockets]
\patch_internal 1.51 Date 4/18/2002 by Pepca
- Fixed: Player-count limit is now interpreted correctly by the server (it was ignored in previous
versions). [Sockets]
*/

/// Control (broadcast) data received by the NetServer...
#if _VBS3
#include <Es\Algorithms\crc32.h>

// create packet and send it off
void SendFileInfo( unsigned8* ptr, int size, struct sockaddr_in &distant )
{ 
  NetChannel *br = GET_ENUM_PEER()->getBroadcastChannel();
  if(br)
  {
    Ref<NetMessage> outAAR = NetMessagePool::pool()->newMessage(size,br);
    if(outAAR)
    {
      outAAR->setDistant(distant);
      outAAR->setFlags(MSG_ALL_FLAGS,MSG_TO_BCAST_FLAG|MSG_MAGIC_FLAG);
      outAAR->setData(ptr,size);
      outAAR->send(true);
    }
  }
}
#endif


void NetServer::Expire(PlayerChallengeSentList &list, unsigned64 timeout)
{
  struct PlayerChallengeSentExpire
  {
    unsigned64 timeout;
    unsigned64 now;
    PlayerChallengeSentExpire(unsigned64 timeout):timeout(timeout),now(getSystemTime()){}

    bool operator () (const PlayerChallengeSent &item) const {return now-item.time>timeout;}
  };

  list.ForEachWithDelete(PlayerChallengeSentExpire(timeout));
}

NetStatus ctrlReceive ( NetMessage *msg, NetStatus event, void *data )
{
        // thread-critical: NetMessagePool::pool()->newMessage(), NetMessage::send(), strncmp(),
        //                  GET_SERVER_PEER()->getPool()->createChannel(), AutoArray::Add()
    if ( !_server || !msg || msg->getLength() < 4 ||
         !(msg->getFlags() & MSG_MAGIC_FLAG) )
        return nsNoMoreCallbacks;           // fatal message error

    unsigned32 magic = *(unsigned32*)msg->getData();
    struct sockaddr_in distant;
    msg->getDistant(distant);

#ifdef NET_LOG_CTRL_RECEIVE
#  ifdef NET_LOG_BRIEF
    NetLog("Pe(%u):rCtrlB(%u.%u.%u.%u:%u,%u,%x)",
           GetServerPeerId(),
           (unsigned)IP4(distant),(unsigned)IP3(distant),(unsigned)IP2(distant),(unsigned)IP1(distant),(unsigned)PORT(distant),
           msg->getLength(),magic);
#  else
    NetLog("Peer(%u)::ctrlReceive: received control message (from=%u.%u.%u.%u:%u, len=%3u, serial=%4u, flags=%04x, magic=%x)",
           GetServerPeerId(),
           (unsigned)IP4(distant),(unsigned)IP3(distant),(unsigned)IP2(distant),(unsigned)IP1(distant),(unsigned)PORT(distant),
           msg->getLength(),msg->getSerial(),(unsigned)msg->getFlags(),magic);
#  endif
#endif

    switch ( magic ) {

        case MAGIC_ENUM_REQUEST:            // enumeration request
            if ( _server->m_enumResponse &&
                 msg->getLength() == sizeof(EnumPacket) ) {
                EnumPacket *ep = (EnumPacket*)msg->getData();
                _server->enterUsr();
                if ( ep->magicApplication == _server->magicApp ) {
                    Ref<NetMessage> out = NetMessagePool::pool()->newMessage(SESSION_PACKET_SIZE,msg->getChannel());
                    if ( out )
                    {
                        out->setDistant(distant);
                        out->setFlags(MSG_ALL_FLAGS,MSG_TO_BCAST_FLAG|MSG_MAGIC_FLAG);
                        _server->session.magic      = MAGIC_ENUM_RESPONSE;
                        _server->session.request    = msg->getSerial();
                        _server->session.numPlayers = _server->users.card();
                        // bot client is always included to maxPlayers and excluded to numPlayers
#if _ENABLE_DEDICATED_SERVER
                        if (IsDedicatedServer())
                        {
                          // exclude the bot client
                          _server->session.maxPlayers--;                
                        }
                        else
#endif
                        {
                          // include the bot client
                          _server->session.numPlayers++;                
                        }

                        // sends the data off, must update it some place else!
                        out->setData((unsigned8*)&(_server->session),SESSION_PACKET_SIZE);
                        out->send(true);    // urgent message

#if _VBS3 && _EXT_CTRL
                        if(!GAAR.IsReplayMode() && (GNetworkManager.IsServer() && IsDedicatedServer()))
                        {
                          // forward declaration
                          bool getLocalAddress( struct sockaddr_in &me, unsigned16 port );

                          struct sockaddr_in me;
                          getLocalAddress(me,0);

                          // do not reply to my own broadcast
                          if( me.sin_addr.S_un.S_addr != distant.sin_addr.S_un.S_addr )
                          {                        
                            // write out server information packet
                            ServerInfo serverInfo;

                            serverInfo.magic = MAGIC_SERVERNAME;
                            strncpy(serverInfo.serverName,_server->sessionName,MAXSERVERNAME-1);
                            SendFileInfo((unsigned8*) &serverInfo,sizeof(serverInfo),distant);                        


                            HANDLE hFind = INVALID_HANDLE_VALUE;
                            WIN32_FIND_DATA FindFileData;

                            unsigned32 magic = MAGIC_FILELIST_RESPONSE;
                            char  path[MAX_PATH] = {0};
                            char *AARSearch      = "AAR\\*.aar";

                            const char* userDir = GMachineSessions.GetUserDir();
                            strncpy(path,userDir,strlen(userDir));
                            strncpy(&path[strlen(userDir)],AARSearch,strlen(AARSearch));

                            hFind = FindFirstFile(path, &FindFileData);
                            if (hFind != INVALID_HANDLE_VALUE)
                            {
                              AARNetFileInfo fileInfo;

                              fileInfo.magic = magic;
                              fileInfo.size  = FindFileData.nFileSizeLow;
                              strncpy(fileInfo.fileName,FindFileData.cFileName,MAXFILENAME-1);

                              SendFileInfo((unsigned8*) &fileInfo,sizeof(fileInfo),distant);

                              while (FindNextFile(hFind, &FindFileData) != 0) 
                              {
                                ZeroMemory((char*) &fileInfo,sizeof(fileInfo));

                                fileInfo.magic = magic;
                                fileInfo.size  = FindFileData.nFileSizeLow;
                                strncpy(fileInfo.fileName,FindFileData.cFileName,MAXFILENAME-1);

                                SendFileInfo((unsigned8*) &fileInfo,sizeof(fileInfo),distant);
                              }

                              FindClose(hFind);
                            }
                          }
                        }
#endif


#if _ENABLE_DEDICATED_SERVER
                        if (IsDedicatedServer())
                        {
                          // reset maxPlayers (is persistent)
                          _server->session.maxPlayers++;                
                        }
#endif
                    }
#ifdef ENUM_LEGACY
                    out = NetMessagePool::pool()->newMessage(SESSION_PACKET_1,msg->getChannel());
                    if ( out ) {
                        out->setDistant(distant);
                        out->setFlags(MSG_ALL_FLAGS,MSG_TO_BCAST_FLAG|MSG_MAGIC_FLAG);
                        _server->session.magic = MAGIC_ENUM_RESPONSE_LEGACY;
                        out->setData((unsigned8*)&(_server->session),SESSION_PACKET_1);
                        out->send(true);    // urgent message
                        }
#endif
                    }
                _server->leaveUsr();
                }
            break;
        case MAGIC_REQUEST_PLAYER:
          if ( _server->m_enumResponse && msg->getLength() == sizeof(RequestPlayerPacket) ) {
            RequestPlayerPacket *rqp = (RequestPlayerPacket*)msg->getData();
            if ( rqp->magicApplication != _server->magicApp )
            {
              // malformed request, do not respond
              break;
            }

            _server->enterUsr();        // lock the user, session etc. structures for a long time..
            // if the challenge already exists, reuse it
            int wasSent = _server->_challengesSent.FindKey(distant);
            if (wasSent<0)
            {
              srand((unsigned int)time(NULL) ^ 0x55555555);
              /* Since RAND_MAX is 16 bit on many systems, make sure we get a 32 bit number */
              int challenge = (rand() << 16 | rand()); 
              NetServer::PlayerChallengeSent sent;
              sent.addr = distant;
              sent.challenge = challenge;
              wasSent = _server->_challengesSent.Add(sent);
            }

            unsigned64 now = getSystemTime();
            ChallengePlayerPacket chp;
            chp.magic = MAGIC_CHALLENGE_PLAYER;
            chp.challenge = _server->_challengesSent[wasSent].challenge;
            _server->_challengesSent[wasSent].time = now;

            _server->Expire(_server->_challengesSent, ACK_PLAYER_TIMEOUT);

            _server->leaveUsr();

            LogF("Server: challenge %x",chp.challenge);

            Ref<NetMessage> out = NetMessagePool::pool()->newMessage(sizeof(chp),msg->getChannel());
            if ( out )
            {
              out->setDistant(distant);
              out->setFlags(MSG_ALL_FLAGS,MSG_TO_BCAST_FLAG|MSG_MAGIC_FLAG);
              out->setData((unsigned8*)&chp,sizeof(chp));
              out->send(true);        // urgent message
            }

            // timeout old challenges
          }
          break;
          
        case MAGIC_CREATE_W_CHALLENGE:
            LogF("Server: create player %d",magic == MAGIC_CREATE_W_CHALLENGE);
            if ( _server->m_enumResponse && msg->getLength() == sizeof(CreatePlayerPacket) || msg->getLength() == sizeof(CreatePlayerPacketChallenge)) {
                CreatePlayerPacket *cpp = (CreatePlayerPacket*)msg->getData();
                _server->enterUsr();        // lock the user, session etc. structures for a long time..

                if (magic==MAGIC_CREATE_W_CHALLENGE && msg->getLength() == sizeof(CreatePlayerPacketChallenge))
                {
                    int challenge = ((CreatePlayerPacketChallenge *)cpp)->challenge;
                    int wasSent = _server->_challengesSent.FindKey(distant);
                    if (wasSent<0)
                    {
                      LogF("Server: Challenge %x not found",challenge);
                      _server->leaveUsr();
                      break;
                    }
                    else if (challenge!=_server->_challengesSent[wasSent].challenge)
                    {
                      LogF("Server: Challenge not matching %x!=%x",challenge,_server->_challengesSent[wasSent].challenge);
                      _server->leaveUsr();
                      break;
                    }
                    else
                    {
                      LogF("Server: MAGIC_CREATE_W_CHALLENGE accepted %x",challenge);
                      _server->_challengesSent.DeleteAt(wasSent);
                    }
                }
                else
                {
                  _server->leaveUsr();
                  break;
                }


                // avoid corrupted or modified packages (usage of reserved ids could cause crash, freeze etc.)
                if ( cpp->magicApplication != _server->magicApp || cpp->uniqueID - UNIQUE_TO_TRY - RESERVED_IDS < 0) {
                    _server->leaveUsr();
                    break;
                    }

                // any pending challenges or create attempts from this address are no longer relevant, remove them
                LogF("Server: old challenges and attempts cleared");
                _server->_challengesSent.DeleteAllKey(distant);
                _server->_oldCreateAttempts.DeleteAllKey(distant);

                ConnectResult result = CROK;
                    // check new player's attributes:

                bool resultSet = false;
                //! Actual multiplayer version
                if (cpp->actualBuild < GNetworkManager.RequiredBuildNo())
                {
                  resultSet = true;
                  result = CRVersion;
                }

                if ( !resultSet )
                if ( cpp->actualVersion < _server->session.requiredVersion ||
                     _server->session.actualVersion < cpp->requiredVersion ||
                     (_server->session.equalModRequired & 1) && stricmp(_server->session.mod, cpp->mod) != 0)
                    result = CRVersion;
                else
                    if ( strncmp(cpp->password,_server->_password,LEN_PASSWORD_NAME) != 0 )
                        result = CRPassword;
                    else
                        result = CROK;
                    // create a new player:
                int player = 0;             // dummy player ID
                int playerEnd;
                NetChannel *ch = NULL;
                if ( result == CROK ) {
                    poolLock.enter();
                    NetPeer *peer = GET_SERVER_PEER();
                    Assert( peer );
                    RefD<NetChannel> findCh;
                    ch = peer->findChannel(distant);
                    if ( ch ) {             // this channel is being used!
                        player = cpp->uniqueID;
                        playerEnd = player - UNIQUE_TO_TRY;
                        do
                            if ( _server->users.get(player,findCh) && (NetChannel*)findCh == ch ) break;
                        while ( --player != playerEnd );
                        if ( player != playerEnd ) { // found => refuse it!
                            poolLock.leave();
                            _server->leaveUsr();
                            break;          // duplicate CreatePlayer message => refuse it
                            }
                            // new connection from the same IP:port => kick off the old player
                        _server->finishDestroyPlayer(_server->channelToPlayer(ch));
                        ch = NULL;
                        poolLock.leave();
                        _server->leaveUsr();
                        SLEEP_MS(DESTROY_WAIT); // waiting till my boss destroys the old player (to be sure)..
                        _server->enterUsr();
                        poolLock.enter();
                        }
                        // creating a new player: first determine its ID..
                    player = cpp->uniqueID;
                    playerEnd = player + UNIQUE_TO_TRY;
                    while ( player != playerEnd &&
                            _server->users.get(player,findCh) ) player++;
                    bool isPrivate = false;
#ifdef NET_LOG_SERVER
#  ifdef NET_LOG_BRIEF
                    NetLog("Pe(%u):ask(%u.%u.%u.%u:%u,'%s',%d,%d,%d,%d)",
                           GetServerPeerId(),(unsigned)IP4(distant),(unsigned)IP3(distant),(unsigned)IP2(distant),(unsigned)IP1(distant),(unsigned)PORT(distant),
                           cpp->name,cpp->uniqueID,player,(int)_server->session.maxPlayers,(int)_server->users.card());
#  else
                    NetLog("Peer(%u): new player is asking for connection (from %u.%u.%u.%u:%u), name = '%s', sentID = %d, validID = %d, maxPlayers = %d, |players| = %d",
                           GetServerPeerId(),(unsigned)IP4(distant),(unsigned)IP3(distant),(unsigned)IP2(distant),(unsigned)IP1(distant),(unsigned)PORT(distant),
                           cpp->name,cpp->uniqueID,player,(int)_server->session.maxPlayers,(int)_server->users.card());
#  endif
#endif
                    int publicPlayers = _server->users.card() + 1; // bot client is connected directly, not contained in _server->users
                    if ( player == playerEnd ||
                         _server->session.maxPlayers > 0 &&
                         publicPlayers >= _server->session.maxPlayers )
                        result = CRSessionFull;
                    else {                  // OK <= new player (at least new FlashPoint run)
                        ch = peer->getPool()->createChannel(distant,peer);
                        if ( !ch ) {
                            result = CRError; // undetermined error
                            RptF("NetServer: createChannel() failed => cannot insert a new player");
                            }
                        else {              // NetChannel is OK
#ifdef NET_LOG_SERVER
#  ifdef NET_LOG_BRIEF
                            NetLog("Ch(%u):acc(%u.%u.%u.%u:%u,'%s',%d,%d)",
                                   ch->getChannelId(),(unsigned)IP4(distant),(unsigned)IP3(distant),(unsigned)IP2(distant),(unsigned)IP1(distant),(unsigned)PORT(distant),
                                   cpp->name,cpp->uniqueID,player);
#  else
                            NetLog("Channel(%u): new player was accepted (from %u.%u.%u.%u:%u), name = '%s', sentID = %d, validID = %d",
                                   ch->getChannelId(),(unsigned)IP4(distant),(unsigned)IP3(distant),(unsigned)IP2(distant),(unsigned)IP1(distant),(unsigned)PORT(distant),
                                   cpp->name,cpp->uniqueID,player);
#  endif
#endif
                            _server->users.put(player,ch);
                            ChannelSupport sup;
                            sup.m_id = player;
                            _server->m_support.put(sockaddrKey(distant),sup);
                            //_server->logUsers();
                            DoAssert(_server->users.checkIntegrity());
                            int index = _server->_createPlayers.Add();
                            CreatePlayerInfo &info = _server->_createPlayers[index];
                            info.player = player;

                            struct sockaddr_in inaddr;
                            msg->getDistant(inaddr);
                            info.inaddr = inaddr.sin_addr.s_addr; //S_un.S_addr;

                            if ( (info.botClient = ((cpp->botClient & 1) != 0)) )
                                _server->botId = player;
                            info.privateSlot = isPrivate;
                            strncpy(info.name,cpp->name,sizeof(info.name));
                            info.name[sizeof(info.name)-1] = (char)0;
                            if ( strcmp(cpp->name,info.name) )
                                RptF("NetServer: name of a new player is too long => truncating to '%S'",info.name);
                            LogF("NetServer: new player (waiting for ProcessPlayers) - session.numPlayers=%d, playerId=%d, bot=%d, name='%s', |users|=%u",
                                 _server->session.numPlayers,info.player,(int)info.botClient,info.name,_server->users.card());
                            strncpy(info.mod,cpp->mod,sizeof(info.mod));
                            info.mod[sizeof(info.mod)-1] = (char)0;
                            ch->setProcessRoutine(serverReceive);
#if VOICE_OVER_NET
                            if ( von.NotNull() && von->getServer().NotNull() )
                              von->getServer()->connect(player);
#endif
                            }
                        }
                    poolLock.leave();
                    }
                _server->leaveUsr();
                    // .. and finally send acknowledgement back to the new user
                AckPlayerPacket app;
                app.magic = MAGIC_ACK_PLAYER;
                app.result = result;
                app.playerNo = player;      // for future reconnections..
                  // ch == NULL in case of failure (but I'm going to send an answer anyway)
                Ref<NetMessage> out = NetMessagePool::pool()->newMessage(sizeof(AckPlayerPacket),ch ? ch : msg->getChannel());
                if ( out ) {
                    if ( !ch ) out->setDistant(distant);
                    out->setFlags(MSG_ALL_FLAGS,MSG_MAGIC_FLAG|(ch?MSG_VIM_FLAG:MSG_FROM_BCAST_FLAG));
                    out->setData((unsigned8*)&app,sizeof(app));
                    out->send(true);        // urgent message
                    }
                if ( ch )                   // initial hand-shake (to initialize band-width..).
                    ch->checkConnectivity(0);
                }
            break;

        case MAGIC_RECONNECT_PLAYER:        // reconnect player
            if ( _server->m_enumResponse && msg->getLength() == sizeof(ReconnectPlayerPacket) ) {
                ReconnectPlayerPacket *rpp = (ReconnectPlayerPacket*)msg->getData();
                _server->enterUsr();
                RefD<NetChannel> ch;
                ConnectResult result = CRError;
                if ( _server->users.get(rpp->playerNo,ch) &&  // player with this ID exists => reconnect it
                     ch->reconnect(distant) == nsOK ) result = CROK;
                _server->leaveUsr();
                    // .. send either acknowledgement back to the user
                AckPlayerPacket app;
                app.magic = MAGIC_ACK_PLAYER;
                app.result = result;
                app.playerNo = rpp->playerNo;
                Ref<NetMessage> out = NetMessagePool::pool()->newMessage(sizeof(AckPlayerPacket),(result==CROK) ? (NetChannel*)ch : msg->getChannel());
                if ( out ) {
                    if ( result != CROK ) out->setDistant(distant);
                    out->setFlags(MSG_ALL_FLAGS,MSG_MAGIC_FLAG|((result==CROK)?MSG_VIM_FLAG:MSG_FROM_BCAST_FLAG));
                    out->setData((unsigned8*)&app,sizeof(app));
                    out->send(true);        // urgent message
                    }
                if ( result == CROK )       // initial hand-shake (to re-initialize band-width..).
                    ch->checkConnectivity(0);
                }
            break;


        }

    return nsNoMoreCallbacks;
}

/// Control (broadcast) data received by the enumerator and client...
NetStatus enumReceive ( NetMessage *msg, NetStatus event, void *data )
{
        // thread-critical: sprintf(), AutoArray::Add(), strncpy()
    if ( (!_enum || !_enum->_running) && !_client ||
         !msg || msg->getLength() < 4 || !(msg->getFlags() & MSG_MAGIC_FLAG) )
        return nsNoMoreCallbacks;           // fatal message error

    unsigned32 magic = *(unsigned32*)msg->getData();
    struct sockaddr_in distant;
    msg->getDistant(distant);
    unsigned32 ip = ntohl(distant.sin_addr.s_addr);
    unsigned16 port = ntohs(distant.sin_port);

#ifdef NET_LOG_ENUM_RECEIVE
    NetLog("Peer(%u)::enumReceive: received control message (from=%u.%u.%u.%u:%u, len=%3u, serial=%4u, flags=%04x, magic=%x)",
           GetClientPeerId(),
           (unsigned)IP4(distant),(unsigned)IP3(distant),(unsigned)IP2(distant),(unsigned)IP1(distant),(unsigned)PORT(distant),
           msg->getLength(),msg->getSerial(),(unsigned)msg->getFlags(),magic);
#endif

    switch ( magic ) 
    {
#if _VBS3
      // messages no longer needed
      // replaced with TCP/IP
        case MAGIC_FILE_REQUEST:
        case MAGIC_FILE_RESPONSE:
          break;
        case MAGIC_SERVERNAME:
          {
            if(msg->getLength() == sizeof(ServerInfo))
            {
              int index = -1;

              GMachineSessions.Enter();

              for( int i = 0; i < GMachineSessions.GetSize(); ++i)
              {
                Session &session = GMachineSessions.Get(i);
                struct sockaddr_in &ip = session.GetDist();

                if(ip.sin_addr.S_un.S_addr == distant.sin_addr.S_un.S_addr)
                {
                  index = i;
                  break;
                }
              }

              if(index >= 0)
              {
                Session &session = GMachineSessions.Get(index);
                session.SetDist(distant);
                session.UpdateTick(::GlobalTickCount());

                ServerInfo *info = (ServerInfo*)msg->getData();;
                session.SetName(info->serverName);
              }
              else
              {
                bool found = false;
                for( int i = 0; i < GMachineSessions.GetSize();++i)
                {
                  // find one that is not being used
                  if(!GMachineSessions.Get(i).GetUsed())
                  {
                    Session &session = GMachineSessions.Get(i);
                    session.UpdateTick(::GlobalTickCount());
                    session.SetDist(distant);
                    session.SetUsed(true);

                    ServerInfo *info = (ServerInfo*)msg->getData();
                    info->serverName[MAXSERVERNAME-1] = '\0';
                    session.SetName(info->serverName);

                    found = true;
                    break;
                  }
                }
                if(!found)
                {
                  // no avaliable session space found, request adding a session
                  // will have to wait until it adds one itself
                  GMachineSessions.AddSession();
                }
              }            
            }

            GMachineSessions.Leave();
          }
          break;
       case MAGIC_FILELIST_RESPONSE:
          {       
            if(msg->getLength() == sizeof(AARNetFileInfo))
            {
              int index = -1;

              GMachineSessions.Enter();

              for( int i = 0; i < GMachineSessions.GetSize(); ++i)
              {
                Session &session = GMachineSessions.Get(i);
                struct sockaddr_in &ip = session.GetDist();

                if(ip.sin_addr.S_un.S_addr == distant.sin_addr.S_un.S_addr)
                {
                  index = i;
                  break;
                }
              }

              // we only going to add files to a machine index that
              // has allready been added.
              if(index >= 0)
              {
                AARNetFileInfo *newFile = (AARNetFileInfo*) msg->getData();
                // append null terminator to strings.
                newFile->fileName[MAXFILENAME-1] = '\0';

                Session &session = GMachineSessions.Get(index);
                // see if we can find a matching file under this same name.
                int sameFileNameIndex = -1;
                AutoArray<AARFileInfo> &fileList = session.GetFileList();
                for( int i = 0; i < fileList.Size(); ++i)
                {
                  if(strncmp(fileList[i].fileName,newFile->fileName,MAXFILENAME)==0)
                  {
                    sameFileNameIndex = i;
                    break;
                  }
                }         

                // add the new file into the list
                if(sameFileNameIndex == -1)
                {
                  // got to find a empty element. we do this with file size of zero;
                  int newFileIndex = -1;
                  for( int i = 0; i < fileList.Size(); ++i )
                  {
                    if(fileList[i].fileSize == 0)
                    {
                      fileList[i].fileSize = newFile->size;
                      strncpy(fileList[i].fileName,newFile->fileName,MAXFILENAME-1);

                      newFileIndex = i;
                      break;
                    }
                  }
                  
                  // no space found for new file information
                  // we request to add an extra element, we have to wait until the next
                  // update before we can add to it
                  if(newFileIndex == -1)
                    session.AddExtraFile();
                }
              }
            }

            GMachineSessions.Leave();
          }
        break; 
#endif
        case MAGIC_ENUM_RESPONSE:           // enumeration response
        {
            if ( msg->getLength() != SESSION_PACKET_2 &&
                 msg->getLength() != SESSION_PACKET_3 )
              break; // wrong size

              // default mod required:
            char mod[MOD_LENGTH]; mod[0] = 0;
            bool equalModRequired = false;
            if ( msg->getLength() >= SESSION_PACKET_3 )
            {
              strncpy(mod, ((SessionPacket *)msg->getData())->mod, MOD_LENGTH);
              mod[MOD_LENGTH-1] = (char)0;
              equalModRequired = (((SessionPacket *)msg->getData())->equalModRequired & 1) != 0;
            }

            SessionPacket *s = (SessionPacket *)msg->getData();
            {
                _enum->enter();

                    // search for existing session record
                int iFound = -1;
                for ( int i = 0; i < _enum->_sessions.Size(); i++ )
                    if ( _enum->_sessions[i].ip == ip && _enum->_sessions[i].port == port ) {
                        iFound = i;
                        break;
                        }
    
                unsigned64 reqTime = msg->getChannel()->getMessageTime(s->request);
                    // actual ping time in milliseconds:
                unsigned pingTime = reqTime ? (unsigned)( (msg->getTime() - reqTime) / 1000 ) : 0;

                if ( iFound < 0 ) {                     // not found
                    iFound = _enum->_sessions.Add();
                    if ( iFound < 0 ) {                 // too much sessions encountered
                        _enum->leave();
                        return nsNoMoreCallbacks;
                        }
                    NetSessionDescription &ndesc = _enum->_sessions[iFound];
                    ndesc.ip       = ip;
                    ndesc.port     = port;
                    ndesc.pingTime = pingTime;
#if _GAMES_FOR_WINDOWS
                    XNetInAddrToString(distant.sin_addr, ndesc.address, sizeof(ndesc.address));
#else
                    sprintf(ndesc.address,"%s:%u",inet_ntoa(distant.sin_addr),(unsigned)port);
#endif
                    }

                NetSessionDescription &desc = _enum->_sessions[iFound];
                // sprintf(desc.name,s->name,desc.address);
                strncpy(desc.name, s->name, LEN_SESSION_NAME);
                desc.name[LEN_SESSION_NAME - 1] = 0;

                desc.actualVersion   = s->actualVersion;
                desc.requiredVersion = s->requiredVersion;
                strncpy(desc.gameType,s->gameType,LEN_GAMETYPE_NAME);
                desc.gameType[LEN_GAMETYPE_NAME-1] = 0;
                desc.gameType[LEN_GAMETYPE_NAME-1] = 0;
                strncpy(desc.missionId,s->missionId,sizeof(desc.missionId)-1);
                desc.missionId[sizeof(desc.missionId)-1] = 0;
                desc.missionLocalizedType = s->missionLocalizedType;
                strncpy(desc.island,s->island,LEN_ISLAND_NAME);
                desc.island[LEN_ISLAND_NAME-1] = 0;
                strncpy(desc.mod,mod,MOD_LENGTH);
                desc.mod[MOD_LENGTH-1] = 0;
                desc.equalModRequired = equalModRequired ? 1 : 0;
                desc.serverState     = s->serverState;
                desc.maxPlayers      = s->maxPlayers;
                desc.numPlayers      = s->numPlayers;
                desc.password        = s->password;
                desc.lastTime        = (unsigned32)(getSystemTime() / 1000);
                desc.pingTime        = (3*desc.pingTime + pingTime + 2) >> 2;
                desc.language = s->language;
                desc.difficulty = s->difficulty;
                desc.timeleft = s->timeleft;
                _enum->leave();
                }
            }
            break;
            case MAGIC_CHALLENGE_PLAYER:
              if ( msg->getLength() == sizeof(ChallengePlayerPacket) && _client)
              {
                ChallengePlayerPacket *chp = (ChallengePlayerPacket*)msg->getData();
                _client->enterSnd();
                if ( _client->challengePlayer ==0 ) // refuse duplicate messages
                {
                  LogF("Client: Received challenge %x",chp->challenge);
                  _client->challengePlayer = chp->challenge;
                }
                _client->leaveSnd();
              }
              break;                              // don't need to lock this..

        }

    return nsNoMoreCallbacks;
}

//---------------------------------------------------------------------------
//  NetSessionEnum:

bool NetSessionEnum::needsRequest ( struct sockaddr_in &addr, unsigned16 port )
{
  bool needs = true;
  enter();
  unsigned32 ip = ntohl(addr.sin_addr.s_addr);
  for ( int i = 0; i < _sessions.Size(); i++ )
  {
    const NetSessionDescription &src = _sessions[i];
    if ( src.ip == ip && src.port >= port && src.port < port + NN_NUM_PORTS_TO_TRY * PORT_INTERVAL &&
         (src.port - port) % PORT_INTERVAL == 0 )
    {
      unsigned32 now = (unsigned32)(getSystemTime() / 1000);
      if ( (now - src.lastTime) < MIN_ENUM_RETRY )
        needs = false;
      break;
    }
  }
  leave();
  return needs;
}

void NetSessionEnum::sendRequest ( NetChannel *br, struct sockaddr_in &addr, unsigned16 port )
{
  Assert( br );
  unsigned16 p;
  EnumPacket request;
  request.magic = MAGIC_ENUM_REQUEST;
  request.magicApplication = magicApp;
  int i;

  for ( p = port, i = 0; i++ < NN_NUM_PORTS_TO_TRY; p += PORT_INTERVAL )
  {
    addr.sin_port = htons(p);
    Ref<NetMessage> msg = NetMessagePool::pool()->newMessage(sizeof(EnumPacket),br);
    if ( msg )
    {
      msg->setDistant(addr);
      msg->setFlags(MSG_ALL_FLAGS,MSG_TO_BCAST_FLAG|MSG_MAGIC_FLAG);
      msg->setData((unsigned8*)&request,sizeof(EnumPacket));
      msg->send();
    }
  }
}


//---------------------------------------------------------------------------
//  NetClient:

ConnectResult NetClient::ReInit ()
{
  Fail("ReInit not used and no longer supported");
  enterAll();
  if ( !channel )                           // not connected => error
  {
    leaveAll();
    return CRError;
  }
    // put together the "ReconnectPlayer" message:
  ReconnectPlayerPacket packet;
  packet.magic = MAGIC_RECONNECT_PLAYER;
  packet.playerNo = playerNo;

    // .. and send it to the server:
  ackPlayer = CRNone;
  challengePlayer = 0;

  // server will receive this message via it's control channel so I must not use VIM flag!
  // I'll send it multiple times if necessary..

    // now I have to wait to server's response..
  unsigned64 now = getSystemTime();
  unsigned64 next = now;                    // re-send time (init = the 1st try)
  unsigned64 timeout = now + 1000 * ACK_PLAYER_TIMEOUT;
  do
  {
    if ( now >= next )
    {
      Ref<NetMessage> msg = NetMessagePool::pool()->newMessage(sizeof(packet),channel);
      if ( !msg ) return CRError;
      msg->setFlags(MSG_ALL_FLAGS,MSG_TO_BCAST_FLAG|MSG_MAGIC_FLAG);
      msg->setData((unsigned8*)&packet,sizeof(packet));
      msg->send(true);                      // urgent
      next = now + 1000 * CREATE_PLAYER_RESEND;
    }
    leaveAll();
    SLEEP_MS(NET_CHECK_WAIT);
    enterAll();
    ProgressRefresh();
    now = getSystemTime();
  } while ( ackPlayer == CRNone && now < timeout );


  if ( ackPlayer != CRNone )                // initial hand-shake (to re-initialize band-width)
    channel->checkConnectivity(0);

#ifdef NET_LOG_CLIENT
#  ifdef NET_LOG_BRIEF
  NetLog("Ch(%u):cli-re(%s,%.3f,%d)",
         channel->getChannelId(),(ackPlayer == CRNone)?"failed":"reconnected",
         1.e-6*(now-timeout+1000*ACK_PLAYER_TIMEOUT),playerNo);
#  else
  NetLog("Channel(%u): NetClient::ReInit: %s after %.3f seconds (playerID=%d)",
         channel->getChannelId(),(ackPlayer == CRNone)?"failed":"reconnected",
         1.e-6*(now-timeout+1000*ACK_PLAYER_TIMEOUT),playerNo);
#  endif
#endif
  ConnectResult result = (ackPlayer == CRNone) ? CRError : (ConnectResult)ackPlayer;
  leaveAll();
  return result;
}

#if _ENABLE_GAMESPY
// NAT Negotiation
struct NATNegResult
{
  NegotiateResult _result;
  sockaddr_in _address;

  NATNegResult()
  {
    // not done yet
    _result = (NegotiateResult)-1;
  }
};

static void NNProgressCallback(NegotiateState state, void *userdata)
{
  // nothing to do
}

static void NNCompletedCallback(NegotiateResult result, SOCKET gamesocket, struct sockaddr_in *remoteaddr, void *userdata)
{
  NATNegResult *info = reinterpret_cast<NATNegResult *>(userdata);
  info->_result = result;
  if (remoteaddr) info->_address = *remoteaddr;
}
#endif

bool NetClient::SendMagicPacket(const void *data, size_t size)
{
  Ref<NetMessage> msgReq = NetMessagePool::pool()->newMessage(size,channel);
  if ( !msgReq )
  {
    return false;
  }
  msgReq->setFlags(MSG_ALL_FLAGS,MSG_TO_BCAST_FLAG|MSG_MAGIC_FLAG);
  msgReq->setData((const unsigned8*)data,size);
  msgReq->send(true);                      // urgent
  return true;
}

/*!
\patch 5248 Date 3/26/2008 by Bebul
- Fixed: NAT negotiation failed issue while connecting to server.
*/

ConnectResult NetClient::Init ( RString address, RString password, bool botClient, int &port,
                                RString player, MPVersionInfo &versionInfo, int magic, CancelNNCallback *cancelNNCallback )
{
#ifdef NET_LOG_CLIENT
#  ifdef NET_LOG_BRIEF
  NetLog("Cli(%s,%d)",
         address.GetLength()?(const char*)address:"none",port);
#  else
  NetLog("NetClient::Init: address=%s, port=%d",
         address.GetLength()?(const char*)address:"none",port);
#  endif
#endif
  enterAll();
  if ( channel )                            // already connected => error
  {
    leaveAll();
    return CRError;
  }
  magicApp = magic;
  struct sockaddr_in daddr;
  if ( address.GetLength() == 0 )           // use "localhost"
  {
    char buf[64];
    sprintf(buf,"127.0.0.1:%d",port);
    address = buf;
  }

#if _ENABLE_GAMESPY
  const char *ptr = address;
  const char *p = strrchr(ptr, '#');
  if (p)
  {
    // NAT Negotiation is needed
    unsigned int cookie = strtoul(p + 1, NULL, 10);
    RString ip;
    decodeURLAddress(RString(ptr, p - ptr), ip, port);        // ip = "ip4.ip3.ip2.ip1", port = "port"

    LogF("NAT Negotiation started for cookie : %d", cookie);

    NATNegResult result;
    enterNN();
    NegotiateError error = NNBeginNegotiationWithSocket(GET_CLIENT_PEER() ? GET_CLIENT_PEER()->GetSocket() : 0, cookie, 0, NNProgressCallback, NNCompletedCallback, &result);
    leaveNN();
    leaveAll(); //NAT negotiation must be processed beyond the enterAll critical section! (or no packets (including NAT pings) can be received)
    if (error != ne_noerror)
    {
      RptF("  NAT Negotiation failed (NNBeginNegotiationWithSocket) - error %d", error);
      return CRError;
    }
    const int NNTimeout = 30*1000; //do not wait more than 30 seconds
    DWORD NNEndTime = ::GetTickCount() + NNTimeout;
    while (result._result == -1)
    { //NOTE: NAT negotiation phase must be processed beyond the enterAll critical section!
      enterNN();
      NNThink();
      leaveNN();
      msleep(5);
      // Callback should return false when application is closed (WM_QUIT) or Escape key was pressed
      if ( cancelNNCallback && !( cancelNNCallback() ) )
      {
        result._result = nr_unknownerror; //connection failed
        enterNN();
        NNFreeNegotiateList(); //cancel negotiators
        leaveNN();
      }
      ProgressRefresh();
      // check NNTimeout
      if ( ::GetTickCount() > NNEndTime )
      {
        result._result = nr_inittimeout; //connection timeout
        enterNN();
        NNFreeNegotiateList(); //cancel negotiators
        leaveNN();
      }
    }
    if (result._result != nr_success)
    {
      RptF("  NAT Negotiation failed (NNThink - result %d)", result._result);
      return CRError;
    }
    enterAll();
    daddr = result._address;
    LogF("  succeeded");
  }
  else
#endif
  {
    RString ip;

    // Xbox: address should contain valid IP address in text format!
    decodeURLAddress(address, ip, port);        // ip = "ip4.ip3.ip2.ip1", port = "port"

    if ( !getHostAddress(daddr,(const char*)ip,port) )
    {
      leaveAll();
      return CRError;
    }
  }
    // now "daddr" contains valid IP address..
  sessionTerminated = false;
  whySessionTerminated = NTROther;

    // create a new NetChannel for client <-> server communication:
  poolLock.enter();
  if ( getPool() )
    channel = getPool()->createChannel(daddr,GET_CLIENT_PEER());
  if ( !channel )
  {
    poolLock.leave();
    leaveAll();
    return CRError;
  }
  poolLock.leave();

  channel->setProcessRoutine(clientReceive,channel);
  
  amIBot = botClient;

  RequestPlayerPacket requestPacket;
  requestPacket.magic = MAGIC_REQUEST_PLAYER;
  requestPacket.magicApplication = magicApp;
  requestPacket.actualVersion = versionInfo.versionActual;

    // put together the "CreatePlayer" message:
  CreatePlayerPacketChallenge packet;
  packet.magic = MAGIC_CREATE_W_CHALLENGE;
  packet.magicApplication = magicApp;
  strncpy(packet.name,player,LEN_PLAYER_NAME);
  packet.name[LEN_PLAYER_NAME-1] = (char)0;
  strncpy(packet.password,password,LEN_PASSWORD_NAME);
  packet.password[LEN_PASSWORD_NAME-1] = (char)0;
  packet.actualVersion = versionInfo.versionActual;
  packet.requiredVersion = versionInfo.versionRequired;
  packet.actualBuild = BUILD_NO;
  strncpy(packet.mod,versionInfo.mod,MOD_LENGTH);
  packet.mod[MOD_LENGTH-1] = (char)0;
  packet.botClient = (botClient ? 1 : 0);
  packet.uniqueID = (int32)(getSystemTime() & 0x7fffffff);
  saturateMax(packet.uniqueID, UNIQUE_TO_TRY + RESERVED_IDS); //always positive number
#ifdef NET_LOG_CLIENT
#  ifdef NET_LOG_BRIEF
  NetLog("Ch(%u):cli(%u.%u.%u.%u:%u,'%s',%d,%d)",
         channel->getChannelId(),
         (unsigned)IP4(daddr),(unsigned)IP3(daddr),(unsigned)IP2(daddr),(unsigned)IP1(daddr),(unsigned)PORT(daddr),
         (const char*)player,botClient?1:0,packet.uniqueID);
#  else
  NetLog("Channel(%u): NetClient::Init: server=%u.%u.%u.%u:%u, player='%s', bot=%d, magic=%d, playerID=%d",
         channel->getChannelId(),
         (unsigned)IP4(daddr),(unsigned)IP3(daddr),(unsigned)IP2(daddr),(unsigned)IP1(daddr),(unsigned)PORT(daddr),
         (const char*)player,botClient?1:0,magic,packet.uniqueID);
#  endif
#endif
    // .. and send it to the server:
  ackPlayer = CRNone;
  challengePlayer = 0;

  // server will receive this message via it's control channel so I must not use VIM flag!
  // I'll send it multiple times if necessary..

	unsigned long int cntStart = GetTickCount();
	NetLog( "XXX connecting %d", cntStart );

    // now I have to wait to server's response..
  unsigned64 now = getSystemTime();
  unsigned64 next = now;                    // re-send time (init = the 1st try)
  unsigned64 timeout = now + 1000 * ACK_PLAYER_TIMEOUT;
  bool challengeReceived = false;
  do
  {
    if ( now >= next || !challengeReceived && challengePlayer!=0)
    {
      if (challengePlayer==0)
      {
        if (!SendMagicPacket(&requestPacket,sizeof(requestPacket)))
        {
          leaveAll();
          return CRError;
        }
        LogF("Client: send MAGIC_REQUEST_PLAYER to %u.%u.%u.%u:%u",(unsigned)IP4(daddr),(unsigned)IP3(daddr),(unsigned)IP2(daddr),(unsigned)IP1(daddr),(unsigned)PORT(daddr));
      }
      else
      { // challenge received, respond by a new packet
        challengeReceived = true;
        packet.challenge = challengePlayer;
        packet.magic = MAGIC_CREATE_W_CHALLENGE;
        if (!SendMagicPacket(&packet,sizeof(packet)))
        {
          leaveAll();
          return CRError;
        }
        LogF("Client: send MAGIC_CREATE_W_CHALLENGE %x",challengePlayer);
      }

      next = now + 1000 * CREATE_PLAYER_RESEND;
    }
            
    leaveAll();

    SLEEP_MS(NET_CHECK_WAIT);
    enterAll();
    ProgressRefresh();
    now = getSystemTime();
#if _ENABLE_GAMESPY
    enterNN();
    NNThink(); // need to be called some time after negotiation is finished
    leaveNN();
#endif
  } while ( ackPlayer == CRNone && now < timeout );

  NetLog( "XXX finished in %d", GetTickCount()-cntStart );

  if ( ackPlayer != CRNone )                // initial hand-shake (to initialize band-width)
  {
    channel->checkConnectivity(0);
    struct sockaddr_in distant;
    channel->getDistantAddress(distant);
    port = ntohs(distant.sin_port);
  }

#ifdef NET_LOG_CLIENT
#  ifdef NET_LOG_BRIEF
  NetLog("Ch(%u):cli(%s,%.3f,%d)",
         channel->getChannelId(),
         (ackPlayer == CRNone)?"failed":"connected",1.e-6*(now-timeout+1000*ACK_PLAYER_TIMEOUT),playerNo);
#  else
  NetLog("Channel(%u): NetClient::Init: %s after %.3f seconds (playerID=%d)",
         channel->getChannelId(),
         (ackPlayer == CRNone)?"failed":"connected",1.e-6*(now-timeout+1000*ACK_PLAYER_TIMEOUT),playerNo);
#  endif
#endif
  ConnectResult result = (ackPlayer == CRNone) ? CRError : (ConnectResult)ackPlayer;
  leaveAll();
  return result;
}

void VoiceDataSent(int pid, int len)
{
  if (_server) _server->NotifyChannelDataSent(pid,len);
}

void NetServer::NotifyChannelDataSent(int player, size_t size)
{
  RefD<NetChannel> channel;
  users.get(player,channel);
  if (channel)
  {
    channel->dataSentAck(size);
  }
}

void destroyVoice ()
{
#if VOICE_OVER_NET
  if ( von )
  {
#ifdef NET_LOG_VOICE
    NetLog("destroyPool: destroying VoNSystem instance");
#endif
    von = NULL;
  }
#endif
}

//---------------------------------------------------------------------------
//  Common implementation (shared with netTransportXbox.cpp):

#include "netTransportCommon.hpp"

#endif  // _ENABLE_MP
