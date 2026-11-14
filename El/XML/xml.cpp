#include <El/elementpch.hpp>
#include "xml.hpp"

#include <El/QStream/qStream.hpp>

///////////////////////////////////////////////////////////
//
// Parsing - SAXParser and related classes implementation
//
///////////////////////////////////////////////////////////

int XMLAttributes::Add(RString name, RString value)
{
	int index = AutoArray<XMLAttribute>::Add();
	XMLAttribute &attribute = Set(index);
	attribute.name = name;
	attribute.value = value;
	return index;
}

const XMLAttribute *XMLAttributes::Find(RString name) const
{
	for (int i=0; i<Size(); i++)
		if (Get(i).name == name) return &Get(i);
	return NULL;
}

XMLAttribute *XMLAttributes::Find(RString name)
{
	for (int i=0; i<Size(); i++)
		if (Get(i).name == name) return &Set(i);
	return NULL;
}

#define ISSPACE(c) ((c) >= 0 && (c) <= 32)

static void SkipSpaces(QIStream &in)
{
	int c = in.get();
	while (!in.eof() && !in.fail() && ISSPACE(c))
	{
		c = in.get();
	}
	if (!in.eof() && !in.fail()) in.unget();
}



/*!
\patch 1.41 Date 1/2/2002 by Jirka
- Fixed: Buffer overflow in XML parsing
*/

static char ReadChar(QIStream &in)
{
  TempString<256> buf;
	int c = in.get();
	while (!in.eof() && !in.fail() && c != ';')
	{
		buf.Add(c);
		c = in.get();
	}
	buf.Add(0);

	if (buf[0] == '#')
		return atoi(cc_cast(buf) + 1);
	else if (stricmp(buf, "amp") == 0)
		return '&';
	else if (stricmp(buf, "apos") == 0)
		return '\'';
	else if (stricmp(buf, "quot") == 0)
		return '\"';
	else if (stricmp(buf, "lt") == 0)
		return '<';
	else if (stricmp(buf, "gt") == 0)
		return '>';
  ErrF("Unknown entity: '%s'", cc_cast(buf));
	return '?';
}

static RString ReadPropertyName(QIStream &in)
{
  TempString<256> buf;
	int c = in.get();
	while (!in.eof() && !in.fail() && !ISSPACE(c) && c != '=')
	{
		buf.Add(c);
		c = in.get();
	}
	if (!in.eof() && !in.fail()) in.unget();
	buf.Add(0);
	return buf;
}

static RString ReadPropertyValue(QIStream &in, bool amp)
{
	SkipSpaces(in);
	int c = in.get();
	if (c != '=')
	{
		in.unget();
		return "";
	}
	
	SkipSpaces(in);
	c = in.get();
	if (c != '"' && c != '\'')
	{
    if (isalnum(c))
    {
      // single word attribute
      TempString<256> buf;
      do
      {
        buf.Add(c);
        c = in.get();
      } while (isalnum(c));
  		in.unget();
    	buf.Add(0);
      return buf;
    }
		in.unget();
		return "";
	}

	int term = c;

  TempString<256> buf;
  do {
    c = in.get();
    if (amp && c == '&') c = ReadChar(in);
    else if (c == term) break;
    buf.Add(c);
  } while (!in.eof() && !in.fail());
	buf.Add(0);
	return buf;
}

static RString ReadTag(QIStream &in)
{
  TempString<256> buf;
	int c = in.get();
	while (!in.eof() && !in.fail() && isalnum(c))
	{
		buf.Add(c);
		c = in.get();
	}
	if (!in.eof() && !in.fail()) in.unget();
	buf.Add(0);
	return buf;
}

static void SkipTag(QIStream &in)
{
	int c = in.get();
	while (!in.eof() && !in.fail() && c != '>')
	{
		c = in.get();
	}
}

static RString ReadText(QIStream &in, bool trim)
{
  if (trim) SkipSpaces(in);

  TempString<2048> buf;
  int c = in.get();
  while (!in.eof() && !in.fail() && c != '<')
  {
    if (c == 0x0a)
    {
      // avoid spaces at line end
      while (buf.Size() > 0 && buf[buf.Size() - 1] == ' ') buf.Delete(buf.Size() - 1);
      // avoid CR LF at begin of the text
      if (buf.Size() == 0) goto ReadTextContinue;
    }
    if (c != 0x0d) // avoid CR LF -> 2 spaces (CRLF and LF are handled well)
    {
      if (ISSPACE(c)) c = ' ';
      else if (c == '&') c = ReadChar(in);
      buf.Add(c);
    }
ReadTextContinue:
    c = in.get();
  }
  if (!in.eof() && !in.fail()) in.unget();
  // RTRIM
  if (trim) while (buf.Size() > 0 && ISSPACE(buf[buf.Size() - 1])) buf.Resize(buf.Size()-1);
  buf.Add(0);
  return buf;
}

bool SAXParser::Parse(QIStream &in)
{
	_abort = false;
	OnStartDocument();

	int c;
	while (!_abort)
	{
		I_AM_ALIVE();
    // if (GProgress) GProgress->Refresh();

		c = in.get();
		if (in.eof())
		{
			OnEndDocument();
			return true;
		}
		if (in.fail())
		{
      RptF("XML parsing error: cannot read the source file");
			return false;
		}
		
		if (c == '<')
		{
			c = in.get();
			if (c == '/')
			{
				RString tag = ReadTag(in);
				OnEndElement(tag);
			}
			else if (isalnum(c))
			{
				in.unget();
				RString tag = ReadTag(in);
				
				// check for attributes
				XMLAttributes attributes;
				SkipSpaces(in);
				c = in.get();
				in.unget();
				while (c != '>' && c != '/' && c != EOF)
				{
					RString name = ReadPropertyName(in);
					if (name.GetLength() == 0)
          {
            RptF("XML parsing error: empty attribute name");
            RptF("  <%s", cc_cast(tag));
            for (int i=0; i<attributes.Size(); i++)
              RptF("    %s = \"%s\"", cc_cast(attributes[i].name), cc_cast(attributes[i].value));
            // try to recovery
            ReadPropertyValue(in, _amp);
          }
          else
          {
  					RString value = ReadPropertyValue(in, _amp);
	  				attributes.Add(name, value);
          }
					SkipSpaces(in);
					c = in.get();
					in.unget();
				}
				OnStartElement(tag, attributes);
				if (c == '/')
				{
					OnEndElement(tag);
					in.get();
					c = in.get();
					in.unget();
				}
				DoAssert(c == '>');
			}
			SkipTag(in);
		}
		else
		{
			// text
			in.unget();
			RString text = ReadText(in, _trim);
			if (text.GetLength() > 0) OnCharacters(text);
		}
	}
	return true;
}
