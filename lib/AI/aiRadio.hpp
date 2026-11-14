#ifdef _MSC_VER
#pragma once
#endif

#ifndef _AI_RADIO_HPP
#define _AI_RADIO_HPP

/******************************************/
/*                                        */
/*              RadioMessages             */
/*                                        */
/******************************************/

enum RadioMessageType
{
	RMTText,
  RMTTalk,
  RMTTask,
  RMTTaskReceived,
  RMTTaskResult,
};

///////////////////////////////////////////////////////////////////////////////
// class RadioMessageText

class RadioMessageText : public RadioMessage
{
protected:
	RString _wave;
	OLinkPerm<AIBrain> _sender;
	RString _senderName;
	float _timeToLive;

public:
	RadioMessageText(RString wave, RString senderName, float ttl=2.0) {_wave = wave; _senderName = senderName; _timeToLive=ttl;}
	RadioMessageText(RString wave, AIBrain *sender, float ttl=2.0) {_wave = wave; _sender = sender; _timeToLive=ttl;}
	RadioMessageText() {}
	LSError Serialize(ParamArchive &ar);
	const char *GetPriorityClass();
	void Transmitted(ChatChannel channel);
  /// Do not send text message over network - serve only as a local background
  virtual bool SendOverNetwork() const {return false;}
	RString GetWave();
	int GetType() const {return RMTText;}
	float GetDuration() const;
	AIBrain *GetSender() const {return _sender;}
	RString GetSenderName() const {return _senderName;}

protected:
  RString PrepareSentence(SentenceParams &params) const;
};

#if _ENABLE_CONVERSATION

#include <RV/KBBase/kbCenter.hpp>

///////////////////////////////////////////////////////////////////////////////
// class RadioMessageTalk

class RadioMessageTalk : public RadioMessage
{
protected:
  Ref<KBMessageInfo> _message;
  OLinkPerm<AIBrain> _sender;
  OLinkPerm<AIBrain> _receiver;
  float _timeToLive;

public:
  RadioMessageTalk(const KBMessageInfo *message, AIBrain *sender, AIBrain *receiver, float ttl = 2.0);
  RadioMessageTalk() {}
  ~RadioMessageTalk();
  
  int GetType() const {return RMTTalk;}
  AIBrain *GetSender() const {return _sender;}

  RString GetWave();
  RString GetText();
  void GetSpeech(RadioSentence &sentence, const RadioChannelParams &cfg);
  float GetDuration() const;

  void Transmitted(ChatChannel channel);

  LSError Serialize(ParamArchive &ar);
  const char *GetPriorityClass();

  bool IsConversation() const { return true; }

protected:
  RString PrepareSentence(SentenceParams &params) const {return RString();}
};

#endif // _ENABLE_CONVERSATION

#if _ENABLE_IDENTITIES

class RadioMessageTask : public RadioMessage, public CountInstances<RadioMessageTask>
{
protected:
  Ref<AITask> _task;
  GameValue _params;

public:
  RadioMessageTask(AITask *task, GameValuePar params)
  {_task = task; _params = params;}
  RadioMessageTask() {}
  LSError Serialize(ParamArchive &ar);
  const char *GetPriorityClass();
  void Transmitted(ChatChannel channel);
  int GetType() const {return RMTTask;}
  AIBrain *GetSender() const;

protected:
  RString PrepareSentence(SentenceParams &params) const;
};

class RadioMessageTaskReceived : public RadioMessage, public CountInstances<RadioMessageTaskReceived>
{
protected:
  Ref<AITask> _task;

public:
  RadioMessageTaskReceived(AITask *task)
  {_task = task;}
  RadioMessageTaskReceived() {}
  LSError Serialize(ParamArchive &ar);
  const char *GetPriorityClass();
  void Transmitted(ChatChannel channel);
  int GetType() const {return RMTTaskReceived;}
  AIBrain *GetSender() const;

protected:
  RString PrepareSentence(SentenceParams &params) const;
};

class RadioMessageTaskResult : public RadioMessage, public CountInstances<RadioMessageTaskResult>
{
protected:
  Ref<AITask> _task;
  TaskState _state;
  GameValue _result;
  RString _sentence;

public:
  RadioMessageTaskResult(AITask *task, TaskState state, GameValue result, RString sentence)
  {_task = task; _state = state; _result = result; _sentence = sentence;}
  RadioMessageTaskResult() {}
  LSError Serialize(ParamArchive &ar);
  const char *GetPriorityClass();
  void Transmitted(ChatChannel channel);
  int GetType() const {return RMTTaskResult;}
  AIBrain *GetSender() const;

protected:
  RString PrepareSentence(SentenceParams &params) const;
};

#endif // _ENABLE_IDENTITIES

#endif


