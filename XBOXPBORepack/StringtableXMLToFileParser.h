#ifdef _MSC_VER
#pragma once
#endif

#ifndef _STRINGTABLE_XML_TO_FILE_PARSER_HPP
#define _STRINGTABLE_XML_TO_FILE_PARSER_HPP

#include <El/Stringtable/stringtableParserXML.hpp>
#include <El/QStream/qStream.hpp>


/// base class for stringtable XML parsers, that also save the data to other file
class StringtableXMLToFileParser : public StringtableParserXMLBase
{
public:
  /// \param outFileName output file name
  StringtableXMLToFileParser(const RString& outFileName);
  virtual ~StringtableXMLToFileParser();

  //@{ Implementation of SAXParser methods
  virtual void OnStartDocument();
  //@}

protected:
  //@{ Implementation of StringtableParserXMLBase methods
  virtual void OnStartElementLanguage(RString name);
  virtual void OnEndElementID(RString name);
  //@}

  RString GetOutFileName() const {return _outFileName;}
  bool GetIsFirstKey() const {return _isFirstKey;}

  /// all languages
  AutoArray<RString> _languages;

private:
  /// if this is the first key
  bool _isFirstKey;
  /// output file name
  RString _outFileName;
};

#endif //#ifndef _STRINGTABLE_XML_TO_FILE_PARSER_HPP