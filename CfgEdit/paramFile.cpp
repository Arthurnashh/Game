// parameter file parser

#include "StdAfx.h"

void ParserMessage(CParamFile &f, const char *message)
{
	CWnd *wnd = AfxGetMainWnd();
	if (wnd)
	{
		char title[256];
		sprintf(title, "Parser error at line %d", f.GetLine());
		wnd->MessageBox(message, title);
	}
}

char CParamFile::GetChar()
{
	if (m_nPosition >= m_nFileSize)
	{
		m_nPosition++;
		return 0;
	}
	char c = ((char *)m_lpBuffer)[m_nPosition];
	if (c == '\n') _line++;
	m_nPosition++;
	return c;
}

void CParamFile::UngetChar()
{
	if (m_nPosition == 0) return;
	m_nPosition--;
	char c = ((char *)m_lpBuffer)[m_nPosition];
	if (c == '\n') _line--;
}

void SkipLine(CParamFile &f)
{
	char c = f.GetChar();
	char prevc = c;
	bool rpt;
	do
	{
		while (c && c != '\r' && c != '\n')
		{
			prevc = c;
			c = f.GetChar();
		}
		rpt = c && (prevc == '\\');
		while (c == '\n' || c == '\r')
		{
			c = f.GetChar();
		}
	}
	while (rpt);
	if (c) f.UngetChar();
}

void SkipComment(CParamFile &f)
{
	char c = f.GetChar();
	if (!c) return;
	while (true)
	{
		while (c != '*')
		{
			c = f.GetChar();
			if (!c) return;
		}
		c = f.GetChar();
		if (!c) return;
		if (c == '/') return;
	}
}

char GetChar(CParamFile &f, bool skipPreprocessed = true)
{
	char c = f.GetChar();
	if (skipPreprocessed) while (c)
	{
		if (c == '#')
		{
			SkipLine(f);
			c = f.GetChar();
		}
		else if (c == '/')
		{
			c = f.GetChar();
			if (c == '/')
			{
				SkipLine(f);
				c = f.GetChar();
			}
			else if (c == '*')
			{
				SkipComment(f);
				c = f.GetChar();
			}
			else
			{
				f.UngetChar();
				return '/';
			}
		}
		else return c;
	}
	return c;
}

static CString GetWord(CParamFile &f, const char *termin)
{
	char buf[2048];
	int len=0;
	buf[len]=0;
	int c = GetChar(f);
	// LTrim the word
	while(isspace(c) ) c = GetChar(f);
	if (c=='"')
	{
		c=GetChar(f, false);
		while(c!='"')
		{
			if( c=='\n' || c=='\r' )
			{
				char message[256];
				sprintf(message, "Config: End of line encountered after %s", buf);
				ParserMessage(f, message);
			}
			if( len<sizeof(buf)-1 ) buf[len++]=c,buf[len]=0;
			c=GetChar(f, false);
		}
	}
	else
	{
		while( !strchr(termin,c) )
		{
			if( c=='\n' || c=='\r' )
			{
				// word terminated - only white spaces or termin now
				for(;;)
				{
					c=GetChar(f);
					if(!isspace(c) ) break;
				}
				if( !strchr(termin,c) )
				{
					char message[256];
					sprintf(message, "Config: '%c' after %s", c, buf);
					ParserMessage(f, message);
				}
				else f.UngetChar();
				goto Return;
			}
			if( len<sizeof(buf)-1 ) buf[len++]=c,buf[len]=0;
			c=GetChar(f);
		}
		f.UngetChar();
		Return:
		// RTrim the word
		while( len>0 && buf[len-1]==' ' ) buf[--len]=0;
	}
	return buf;
}

static CString GetAlphaWord(CParamFile &f)
{
	char buf[2048];
	int len=0;
	int c=GetChar(f);
	while(isspace(c) ) c = GetChar(f);
	while( isalnum(c) || c=='_' )
	{
		if( len<sizeof(buf) ) buf[len++]=c;
		c=GetChar(f);
	}
	f.UngetChar();
	buf[len]=0;
	return buf;
}

ParamClass::~ParamClass()
{
	Clear();
}

void ParamClass::Clear()
{
	for (int i=0; i<_entries.GetSize(); i++)
	{
		if (_entries[i])
		{
			delete _entries[i];
			_entries[i] = NULL;
		}
	}
	_entries.RemoveAll();
}

CString ParamClass::GetContext(const char *member) const
{
	char buf1[512];
	bool first=false;
	if (member) strcpy(buf1, member), first = true;
	else *buf1=0;
	const ParamClass *src = this;
	while (src)
	{
		char buf2[512];
		strncpy(buf2, buf1, sizeof(buf1));
		if (src->GetName())
			strncpy(buf1, src->GetName(), sizeof(buf1));
		else
		{
			return "";
		}
		if (first)
			strncat(buf1, ".", sizeof(buf1)), first=false;
		else
			strncat(buf1, "::", sizeof(buf1));
		strncat(buf1, buf2, sizeof(buf1));
		src = src->_parent;
	}
	return buf1;
}

