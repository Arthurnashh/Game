#include <El/elementpch.hpp>
#include "iExtParser.hpp"

#include <El/QStream/qbStream.hpp>
#include <El/XML/xml.hpp>
#include "stringtableImpl.hpp"
#include "stringtableParserXML.hpp"

/// Implementation of extended parser enabling stringtable stored in the XML file
class StringtableXMLParserFunctions : public StringtableExtParserFunctions
{
public:
  virtual bool Open(const char *filename, QIFStreamB &in);
  virtual bool Open(QFBank &bank, const char *filename, QIFStreamB &in);
  virtual bool Parse(QIStream &in, StringTableDynamic &stringtable, RString language);
};

bool StringtableXMLParserFunctions::Open(const char *filename, QIFStreamB &in)
{
  const char *ext = strrchr(filename, '.');
  RString filenameXML(filename);
  if (ext && stricmp(ext, ".csv") == 0)
  {
    filenameXML = RString(filename, ext - filename) + RString(".xml");
  }

  if (QFBankQueryFunctions::FileExists(filenameXML))
  {
    in.AutoOpen(filenameXML);
    return true;
  }

  return false;
}

bool StringtableXMLParserFunctions::Open(QFBank &bank, const char *filename, QIFStreamB &in)
{
  const char *ext = strrchr(filename, '.');
  if (ext && stricmp(ext, ".csv") == 0)
  {
    RString filenameXML = RString(filename, ext - filename) + RString(".xml");
    if (bank.FileExists(filename))
    {
      in.open(bank, filenameXML);
      return true;
    }
  }
  return false;
}

/// what column was assigned to the selected language
enum LanguageMatch
{
  LMNone,
  LMFirst,
  LMEnglish,
  LMOriginal,
  LMCorrect
};

/// XML parser used to process stringtable
class StringtableParser : public StringtableParserXMLBase
{
public:
  StringtableParser(StringTableDynamic &stringtable, RString language);
  LanguageMatch GetResult() const {return _tableWorst;}

protected:
  //@{ Implementation of StringtableParserXMLBase methods
  /// called when tag <Text ID=..> or <Key ID=..> starts
  virtual void OnStartElementID(RString name);
  /// called when language tag (<English>,..) starts
  virtual void OnStartElementLanguage(RString name);
  /// called when tag <Text ID=..> or <Key ID=..> ends
  virtual void OnEndElementID(RString name);
  /// called when language tag (<English>,..) ends
  virtual void OnEndElementLanguage(RString name);
  //@}

  /// Check if _lastText can be used as _text
  void HandleLastText();

private:
  StringTableDynamic &_stringtable;
  /// Language we want to load
  RString _currentLanguage;
  /// Best text found for the current id (corresponding to _lineBest)
  RString _text;

  /// The best language matching for the current id
  LanguageMatch _lineBest;
  /// The worst language matching for the whole table
  LanguageMatch _tableWorst;
};

StringtableParser::StringtableParser(StringTableDynamic &stringtable, RString language)
: StringtableParserXMLBase(), _stringtable(stringtable), _currentLanguage(language), _lineBest(LMNone), _tableWorst(LMCorrect)
{
}

void StringtableParser::OnStartElementID(RString name)
{
  _text = RString();
  _lineBest = LMNone;
}

void StringtableParser::OnStartElementLanguage(RString name)
{
  // nothing needed here
}

void StringtableParser::OnEndElementID(RString name)
{
  if (_lineBest < _tableWorst)
  {
    _tableWorst = _lineBest;
  }
  _stringtable.Add(GetId(), _text);
}

void StringtableParser::OnEndElementLanguage(RString name)
{
  // check the languages match by priority
  if (stricmp(GetLanguage(), _currentLanguage) == 0)
  {
    _text = GetLastText();
    _lineBest = LMCorrect;
    return;
  }
  if (_lineBest == LMCorrect) return;

  if (stricmp(GetLanguage(), "Original") == 0)
  {
    _text = GetLastText();
    _lineBest = LMOriginal;
    return;
  }
  if (_lineBest == LMOriginal) return;

  if (stricmp(GetLanguage(), "English") == 0)
  {
    _text = GetLastText();
    _lineBest = LMEnglish;
    return;
  }
  if (_lineBest == LMEnglish) return;

  if (_lineBest == LMNone)
  {
    _text = GetLastText();
    _lineBest = LMFirst;
  }
}

bool StringtableXMLParserFunctions::Parse(QIStream &in, StringTableDynamic &stringtable, RString language)
{
  StringtableParser parser(stringtable, language);
  parser.Parse(in);
  return parser.GetResult() == LMCorrect;
}

static StringtableXMLParserFunctions GStringtableXMLParserFunctions;
extern StringtableExtParserFunctions *GStringtableExtParserFunctions = &GStringtableXMLParserFunctions;
