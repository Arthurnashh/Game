#ifdef _MSC_VER
#pragma once
#endif

#ifndef _STRINGTABLE_PARSER_XML_HPP
#define _STRINGTABLE_PARSER_XML_HPP

#include <El/XML/xml.hpp>

/// Base class for stringtable XML parsers
class StringtableParserXMLBase : public SAXParser
{
public:
  StringtableParserXMLBase();
  virtual ~StringtableParserXMLBase();

  //@{ Implementation of SAXParser methods
  virtual void OnStartDocument();
  virtual void OnStartElement(RString name, XMLAttributes &attributes);
  virtual void OnEndElement(RString name);
  virtual void OnCharacters(RString chars);
  //@}


protected:
  /// called when tag <Text ID=..> or <Key ID=..> starts
  virtual void OnStartElementID(RString name) = 0;
  /// called when language tag (<English>,..) starts
  virtual void OnStartElementLanguage(RString name) = 0;
  /// called when tag <Text ID=..> or <Key ID=..> ends
  virtual void OnEndElementID(RString name) = 0;
  /// called when language tag (<English>,..) ends
  virtual void OnEndElementLanguage(RString name) = 0;

  RString GetIdTag() const { return _idTag; }
  RString GetId() const { return _id; }
  RString GetLanguage() const { return _language; }
  RString GetLastText() const { return _lastText; }
  

private:
  /// Currently parsing string id tag (<Text> or <Key>)
  RString _idTag;
  /// Currently parsing string id (<Text ID="..."> or <Key ID="...">)
  RString _id;
  /// Currently parsing language (<English>, ...)
  RString _language;
  /// The last text inside the language tag
  RString _lastText;
}; 

#endif //_STRINGTABLE_PARSER_XML_HPP
