#include <El/elementpch.hpp>
#include <El/ParamFile/paramFile.hpp>
#include <El/ParamArchive/paramArchive.hpp>

#include "kbTopic.hpp"

extern ParamFile Pars;

//////////////////////////////////////////////////////////////////////////
//
// UI elements
//

KBUIRoot::KBUIRoot(const KBTopic *topic, ParamEntryVal cls)
{
  for (int i=0; i<cls.GetEntryCount(); i++)
  {
    ParamEntryVal entry = cls.GetEntry(i);
    if (!entry.IsClass()) continue;
    // which type of child
    ConstParamEntryPtr sentence = entry.FindEntry("sentence");
    if (sentence)
      // leaf
      _childs.Add(new KBUISentence(topic, entry));
    else
      // inner node
      _childs.Add(new KBUISubtopic(topic, entry));
  }
  _childs.Compact();
}

KBUIRespondingRoot::KBUIRespondingRoot(const KBTopic *topic, ParamEntryVal cls)
: KBUIRoot(topic, cls)
{
  _respondTo = cls.GetName();
}

bool KBUIRespondingRoot::IsRespondingTo(RString to)
{
  return stricmp(to, _respondTo) == 0;
}

KBUISubtopic::KBUISubtopic(const KBTopic *topic, ParamEntryVal cls)
: KBUIRoot(topic, cls)
{
  _condition = cls >> "show";
  _text = cls >> "text";

  ConstParamEntryPtr entry = cls.FindEntry("shortcuts");
  if (entry)
  {
    int n = entry->GetSize();
    _shortcuts.Realloc(n);
    _shortcuts.Resize(n);
    for (int i=0; i<n; i++) _shortcuts[i] = (*entry)[i];
  }
}

KBUISentence::KBUISentence(const KBTopic *topic, ParamEntryVal cls)
{
  _condition = cls >> "show";
  _text = cls >> "text";

  ConstParamEntryPtr entry = cls.FindEntry("shortcuts");
  if (entry)
  {
    int n = entry->GetSize();
    _shortcuts.Realloc(n);
    _shortcuts.Resize(n);
    for (int i=0; i<n; i++) _shortcuts[i] = (*entry)[i];
  }

  _template = topic->FindMessageTemplate(cls >> "sentence");

  // avoid duplicate writing of text
  if (_text.GetLength() == 0 && _template) _text = _template->GetText();

  ParamEntryVal arguments = cls >> "Arguments";
  for (int i=0; i<arguments.GetEntryCount(); i++)
  {
    ParamEntryVal entry = arguments.GetEntry(i);
    KBUIArgumentInfo &argument = _arguments.Append();
    argument._name = entry.GetName();
    ConstParamEntryPtr check = entry.FindEntry("config");
    if (check)
      argument._argument = topic->FindUIArgument(*check);
    else
    {
      KBUIArgument *root = new KBUIArgument(NULL);
      const KBParameter *parameter = _template->GetParameter(argument._name);
      if (parameter)
      {
        AutoArray<RString> values;
        parameter->GetValues(values);
        int n = values.Size();
        root->_childs.Realloc(n);
        root->_childs.Resize(n);
        for (int i=0; i<n; i++)
        {
          KBUIArgument *child = new KBUIArgument(root);
          child->_value = values[i];
          root->_childs[i] = child;
        }
      }
      argument._argument = root;
    }
    check = entry.FindEntry("default");
    if (check)
      argument._default = *check;
    check = entry.FindEntry("text");
    if (check)
      argument._text = *check;
    check = entry.FindEntry("speech");
    if (check)
      LoadSpeech(argument._speech, *check);
  }
  _arguments.Compact();
  if (_arguments.Size() > 0)
  {
    _selectedArgument = 0;
    _type = KBUITNone;
  }
  else
  {
    _selectedArgument = -1;
    _type = KBUITTell;
  }
}

KBUISentence::KBUISentence(const KBUISentence &src)
{
  _condition = src._condition;
  _text = src._text;
  _shortcuts = src._shortcuts;
  _template = src._template;
  _arguments = src._arguments;
  _selectedArgument = src._selectedArgument;
  _type = src._type;
}

int KBUISentence::NChilds() const
{
  if (_selectedArgument == -1) return 0;
  int count = 1; // tell
  const KBUIArgument *argument = _arguments[_selectedArgument]._argument;
  if (argument)
  {
    count += argument->_childs.Size(); // childs
    if (argument->_parent) count++; // parent
  }
  if (_arguments.Size() > 1) count++; // prev
  if (_arguments.Size() > 2) count++; // next != prev
  return count;
}

