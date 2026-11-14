#include "stdafx.h"
#include "StringtableXMLToFileParser.h"

StringtableXMLToFileParser::StringtableXMLToFileParser(const RString& outFileName)
:StringtableParserXMLBase(), _isFirstKey(true), _outFileName(outFileName)
{
}

StringtableXMLToFileParser::~StringtableXMLToFileParser()
{
}

void StringtableXMLToFileParser::OnStartDocument()
{
  StringtableParserXMLBase::OnStartDocument();
  _isFirstKey = true;
  _languages.Clear();
}

void StringtableXMLToFileParser::OnStartElementLanguage(RString name)
{
  if (_isFirstKey)
  {
    _languages.Add(name);
  }
  else
  {
    bool isOk = false;
    // check 
    for (int i=0; i<_languages.Size(); ++i)
    {
      if (_languages[i] == name)
      {
        isOk = true;
      }
    }
    if (!isOk)
    {
      RptF("Invalid language \"%s\" for key \"%s\"", cc_cast(name), cc_cast(GetId()));
    }
  }
}

void StringtableXMLToFileParser::OnEndElementID(RString name)
{
  _isFirstKey = false;
}

