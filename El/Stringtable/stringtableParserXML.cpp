#include <El/elementpch.hpp>
#include "stringtableParserXML.hpp"

StringtableParserXMLBase::StringtableParserXMLBase():SAXParser()
{
}

StringtableParserXMLBase::~StringtableParserXMLBase()
{
}

void StringtableParserXMLBase::OnStartDocument()
{
  SAXParser::OnStartDocument();
  _idTag = RString();
  _id = RString();
  _language = RString();
  _lastText = RString();
}


void StringtableParserXMLBase::OnStartElement(RString name, XMLAttributes &attributes)
{
  if (_id.GetLength() > 0)
  {
    // inside <Text> ... <Text/> - start of language tag
    if (_language.GetLength() > 0)
    {
      RptF("Unexpected stringtable format inside <Text ID=\"%s\"><%s>", cc_cast(_id), cc_cast(_language));
    }
    _language = name;
    _lastText = RString();

    OnStartElementLanguage(name);
  }
  else if (strcmp(name, "Text") == 0 || strcmp(name, "Key") == 0)
  {
    // start of <Key ID=..> or <Text ID=..> tag
    _idTag = name;
    const XMLAttribute *attr = attributes.Find("ID");
    if (attr)
    {
      _id = attr->value;
      _language = RString();
      OnStartElementID(name);
    }
  }
  
}

void StringtableParserXMLBase::OnEndElement(RString name)
{
  if (strcmp(name, _idTag) == 0)
  {
    // end of <Key ID=..> or <Text ID=..> tag
    if (_language.GetLength() > 0)
    {
      RptF("Unexpected stringtable format inside <Text ID=\"%s\"><%s>", cc_cast(_id), cc_cast(_language));
      _language = RString();
    }
    if (_id.GetLength() > 0)
    {
      OnEndElementID(name);
      _id = RString();
    }
    _idTag = RString();
  }
  else if (_language.GetLength() > 0)
  {
    // end of language tag
    OnEndElementLanguage(name);
    _language = RString();
  }
}

void StringtableParserXMLBase::OnCharacters(RString chars)
{
  if (_language.GetLength() > 0) 
    _lastText = chars;
}
