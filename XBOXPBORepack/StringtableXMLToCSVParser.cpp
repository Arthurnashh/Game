#include "stdafx.h"
#include "StringtableXMLToCSVParser.h"

StringtableXMLToCSVParser::StringtableXMLToCSVParser(const RString& csvFileName)
:StringtableXMLToFileParser(csvFileName), _languageCounter(0) 
{
}

StringtableXMLToCSVParser::~StringtableXMLToCSVParser()
{
}

void StringtableXMLToCSVParser::OnStartDocument()
{
  StringtableXMLToFileParser::OnStartDocument();
  _keyTexts.Clear();  
  _languageCounter = 0;
  OpenStream();
}

void StringtableXMLToCSVParser::OnEndDocument()
{
  StringtableXMLToFileParser::OnEndDocument();
  CloseStream();
}

void StringtableXMLToCSVParser::OnStartElementID(RString name)
{
  if (GetIsFirstKey())
  {
    _keyTexts.Clear();
  }
  else
  {
    DoAssert(_keyTexts.Size() == _languages.Size());
    for (int i=0; i<_keyTexts.Size(); ++i)
    {
      _keyTexts[i] = RString();
    }
  }
  _languageCounter = 0;
}

void StringtableXMLToCSVParser::OnEndElementID(RString name)
{
  if (GetIsFirstKey())
  {
    // first key - save languages to file
    SaveLanguages();
  }
  SaveKeyTexts();
  StringtableXMLToFileParser::OnEndElementID(name);
}

void StringtableXMLToCSVParser::OnEndElementLanguage(RString name)
{
  if (GetId().GetLength() > 0)  
  {
    if (GetIsFirstKey())
    {
      _keyTexts.Add(GetLastText());
    }
    else
    {
      DoAssert(_keyTexts.Size() == _languages.Size());
      // find language
      int index = -1;
      DoAssert(_languageCounter >= 0 && _languageCounter < _languages.Size())
      if (_languageCounter >= 0 && _languageCounter < _languages.Size())
      {
        // try to find language manually
        if (_languages[_languageCounter] != name)
        {
          for (int i=0; i<_languages.Size(); ++i)
          {
            if (_languages[i] == name)
            {
              index = i;
              break;
            }
          }
        }
        else
        {
          index = _languageCounter;
        }
      }
      if (index >= 0 && index < _keyTexts.Size())
      {
        _keyTexts[index] = GetLastText();
      }
      else
      {
        RptF("Unexpected language \"%s\" for key \"%s\"", cc_cast(name), cc_cast(GetId()));
      }

    }
  }
  _languageCounter++;
}



void StringtableXMLToCSVParser::OpenStream()
{
  _outStream.open(GetOutFileName());
}

void StringtableXMLToCSVParser::CloseStream()
{
  _outStream.close();
}

void StringtableXMLToCSVParser::SaveLanguages()
{
  if (_outStream.fail())
    return;

  _outStream << "LANGUAGE";
  for (int i=0; i < _languages.Size(); ++i)  
  {
    _outStream << ",\"" << _languages[i] << "\"";
  }
  _outStream << "\n";

}

void StringtableXMLToCSVParser::SaveKeyTexts()
{
  if (_outStream.fail())
    return;

  DoAssert(_languages.Size() == _keyTexts.Size());
  if (_languages.Size() != _keyTexts.Size())
  {
    RptF("Invalid number of languages for Text ID=\"%s\"", cc_cast(GetId()));
  }

  _outStream << GetId();
  for (int i=0; i < _keyTexts.Size(); ++i)  
  {
    _outStream << ",\"" <<_keyTexts[i] << "\"";
  }
  _outStream << "\n";
}
