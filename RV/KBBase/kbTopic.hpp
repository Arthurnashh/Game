#ifdef _MSC_VER
#pragma once
#endif

#ifndef _KB_TOPIC_HPP
#define _KB_TOPIC_HPP

#include "kbMessage.hpp"

#include "Es/Containers/rStringArray.hpp"

class KBTopic;

/// Argument value hierarchy node
struct KBUIArgument : public RefCount
{
  RString _value;
  const KBUIArgument *_parent;
  RefArray<const KBUIArgument> _childs;

  KBUIArgument(const KBUIArgument *parent) {_parent = parent;}
  KBUIArgument(const KBUIArgument *parent, ParamEntryVal cls);
};

/// Current argument value selection
struct KBUIArgumentInfo
{
  /// argument name
  RString _name;
  /// text of argument
  RString _text;
  /// default text used when argument is selected and argument value is empty
  RString _default;
  /// speech of argument
  AutoArray<RString> _speech;
  /// current argument value
  Ref<const KBUIArgument> _argument;
};
TypeIsMovable(KBUIArgumentInfo)

/// type of UI item - hint for shortcut selection
enum KBUIType
{
  KBUITNone,
  KBUITTell,
  KBUITParent,
  KBUITChild,
  KBUITPrev,
  KBUITNext
};

/// Base member for description of UI state
class KBUINode : public RefCount
{
public:
  virtual int NChilds() const = 0;
  virtual KBUINode *GetChild(int i) const = 0;

  virtual RString GetCondition() const {return RString();}
  virtual RString GetText() const  {return RString();}
  virtual const AutoArray<int> *GetShortcuts() const {return NULL;}
  virtual KBUIType GetType() const {return KBUITNone;}
  virtual const KBMessage *GetSentence() const  {return NULL;}
};

/// root UI node
class KBUIRoot : public KBUINode
{
protected:
  RefArray<KBUINode> _childs;

public:
  KBUIRoot(const KBTopic *topic, ParamEntryVal cls);

  virtual int NChilds() const {return _childs.Size();}
  virtual KBUINode *GetChild(int i) const {return _childs[i];}
};

/// responding root UI node
class KBUIRespondingRoot : public KBUIRoot
{
protected:
  RString _respondTo;

public:
  KBUIRespondingRoot(const KBTopic *topic, ParamEntryVal cls);

  bool IsRespondingTo(RString to);
};

/// inner UI node
class KBUISubtopic : public KBUIRoot
{
protected:
  RString _condition;
  RString _text;
  AutoArray<int> _shortcuts;

public:
  KBUISubtopic(const KBTopic *topic, ParamEntryVal cls);

  virtual RString GetCondition() const {return _condition;}
  virtual RString GetText() const  {return _text;}
  virtual const AutoArray<int> *GetShortcuts() const {return &_shortcuts;}
};

/// leaf UI node
class KBUISentence : public KBUINode
{
protected:
  RString _condition;
  RString _text;
  AutoArray<int> _shortcuts;
  Ref<const KBMessageTemplate> _template;
  AutoArray<KBUIArgumentInfo> _arguments;
  int _selectedArgument; // index of argument we are selecting
  KBUIType _type; // shortcut hint

public:
  KBUISentence(const KBTopic *topic, ParamEntryVal cls);
  KBUISentence(const KBUISentence &src);

  virtual int NChilds() const;
  virtual KBUINode *GetChild(int i) const;

  virtual RString GetCondition() const {return _condition;}
  virtual RString GetText() const;
  virtual const AutoArray<int> *GetShortcuts() const {return &_shortcuts;}
  virtual KBUIType GetType() const {return _type;}
  virtual const KBMessage *GetSentence() const;

protected:
  // implementation
  void PrepareArguments(KBArguments &arguments) const;
};

/// Argument value selection description
struct KBUIArgumentDesc
{
  /// config name
  RString _config;
  /// current argument value
  Ref<const KBUIArgument> _argument;

  void Load(ParamEntryVal cls);
};
TypeIsMovableZeroed(KBUIArgumentDesc)

struct SpecialWord
{
  RString _name;
  RString _textBeforeVocal;
  AutoArray<RString> _speechBeforeVocal;
  RString _textBeforeConsonant;
  AutoArray<RString> _speechBeforeConsonant;

  void Load(ParamEntryVal cls);
};
TypeIsMovable(SpecialWord)

/// Conversation topic
class KBTopic : public RefCount
{
protected:
  /// unique id of theme
  RString _name;
  /// name of definition file
  RString _filename;
  /// name of handler responsible to AI reactions
  RString _handlerAI;
  /// name of handler responsible to player reactions
  GameValue _handlerPlayer;

  /// message templates (types)
  RefArray<const KBMessageTemplate> _templates;

  /// default UI root
  Ref<KBUIRoot> _uiDefault;

  /// responding UI roots
  RefArray<KBUIRespondingRoot> _uiResponding;

  /// UI description of arguments
  AutoArray<KBUIArgumentDesc> _uiArguments;

  // special words
  AutoArray<SpecialWord> _special;
  // exceptions
  FindArrayRStringCI _startWithVocal;
  FindArrayRStringCI _startWithConsonant;

public:
  KBTopic() {}
  KBTopic(ForSerializationOnly) {}

  /// second phase of creation (need to be able to call virtual functions)
  virtual void Init(RString name, RString filename, RString handlerAI, GameValuePar handlerPlayer);

  virtual KBMessageTemplate *CreateTemplate() const;
  virtual KBMessageTemplate *CreateTemplate(const KBMessageTemplate &src, RString text, GameValuePar speech) const;

  // access to members
  RString GetName() const {return _name;}
  RString GetFilename() const {return _filename;}
  RString GetHandlerAI() const {return _handlerAI;}
  const GameValue &GetHandlerPlayer() const {return _handlerPlayer;}
  
  // translation of messages
  RString GetText(const KBMessage *message) const;
  void GetSpeech(AutoArray<RString> &speech, const KBMessage *message) const;

  /// replace the special words, like "#article"
  void CheckMessage(KBMessage *message) const;

  // UI support
  const KBUINode *GetUIRoot(const KBMessage *message) const;
  int NChilds(const KBUINode *node) const;
  const KBUINode *GetChild(const KBUINode *node, int i) const;
  RString GetCondition(const KBUINode *node) const;
  RString GetText(const KBUINode *node) const;
  const KBMessage *GetSentence(const KBUINode *node) const;

  const KBMessageTemplate *FindMessageTemplate(RString name) const;
  const KBUIArgument *FindUIArgument(RString name) const;

  // serialization
  // enable dynamic serialization
  DECL_SERIALIZE_TYPE_INFO_ROOT(KBTopic);
  // instances of DiaryPage are valid
  DECL_SERIALIZE_TYPE_INFO(KBTopic, KBTopic);

  LSError Serialize(ParamArchive &ar);

protected:
  // implementation
  void ReplaceSpecial(const KBMessage *message, KBArgument &argument, const SpecialWord &special) const;
  bool StartWithVocal(RString word) const;

  void Load(ParamEntryVal cls);
};

#endif
