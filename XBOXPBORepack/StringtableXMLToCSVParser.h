#ifdef _MSC_VER
#pragma once
#endif

#ifndef _STRINGTABLE_XML_TO_CSV_PARSER_HPP
#define _STRINGTABLE_XML_TO_CSV_PARSER_HPP

#include <El/QStream/qStream.hpp>
#include "stringtableXMLToFileParser.h"


/// parses XML stringtable and saves it to CSV
class StringtableXMLToCSVParser : public StringtableXMLToFileParser
{
public:
  /// \param cvsFileName file where to save csv
  StringtableXMLToCSVParser(const RString& csvFileName);
  virtual ~StringtableXMLToCSVParser();

  //@{ Implementation of SAXParser methods
  virtual void OnStartDocument();
  virtual void OnEndDocument();
  //@}

protected:
  //@{ Implementation of StringtableParserXMLBase methods
  virtual void OnStartElementID(RString name);
  virtual void OnEndElementID(RString name);
  virtual void OnEndElementLanguage(RString name);
  //@}


private:
  /// saves languages to output
  void SaveLanguages();
  /// saves key and its text to output
  void SaveKeyTexts();

  void OpenStream();
  void CloseStream();

  /// current texts for key
  AutoArray<RString> _keyTexts;
  /// output stream
  QOFStream _outStream;
  /// counter of languages inside <Text..> or <Key..>
  int _languageCounter;
};

#endif //#ifndef _STRINGTABLE_XML_TO_CSV_PARSER_HPP