void ParamClass::Parse(CParamFile &f)
{
	int c;
	// parse section content
	for(;;)
	{ 
		c=GetChar(f);
		while(isspace(c) ) c=GetChar(f);
		if( c == 0 )
		{
			return;
		}
		if( c=='}' )
		{
			c=GetChar(f);
			while(isspace(c) || c==';' ) c=GetChar(f);
			f.UngetChar();
			break; // section end reached
		}
		f.UngetChar();
		CString word;
		word=GetAlphaWord(f);
		// word is entry name
		ParamClass *newEntry = NULL;
		if( !strcmp(word,"class") )
		{
			word=GetAlphaWord(f);
			ParamClass *section=new ParamClass();
			section->_name=word;
			section->_parent=this;
			section->_type=PTClass;
			section->_line=f.GetLine();
			// section header
			int c=GetChar(f);
			while(isspace(c) ) c = GetChar(f);
			if( c==':' )
			{
				// base class
				CString base=GetAlphaWord(f);
				section->_base = base;
				c=GetChar(f);
			}
			// find opening brace
			while( c!='{' )
			{
				if(!isspace(c) )
				{
					char message[256];
					sprintf(message, "%s: '%c' encountered instead of '{'", (const char *)GetContext(), c);
					ParserMessage(f, message);
					return;
				}
				c=GetChar(f);
			}
			// parse section content
			section->Parse(f);
			newEntry=section;
		}
		else if (!strcmp(word, "enum"))
		{
			// "enum" may be forgotten now
			word=GetAlphaWord(f);

			ParamClass *value = new ParamClass();
			value->_name = word;
			value->_parent = this;
			value->_type = PTEnum;
			value->_line = f.GetLine();

			newEntry = value;

			// find opening brace
			c=GetChar(f);
			while (c != '{')
			{
				if (!isspace(c))
				{
					char message[256];
					sprintf(message, "%s: '%c' encountered instead of '{'", (const char *)GetContext(), c);
					ParserMessage(f, message);
					return;
				}
				c = GetChar(f);
			}
			do
			{
				word = GetAlphaWord(f);
				CString name = word;
				c = GetChar(f);
				while (isspace(c)) c = GetChar(f);
				if (c == '=')
				{
					c = GetChar(f);
					while (isspace(c)) c = GetChar(f);
					f.UngetChar();
					word = GetWord(f,",}");
				}
			} while (c == ',');
			if (c == '}')
			{
				c = GetChar(f);
				while (isspace(c) || c == ';') c = GetChar(f);
				f.UngetChar();
			}
			else
			{
				char message[256];
				sprintf(message, "%s: '%c' encountered instead of '}'", (const char *)GetContext(), c);
				ParserMessage(f, message);
				return;
			}
		}
		else
		{
			// word should be value or array
			c=GetChar(f);
			if( c=='[' )
			{
				// word is array name
				ParamClass *array=new ParamClass();
				array->_name = word;
				array->_parent=this;
				array->_type = PTValue;
				array->_line=f.GetLine();
				c=GetChar(f);
				while(isspace(c) ) c=GetChar(f);
				if( c!=']' )
				{
					char message[256];
					sprintf(message, "Config: %s: '%c' encountered instead of ']'", (const char *)GetContext(word), c);
					ParserMessage(f, message);
				}
				c=GetChar(f);
				while(isspace(c) ) c=GetChar(f);
				if( c!='=' )
				{
					char message[256];
					sprintf(message, "Config: %s: '%c' encountered instead of '='",(const char *)GetContext(word),c);
					ParserMessage(f, message);
				}
				c=GetChar(f);
				while(isspace(c) ) c=GetChar(f);
				if (c == '{')
				{
					// regular array
					bool isFirst=true;
					for(;;)
					{
						word = GetWord(f,",;}");
						c = GetChar(f);
						if( c==',' || c==';' || !isFirst || strlen(word)>0 )
						{
							isFirst=false;
						}
						if( c=='}' ) break; // array terminated
						if( c<0 )
						{
							char message[256];
							sprintf(message, "%s: EOF encountered.",(const char *)GetContext(array->GetName()));
							ParserMessage(f, message);
						}
					}
					c=GetChar(f);
					while(isspace(c) ) c=GetChar(f);
					if( c!=';' )
					{
						char message[256];
						sprintf(message, "%s: '%c' encountered instead of ';'",(const char *)GetContext(array->GetName()),c);
						ParserMessage(f, message);
					}
				}
				else
				{
					// define
					f.UngetChar();
					word=GetWord(f,";\n\r");
					c=GetChar(f);
					if( c!=';' && c!='\n' && c!='\r' )
					{
						char message[256];
						sprintf(message, "'%s': '%c' encountered instead of ';'",(const char *)GetContext(array->GetName()),c);
						ParserMessage(f, message);
						return;
					}
				}
				newEntry=array;
			}
			else
			{
				while(isspace(c) ) c=GetChar(f);
				if( c!='=' )
				{
/*
					char message[256];
					sprintf(message, "'%s': '%c' encountered instead of '='",(const char *)GetContext(),c);
					ParserMessage(f, message);
					return;
*/
					SkipLine(f);
					goto OnError;
				}
				ParamClass *value=new ParamClass();
				value->_name = word;
				value->_parent=this;
				value->_type = PTValue;
				value->_line=f.GetLine();
				c=GetChar(f);
				while(isspace(c) ) c=GetChar(f);
				f.UngetChar();
				word=GetWord(f,";\n\r");
				c=GetChar(f);
				if( c!=';' && c!='\n' && c!='\r' )
				{
					char message[256];
					sprintf(message, "'%s': '%c' encountered instead of ';'",(const char *)GetContext(value->GetName()),c);
					ParserMessage(f, message);
					return;
				}
				newEntry=value;
			}
		}
OnError:
		// check for overload
		if (newEntry) _entries.Add(newEntry);
	}
	// class parsed
}
