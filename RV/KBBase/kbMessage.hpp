#ifdef _MSC_VER
#pragma once
#endif

#ifndef _KB_MESSAGE_HPP
#define _KB_MESSAGE_HPP

#include <Es/Strings/rString.hpp>
#include <Es/Containers/array.hpp>

#include <El/Evaluator/expressImpl.hpp>
class ParamEntryVal;

struct KBArgument;

/// parameter - argument type
class KBParameter : public RefCount
{
protected:
  /// unique identification
  RString _name;

public:
  KBParameter(RString name) {_name = name;}
  RString GetName() const {return _name;}

  /// collect all values of parameter for UI
  virtual void GetValues(AutoArray<RString> &values) const {}

  /// fill the argument structure based on parameter type
  virtual bool CreateArgument(KBArgument &result, GameValuePar value) const;

  /// fill the argument structure based on parameter type
  virtual bool CreateArgument(KBArgument &result, RString value) const;
};

/// list of parameters
class KBParameters : public RefArray<KBParameter> 
{
  typedef RefArray<KBParameter> base;

public:
  KBParameter *Get(RString name) const;
};

/// message template (type)
class KBMessageTemplate : public RefCount
{
protected:
  /// unique ID of template
  RString _name;
  /// text of template
  RString _text;
  /// speech of template
  AutoArray<RString> _speech;
  /// parameters
  KBParameters _params;

public:
  KBMessageTemplate() {}
  KBMessageTemplate(const KBMessageTemplate &src, RString text, GameValuePar speech);
  /// load form config definition
  void Load(ParamEntryVal cls, ParamEntryVal arguments);
  /// create a parameter depending on its type
  virtual KBParameter *CreateParameter(ParamEntryVal cls, ParamEntryVal arguments);

  // access to variables
  RString GetName() const {return _name;}
  RString GetText() const {return _text;}
  const AutoArray<RString> &GetSpeech() const {return _speech;}
  int NParameters() const {return _params.Size();}
  const KBParameter *GetParameter(int i) const {return _params[i];}
  const KBParameter *GetParameter(RString name) const {return _params.Get(name);}
};

/// argument instance
struct KBArgument
{
  RString _name;
  GameValue _value;
  RString _text;
  AutoArray<RString> _speech;

  LSError Serialize(ParamArchive &ar);
};
TypeIsMovable(KBArgument)

/// list of arguments
class KBArguments : public AutoArray<KBArgument>
{
  typedef AutoArray<KBArgument> base;

public:
  const KBArgument *Get(RString name) const;
  int Set(RString name, GameValuePar value, RString text, const AutoArray<RString> &speech);
  int Set(GameValuePar argument);
};

/// message instance
class KBMessage : public RefCount
{
protected:
  Ref<const KBMessageTemplate> _type;
  KBArguments _args;

public:
  KBMessage(const KBMessageTemplate *type) {_type = type;}

  // access to variables
  const KBMessageTemplate *GetType() const {return _type;}
  KBArguments &GetArguments() {return _args;}
  int NArguments() const {return _args.Size();}
  const KBArgument &GetArgument(int i) const {return _args[i];}
  const KBArgument *GetArgument(RString name) const
  {
    return _args.Get(name);
  }
  int SetArgument(RString name, GameValuePar value, RString text, const AutoArray<RString> &speech)
  {
    return _args.Set(name, value, text, speech);
  }
  int SetArgument(GameValuePar argument);

  // translate message into sentence
  RString GetText() const;
  void GetSpeech(AutoArray<RString> &speech) const;
};

void LoadSpeech(AutoArray<RString> &result, ParamEntryVal entry);

class IKBArgumentFunc
{
public:
  virtual RString operator ()(const KBArgument *arg) const = 0;
};

RString ParseTemplate(RString format, const KBArguments &args, const IKBArgumentFunc &func);

RString FormatPhrase(RString format, const KBArguments &args);
void FormatSpeech(AutoArray<RString> &result, const AutoArray<RString> &format, const KBArguments &args);

#endif
