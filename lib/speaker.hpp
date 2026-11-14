#ifdef _MSC_VER
#pragma once
#endif

#ifndef _SPEAKER_HPP
#define _SPEAKER_HPP

//#include "loadSave.hpp"

#include <El/Time/time.hpp>
#include "Network/networkObject.hpp"
#include "soundsys.hpp"
#include "location.hpp"
#include "vehicleAI.hpp"
#include <El/ParamArchive/serializeClass.hpp>
#include <El/ParamFile/paramFile.hpp>

DECL_ENUM(ChatChannel)

class ParamEntry;
class AIBrain;

class BasicSpeaker: public RefCount
{
  /// what radio protocol is used
  RString _protocol;
  /// directory for the full and single voice dubbing
	AutoArray<RString> _directories;
	
	void Load(ParamEntryPar cfg);
	
	public:
	BasicSpeaker(RString speaker);
	BasicSpeaker(ParamEntryPar cfg);
	AbstractWave *Say(Person *who3D, RString id, float pitch, bool loop, RString wordsSubclass);
  RString GetProtocol() const {return _protocol;}
};

class Speaker
{
	Ref<BasicSpeaker> _basic;
	float _pitch;

	public:
	Speaker(){}
	Speaker( Ref<BasicSpeaker> basic, float pitch )
	:_basic(basic),_pitch(pitch)
	{}

	bool IsSpeakerValid() const {return _basic!=NULL;}
	AbstractWave *Say(Person *who3D, RString id, RString wordsSubclass) {return _basic->Say(who3D, id, _pitch, false, wordsSubclass);}
	AbstractWave *SayNoPitch(Person *who3D, RString id, bool loop, RString wordsSubclass) {return _basic->Say(who3D, id, 1, loop, wordsSubclass);}
  RString GetProtocol() const {return _basic ? _basic->GetProtocol() : RString();}
};

class RadioChannel;
class SentenceParams;

struct RadioWord
{
  RString id;
  float pauseAfter;
};
TypeIsMovableZeroed(RadioWord);

class RadioSentence : public AutoArray<RadioWord>
{
public:
  // wordsClass when present is a subclass name of class words to use to get word wave file
  RString wordsClass;
public:
  int Add(RString id, float pauseAfter)
  {
    int index = AutoArray<RadioWord>::Add();
    Set(index).id = id;
    Set(index).pauseAfter = pauseAfter;
    return index;
  };
  void Say(Person *who, RadioChannel *channel, Speaker *speaker);
};

struct RadioChannelParams
{
  RString noise;
  RString micOuts;
  float pauseAfterWord;
  float pauseInNumber;
  float pauseAfterNumber;
  float pauseInUnitList;
  float pauseAfterUnitList;
  bool sound3D;
};

class RadioMessage: public RefCountWithLinks
{
protected:
	int _priority;
	Time _timeOut;
	bool _transmitted;
	mutable bool _playerMsg; // set during PrepareSentence

public:
	RadioMessage();
	virtual ~RadioMessage() {}
	bool IsTransmitted() const {return _transmitted;}
	void SetTransmitted() {_transmitted = true;}
	bool IsPlayerMsg() const {return _playerMsg;}
	void SetPlayerMsg(bool set = true) const {_playerMsg = set;}

	virtual void Transmitted(ChatChannel channel) {} // message completed
	virtual void Canceled() {} // message canceled

	virtual float GetDuration() const;
	virtual int GetType() const = 0;
	int GetPriority() const {return _priority;}
	void SetPriority(int priority) {_priority = priority;}
	virtual const char *GetPriorityClass() = 0;

  virtual bool IsConversation() const { return false; }

	Time GetTimeOut() const {return _timeOut;}
	void SetTimeOut( Time time ) {_timeOut=time;}

  /// return the name of the sentence to tell and its parameters
	virtual RString PrepareSentence(SentenceParams &params) const = 0;
	virtual AIBrain *GetSender() const = 0;
	virtual Speaker *GetSpeaker() const;
	
  virtual bool SendOverNetwork() const {return true;}
	virtual RString GetText() {return "";}
	virtual RString GetWave() {return "";}
  virtual void GetSpeech(RadioSentence &sentence, const RadioChannelParams &cfg) {}
	virtual RString GetSenderName() const {return "";}
  RString GetDebugName() const;
  bool CanSenderSpeak() const;
  
	virtual LSError Serialize(ParamArchive &ar);
	static RadioMessage *CreateObject(ParamArchive &ar);

