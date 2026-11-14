//-----------------------------------------------------------------------------
// File: enf_parser.cpp
//
// Desc: Simple document model tokenizer
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
#include "common/enf_inttypes.h"
#include "Common/enf_mathlib.h"
#include "common/enf_parser.h"
#include "common/enf_staticbuffer.h"
#include "common/enf_memorybuffer.h"
#include "common/enf_streams.h"
#include "system/enf_memory.h"

#include <stdio.h>

namespace enf
{
const char* CParser::TInclude;
const char* CParser::TDefine;
const char* CParser::TIfDef;
const char* CParser::TIfNDef;
const char* CParser::TElse;
const char* CParser::TEndif;
const char* CParser::TComment;
const char* CParser::TLComment;
const char* CParser::TRComment;
const char* CParser::TPlusPlus;
const char* CParser::TMinusMinus;
const char* CParser::TLShift;
const char* CParser::TRShift;	
const char* CParser::TLShiftAssign;
const char* CParser::TRShiftAssign;
const char* CParser::TMinusAssign;
const char* CParser::TPlusAssign;
const char* CParser::TAsteriskAssign;
const char* CParser::TSlashAssign;
const char* CParser::TOrAssign;
const char* CParser::TPowerAssign;
const char* CParser::TLessEqual;
const char* CParser::TGreaterEqual;
const char* CParser::TAndAssign;
const char* CParser::TNotEqual;
const char* CParser::TEqual;
const char* CParser::TOrOr;
const char* CParser::TAndAnd;
const char* CParser::TOr;
const char* CParser::TGreater;
const char* CParser::TLess;
const char* CParser::TAssign;
const char* CParser::TAnd;
const char* CParser::TPower;
const char* CParser::TMinus;
const char* CParser::TPlus;
const char* CParser::TAsterisk;
const char* CParser::TSlash;
const char* CParser::TLBracket;
const char* CParser::TRBracket;
const char* CParser::TLCurly;
const char* CParser::TRCurly;
const char* CParser::TComma;
const char* CParser::TSeparator;
const char* CParser::TDoubleDot;
const char* CParser::TFourDot;
const char* CParser::TLSBracket;
const char* CParser::TRSBracket;
const char* CParser::TDot;
const char* CParser::TExclamation;
const char* CParser::TSizeof;
const char* CParser::TPercent;
const char* CParser::TBackslash;
const char* CParser::THash;
const char* CParser::TDLine;
const char* CParser::TDFile;

struct CParser::DToken
{
	ENF_DECLARE_SMALLOBJECT(CParser::DToken);
	const char*	string;
	DToken*		next;
};

SArray<CParser::TTDef, 32> CParser::m_StaticDefs;
CParser::DToken*	CParser::m_pDTokens[256];
StaticBuffer* CParser::m_pStatic = NULL;

bool CParser::m_bInitialized = false;
CParser::TTDef* CParser::m_pCDef;

const char	CParser::SequenceError[] 	= "Out of sequence";
const char	CParser::ParmParseError[]	= "Error in parameters";
const char	CParser::UnexpEolError[]	= "Unexpected end of line";
const char	CParser::UnexpEofError[]	= "Unexpected end of file";
const char	CParser::ExpectedError[]	= "Expected '%s' ,got '%s'";
const char	CParser::GarbageError[]		= "Unexpected '%s'";
const char	CParser::ExpectedTypeError[] = "Expected %s type, got '%s'";

//-----------------------------------------------------------------------
void CParser::LError(const char *err,...)
{
	m_bCLF = true;

va_list argptr;
static char tmp[4096];

	va_start (argptr, err);
	vsprintf (tmp, err, argptr);
	va_end (argptr);

	if(m_pErrorHandler)
	{
		if(CLine && CLine < ELine)
			m_pErrorHandler->Error(m_Modules[CLine->m_iModule]->GetName(), CLine->m_iSourceLine, tmp);
		else if(!m_Modules.IsEmpty())
			m_pErrorHandler->Error(m_Modules[0]->GetName(), m_Modules[0]->m_iSourceLine, tmp);
		else
			m_pErrorHandler->Error("", 0, tmp);
	}
	else
	{
		//LogF (LL_DEBUG, tmp);
    LogF(tmp);
	}
}

//-----------------------------------------------------------------------
ParseResume CParser::InternalError(ParseError errcode, const char* modname, int srcline)
{
	SString<> temp;

	switch(errcode)
	{
	case PE_MISSING_FILE:
		temp.format("CParser: Missing file on line %d, module %s", srcline, modname);
		break;

	case PE_MISSPLACED_END_OF_COMMENT:
		temp.format("CParser: Missplaced end of comment on line %d, module %s", srcline, modname);
		break;

	case PE_MISSING_END_OF_COMMENT:
		temp.format("CParser: Missing end of comment on line %d, module %s", srcline, modname);
		break;

	case PE_PARSE_NUMBER:
		temp.format("CParser: Can't parse number on line %d, module %s", srcline, modname);
		break;

	case PE_PARSE_QUOTED_STRING:
		temp.format("CParser: quoted string not closed on line %d, module %s", srcline, modname);
		break;

	case PE_OUT_OF_MEMORY:
		temp.format("CParser: Out of memory on line %d, module %s", srcline, modname);
		break;
	}
	return Error(errcode, temp.cstr(), modname, srcline);
}

//-----------------------------------------------------------------------
const char* CParser::AddDefToken(const char *string)
{
	uint pos = (uint)string[0];

	DToken *token = (DToken *)m_pStatic->AllocData(sizeof(DToken));
	token->string = (const char*)m_pStatic->AllocData(strlen(string) + 1, (const void*)string);
	
	DToken **prev = &m_pDTokens[pos];

	while(*prev)
		prev = &(*prev)->next;

	*prev = token;
	token->next = NULL;

	return token->string;
}

//-----------------------------------------------------------------------
void CParser::Cleanup()
{
	SAFE_DELETE(m_pStatic);
	m_bInitialized = false;
}

//-----------------------------------------------------------------------
void CParser::Init()
{
	if(m_bInitialized)
		return;

	m_bInitialized = true;

	ZeroMem(m_pDTokens, sizeof(m_pDTokens));

	m_pStatic = CreateStaticBuffer(8192, false);

	TInclude		= AddDefToken("#include");
	TDefine		= AddDefToken("#define");
	TIfDef		= AddDefToken("#ifdef");
	TIfNDef		= AddDefToken("#ifndef");
	TElse			= AddDefToken("#else");
	TEndif		= AddDefToken("#endif");
	TComment		= AddDefToken("//");
	TLComment	= AddDefToken("/*");
   TRComment	= AddDefToken("*/");
	TPlusPlus	= AddDefToken("++");
	TMinusMinus = AddDefToken("--");
	TLShiftAssign = AddDefToken("<<=");
   TRShiftAssign = AddDefToken(">>=");
	TLShift		= AddDefToken("<<");
	TRShift		= AddDefToken(">>");
	
	TMinusAssign = AddDefToken("-=");
   TPlusAssign	= AddDefToken("+=");
   TAsteriskAssign = AddDefToken("*=");
   TSlashAssign = AddDefToken("/=");
   TOrAssign	= AddDefToken("|=");
	TPowerAssign	= AddDefToken("^=");
   TLessEqual	= AddDefToken("<=");
   TGreaterEqual = AddDefToken(">=");
   TAndAssign	= AddDefToken("&=");
   TNotEqual	= AddDefToken("!=");

   TEqual		= AddDefToken("==");
	TOrOr			= AddDefToken("||");
	TAndAnd		= AddDefToken("&&");
	TOr			= AddDefToken("|");
	TGreater		= AddDefToken(">");
	TLess			= AddDefToken("<");
	TAssign		= AddDefToken("=");
	TAnd			= AddDefToken("&");
	TPower		= AddDefToken("^");
   TMinus		= AddDefToken("-");
   TPlus			= AddDefToken("+");
   TAsterisk	= AddDefToken("*");
   TSlash		= AddDefToken("/");
   TLBracket	= AddDefToken("(");
   TRBracket	= AddDefToken(")");
   TLCurly		= AddDefToken("{");
   TRCurly		= AddDefToken("}");
   TComma		= AddDefToken(",");
   TSeparator	= AddDefToken(";");
	TFourDot		= AddDefToken("::");
   TDoubleDot	= AddDefToken(":");
	TLSBracket	= AddDefToken("[");
	TRSBracket	= AddDefToken("]");
   TDot			= AddDefToken(".");
   TExclamation = AddDefToken("!");
   TPercent		= AddDefToken("%");
	TSizeof		= AddDefToken("sizeof");
   TBackslash	= AddDefToken("\\");
	THash			= AddDefToken("#");

	TDLine		= AddDefToken("__LINE__");
	TDFile		= AddDefToken("__FILE__");
}

struct ErrorLine
{
	TLine*	Line;
	char*		Text;
};

//-----------------------------------------------------------------------
bool CParser::Garbage()
{
	if(!EndOfLine())
	{
		LError(GarbageError, GetToken(CParm)->Text);
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------
bool CParser::EndOfLine()
{
	if(CLine == NULL)
		return true;
	return(CParm >= CLine->m_iNumTokens);
}

//-----------------------------------------------------------------------
bool CParser::AssertEOL()
{
	if(EndOfLine())
	{
		LError(UnexpEolError);
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------
bool CParser::NextLine(bool force)
{
	if(CLine == NULL)
		return false;

	CLine++;
	if(CLine < ELine)
	{
		CParm = 0;
		CToken = m_Tokens.GetArray() + CLine->m_iFirstToken;
		return true;
	}

	if(force)
		LError(UnexpEofError);

	return false;
}

//-----------------------------------------------------------------------
bool CParser::NextToken(bool force)
{
	CParm++;

	if(!force)
	{
		if(EndOfLine())
			return false;
	}
	else
	{
		if(!AssertEOL())
			return false;
	}

	CToken = GetToken(CParm);
	return true;
}

//-----------------------------------------------------------------------
bool CParser::AssertToken(const char *str)
{
	if(!AssertEOL())
		return false;

	if(!TTDefEqual(GetToken(CParm), str))
	{
		LError(ExpectedError, str, GetToken(CParm)->Text);
		return false;
	}

	NextToken();

	return true;
}

const char *TTypeNames[6] = {"", "hexadecimal", "number", "symbol", "quoted string", "string"};

//-----------------------------------------------------------------------
bool CParser::AssertType(TokenType type)
{
	if(GetToken(CParm)->Type != type)
	{
		LError(ExpectedTypeError, TTypeNames[type], GetToken(CParm)->Text);
		return false;
	}

	return true;
}

//----------------------------------------------------------------------
char* Unquote(char *dst, const char *src, int len)
{
	char* res = dst;

	if(*src == '"')
   {
   	CopyMem(dst, src + 1, len - 2);
      dst[len - 2] = 0;
   }
   else
   {
   	CopyMem(dst, src, len);
      dst[len] = 0;
   }
	return res;
}

//----------------------------------------------------------------------
TModule::TModule(CStr name, int size)
{
	m_iFirstLine = m_iLastLine = 0;
   m_iSourceLine = 0;
	m_bNewLine = false;
	if(!name.IsNull())
		m_strName = name.cstr();
	m_Size = enf_max(((size * 3) / 2), 4000);
	m_strPosition = m_strBuffer = (char*)GMemFunctions->New(m_Size, __FILE__, __LINE__);
	Assert(m_strBuffer);
}

//----------------------------------------------------------------------
TModule::~TModule()
{
	if(m_strBuffer)
	{
		GMemFunctions->Delete(m_strBuffer);
		m_strBuffer = NULL;
	}
}

//----------------------------------------------------------------------
CParser::TTDef* CParser::AddStaticDef(const char* name)
{
	for(uint n = 0; n < m_StaticDefs.GetCardinality(); n++)
	{
		if(enf::strcmp(m_StaticDefs[n].m_strName, name) == 0)
			return &m_StaticDefs[n];
	}
	TTDef& def = m_StaticDefs.Append();
	def.m_strName = name;
	def.m_iNumTokens = 0;
	def.m_iFirstToken = 0;
	return &def;
}

//----------------------------------------------------------------------
CParser::TTDef* CParser::AddDef(const char* name)
{
	for(uint n = 0; n < m_Defs.GetCardinality(); n++)
	{
		if(enf::strcmp(m_Defs[n].m_strName, name) == 0)
			return &m_Defs[n];
	}
	TTDef& def = m_Defs.Append();
	def.m_strName = name;
	def.m_iNumTokens = 0;
	def.m_iFirstToken = m_Tokens.GetCardinality();
	return &def;
}

//----------------------------------------------------------------------
CParser::CParser(): m_iCurrentModule(0), m_Res(PE_OK), m_iFirstMacro(-1), m_pErrorHandler(NULL)
{
	Init();
	m_Defs.Clear();
	for(uint n = 0; n < m_StaticDefs.GetCardinality(); n++)
	{
		AddDef(m_StaticDefs[n].m_strName);
	}
}

//----------------------------------------------------------------------
void CParser::Clear()
{
	ForEach<DeleteItem>(m_Modules);

	m_Lines.Clear();
	m_Tokens.Clear();
	m_Modules.Clear();
}

//----------------------------------------------------------------------
CParser::~CParser()
{
	Clear();
}

//----------------------------------------------------------------------
TModule *CParser::FindModule(CStr name)
{
	for(uint n = 0; n < m_Modules.GetCardinality(); n++)
	{
		if(name.cmpi(m_Modules[n]->GetName()) == 0)
			return m_Modules[n];
	}
	return NULL;
}

//----------------------------------------------------------------------
TLine *CParser::FindSourceLine(uint mod, int srcline)
{
	for(uint n = 0; n < m_Lines.GetCardinality(); n++)
	{
		if(m_Lines[n].m_iSourceLine == srcline &&
			m_Lines[n].m_iModule == mod)
			return &m_Lines[n];
	}

	return NULL;
}

//----------------------------------------------------------------------
TModule *CParser::AddModule(CStr name, int size)
{
	TModule* tm = new TModule(name, size);
	if(tm)
   {
		m_iCurrentModule = m_Modules.Insert(tm);
		tm->m_iFirstLine = m_Lines.GetCardinality();
		tm->m_iLastLine = m_Lines.GetCardinality();
   }

   return tm;
}

//----------------------------------------------------------------------
const char *CParser::FindToken(const char *sample, const char *end, int *len)
{
	for(DToken *token = m_pDTokens[(ubyte)sample[0]]; token; token = token->next)
   {
		for(int l = 0; true; l++)
      {
			if(!token->string[l])
			{
				*len = l;
         	return token->string;
			}

         if(&sample[l] == end)
         	break;

			if(token->string[l] != sample[l])
         	break;
      }
   }

   return NULL;
}

//----------------------------------------------------------------------
const char *CParser::FindToken(const char *sample, int *len)
{
	for(DToken *token = m_pDTokens[(ubyte)sample[0]]; token; token = token->next)
   {
		for(int l = 0;; l++)
      {
			if(!token->string[l])
			{
				*len = l;
         	return token->string;
			}

         if(!sample[l])
         	break;

			if(token->string[l] != sample[l])
         	break;
      }
   }
   return NULL;
}

//----------------------------------------------------------------------
TLine *CParser::Search(const char *str, int flags, TLine *from)
{
	uint mod = from->m_iModule;

	for(uint n = 0; n < m_Modules.GetCardinality(); n++)
	{
		if(flags & SF_ALLMODULES)
			mod = n;

		for(uint l = 0; l < m_Lines.GetCardinality(); l++)
		{
			if(!(flags & SF_FROMTHETOP))
			{
				if(m_Lines[l].m_iSourceLine <= from->m_iSourceLine)
					continue;
			}

			if(m_Lines[l].m_iModule != mod)
				continue;

			for(int t = 0; t < m_Lines[l].m_iNumTokens; t++)
			{
				const TToken* tt = GetToken(l, t);

				if(flags & SF_WHOLEWORD)
				{
					if(flags & SF_MATCHCASE)
					{
						if(!strcmp(str, tt->Text))
							return &m_Lines[l];
					}
					else
					{
						if(!enf_strcmpi(str, tt->Text))
							return &m_Lines[l];
					}
				}
				else
				{
					if(flags & SF_MATCHCASE)
					{
						if(strstr(str, tt->Text))
							return &m_Lines[l];
					}
					else
					{
						if(strstr(str, tt->Text))
							return &m_Lines[l];
					}
				}
			}
		}
		if(!(flags & SF_ALLMODULES))
			break;
	}
	return NULL;
}

//----------------------------------------------------------------------
CParser::TTDef	*CParser::FindDef(const char *name, int len)
{
	if(m_iFirstMacro < 0)
		return NULL;

	for(uint n = m_iFirstMacro; n < m_Defs.GetCardinality(); n++)
	{
		if(m_Defs[n].m_iNumTokens == 0)
			continue;

		if(!strncmp(m_Defs[n].m_strName, name, len) && strlen(m_Defs[n].m_strName) == len)
			return &m_Defs[n];
	}
	return NULL;
}

//----------------------------------------------------------------------
bool CParser::IfDef(const char *name, int len)
{
	for(uint n = 0; n < m_Defs.GetCardinality(); n++)
	{
		if(!strncmp(m_Defs[n].m_strName, name, len) && strlen(m_Defs[n].m_strName) == len)
			return true;
	}
	return false;
};

//----------------------------------------------------------------------
const char* TModule::Alloc(const char* buff, int len)
{
	Assert((m_strPosition + len + 1) < (m_strBuffer + m_Size));

	char* ret = m_strPosition;
	m_strPosition += len + 1;
	CopyMem(ret, buff, len);
	ret[len] = 0;
	return ret;
}

//----------------------------------------------------------------------
bool CParser::AddToken(TokenType type, int spaces, const char *token, int len)
{
	if(m_Modules[m_iCurrentModule]->m_bNewLine)
   {
		TLine& line = m_Lines.Append();

      line.m_iSourceLine = m_Modules[m_iCurrentModule]->m_iSourceLine;
      line.m_iModule = m_iCurrentModule;
      line.m_iNumTokens = 0;
      line.m_iFirstToken = m_Tokens.GetCardinality();

		m_Modules[m_iCurrentModule]->m_iLastLine++;
	   m_Modules[m_iCurrentModule]->m_bNewLine = false;
   }

	//get memory from current module
	if(type == TT_STRING)
	{
		TTDef* td = FindDef(token, len);

		if(td != NULL)
		{
			for(int t = 0; t < td->m_iNumTokens; t++)
			{
				const TToken& tt = m_Tokens[td->m_iFirstToken + t];

				if(tt.Text == TBackslash)
				{
					m_Modules[m_iCurrentModule]->m_bNewLine = true;
					continue;
				}

				AddToken((TokenType)tt.Type, tt.Spaces, tt.Text, -1);
			}
		}
	}

	TToken& ntoken = m_Tokens.Append();

	//is it complete static token?
	if(len == -1 || (CParser::m_pStatic->IsAllocated(token) && token[len] == 0))
	   ntoken.Text = token;
	else
	{
		if(type == TT_QUOTE)
		   ntoken.Text = m_Modules[m_iCurrentModule]->Alloc(token + 1, len - 2);
		else
		   ntoken.Text = m_Modules[m_iCurrentModule]->Alloc(token, len);
	}

   //and pointer from static array

   ntoken.Type = (short)type;
   ntoken.Spaces = spaces;

	if(m_pCDef)
	{
		int mac = (int)(m_pCDef - m_Defs.GetArray());

		if(m_iFirstMacro < 0)
			m_iFirstMacro = mac;

		m_pCDef->m_iNumTokens++;
	}
	else
		m_Lines[m_Lines.GetCardinality() - 1].m_iNumTokens++;

   return true;
}

//----------------------------------------------------------------------
const char *CParser::SkipSpaces(const char *start, const char *end, int *s)
{
	*s = 0;

	while(start < end)
   {
   	if(*start == 0x20)
			*s += 1;
		else
   	if(*start == 0x09)
			*s += 3;
		else
			break;

   	start++;
   }

   return start;
}

//----------------------------------------------------------------------
const char *CParser::ParseString(const char *start, const char *end, TokenType& ttype)
{
	if(*start == '"')	//quoted string
   {
   	ttype = TT_QUOTE;

   	start++;
		while(start < end)
      {
      	if(*start == 0x0d ||
         	*start == 0x0a)
			{
				ttype = TT_NONE;
				return start;
            //return NULL;	//take as error
			}

			if(*start == '"')
         	return start + 1;

         start++;
      }
		return NULL;	//take as error
   }
   else
   {
   	ttype = TT_STRING;

   	while(start < end)
      {
      	//break is tab, space or eol
         if(*start == '\t' || *start == ' ' || *start == 0x0d || *start == 0x0a)
         	break;

         //check if new token
			int dummy;
			if(FindToken(start, end, &dummy)) //TODO: Store it. Avoid searching twice
         	break;

         start++;
      }
   }

   return start;
}

//----------------------------------------------------------------------
const char *CParser::ParseString(const char *start, TokenType& ttype)
{
	if(*start == '"')	//quoted string
   {
   	ttype = TT_QUOTE;

   	start++;
		while(*start)
      {
      	if(*start == 0x0d || *start == 0x0a)
			{
				ttype = TT_NONE;
				return start;
           // return NULL;	//take as error
			}
			if(*start == '"')
         	return start + 1;

         start++;
      }
		return NULL;	//take as error
   }
   else
   {
   	ttype = TT_STRING;

   	while(*start)
      {
      	//break is tab, space or eol
         if(*start == '\t' || *start == ' ' || *start == 0x0d || *start == 0x0a)
         	break;

         //check if new token
			int dummy;
			if(FindToken(start, &dummy)) //TODO: Store it. Avoid searching twice
         	break;

         start++;
      }
   }

   return start;
}

//----------------------------------------------------------------------
const char *CParser::ParseNumber(const char *start, const char *end, TokenType& ttype)
{
	if(*start != '-' && (*start < '0' || *start > '9'))
   	return NULL;

bool allowsign = true, allowdot = true;
int nnum = 0, ndec = 0;

bool allowhexa = (*start == '0');
bool hexa = false;
bool exponent = false;

	while(start < end)
   {
   	if(allowhexa)
      {
      	if(*start == 'x')
         {
         	start++;
            allowhexa = false;
            hexa = true;
            continue;
         }
      }

   	if(allowsign)
      {
      	if(*start == '-')
         {
         	start++;
            allowsign = false;
            continue;
         }
      }

   	if(allowdot)
      {
      	if(*start == '.')
         {
         	start++;
            allowdot = false;
            continue;
         }
      }

      if(!((*start >= '0' && *start <= '9') ||
           (hexa && *start >= 'a' && *start <= 'f')))
		{
			if(!exponent && *start == 'e')
			{
				exponent = true;
				start++;
				if(*start == '-')
					start++;
				continue;
			}

			//TODO: accept only defined tokens as number terminator. Everything else should be error!
			if(*start == '_')
				return NULL;

         break;
		}

		if(!allowdot || hexa)
      	ndec++;
      else
      	nnum++;
      start++;
   }

   if(!allowdot && ndec == 0)
   	return NULL;	//should be taken as error

   if(hexa && ndec == 0)
   	return NULL;	//should be taken as error

   if(nnum == 0)
   	return NULL;

   if(hexa)
		ttype = TT_HEXA;
   else
   	ttype = TT_NUMBER;

   return start;
}

//----------------------------------------------------------------------
const char *CParser::ParseNumber(const char *start, TokenType& ttype)
{
	if(*start != '-' && (*start < '0' || *start > '9'))
   	return NULL;

bool allowsign = true, allowdot = true;
int nnum = 0, ndec = 0;

bool allowhexa = (*start == '0');
bool hexa = false;

	while(*start)
   {
   	if(allowhexa)
      {
      	if(*start == 'x')
         {
         	start++;
            allowhexa = false;
            hexa = true;
            continue;
         }
      }

   	if(allowsign)
      {
      	if(*start == '-')
         {
         	start++;
            allowsign = false;
            continue;
         }
      }

   	if(allowdot)
      {
      	if(*start == '.')
         {
         	start++;
            allowdot = false;
            continue;
         }
      }

      if(!((*start >= '0' && *start <= '9') ||
           (hexa && *start >= 'a' && *start <= 'f')))
         break;

		if(!allowdot || hexa)
      	ndec++;
      else
      	nnum++;
      start++;
   }

   if(!allowdot && ndec == 0)
   	return NULL;	//should be taken as error

   if(hexa && ndec == 0)
   	return NULL;	//should be taken as error

   if(nnum == 0)
   	return NULL;

   if(hexa)
		ttype = TT_HEXA;
   else
   	ttype = TT_NUMBER;

   return start;
}

//----------------------------------------------------------------------
uint CParser::Unparse(const TToken* tokens, uint numtokens, char* buff, uint size) const
{
	int pos = 0;
	const char* end = buff + size;

	for(uint t = 0; t < numtokens; t++)
	{
		if((buff + tokens->Spaces + 1) >= end)
			break;

		FillMem(buff, tokens->Spaces, ' ');
		buff += tokens->Spaces;

		if(tokens->Type == TT_QUOTE)
			*buff++ = '\"';

		const char* src = tokens->Text;

		while(*src && buff < (end - 1))
			*buff++ = *src++;

		if(tokens->Type == TT_QUOTE && buff < (end - 1))
			*buff++ = '\"';

		tokens++;
	}
	*buff = 0;
	
	return (size - (end - buff));
}

//----------------------------------------------------------------------
bool CParser::Unparse(const TLine *fline, const TLine *lline, char *buff, int size) const
{
	int pline = fline->m_iSourceLine - 1;

	while(fline <= lline)
	{
		for(uint n = 0; n < fline->m_iNumTokens; n++)
		{
			const TToken* tt = GetToken(fline, n);
			int l = strlen(tt->Text);

			//add preceding spaces
			FillMem(buff, tt->Spaces, ' ');
			buff += tt->Spaces;

			//add token
			CopyMem(buff, tt->Text, l + 1);
			buff += l;
		}

		//add EOLs
		for(int n = 0; n < (fline->m_iSourceLine - pline); n++)
			*buff++ = '\n';

		pline = fline->m_iSourceLine;
		fline++;
	}

	
	*buff = 0;
	return true;
}

/*
//----------------------------------------------------------------------
ParseError CParser::operator <<(const DString& str)
{
	Clear();
	return Parse("_memory_", str.cstr(), str.Length());
}

//----------------------------------------------------------------------
ParseError CParser::operator <<(CStr str)
{
	Clear();
	return Parse("_memory_", str.cstr(), str.Length());
}
*/

//----------------------------------------------------------------------

ParseError CParser::Parse(CStr mod, const char* ptr, int size)
{
	if(!mod.IsNull())
	{
		TModule* tmod = FindModule(mod);
		if(tmod != NULL)
		{
			for(int n = tmod->m_iFirstLine; n < tmod->m_iLastLine; n++)
			{
				m_Lines.Insert(m_Lines[n]);
			}
			return PE_OK;
		}
	}
 
	uint prev = m_iCurrentModule;

	if(AddModule(mod, size) == NULL)
	{
		m_Res = PE_OUT_OF_MEMORY;
		if(InternalError(PE_OUT_OF_MEMORY, mod.cstr(), 1) == PR_BREAK_PARSING)
			return PE_OUT_OF_MEMORY;
	}

char tname[FILENAME_MAX];
SString<256> buff;

	const char* lptr = ptr;
	const char* eptr = ptr + size;

	bool incl = false;
	bool defn = false;
   bool comm = false;
	bool lcomm = false;
	int spaces = 0;
	bool ifdefs[32];
	int ifdef = 0;

	m_pCDef = NULL;
   m_Modules[m_iCurrentModule]->m_iSourceLine = 1;
   m_Modules[m_iCurrentModule]->m_bNewLine = true;

//	m_Modules[m_iCurrentModule]->m_iCRC = GetCRC((ubyte*)ptr, size);

   while(lptr < eptr)
   {
      int l,s;

      const char* t = SkipSpaces(lptr, eptr, &s);

		spaces += s;

      if(t != lptr)
      {
      	lptr = t;
         continue;
      }

      //skip cr
      if(*lptr == 0x0d)
      {
      	lptr++;
         continue;
      }

      //check lf
      if(*lptr == 0x0a)
      {
      	lptr++;
			m_Modules[m_iCurrentModule]->m_iSourceLine++;

         lcomm = false;
         incl = false;
			spaces = 0;

			if(defn)
			{
				if(m_pCDef->m_iNumTokens < 1 || m_Tokens[m_pCDef->m_iFirstToken + m_pCDef->m_iNumTokens - 1].Text != TBackslash)
				{
					defn = false;
					m_pCDef = NULL;
				}
			}
			else
				m_Modules[m_iCurrentModule]->m_bNewLine = true;

         continue;
      }

      //if rest of line commented, skip
      if(lcomm)
      {
      	lptr++;
         continue;
      }

      //if we are in commented block, find eoc
      if(comm)
      {
			if((eptr - lptr) >= 2 && lptr[0] == '*' && lptr[1] == '/')
			{
				lptr += 2;
				comm = false;
			}
			else
      		lptr++;

         continue;
      }

		//if we are under false condition, skip
		if(ifdef > 0)
		{
		   if(!strncmp(lptr, TElse, 4))
         {
				ifdefs[ifdef] = !ifdefs[ifdef];
				lptr += 5;
            continue;
         }

			if(!strncmp(lptr, TEndif, 5))
			{
				ifdef--;
				lptr += 6;
				continue;
			}

			if(ifdefs[ifdef] == false)
			{
				lptr++;
				continue;
			}
		}

      if(!comm && !lcomm)
      {
			TokenType ttype;
         //try to parse number
         //numbers must precede FindToken because of '-' and '.'
         t = ParseNumber(lptr, eptr, ttype);
         if(t)
         {
				AddToken(ttype, spaces, lptr, t - lptr);
            lptr = t;
				spaces = 0;
            continue;
         }
		}


   	//try to find defined token
		const char* td = FindToken(lptr, eptr, &l);
		if(td != NULL)
      {
      	incl = false;	//allow empty #include

         lptr += l;	//skip token

      	if(td == TComment)
         {
         	lcomm = true;
            continue;
         }

      	if(td == TLComment)
         {
         	comm = true;
            continue;
         }

         if(td == TInclude)
         {
         	incl = true;
            continue;
         }

         if(td == TDefine)
         {
         	defn = true;
            continue;
         }
			
			if(td == TDLine)
			{
				buff.format("\"%d\"", m_Modules[m_iCurrentModule]->m_iSourceLine);
				AddToken(TT_QUOTE, spaces, buff.cstr(), buff.Length());
				spaces = 0;
				continue;
			}

			if(td == TDFile)
			{
				buff.format("\"%s\"", m_Modules[m_iCurrentModule]->GetName());
				AddToken(TT_QUOTE, spaces, buff.cstr(), buff.Length());
				spaces = 0;
				continue;
			}

         if(td == TIfDef || td == TIfNDef)
         {
		      lptr = SkipSpaces(lptr, eptr, &s);
				TokenType ttype;
				t = ParseString(lptr, eptr, ttype);

				if(t != lptr && lptr != eptr && *lptr != 0x0a)
				{
         		ifdef++;
					ifdefs[ifdef] = IfDef(lptr, t - lptr) ^ (td == TIfNDef);
					lptr += (t - lptr);
				}

            continue;
         }

			//add token
         AddToken(TT_DEF, spaces, td, l);
			spaces = 0;
         continue;
      }

      if(comm || lcomm)
      	continue;

      //try to find free tokens
		TokenType ttype;
      t = ParseString(lptr, eptr, ttype);
		if(ttype == TT_NONE)
		{
			m_Res = PE_PARSE_QUOTED_STRING;
			LError("CParser: quoted string not closed on line %d, module %s", m_Modules[m_iCurrentModule]->m_iSourceLine, m_Modules[m_iCurrentModule]->GetName());
			//return PE_PARSE_QUOTED_STRING;
		}

      if(t > lptr)
      {
      	if(incl)
         {
         	incl = false;
            Unquote(tname, lptr, t - lptr);

				//carry error from included file
				ParseError perr = Parse(tname, false);
				if(perr != PE_OK)
				{
					LError("Can't found file '%s'", tname);
					m_Res = perr;
				}
         }
         else
			{
      		if(defn && m_pCDef == NULL)
					m_pCDef = AddDef(CParser::m_pStatic->AddString(lptr, t - lptr));
				else
				{
         		AddToken(ttype, spaces, lptr, t - lptr);
				}
			}

			lptr = t;
			spaces = 0;
         continue;
      }

      //nothing works, alloc one char as token - is it neccessary?
      AddToken(TT_STRING, spaces, lptr, 1);
      lptr++;
   }

   m_iCurrentModule = prev;

	Assert(lptr == eptr);
	if(GetNumLines() > 0)
		BeginLine(0);
   
	return m_Res;
}

//----------------------------------------------------------------------
ParseError CParser::Parse(CStr filename, bool clearContent)
{
	if(filename.IsNull())
		return PE_MISSING_FILE;

	CLine = NULL;

	if(clearContent)
		Clear();
  CFileInputStream instream(filename.cstr());
  if(!instream.IsOpen())
    return PE_MISSING_FILE;

  size_t size = instream.GetSize();
  MemoryBuffer mem(size + 1);
  void* ptr = ptr = mem.Append(size + 1);
  instream.Read(ptr, size);
  ((char*)ptr)[size] = 0;

	ParseError res = Parse(filename, (char *)ptr, size);

	if(res == PE_OK)
	{
		BeginLine(0);
	}

	return res;
}

//----------------------------------------------------------------------
const TLine& CParser::BeginLine(uint line)
{
	Assert(line < m_Lines.GetCardinality());
	CLine = m_Lines.GetArray() + line;
	ELine = m_Lines.GetArray() + m_Lines.GetCardinality();
	CParm = 0;
	CToken = &m_Tokens[CLine->m_iFirstToken];
	return *CLine;
}


//----------------------------------------------------------------------
ParseError CParser::Parse(CInputStream* instream, bool clearContent)
{
	if(!instream->IsOpen())
		return PE_MISSING_FILE;

	if(clearContent)
		Clear();

	MemoryBuffer mbuff(instream->GetSize());
	
	ubyte* ptr = mbuff.Append(instream->GetSize());
	instream->Read(ptr, instream->GetSize());

	ParseError res = Parse(NULL, (const char*)mbuff.GetData(), mbuff.GetSize());

	if(res == PE_OK && GetNumLines() > 0)
	{
		BeginLine(0);
	}

	return res;
}

}