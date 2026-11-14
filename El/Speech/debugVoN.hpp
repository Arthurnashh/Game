// This file only defines some defines for debugging purposes
#ifndef DEBUG_VON_HPP
#define DEBUG_VON_HPP

#include <Es/platform.hpp>
#include <El/QStream/qStream.hpp>
#include <Es/Strings/rString.hpp>

#if MIMIC_108
#define USE_NAT_NEGOTIATION_MANAGER 0
#else
#define USE_NAT_NEGOTIATION_MANAGER 1
#endif

#if !_SUPER_RELEASE && _ENABLE_VON

#   define DEBUG_VON 0
#   define DEBUG_VON_DIAGS 1
#   define DEBUG_IDENTITIES 1
#   define TEST_INIT_BUFFER 0
#   define HEAR_YOURSELF 0
#   define DEBUG_VON_KEEP_ALIVE
#   define DEBUG_VON_DETECT_NAT_TYPE 1
#   define DIAGNOSE_SPEEX_ESTFRAMES 0
#   define DEBUG_VON_SAVE_BUFFERS 0
#   define DEBUG_VON_SILENCE_RATIO 0
//#   define DEBUG_VON_RETRANSLATE

//#   define NET_LOG_VOICE

#if HEAR_YOURSELF
extern bool hearYourself;
#endif

#elif BETA_TESTERS_DEBUG_VON

#   define DEBUG_VON 0
#   define DEBUG_IDENTITIES 0
#   define TEST_INIT_BUFFER 0
#   define DEBUG_VON_DIAGS 0
#   define HEAR_YOURSELF 0
#   define DEBUG_VON_DETECT_NAT_TYPE 1
#   define DIAGNOSE_SPEEX_ESTFRAMES 0
#   define DEBUG_VON_SAVE_BUFFERS 0
#   define DEBUG_VON_SILENCE_RATIO 0

#else // To disable debugging, set everything to 0

#   define DEBUG_VON 0
#   define DEBUG_IDENTITIES 0
#   define TEST_INIT_BUFFER 0
#   define DEBUG_VON_DIAGS 0
#   define HEAR_YOURSELF 0
#   define DEBUG_VON_DETECT_NAT_TYPE 1
#   define DIAGNOSE_SPEEX_ESTFRAMES 0
#   define DEBUG_VON_SAVE_BUFFERS 0
#   define DEBUG_VON_SILENCE_RATIO 0

#endif

#define DEBUG_VON_BANK_FEATURES(XX) \
  XX(VoNFeature_P2PManager, "", true) \
  XX(VoNFeature_NetTopology, "", true) \
  XX(VoNFeature_GetClientAddressFailed, "BAD: Get client address failed!", false) \
  XX(VoNFeature_PacketSizeOverflow, "BAD: Packet Size Overflow!", true) \
  XX(VoNFeature_VoNPacketSizeOverflow, "BAD: Voice Packet Size Overflow!", true) \
  XX(VoNFeature_MaxRetranslateTargetsOverflow, "BAD: MaxRetranslateTargets Overflow!", true) \
  XX(VoNFeature_RetranslateToUnconnected, "BAD: Retranslate request out of my peerToPeer set!", true) \
  XX(VoNFeature_SendEmptyVoice, "BAD: Sending empty voice!", true) \
  XX(VoNFeature_RecvEmptyVoice, "BAD: Got empty voice packet!", true) \
  XX(VoNFeature_NN, "", false) \
//XX(VoNFeature_NAME, defaultString, reset)

#define DEBUG_VON_BANK_ENUMS(enumName, string, reset) enumName, 
#define DEBUG_VON_BANK_STRINGS(enumName, string, reset) string, 
#define DEBUG_VON_BANK_NULL_STRING(enumName, string, reset) RString(""), 
#define DEBUG_VON_BANK_STORE_RESET(enumName, string, reset) reset, 

#define DEBUG_VON_BANK_GLOBAL \
  const char *DebugVoNFeaturesDefault[] = { \
  DEBUG_VON_BANK_FEATURES(DEBUG_VON_BANK_STRINGS) \
  }; \
  RString DebugVoNFeaturesStrings[] = { \
  DEBUG_VON_BANK_FEATURES(DEBUG_VON_BANK_NULL_STRING) \
  }; \
  bool DebugVoNReset[] = { \
  DEBUG_VON_BANK_FEATURES(DEBUG_VON_BANK_STORE_RESET) \
  }; \

