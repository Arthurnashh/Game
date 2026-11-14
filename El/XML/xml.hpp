#ifdef _MSC_VER
#pragma once
#endif

#ifndef _XML_HPP
#define _XML_HPP

#include <Es/Strings/rString.hpp>
#include <Es/Types/pointers.hpp>

class QIStream;

/*!
	\file
	Interface file xml.hpp
*/

//! Single attribute value
/*!
	Represents single attribute value.
	Example: <TAG name=value>
*/
struct XMLAttribute
{
	//! attribute name
	RString name;
	//! attribute value
	RString value;
};
TypeIsMovableZeroed(XMLAttribute);

//! List of attributes
/*!
	\todo Use other structure for faster finding of arguments (f.e. hash table)
*/
class XMLAttributes : public AutoArray<XMLAttribute>
{
public:
	//! add attribute into list
	/*!
		\param name attribute name
		\param value attribute value
		\return index of attribute in array
	*/
	int Add(RString name, RString value);
	//! find attribute
	/*!
		\param name attribute name
		\return attribute
	*/
	const XMLAttribute *Find(RString name) const;
	//! find attribute
	/*!
		\param name attribute name
		\return attribute
	*/
	XMLAttribute *Find(RString name);
};

//! Simple API for XML Parser
/*!
	Usage:
		- derive class from SAX Parser and implement virtual methods
			OnStartDocument, OnEndDocument, OnStartElement, OnEndElement and OnCharacters
		- call method Parse
		- you can abort parsing by Abort method
		(see class SquadParser)
*/
class SAXParser
{
protected:
	bool _abort;
	bool _trim;
  bool _amp; /// & in attribute value is interpreted as in text (false when & is interpreted simply as &)

public:
	//! constructor
	SAXParser() {_abort = false; _trim = true; _amp = true;}
  virtual ~SAXParser() {}

	//! download file from <url> and parse it (virtual methods are called during parsing)
	/*!
		\param url URL address of source file
		\return true if file parsed and no errors occurs
	*/
	bool Parse(QIStream &in);
	//! abort parsing
	void Abort() {_abort = true;}

	//! called during parsing on the beginning of the document
	virtual void OnStartDocument() {}
	//! called during parsing when end of the document is reached
	virtual void OnEndDocument() {}
	//! called during parsing when element start is reached
	/*!
		\param name name of element
		\param attributes list of attributes
	*/
	virtual void OnStartElement(RString name, XMLAttributes &attributes) {}
	//! called during parsing when element end is reached
	/*!
		\param name name of element
	*/
	virtual void OnEndElement(RString name) {}
	//! called during parsing for each texts (within elements)
	/*!
		\param chars text
	*/
	virtual void OnCharacters(RString chars) {}
};

#endif