KBUINode *KBUISentence::GetChild(int i) const
{
  if (_selectedArgument == -1) return NULL;
  if (i-- == 0)
  {
    // tell
    KBUISentence *child = new KBUISentence(*this);
    child->_selectedArgument = -1;
    child->_type = KBUITTell;
    return child;
  }
  const KBUIArgument *argument = _arguments[_selectedArgument]._argument;
  if (argument)
  {
    if (i < argument->_childs.Size())
    {
      // child
      KBUISentence *child = new KBUISentence(*this);
      child->_arguments[_selectedArgument]._argument = argument->_childs[i];
      child->_type = KBUITChild;
      return child;
    }
    i -= argument->_childs.Size();
    if (argument->_parent && i-- == 0)
    {
      // parent
      KBUISentence *child = new KBUISentence(*this);
      child->_arguments[_selectedArgument]._argument = argument->_parent;
      child->_type = KBUITParent;
      return child;
    }
  }
  if (_arguments.Size() > 1 && i-- == 0)
  {
    // prev
    KBUISentence *child = new KBUISentence(*this);
    child->_selectedArgument--;
    if (child->_selectedArgument < 0) child->_selectedArgument = _arguments.Size() - 1;
    child->_type = KBUITPrev;
    return child;
  }
  if (_arguments.Size() > 2 && i-- == 0)
  {
    // next
    KBUISentence *child = new KBUISentence(*this);
    child->_selectedArgument++;
    if (child->_selectedArgument >= _arguments.Size()) child->_selectedArgument = 0;
    child->_type = KBUITNext;
    return child;
  }
  return NULL;
}

class ExtractUIText : public IKBArgumentFunc
{
protected:
  RString _name;
  RString _default;

public:
  ExtractUIText(RString name, RString def) : _name(name), _default(def) {}
  virtual RString operator ()(const KBArgument *arg) const
  {
    if (stricmp(arg->_name, _name) == 0)
    {
      RString text = arg->_text;
      if (text.GetLength() == 0) text = _default;
      return RString("<t color=\"#cc9900\">") + text + RString("</t>");
    }
    else
    {
      return arg->_text;
    }
  }
};

void KBUISentence::PrepareArguments(KBArguments &arguments) const
{
  for (int i=0; i<_arguments.Size(); i++)
  {
    const KBUIArgumentInfo &info = _arguments[i];
    if (info._argument == NULL) continue;

    const KBParameter *parameter = _template->GetParameter(info._name);
    if (!parameter) continue;

    KBArguments temp;
    KBArgument &argument = temp.Append();
    parameter->CreateArgument(argument, info._argument->_value);
    if (argument._text.GetLength() == 0 || info._text.GetLength() == 0)
    {
      // no additional formatting
      arguments.Set(argument._name, argument._value, argument._text, argument._speech);
    }
    else
    {
      // process additional formatting
      RString text = FormatPhrase(info._text, temp);
      AutoArray<RString> speech;
      FormatSpeech(speech, info._speech, temp);
      arguments.Set(argument._name, argument._value, text, speech);
    }
  }
  arguments.Compact();
}

RString KBUISentence::GetText() const
{
  RString name, def;
  if (_selectedArgument >= 0)
  {
    name = _arguments[_selectedArgument]._name;
    def = _arguments[_selectedArgument]._default;
  }

  ExtractUIText func(name, def);  

  KBArguments arguments;
  PrepareArguments(arguments);

  return ParseTemplate(_text, arguments, func);
}

const KBMessage *KBUISentence::GetSentence() const
{
  if (_selectedArgument == -1)
  {
    KBMessage *message = new KBMessage(_template);
    PrepareArguments(message->GetArguments());
    return message;
  }
  else return NULL;
}

KBUIArgument::KBUIArgument(const KBUIArgument *parent, ParamEntryVal cls)
{
  _parent = parent;
  _value = cls >> "value";
  for (int i=0; i<cls.GetEntryCount(); i++)
  {
    ParamEntryVal entry = cls.GetEntry(i);
    if (!entry.IsClass()) continue;
    _childs.Add(new KBUIArgument(this, entry));
  }
  _childs.Compact();
}

void KBUIArgumentDesc::Load(ParamEntryVal cls)
{
  _config = cls.GetName();
  _argument = new KBUIArgument(NULL, cls);
}

void SpecialWord::Load(ParamEntryVal cls)
{
  _name = cls.GetName();
  _textBeforeVocal = cls >> "textBeforeVocal";
  LoadSpeech(_speechBeforeVocal, cls >> "speechBeforeVocal");
  _textBeforeConsonant = cls >> "textBeforeConsonant";
  LoadSpeech(_speechBeforeConsonant, cls >> "speechBeforeConsonant");
}

