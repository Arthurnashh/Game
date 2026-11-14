#ifdef _MSC_VER
#pragma once
#endif

#ifndef _SENTENCES_HPP
#define _SENTENCES_HPP

class SentenceParam : public RefCount
{
public:
  SentenceParam() {}
  virtual ~SentenceParam() {}

  virtual void Say(RadioSentence &sentence, const RadioChannelParams &cfg, int mode) const = 0;
  virtual RString Write(int mode) const = 0;
};

#include <Es/Memory/normalNew.hpp>

/// text will be spelled
class SentenceParamText : public SentenceParam
{
private:
  RString _text;

public:
  SentenceParamText(RString text) : _text(text) {}

  virtual void Say(RadioSentence &sentence, const RadioChannelParams &cfg, int mode) const;
  virtual RString Write(int mode) const;

  USE_FAST_ALLOCATOR
};

/// number will be spelled
class SentenceParamNumber : public SentenceParam
{
private:
  int _number;

public:
  SentenceParamNumber(int number) : _number(number) {}

  virtual void Say(RadioSentence &sentence, const RadioChannelParams &cfg, int mode) const;
  virtual RString Write(int mode) const;

  USE_FAST_ALLOCATOR
};

/// number will be spelled in selected format
class SentenceParamFormat : public SentenceParam
{
private:
  RString _format;
  int _number;

public:
  SentenceParamFormat(RString format, int number) : _format(format), _number(number) {}

  virtual void Say(RadioSentence &sentence, const RadioChannelParams &cfg, int mode) const;
  virtual RString Write(int mode) const;

  USE_FAST_ALLOCATOR
};

/// single word
class SentenceParamWordText : public SentenceParam
{
private:
  RString _say;
  RString _write;

public:
  SentenceParamWordText(RString say, RString write) : _say(say), _write(write) {}

  virtual void Say(RadioSentence &sentence, const RadioChannelParams &cfg, int mode) const;
  virtual RString Write(int mode) const;

  USE_FAST_ALLOCATOR
};

/// few words
class SentenceParamPhrase : public SentenceParam
{
private:
  AutoArray<RString> _say;
  RString _write;

public:
  SentenceParamPhrase(const AutoArray<RString> &say, RString write) : _say(say), _write(write) {}

  virtual void Say(RadioSentence &sentence, const RadioChannelParams &cfg, int mode) const;
  virtual RString Write(int mode) const;

  USE_FAST_ALLOCATOR
};

#if _ENABLE_INDEPENDENT_AGENTS
/// Team member callsign
class SentenceParamTeamMember : public SentenceParam
{
private:
  AITeamMemberLink _member;

public:
  SentenceParamTeamMember(AITeamMember *member) : _member(member) {}

  virtual void Say(RadioSentence &sentence, const RadioChannelParams &cfg, int mode) const;
  virtual RString Write(int mode) const;

  USE_FAST_ALLOCATOR
};

/// Common argument (interpreted by mode)
class SentenceParamGameValue : public SentenceParam
{
private:
  GameValue _value;
  AITeamMemberLink _sender;
  AITeamMemberLink _receiver;

public:
  SentenceParamGameValue(GameValuePar value, AITeamMember *sender, AITeamMember *receiver)
    : _value(value), _sender(sender), _receiver(receiver) {}

  virtual void Say(RadioSentence &sentence, const RadioChannelParams &cfg, int mode) const;
  virtual RString Write(int mode) const;

  USE_FAST_ALLOCATOR
};
#endif

class SentenceParamDistance : public SentenceParam
{
private:
  enum {ModeDistMeters=1};
  float         _distance;        // distance to target (can be from GroupCore or Sender or Unit or Location)
  const float (&_distLevels)[2];  // distance levels to distinguish between close, far
  const RadioMessage *_msg;             // needed to get the radio protocol used

  ParamEntryPtr GetVariantCls(int mode) const;
public:
  SentenceParamDistance(float distance, const float (&distLevels)[2], const RadioMessage *msg)
    : _distance(distance), _distLevels(distLevels), _msg(msg) {}