#define DEBUG_VON_BANK_DECLARE \
  enum DebugVoNFeatures \
  { \
  DEBUG_VON_BANK_FEATURES(DEBUG_VON_BANK_ENUMS) \
  }; \

extern RString DebugVoNFeaturesStrings[];
extern const char *DebugVoNFeaturesDefault[];
extern bool DebugVoNReset[];

struct ConvPair
{
  int _dpnid;
  RString _id;
  RString _name;
  ConvPair() {}
  ConvPair(int dpnid, RString id, RString name) : _dpnid(dpnid), _id(id), _name(name) {}
  ConvPair(const ConvPair &pair) : _dpnid(pair._dpnid), _id(pair._id), _name(pair._name) {}
  ConvPair &operator = ( const ConvPair &src ) { _dpnid=src._dpnid; _id=src._id; _name = src._name; }
  ClassIsMovable(ConvPair);
};

class DebugVoNBank
{
public:
  DEBUG_VON_BANK_DECLARE

  DebugVoNBank() : _timeEnd(NotRunning), _numPlayers(0), _numResponses(0), _natTypeTriggered(false) 
  {
    Reset(true);
  };

  void SetFeature(enum DebugVoNFeatures feature)
  {
    if (feature<VoNFeature_NN)
      DebugVoNFeaturesStrings[(int)feature] = DebugVoNFeaturesDefault[(int)feature];
  }

  void SetFeature(enum DebugVoNFeatures feature, RString text)
  {
    if (feature<VoNFeature_NN)
      DebugVoNFeaturesStrings[(int)feature] = text;
  }

  void UpdateFeature(enum DebugVoNFeatures feature, RString text)
  {
    if (feature<VoNFeature_NN)
    {
      if (!DebugVoNFeaturesStrings[(int)feature].IsEmpty()) 
        DebugVoNFeaturesStrings[(int)feature] = DebugVoNFeaturesStrings[(int)feature] + RString("\n") + text;
      else 
        DebugVoNFeaturesStrings[(int)feature] = text;
    }
  }

  // Reset features that are resettable (or all)
  void Reset(bool all=false)
  {
    for (int i=0; i<VoNFeature_NN; i++)
    {
      if (all || DebugVoNReset[i]) DebugVoNFeaturesStrings[i] = RString("");
    }
    ResetSpecial(all);
  }

  //////////////////////////////////////////////////////////////////////////////
  // On Server, there should be also something to control collecting phase.
  // It works as follows:
  //   1. server gets #debug von
  //   2. server uses the DebugVoNBank instance to store VoN Net Topology info
  //      and send NCMTAskVoNDiagnostics message to each player
  //   3. server set in its DebugVoNBank instance that collecting is running (StartCollecting)
  //   4. each client respond to server using the NCMTAskVoNDiagnostics too
  //   5. after all NCMTAskVoNDiagnostics responses come or timeout elapses, the report is written
  //
  struct VoNFeaturesClientInfo
  {
    RString _id;    //player id (based on cd key or #dpnid)
    RString _name;
    RString _info;  //collected data
    VoNFeaturesClientInfo() {}
    ClassIsMovable(VoNFeaturesClientInfo);
  };
  static const DWORD NotRunning = 0xFFFFFFFF;
  DWORD _timeEnd;  //timeout when collecting phase should stop even when some client does not respond
  int _numPlayers; //number of response to wait for when collecting
  int _numResponses; //current number of collected responses
  AutoArray<VoNFeaturesClientInfo> _clientInfos;  //collected data
  RString _natType; // NAT type si collected separately

  // start collecting data from clients, set the timeout
  void StartCollecting(DWORD startTime, int numPlayers)
  {
    _clientInfos.Resize(0);
    const DWORD timeout = 30*1000; //30s
    _timeEnd = startTime + timeout;
    _numPlayers = numPlayers;
    if (_timeEnd == NotRunning) _timeEnd--; //paranoid
  }

  // stop collecting, after data are collected and processed
  void StopCollecting()
  {
    _timeEnd = NotRunning;
    _numPlayers = _numResponses = 0;
  }

  // true when #debug von was triggered and data are still collecting
  bool IsRunning() { return (_timeEnd!=NotRunning); }

  // Add client response
  void AddClientResponse(RString id, RString name, RString response)
  {
    int ix = _clientInfos.Add();
    _clientInfos[ix]._id = id;
    _clientInfos[ix]._name = name;
    _clientInfos[ix]._info = response;
    _numResponses++;
  }