  // helpers to get proper default values also outside the RadioMessage descendants
  static float GetDefaultGroupCompactness() {return 1;}
  static float GetDefaultDistanceToUnit() {return 1;}
  static float GetDefaultDistanceToSender() {return 1;}
  static float GetDefaultDistanceToGroup() {return 1;}
  static float GetDefaultDistanceToLocation() {return 1;}
  static float GetDefaultGroupCoreRadius() {return 0;}
  static float GetDefaultUnitDistanceFactor() {return 0;}
  static float GetDefaultInsideLocation() {return 0;}
  static float GetDefaultMoveToObject() {return 0;}
  static float GetDefaultDistanceToRecipients() {return 1.0f;}  //avoids zero division
  static float GetDefaultRecipientsRadius() {return 1.0f;} //avoids zero division
  // simple expression variables for sentence variants
  virtual float GetSERank() const;
  virtual float GetSECaptive() const;
  virtual float GetSESafe() const;
  virtual float GetSECombat() const;
  virtual float GetSEStealth() const;
  virtual float GetSEMorale() const;
  virtual float GetSEGroupCompactness() const {return GetDefaultGroupCompactness();}
  virtual float GetSEDistanceToUnit() const {return GetDefaultDistanceToUnit();}
  virtual float GetSEDistanceToSender() const {return GetDefaultDistanceToSender();}
  virtual float GetSEDistanceToGroup() const {return GetDefaultDistanceToGroup();}
  virtual float GetSEDistanceToLocation() const {return GetDefaultDistanceToLocation();}
  virtual float GetSEGroupCoreRadius() const {return GetDefaultGroupCoreRadius();}
  virtual float GetSEUnitDistanceFactor() const {return GetDefaultUnitDistanceFactor();}
  virtual float GetSEInsideLocation() const {return GetDefaultInsideLocation();}
  virtual float GetSEMoveToObject() const {return GetDefaultMoveToObject();}
  virtual float GetSEDistanceToRecipients() const {return GetDefaultDistanceToRecipients();}  //avoids zero division
  virtual float GetSERecipientsRadius() const {return GetDefaultRecipientsRadius();} //avoids zero division

};

/// one radio channel (may be used for group radio , vehicle intercom ...)
class RadioChannel : public RemoveLinks, public SerializeClass
{
	bool _audible; // some channels are only virtual - no acoustics, no words
	ChatChannel _chatChannel;
	//OLink(NetworkObject) _object;
	/// various channels have different priorities
	int _level; 
  RadioChannelParams _params;

	RefAbstractWave _saying; // queue to this wave
	/// in case playback fails from any unknown reason, we want to timeout
	/**
	This is to prevent game waiting for the speech audio forever.
	*/
	Time _sayingTimeout;
	
	RefAbstractWave _noise; // noise channel
	Speaker _speaker;

	float _pauseAfterMessage;
	Ref<RadioMessage> _actualMsg;
	RefArray<RadioMessage> _radioQueue;

	// messageQueue holds all waiting messages
	// active message is converted to words and added to word queue
	
	void NextMessage(NetworkObject *object);

	RadioMessage *CreateMessage(int type);

	public:
  AbstractWave* Saying() { return _saying; }

	RadioChannel(ChatChannel chatChannel, ParamEntryPar cfg);
	void Simulate( float deltaT, NetworkObject *object );
	//! process all waiting messages as transmitted
	void SilentProcess();
	bool Done() const;
	void Say(Person *who, Speaker *speaker, RString id, float pauseAfter, bool transmit, RString wordsSubclass);
	void Say(RString waveName, AIBrain *sender, RString senderName, RString player, float duration);
	void Transmit(RadioMessage *msg);
	void Cancel( RadioMessage *msg, NetworkObject *object);
	/// abort message which is curently being played and never play it again, consider it canceled
	void CancelActualMessage();
	/// abort message which is curently being played and never play it again, consider it transmitted
	void FinishActualMessage();
	/// abort message which is curently being played, play it again once it is possible
	void InterruptActualMessage();
	
  /// we need to say something more important on some other channel
	void StopSpeaking(AIBrain *sender);
	
	void Replace( RadioMessage *msg, RadioMessage *with );

  void SetTimeout(Time time){_sayingTimeout = time;}
	void CancelAllMessages(NetworkObject *object);

  /// check channel priority
  int GetLevel() const {return _level;}
	bool IsAudible() const {return _audible;}
	void SetAudible( bool audible );

  /// check if given unit is speaking or about to start speaking
  bool CheckSpeaking(AIBrain *sender) const;
	bool IsEmpty() const {return !_actualMsg && _radioQueue.Size() == 0;}
	bool IsSilent() const;