  virtual void Say(RadioSentence &sentence, const RadioChannelParams &cfg, int mode) const;
  virtual RString Write(int mode) const;

  USE_FAST_ALLOCATOR
};

class SentenceParamDirection : public SentenceParam
{
private:
  enum {ModeDirCompass=1, ModeDirHours};
  bool          _forceCompass;    // to force compass directions (north, north-east,...)
  // direction is always the atan2
  float         _relDir;          // for non compass direction (examples: left/right, at 5 o'clock)
  float         _absDir;          // for compass directions
  const RadioMessage *_msg;             // needed to get the radio protocol used

  ParamEntryPtr GetVariantCls(int mode) const;
public:
  SentenceParamDirection(const RadioMessage *msg, float relDir, float absDir, bool forceCompass)
    : _msg(msg), _forceCompass(forceCompass), _relDir(relDir), _absDir(absDir) {}

  virtual void Say(RadioSentence &sentence, const RadioChannelParams &cfg, int mode) const;
  virtual RString Write(int mode) const;

  USE_FAST_ALLOCATOR
};

class SentenceParamLocation : public SentenceParam
{
private:
  const Location *_location;
public:
  SentenceParamLocation(const Location *location)
    : _location(location) {}

  virtual void Say(RadioSentence &sentence, const RadioChannelParams &cfg, int mode) const;
  virtual RString Write(int mode) const;

  USE_FAST_ALLOCATOR
};

struct AggrTargetWord
{
  const EntityAIType* _type;
  bool _singular;
  AggrTargetWord() {}
  AggrTargetWord(const EntityAIType *type, bool singular) : _type(type), _singular(singular) {}
};
TypeIsMovable(AggrTargetWord);

class SentenceParamAggrTargets : public SentenceParam
{
private:
  AutoArray<AggrTargetWord> _speechList;
  bool  _singular;
  RString _text;
  const RadioMessage *_msg;             // needed to get the radio protocol used

  RString GetVariantCls(int mode) const;
public:
  SentenceParamAggrTargets(AutoArray<AggrTargetWord> &speechList, RString text, const RadioMessage *msg) : _text(text), _speechList(speechList), _msg(msg) {}

  virtual void Say(RadioSentence &sentence, const RadioChannelParams &cfg, int mode) const;
  virtual RString Write(int mode) const;

  USE_FAST_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>

class SentenceParams : public StaticArray< Ref<SentenceParam> >
{
	typedef StaticArray< Ref<SentenceParam> > base;

public:
	int Add(RString text)
	{
		return base::Add(new SentenceParamText(text));
	}
	int Add(int number)
	{
    return base::Add(new SentenceParamNumber(number));
	}
	int Add(RString format, int number)
	{
    return base::Add(new SentenceParamFormat(format, number));
	}

	void AddWord(RString say, RString write)
	{
    base::Add(new SentenceParamWordText(say, write));
	}
  void AddPhrase(const AutoArray<RString> &say, RString write)
  {
    base::Add(new SentenceParamPhrase(say, write));
  }
	void AddAzimutRelDir(Vector3Par dir);
	void AddAzimutDir(Vector3Par dir);
	void AddAzimut(Vector3Par pos);
	void AddDistance(float dist);
	void AddRelativePosition(Vector3Par dir);

  // Report target helper parameters
  void AddDistance(float dist, int martaTypeIx, const RadioMessage *msg);
  void AddDirection(float relDir, float absDir, bool forceCompass, const RadioMessage *msg);
  void AddLocation(const Location *location);
  void AddAggrTargets(AutoArray<AggrTargetWord> &speechList, RString text, const RadioMessage *msg);

#if _ENABLE_INDEPENDENT_AGENTS
  void Add(AITeamMember *member)
  {
    base::Add(new SentenceParamTeamMember(member));
  }
  void Add(GameValuePar value, AITeamMember *sender, AITeamMember *receiver)
  {
    base::Add(new SentenceParamGameValue(value, sender, receiver));
  }
#endif
};

#endif