  // Get the Report of collected features
  RString GetDebugString(Array<ConvPair> &convTable)
  {
    QOStrStream out;
    if (_clientInfos.Size())
    { // add client responses too
      for (int i=0; i<_clientInfos.Size(); i++)
      {
        out << "=Player=: " << _clientInfos[i]._id << " : " << _clientInfos[i]._name << "\n";
        out << _clientInfos[i]._info;
      }
    }
    else
    {
      for (int i=0; i<VoNFeature_NN; i++)
      {
        if ( !DebugVoNFeaturesStrings[i].IsEmpty() )
        {
          out << DebugVoNFeaturesStrings[i] << "\n";
        }
      }
      ReportSpecial(out, convTable);
      if (!_natType.IsEmpty()) out << _natType << "\n";
    }
    return RString(out.str(),out.pcount());
  }

  bool IsReportReady(DWORD tickCount)
  {
    return (_timeEnd!=NotRunning && ( _timeEnd<tickCount || _numResponses==_numPlayers) );
  }

  /// natTypeTriggered servers only to ensure NAT type detection is processed only once
  bool _natTypeTriggered;
  void TriggerNatTypeDetection() { _natTypeTriggered = true; }
  bool NatTypeDetectionTriggered() { return _natTypeTriggered; }

  /// Special diagnostics which are triggered from other threads (cannot use RString - MT Unsafe)
  void ResetSpecial(bool all=false)
  {
    _spIPchannel = -1;        //specialIPMess
    _spRetranslateFrom = -1;  //specialRetranslate 
    _spEmptyVoice = false;    //specialEmptyVoice
    (void) all;
  }
  static RString Dpid2Cdid(int dpid, Array<ConvPair> &convTable)
  {
    for (int i=0; i<convTable.Size(); i++)
    {
      if (dpid==convTable[i]._dpnid) return convTable[i]._id;
    }
    if (dpid==2) return RString("Server");
    return RString("Unknown");
  }
  void ReportSpecial(QOStrStream &out,Array<ConvPair> &convTable)
  {
    //specialIPMess
    if (_spIPchannel!=-1)
    {
      RString strId = Dpid2Cdid(_spIPchannel, convTable);
      out << Format("BAD: IPs do not match: id=%s, recv=%d.%d.%d.%d:%d, tgt=%d.%d.%d.%d:%d\n",
        cc_cast(strId),
        _spIPfromAddr&0xff, (_spIPfromAddr>>8)&0xff, (_spIPfromAddr>>16)&0xff, (_spIPfromAddr>>24)&0xff, 
        _spIPfromPort, 
        _spIPtgtAddr&0xff, (_spIPtgtAddr>>8)&0xff, (_spIPtgtAddr>>16)&0xff, (_spIPtgtAddr>>24)&0xff, 
        _spIPtgtPort
        );
    }
    //specialRetranslate
    if (_spRetranslateFrom!=-1)
    {
      RString strFrom = Dpid2Cdid(_spRetranslateFrom, convTable);
      RString strTo = Dpid2Cdid(_spRetranslateTo, convTable);
      out << Format("BAD: Retranslate request (from: %s to: %s) is out of my peerToPeer set!\n",
        cc_cast(strFrom), cc_cast(strTo)
        );
    }
    //specialEmptyVoice
    if (_spEmptyVoice)
    {
      RString strFrom = Dpid2Cdid(_spEmptyVoiceFrom, convTable);
      RString strTo = Dpid2Cdid(_spEmptyVoiceTo, convTable);
      out << Format("BAD: Empty voice (from: %s to: %s)\n", cc_cast(strFrom), cc_cast(strTo));
    }
  }
  // Special: "IP address of received packet does not match my TransmitTarget one"
  int _spIPchannel, _spIPfromAddr, _spIPfromPort, _spIPtgtAddr, _spIPtgtPort;
  void SetSpecialIPMess(int channel, int fromAddr, int fromPort, int tgtAddr, int tgtPort)
  {
    _spIPchannel = channel;
    _spIPfromAddr = fromAddr; _spIPfromPort = fromPort;
    _spIPtgtAddr = tgtAddr; _spIPtgtPort = tgtPort;
  }
  // Special: "request to retranslate packet out of my PeerToPeer list"
  int _spRetranslateFrom, _spRetranslateTo;
  void SetSpecialRetranslate(int from, int to)
  {
    _spRetranslateFrom = from;
    _spRetranslateTo = to;
  }
  // Special: "request to retranslate packet out of my PeerToPeer list"
  bool _spEmptyVoice;
  int _spEmptyVoiceFrom, _spEmptyVoiceTo;
  void SetSpecialEmptyVoice(int from, int to)
  {
    _spEmptyVoice = true;
    _spEmptyVoiceFrom = from;
    _spEmptyVoiceTo = to;
  }
};

