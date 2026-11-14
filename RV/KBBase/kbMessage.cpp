#include <El/elementpch.hpp>
#include "kbMessage.hpp"

#include <El/ParamFile/paramFile.hpp>
#include <El/ParamArchive/paramArchive.hpp>

bool KBParameter::CreateArgument(KBArgument &result, GameValuePar value) const
{
  // translation not supported for simple argument
  return true;
}

bool KBParameter::CreateArgument(KBArgument &result, RString value) const
{
  // temporary implementation
  result._name = _name;
  result._text = value;
  return true;
}

KBParameter *KBParameters::Get(RString name) const
{
  for (int i=0; i<Size(); i++)
    if (stricmp(base::Get(i)->GetName(), name) == 0) return base::Get(i);
  return NULL;
}

KBMessageTemplate::KBMessageTemplate(const KBMessageTemplate &src, RString text, GameValuePar speech)
{
  _name = src._name;
  _text = text;
  const GameArrayType &array = speech;
  int n = array.Size();
  _speech.Realloc(n);
  _speech.Resize(n);
  for (int i=0; i<n; i++) _speech[i] = array[i];
  _params = src._params;
}

void KBMessageTemplate::Load(ParamEntryVal cls, ParamEntryVal arguments)
{
  _name = cls.GetName();
  _text = cls >> "text";
  LoadSpeech(_speech, cls >> "speech");
  // _params
  ParamEntryVal args = cls >> "Arguments";
  for (int i=0; i<args.GetEntryCount(); i++)
  {
    ParamEntryVal entry = args.GetEntry(i);
    _params.Add(CreateParameter(entry, arguments));
  }
}

KBParameter *KBMessageTemplate::CreateParameter(ParamEntryVal cls, ParamEntryVal arguments)
{
  RString type = cls >> "type";
  if (stricmp(type, "simple") == 0)
  {
    return new KBParameter(cls.GetName());
  }
  RptF("Unknown argument type: %s", cc_cast(type));
  return new KBParameter(cls.GetName());
}

LSError KBArgument::Serialize(ParamArchive &ar)
{
  CHECK(ar.Serialize("name", _name, 1))
  CHECK(ar.Serialize("value", _value, 1))
  CHECK(ar.Serialize("text", _text, 1))
  CHECK(ar.SerializeArray("speech", _speech, 1))
  return LSOK;
}

const KBArgument *KBArguments::Get(RString name) const
{
  for (int i=0; i<Size(); i++)
    if (stricmp(base::Get(i)._name, name) == 0) return &base::Get(i);
  return NULL;
}

int KBArguments::Set(RString name, GameValuePar value, RString text, const AutoArray<RString> &speech)
{
  for (int i=0; i<Size(); i++)
  {
    KBArgument &arg = base::Set(i);
    if (stricmp(arg._name, name) == 0)
    {
      arg._value = value;
      arg._text = text;
      arg._speech = speech;
      return i;
    }
  }
  KBArgument &arg = Append();
  arg._name = name;
  arg._value = value;
  arg._text = text;
  arg._speech = speech;
  return Size() - 1;
}

int KBArguments::Set(GameValuePar argument)
{
  const GameArrayType &array = argument;
  // size and types should be already checked
  RString name = array[0];
  GameValuePar value = array[1];
  RString text = array[2];
  const GameArrayType &speech = array[3];
  int n = speech.Size();

  int index = -1;
  for (int i=0; i<Size(); i++)
  {
    if (stricmp(base::Get(i)._name, name) == 0)
    {
      index = i;
      break;
    }
  }
  if (index < 0) index = Add();
  KBArgument &arg = base::Set(index);
  arg._name = name;
  arg._value = value;
  arg._text = text;
  arg._speech.Realloc(n);
  arg._speech.Resize(n);
  for (int i=0; i<n; i++) arg._speech[i] = speech[i];
  return index;
}

int KBMessage::SetArgument(GameValuePar argument)
{
  const GameArrayType &array = argument;

  if (array.Size() == 4) return _args.Set(argument);

  Assert(array.Size() == 2);
  // access to parameter
  if (!_type) return -1;
  const KBParameter *parameter = _type->GetParameter( array[0].operator GameStringType() );
  if (!parameter) return -1;

  // create the argument
  KBArgument arg;
  if (!parameter->CreateArgument(arg, array[1])) return -1;

  return SetArgument(arg._name, arg._value, arg._text, arg._speech);
}

class ExtractText : public IKBArgumentFunc
{
public:
  virtual RString operator ()(const KBArgument *arg) const
  {
    return arg->_text;
  }
};

RString KBMessage::GetText() const
{
  ExtractText func;
  return ParseTemplate(_type->GetText(), _args, func);
}

void KBMessage::GetSpeech(AutoArray<RString> &speech) const
{
  FormatSpeech(speech, _type->GetSpeech(), _args);
}

void LoadSpeech(AutoArray<RString> &result, ParamEntryVal entry)
{
  Assert(entry.IsArray());
  int n = entry.GetSize();
  result.Realloc(n);
  result.Resize(n);
  for (int i=0; i<n; i++) result[i] = entry[i];
}

// translate message into sentence
RString ParseTemplate(RString format, const KBArguments &args, const IKBArgumentFunc &func)
{
  RString result;

  const char *ptr = format;
  const char *arg = strchr(ptr, '%');
  while (arg)
  {
    result = result + RString(ptr, arg - ptr);
    ptr = arg + 1;
    if (*ptr == '%')
    {
      result = RString(result, "%");
      ptr++;
    }
    else
    {
      const char *begin = ptr;
      while (*ptr && __iscsym(*ptr)) ptr++;
      RString name(begin, ptr - begin);
      const KBArgument *arg = args.Get(name);
      if (arg)
      {
        result = result + func(arg);
      }
    }

    arg = strchr(ptr, '%');
  }
  result = RString(result, ptr);

  // remove obsolete spaces
  ptr = result;
  RString result2;
  arg = strchr(ptr, ' ');
  while (arg)
  {
    result2 = result2 + RString(ptr, arg - ptr);
    ptr = arg + 1;
    while (*ptr == ' ') ptr++;
    if (*ptr != ',' && *ptr != ';' && *ptr != '.' && *ptr != '!' && *ptr != '?')
    {
      // correct space
      result2 = result2 + RString(" ");
    }
    arg = strchr(ptr, ' ');
  }
  result2 = RString(result2, ptr);
  return result2;
}

RString FormatPhrase(RString format, const KBArguments &args)
{
  ExtractText func;
  return ParseTemplate(format, args, func);
}

void FormatSpeech(AutoArray<RString> &result, const AutoArray<RString> &format, const KBArguments &args)
{
  result.Clear();
  for (int i=0; i<format.Size(); i++)
  {
    RString name = format[i];
    if (name[0] == '%')
    {
      const KBArgument *arg = args.Get((const char *)name + 1);
      if (arg)
      {
        for (int j=0; j<arg->_speech.Size(); j++) result.Add(arg->_speech[j]);
      }
    }
    else
    {
      result.Add(name);
    }
  }
}