  const RadioChannelParams &GetParams() const {return _params;}

	LSError Serialize(ParamArchive &ar);

  void SentChatMessage(RadioMessage *message, AIBrain *sender, NetworkObject *object);

/*
  RadioMessage *FindNextMessage(int type, int& index) const; // first for index < 0
	RadioMessage *FindPrevMessage(int type, int& index) const; // last for index >= NMessages
	RadioMessage *GetNextMessage(int& index) const; // first for index < 0
	RadioMessage *GetPrevMessage(int& index) const; // last for index >= NMessages
*/
  template <typename Function>
  bool ForEachMessage(const Function &func)
  {
    int n = _radioQueue.Size();
    for (int i=n-1; i>=0; i--)
    {
      RadioMessage *msg = _radioQueue[i];
      if (msg)
      {
        // channel, message, actual
        if (func(this, msg, false)) return true;
      }
    }
    if (_actualMsg)
    {
      // channel, message, actual
      if (func(this, _actualMsg, true)) return true;
    }
    return false;
  }
  template <typename Function>
  bool ForEachMessage(const Function &func) const
  {
    int n = _radioQueue.Size();
    for (int i=n-1; i>=0; i--)
    {
      const RadioMessage *msg = _radioQueue[i];
      if (msg)
      {
        // channel, message, actual
        if (func(this, msg, false)) return true;
      }
    }
    if (_actualMsg)
    {
      // channel, message, actual
      if (func(this, _actualMsg, true)) return true;
    }
    return false;
  }
	RadioMessage *GetActualMessage() const {return _actualMsg;}
};

// Helper class to collect information for Movement Orders commands
class RadioMoveToHelper
{
private:
  //@( Helper values to answer later Simple Expression questions
  float _distanceToGroup;
  float _distanceToLocation;
  float _distanceToUnit;
  float _groupCoreRadius;
  float _groupCompactness;
  float _moveToObject;
  const Location *_location;
  float _absLocationDir;
  Vector3 _recipientsCenterPos;
  float _distanceToRecipients;
  float _recipientsRadius;
  float _isInsideLocation;
  //@)
public:
  RadioMoveToHelper();
  void PrepareSEHelperVariables(AIBrain *first, Vector3Par destination, Target *tgtE, TargetId tgtF);
  void SetParams(SentenceParams &params, const RadioMessage *msg, AIBrain *first, Vector3Par destination);

  float GetDistanceToGroup() const { return _distanceToGroup; }
  float GetDistanceToLocation() const { return _distanceToLocation; }
  float GetDistanceToUnit() const { return _distanceToUnit; }
  float GetGroupCoreRadius() const { return _groupCoreRadius; }
  float GetGroupCompactness() const { return _groupCompactness; }
  float GetMoveToObject() const { return _moveToObject; }
  float GetDistanceToRecipients() const { return _distanceToGroup; }
  float GetRecipientsRadius() const { return _recipientsRadius; }
  float GetIsInsideLocation() const { return _isInsideLocation; }
  const Location *GetLocation() const { return _location; }
  float GetAbsLocationDir() const { return _absLocationDir; }
  Vector3 GetRecipientsCenterPos() const { return _recipientsCenterPos; }
};

// Helper class to use as a base class for RadioMessages using RadioMoveToHelper
// It makes SimpleExpressions for these radio messages ready, whenever RadioMoveToHelper.
class RadioMessageWithMove : public RadioMessage
{
protected:
  //@( Helper to answer later Simple Expression questions and prepare Parameters for the MoveTo sentence
  mutable RadioMoveToHelper _moveToHelper;
  //@)
public:
  float GetSEDistanceToGroup() const { return _moveToHelper.GetDistanceToGroup(); }
  float GetSEDistanceToLocation() const { return _moveToHelper.GetDistanceToLocation(); }
  float GetSEInsideLocation() const { return _moveToHelper.GetIsInsideLocation(); }
  float GetSEDistanceToUnit() const { return _moveToHelper.GetDistanceToUnit(); }
  float GetSEGroupCoreRadius() const { return _moveToHelper.GetGroupCoreRadius(); }
  float GetSEGroupCompactness() const { return _moveToHelper.GetGroupCompactness(); }
  float GetSEMoveToObject() const { return _moveToHelper.GetMoveToObject() ? 1.0 : 0; }
  float GetSEDistanceToRecipients() const { return _moveToHelper.GetDistanceToRecipients(); }
  float GetSERecipientsRadius() const { return _moveToHelper.GetRecipientsRadius(); }

  RadioMessageWithMove() {}
};

#endif