//////////////////////////////////////////////////////////////////////////
//
// KBTopic
//

void KBTopic::Init(RString name, RString filename, RString handlerAI, GameValuePar handlerPlayer)
{
  _name = name;
  _filename = filename;
  _handlerAI = handlerAI;
  _handlerPlayer = handlerPlayer;

  if (_filename.GetLength() > 0 && _filename[0] == '#')
  {
    Load(Pars >> "CfgTalkTopics" >> _filename.Substring(1, INT_MAX));
  }
  else
  {
    GameDataNamespace globals(NULL, RString(), false); // TODO: parsing namespace if access to globals needed

    ParamFile file;
    if (file.ParseBinOrTxt(_filename, NULL, NULL, &globals)) Load(file);
  }
}

void KBTopic::Load(ParamEntryVal cls)
{
  // sentences
  ParamEntryVal sentences = cls >> "Sentences";
  ParamEntryVal arguments = cls >> "Arguments";
  for (int i=0; i<sentences.GetEntryCount(); i++)
  {
    KBMessageTemplate *messageTemplate = CreateTemplate();
    if (messageTemplate)
    {
      messageTemplate->Load(sentences.GetEntry(i), arguments);
      _templates.Add(messageTemplate);
    }
  }
  _templates.Compact();

  // UI
  ConstParamEntryPtr ui = cls.FindEntry("UI");
  if (ui)
  {
    // UI - parameters
    ParamEntryVal arguments = (*ui) >> "Arguments";
    for (int i=0; i<arguments.GetEntryCount(); i++)
    {
      ParamEntryVal entry = arguments.GetEntry(i);
      _uiArguments.Append().Load(entry);
    }
    _uiArguments.Compact();

    // UI - responses
    ParamEntryVal responses = (*ui) >> "Responses";
    for (int i=0; i<responses.GetEntryCount(); i++)
    {
      ParamEntryVal entry = responses.GetEntry(i);
      if (stricmp(entry.GetName(), "default") == 0)
      {
        Assert(!_uiDefault);
        _uiDefault = new KBUIRoot(this, entry);
      }
      else
      {
        _uiResponding.Add(new KBUIRespondingRoot(this, entry));
      }
    }
    _uiResponding.Compact();
  }

  // language exceptions

  ParamEntryVal array = cls >> "startWithVocal";
  int n = array.GetSize();
  _startWithVocal.Realloc(n);
  _startWithVocal.Resize(n);
  for (int i=0; i<n; i++)
    _startWithVocal[i] = array[i];

  array = cls >> "startWithConsonant";
  n = array.GetSize();
  _startWithConsonant.Realloc(n);
  _startWithConsonant.Resize(n);
  for (int i=0; i<array.GetSize(); i++)
    _startWithConsonant[i] = array[i];

  ParamEntryVal special = cls >> "Special";
  for (int i=0; i<special.GetEntryCount(); i++)
  {
    ParamEntryVal entry = special.GetEntry(i);
    if (entry.IsClass())
      _special.Append().Load(entry);
  }
  _special.Compact();
}

KBMessageTemplate *KBTopic::CreateTemplate() const
{
  return new KBMessageTemplate();
}

KBMessageTemplate *KBTopic::CreateTemplate(const KBMessageTemplate &src, RString text, GameValuePar speech) const
{
  return new KBMessageTemplate(src, text, speech);
}

RString KBTopic::GetText(const KBMessage *message) const
{
  return message->GetText();
}

void KBTopic::GetSpeech(AutoArray<RString> &speech, const KBMessage *message) const
{
  message->GetSpeech(speech);
}

void KBTopic::CheckMessage(KBMessage *message) const
{
  KBArguments &arguments = message->GetArguments();
  for (int i=arguments.Size()-1; i>=0; i--) // from the back
  {
    KBArgument &argument = arguments[i];
    if (argument._text[0] == '#')
    {
      const char *name = cc_cast(argument._text) + 1;
      for (int i=0; i<_special.Size(); i++)
      {
        const SpecialWord &special = _special[i];
        if (stricmp(special._name, name) == 0)
        {
          ReplaceSpecial(message, argument, special);
          break;
        }
      }
    }
  }
}

bool KBTopic::StartWithVocal(RString word) const
{
  Assert(word.GetLength() > 0);
  switch (word[0])
  {
  case 'a':
  case 'e':
  case 'i':
  case 'o':
  case 'u':
    // vocal, not found in exceptions
    return _startWithConsonant.Find(word) < 0;
  default:
    // consonant, found in exceptions
    return _startWithVocal.Find(word) >= 0;
  }
}

