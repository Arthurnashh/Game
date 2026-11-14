#ifdef _MSC_VER
#pragma once
#endif

#ifndef _STRINGTABLE_XML_TO_BIN_PARSER_HPP
#define _STRINGTABLE_XML_TO_BIN_PARSER_HPP

#include <El/QStream/qStream.hpp>
#include "stringtableXMLToFileParser.h"


/// translations for all keys in one language
struct LanguageTranslations
{
  LanguageTranslations(RString lang = RString()): language(lang) {}

  /// what language are these translations for
  RString language;
  /// translations for all keys in one language (order corresponds to order of keys in _keys table)
  AutoArray<RString> translations;
};

TypeIsMovable(LanguageTranslations);

/** 
Parses XML stringtable and saves it binary file:
*/
class StringtableXMLToBINParser : public StringtableXMLToFileParser
{
public:
  /// \param binFileName file where to save binary data
  StringtableXMLToBINParser(const RString& binFileName);
  virtual ~StringtableXMLToBINParser();

  //@{ Implementation of SAXParser methods
  virtual void OnStartDocument();
  virtual void OnEndDocument();
  //@}

protected:
  //@{ Implementation of StringtableParserXMLBase methods
  virtual void OnStartElementID(RString name);
  virtual void OnStartElementLanguage(RString name);
  virtual void OnEndElementID(RString name);
  virtual void OnEndElementLanguage(RString name);
  //@}


private:
  void SaveBinFile();

  /// all keys
  AutoArray<RString> _keys;

  /// all translations (order corresponds to the order of languages in _languages)
  AutoArray<LanguageTranslations> _translationTable;

  /// counter for languages inside <Text..> or <Key..> tags
  int _languageCounter;
};

#endif //#ifndef _STRINGTABLE_XML_TO_BIN_PARSER_HPP