/// Global for collecting VoN Diagnostics on client & server
extern DebugVoNBank GDebugVoNBank;

#if DEBUG_VON_SAVE_BUFFERS
/// used to store sound data and save them to file
class DebugVoNSaveBuffers
{
public:
  /// sound buffer 
  class SoundBuffer
  {
  public:
    typedef short TElementType;

    SoundBuffer(RString name, unsigned length)
      :_name(name), _offset(0), _saveCount(0), _bufSaved(true), _lastActiveTime(0), _messagesEnabled(true)
    {
      _buffer.Resize(length);
    }
    /// adds value
    void Add(TElementType val)
    {
      if (_offset >= _buffer.Size())
        Save();

      if (_offset < _buffer.Size())
        _buffer[_offset++] = val;
      OnChanged();
    }
    /// adds message
    void AddMessage(RString msg)
    {
      if (!_messagesEnabled)
        return;
      _messages.Add(msg);
      OnChanged();
    }

    /// saves only if _bufSaved is false and min. deltaTime passed since last activity
    void SaveTimed(unsigned deltaTime)
    {
      if (!_bufSaved && (::GetTickCount() > _lastActiveTime + deltaTime))
      {
        Save();
        _bufSaved = true;
      }
    }

    /// saves and resets offset
    void Save()
    {
      if (_offset > 0 || _messages.Size() > 0)
      {
        RString fileNameWav = Format("%s_%d.wav", _name.Data(), _saveCount );
        RString fileNameTxt = Format("%s_%d.txt", _name.Data(), _saveCount );

        if (_offset > 0)
        {
          bool DebugVoNSaveWav(const char* fileName, const char* data, unsigned dataSize);
          DebugVoNSaveWav(cc_cast(fileNameWav), (const char*)_buffer.Data(),_offset*sizeof(TElementType));
        }

        FILE *f = fopen(cc_cast(fileNameTxt.Data()), "w");
        if (f)
        {
          for (int i=0; i<_messages.Size(); ++i)
          {
            const RString& message = _messages[i];
            fprintf(f, "%s\n", cc_cast(message));
          }
          fclose(f);
        }

        //FILE *f = fopen(fileName.Data(), "wb");
        //if (f)
        //{
        //  fwrite(_buffer.Data(), sizeof(TElementType), _offset, f);
        //  fclose(f);
        //}
        _offset = 0;
        _saveCount++;
        _messages.Clear();
      }
    }

    // enables.disables adding of messages
    void EnableMessages(bool enable)  { _messagesEnabled = enable;}
    bool MessagesEnabled() const {return _messagesEnabled;}
  private:
    void OnChanged()
    {
      _bufSaved = false;
      _lastActiveTime = ::GetTickCount();
    }

    /// name of buffer (used for file name generation)
    RString _name;
    /// buffer for data
    AutoArray<TElementType> _buffer;    
    /// current offset
    int _offset;
    /// number of saves
    unsigned _saveCount;
    /// message buffer
    AutoArray<RString> _messages;
    /// if buffer was saved
    bool _bufSaved;
    /// last time when the buffer was active
    unsigned _lastActiveTime;
    /// if adding of messages is enabled
    bool _messagesEnabled;
  };

  static const unsigned BUF_SIZE = 8000*2*100; //100s

  DebugVoNSaveBuffers()
    :_recordBuf("speech_record", BUF_SIZE),
    _sendBuf("speech_send", BUF_SIZE),
    _receiveBuf("speech_receive", BUF_SIZE),
    _replayBuf("speech_replay", BUF_SIZE)
  {}

  SoundBuffer& GetRecordBuf() {return _recordBuf;} 
  SoundBuffer& GetSendBuf() {return _sendBuf;} 
  SoundBuffer& GetReceiveBuf() {return _receiveBuf;} 
  SoundBuffer& GetReplayBuf() {return _replayBuf;} 

private:
  /// what is recorded
  SoundBuffer _recordBuf;
  /// what is sent
  SoundBuffer _sendBuf;
  /// what is received
  SoundBuffer _receiveBuf;
  /// what is replayed
  SoundBuffer _replayBuf;

};

extern DebugVoNSaveBuffers GDebugVoNSaveBuffers;


#endif //DEBUG_VON_SAVE_BUFFERS

#endif
