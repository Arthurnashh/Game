#include "stdafx.h"
#include <El/QStream/serializeBin.hpp>
#include <El/Stringtable/stringtableImpl.hpp>
#include "StringtableXMLToBINParser.h"

StringtableXMLToBINParser::StringtableXMLToBINParser(const RString& binFileName)
:StringtableXMLToFileParser(binFileName), _languageCounter(0)
{
}

StringtableXMLToBINParser::~StringtableXMLToBINParser()
{  
}

void StringtableXMLToBINParser::OnStartDocument()
{
  StringtableXMLToFileParser::OnStartDocument();  
  _keys.Clear();
  _translationTable.Clear();
  _languageCounter = 0;
}

void StringtableXMLToBINParser::OnEndDocument()
{
  SaveBinFile();
}

void StringtableXMLToBINParser::OnStartElementID(RString name)
{
  if (GetId().IsEmpty())
    return;

  DoAssert(_keys.Size() == 0 || !GetIsFirstKey());
  _keys.Add(GetId());
  _languageCounter = 0;
}

void StringtableXMLToBINParser::OnStartElementLanguage(RString name)
{
  if (GetId().IsEmpty())
    return;

  StringtableXMLToFileParser::OnStartElementLanguage(name);

  if (GetIsFirstKey())
  {
    _translationTable.Add(LanguageTranslations(name));
  }
}

void StringtableXMLToBINParser::OnEndElementID(RString name)
{ 
  if (GetId().IsEmpty())
    return;

  DoAssert(_languages.Size() == _translationTable.Size());

  /// check if we have translations for all languages for this key
  for (int i=0; i<_translationTable.Size(); ++i)
  {
    if (_translationTable[i].translations.Size() != _keys.Size())
    {
      // no translation
      DoAssert(_translationTable[i].translations.Size() == _keys.Size() - 1);
      RptF("No translation for key \"%s\" and language \"%s\". Added empty string as translation", 
        cc_cast(GetId()), cc_cast(_translationTable[i].language));
      // add empty string as translation
      _translationTable[i].translations.Add(RString());
    }
    DoAssert(_translationTable[i].translations.Size() == _keys.Size());
    if (_translationTable[i].translations.Size() != _keys.Size())
    {
      // this is serous error, _translation table is corrupted
      RptF("Fatal error - corrupted translation table for language \"%s\"", cc_cast(_translationTable[i].language));
    }
  }
  
  StringtableXMLToFileParser::OnEndElementID(name);  
}

void StringtableXMLToBINParser::OnEndElementLanguage(RString name)
{
  if (GetId().IsEmpty())  
    return;

  // add translation for this language
  DoAssert(_languageCounter >= 0 && _languageCounter < _translationTable.Size()) ;
  if (_languageCounter >= 0 && _languageCounter < _translationTable.Size())
  {
    int index = -1;
    if (_translationTable[_languageCounter].language != name)
    {
      // try to find language manually
      for (int i=0; i < _translationTable.Size(); ++i)
      {
        if (_translationTable[i].language == name)
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

    if (index >= 0 && index < _translationTable.Size())
    {
      // add translation 
      _translationTable[index].translations.Add(GetLastText());
    }
    else
    {
      RptF("Unexpected language \"%s\" for key \"%s\"", cc_cast(name), cc_cast(GetId()));
    }

  }
  else
  {
    RptF("Invalid value of _languageCounter: %d ", _languageCounter);
  }

  _languageCounter++;
}

void StringtableXMLToBINParser::SaveBinFile()
{
  QOFStream out;
  out.open(GetOutFileName().Data());
  if (out.fail() || out.error())
  {
    RptF("Failed to open file \"%s\" for writing.", cc_cast(GetOutFileName()));
    return;
  }

  DoAssert(_languages.Size() == _translationTable.Size());
  if (_languages.Size() != _translationTable.Size())
  {
    RptF("Mismatch of number of languages and size of translation table. Save aborted.");
    return;
  }

  /// offsets of translations from the start of the stream
  AutoArray<int> offsets;
  for (int i=0; i<_languages.Size(); ++i)
    offsets.Add(0);

  SerializeBinStream bs(&out);

  // version
  if (!StringTableDynamic::TransferBinVersion(bs))
  {
    bs.SetError(bs.EBadFileType);
    return;
  }

  // save languages
  bs.TransferBasicArray(_languages);

  // save offsets
  // at this moment all offsets are 0 - they will be computed and resaved later
  int offsetsStartPos = out.tellp();
  bs.TransferBasicArray(offsets);
  int offsetsEndPos = out.tellp();

  // save keys
  bs.TransferBasicArray(_keys);

  DoAssert(offsets.Size() == _translationTable.Size());

  // save translations
  for (int i=0; i<_translationTable.Size(); ++i)
  {
    offsets[i] = out.tellp();
    bs.TransferBasicArray(_translationTable[i].translations);
  }
  
  // resave offsets
  out.seekp(offsetsStartPos, QIOS::beg);
  DoAssert(out.tellp() == offsetsStartPos);
  bs.TransferBasicArray(offsets);
  DoAssert(out.tellp() == offsetsEndPos);

  out.close();
}