#define ISSPACE(c) ((c) >= 0 && (c) <= 32)

RString GetWord(const char *&text)
{
  // skip white spaces
  while (*text && ISSPACE(*text)) text++;

  // get the word
  const char *ptr = text;
  while (*ptr && !ISSPACE(*ptr)) ptr++;
  RString word(text, ptr - text);

  text = ptr;
  return word;
}

void KBTopic::ReplaceSpecial(const KBMessage *message, KBArgument &argument, const SpecialWord &special) const
{
  if (!message->GetType()) return;

  const char *text = message->GetType()->GetText();
  RString pattern = RString("%") + argument._name;
  while (*text)
  {
    RString word = GetWord(text);
    if (stricmp(word, pattern) == 0)
    {
      // search for the next word
      RString nextWord;
      while (*text && nextWord.GetLength() == 0)
      {
        nextWord = GetWord(text);
        if (nextWord[0] == '%')
        {
          const KBArgument *arg = message->GetArgument(cc_cast(nextWord) + 1);
          if (arg)
          {
            const char *argText = arg->_text;
            nextWord = GetWord(argText);
          }
        }
      }
      if (nextWord.GetLength() > 0)
      {
        if (StartWithVocal(nextWord))
        {
          argument._text = special._textBeforeVocal;
          argument._speech = special._speechBeforeVocal;
        }
        else
        {
          argument._text = special._textBeforeConsonant;
          argument._speech = special._speechBeforeConsonant;
        }
      }
      return;
    }
  }
}

const KBUINode *KBTopic::GetUIRoot(const KBMessage *message) const
{
  if (!message) return _uiDefault;

  const KBMessageTemplate *type = message->GetType();
  if (!type) return _uiDefault;

  RString name = type->GetName();
  if (name.GetLength() > 0)
  {
    for (int i=0; i<_uiResponding.Size(); i++)
    {
      KBUIRespondingRoot *node = _uiResponding[i];
      if (node->IsRespondingTo(name)) return node;
    }
  }
  return _uiDefault;
}

int KBTopic::NChilds(const KBUINode *node) const
{
  return node ? node->NChilds() : 0;
}

const KBUINode *KBTopic::GetChild(const KBUINode *node, int i) const
{
  return node ? node->GetChild(i) : NULL;
}

RString KBTopic::GetCondition(const KBUINode *node) const
{
  return node ? node->GetCondition() : RString();
}

RString KBTopic::GetText(const KBUINode *node) const
{
  return node ? node->GetText() : RString();
}

const KBMessage *KBTopic::GetSentence(const KBUINode *node) const
{
  return node ? node->GetSentence() : NULL;
}

const KBMessageTemplate *KBTopic::FindMessageTemplate(RString name) const
{
  for (int i=0; i<_templates.Size(); i++)
  {
    const KBMessageTemplate *templ = _templates[i];
    if (stricmp(templ->GetName(), name) == 0) return templ;
  }
  return NULL;
}

const KBUIArgument *KBTopic::FindUIArgument(RString name) const
{
  for (int i=0; i<_uiArguments.Size(); i++)
  {
    if (stricmp(_uiArguments[i]._config, name) == 0) return _uiArguments[i]._argument;
  }
  return NULL;
}

DEFINE_SERIALIZE_TYPE_INFO_ROOT(KBTopic);
DEFINE_SERIALIZE_TYPE_INFO(KBTopic, KBTopic);

LSError KBTopic::Serialize(ParamArchive &ar)
{
  SERIALIZE_TYPE_INFO_ROOT(KBTopic);

  CHECK(ar.Serialize("name", _name, 1, RString()))
  CHECK(ar.Serialize("filename", _filename, 1, RString()))
  if (ar.IsLoading() && ar.GetPass() == ParamArchive::PassFirst)
  {
    if (_filename.GetLength() > 0 && _filename[0] == '#')
    {
      Load(Pars >> "CfgTalkTopics" >> _filename.Substring(1, INT_MAX));
    }
    else
    {
      GameDataNamespace globals(NULL, RString(), false); // TODO: parsing namespace if access to globals needed

      ParamFile file;
      if (file.Parse(_filename, NULL, NULL, &globals) == LSOK) Load(file);
    }
  }

  CHECK(ar.Serialize("handlerAI", _handlerAI, 1, RString()))
  
  void *oldParams = ar.GetParams();
  ar.SetParams(&GGameState);

  CHECK(ar.Serialize("handlerPlayer", _handlerPlayer, 1, GameValue()))

  // serialization of variables finished
  ar.SetParams(oldParams);
  return LSOK;
}
