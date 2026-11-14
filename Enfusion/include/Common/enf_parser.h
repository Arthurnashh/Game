//-----------------------------------------------------------------------------
// File: enf_parser.cpp
//
// Desc: Simple document model tokenizer
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_PARSER_H
#define ENF_PARSER_H

#include "enf_config.h"
#include "El/Math/mathDefs.hpp"
#include "common/enf_types.h"
#include "Common/enf_strings.h"


namespace enf
{
class CInputStream;

/** @addtogroup Common
@{*/
/*
template<class T, class Orig> class Enum
{
protected:
	T	value;
public:
	Enum();
	Enum(Orig v): value((T)v) {}
	Enum& operator=(
};
*/
//----------------------------------------------------------------------
class TModule
{
friend class CParser;

private:
	SString<128>	m_strName;
	char*		m_strBuffer;
	char*		m_strPosition;
	int		m_Size;
	int		m_iFirstLine;
	int		m_iLastLine;

	//working vars
	bool		m_bNewLine;

public:
	ENF_DECLARE_SMALLOBJECT(TModule);

	int		m_iSourceLine;

	const char* Alloc(const char* buff, int len);

	const char* GetName() const
	{
		return m_strName.cstr();
	}

	TModule(CStr name, int size);
	~TModule();
};

enum TokenType
{
	TT_NONE,
	TT_HEXA		= 1,
	TT_NUMBER,
	TT_DEF,
	TT_QUOTE,
	TT_STRING
};

struct TToken
{
	const char*	Text;
   short			Type;
	short			Spaces;
};

struct TLine
{
	uint			m_iModule;
	uint			m_iFirstToken;
	ushort		m_iSourceLine;
	ushort		m_iNumTokens;
	TLine():
		m_iModule(0),
		m_iFirstToken(0),
		m_iSourceLine(0),
		m_iNumTokens(0)
	{
	}
};

enum ParseResume
{
	PR_SKIP_TOKEN,
   PR_SKIP_LINE,
   PR_SKIP_MODULE,
   PR_BREAK_PARSING
};

enum ParseError
{
	PE_OK,
	PE_MISSING_FILE,
   PE_MISSPLACED_END_OF_COMMENT,
   PE_MISSING_END_OF_COMMENT,
   PE_PARSE_NUMBER,
	PE_PARSE_QUOTED_STRING,
   PE_OUT_OF_MEMORY
};

enum SearchFlags
{
	SF_MATCHCASE	= BITF(0),
	SF_WHOLEWORD	= BITF(1),
	SF_ALLMODULES	= BITF(2),
	SF_FROMTHETOP	= BITF(3)
};

class CParseErrorHandler
{
public:
	virtual ~CParseErrorHandler() {}
	virtual void Error(const char* module, uint line, const char *err) {}
};


class PUBLIC_API CParser
{
protected:
	struct TTDef
	{
		const char*	m_strName;
		uint			m_iFirstToken;
		int			m_iNumTokens;
	};

	struct DToken;

	static bool		m_bInitialized;
	ParseError		m_Res;
	uint				m_iCurrentModule;
  Array<TLine>	m_Lines;
	static TTDef*	m_pCDef;
	Array<TTDef>	m_Defs;
	static SArray<TTDef,32> m_StaticDefs;

	int				m_iFirstMacro;
	CParseErrorHandler* m_pErrorHandler;

	Array<TToken>	m_Tokens;
	Array<TModule*>m_Modules;

	static DToken*	m_pDTokens[256];

	bool IfDef(const char *name, int len);
	TTDef* FindDef(const char *name, int len);

	TModule* FindModule(CStr name);
	TLine* FindSourceLine(uint mod, int srcline);

	static class StaticBuffer* m_pStatic;
	static void		Init();
	ParseResume InternalError(ParseError errcode, const char* modname, int srcline);
	//virtual bool LoadFile(CStr name, void** ptr, size_t* len)=0;
	//virtual void UnloadFile(void* ptr)=0;

	virtual ParseResume Error(ParseError errcode, const char* errdesc, const char *modname, int srcline)
	{
		return PR_BREAK_PARSING;
	}
	static const char* AddDefToken(const char *token);

public:
	ENF_DECLARE_MMOBJECT(CParser);

	const static char	SequenceError[];
	const static char	ParmParseError[];
	const static char	UnexpEolError[];
	const static char	UnexpEofError[];
	const static char	ExpectedError[];
	const static char	GarbageError[];
	const static char	ExpectedTypeError[];

	const static char* TInclude;
	const static char* TDefine;
	const static char* TComment;
	const static char* TLComment;
	const static char* TRComment;
	const static char* TPlusPlus;
	const static char* TMinusMinus;
	const static char* TLShift;
	const static char* TRShift;	
	const static char* TLShiftAssign;
	const static char* TRShiftAssign;
	const static char* TMinusAssign;
	const static char* TPlusAssign;
	const static char* TAsteriskAssign;
	const static char* TSlashAssign;
	const static char* TOrAssign;
	const static char* TAndAssign;
	const static char* TPowerAssign;
	const static char* TNotEqual;
	const static char* TEqual;
	const static char* TOrOr;
	const static char* TAndAnd;
	const static char* TOr;
	const static char* TGreater;
	const static char* TLess;
	const static char* TLessEqual;
	const static char* TGreaterEqual;
	const static char* TAssign;
	const static char* TAnd;
	const static char* TPower;
	const static char* TMinus;
	const static char* TPlus;
	const static char* TAsterisk;
	const static char* TSlash;
	const static char* TLBracket;
	const static char* TRBracket;
	const static char* TLCurly;
	const static char* TRCurly;
	const static char* TComma;
	const static char* TSeparator;
	const static char* TDoubleDot;
	const static char* TFourDot;

	const static char* TLSBracket;
	const static char* TRSBracket;
	const static char* TDot;
	const static char* TExclamation;
	const static char* TSizeof;
	const static char* TBackslash;
	const static char* TElse;
	const static char* TEndif;
	const static char* TIfDef;
	const static char* TIfNDef;
	const static char* TPercent;
	const static char* THash;

	const static char* TDLine;
	const static char* TDFile;

	static void Cleanup();

	void LError(const char *,...);

	//!Sets handler which receive all errors
	void SetErrorHandler(CParseErrorHandler* handler)
	{
		m_pErrorHandler = handler;
	}

	TTDef* AddDef(const char* name);
	static TTDef* AddStaticDef(const char* name);

	bool		m_bCLF; //utility flag, ktery informuje, ze byl volan LError
	const TLine*	CLine;
	const TLine*	ELine;
	TToken*	CToken;
	int		CParm;

  TModule* AddModule(CStr name, int size);
	bool AddToken(TokenType type, int spaces, const char *token, int len = -1);

/** \name Utilities
	Utility methods for string parsing
*/
//@{
	static const char* FindToken(const char *sample, const char *end, int *len);
	static const char* FindToken(const char *sample, int *len);
	static const char* SkipSpaces(const char *start, const char *end, int *s);	
	ENF_INLINE static const char* SkipSpaces(const char *start)
	{
		while(*start == 0x20 || *start == 0x09)
   		start++;

		return start;
	}

	static const char* ParseNumber(const char *start, const char *end, TokenType& ttype);
	static const char* ParseNumber(const char *start, TokenType& ttype);
	static const char* ParseString(const char *start, const char *end, TokenType& ttype);
	static const char* ParseString(const char *start, TokenType& ttype);
//@}

/** \name Iterator
	Sequential iterator over parsed tokens
*/
//@{
	//!Tells whether we are at the end of line
	bool EndOfLine();
	/*!
	Tells whether we are at the end of line.
	If not it outputs error and returns false
	*/
	bool AssertEOL();
	/*!
	Tells whether it is the desired token.
	If not it outputs error and returns false
	*/
	bool AssertToken(const char* str);
	/*!
	Tells whether it is token of desired type.
	If not it outputs error and returns false
	*/
	bool AssertType(TokenType type);
	/*!
	Tells whether there are some tokens left.
	If yes, it outputs error and returns false
	*/
	bool Garbage();
	/*!
	Moves to the next token on the line.
	\param force When true, it expects token. If there is not any, it outputs error
	*/
	bool NextToken(bool force = false);
	/*!
	Moves to the next line.
	\param force When true, it expects line. If there is not any, it outputs error
	*/
	bool NextLine(bool force = false);
	//!Returns order number of token on the line
	int GetCurrentToken() const
	{
		return CParm;
	}
	//!Sets current token by its order number on the line
	int CurrentToken(int token)
	{
		if(token <= CLine->m_iNumTokens)
		{
			CParm = token;
			CToken = m_Tokens.GetArray() + CLine->m_iFirstToken + token;
		}
		return CParm;
	}

	//!Returns a token on the line
	ENF_INLINE TToken* GetToken(uint token)
	{
		Assert(token < CLine->m_iNumTokens);
		return &m_Tokens[CLine->m_iFirstToken + token];
	}

	/*!
	Restarts sequential iterator
	\param line The line we are restarted to
	*/
	const TLine& BeginLine(uint line);
//@}
	
/** \name Random access
	Random access over parsed lines API
*/
//@{
	uint GetNumModules() const
	{
		return m_Modules.GetCardinality();
	}
	TModule* GetModule(uint n) const
	{
		return m_Modules[n];
	}

	ENF_INLINE uint GetNumLines() const
	{
		return m_Lines.GetCardinality();
	}
	ENF_INLINE const TLine& GetLine(uint line) const
	{
		return m_Lines[line];
	}
	ENF_INLINE TToken* GetToken(uint line, uint token)
	{
		Assert(token < m_Lines[line].m_iNumTokens);
		return &m_Tokens[m_Lines[line].m_iFirstToken + token];
	}

	ENF_INLINE const TToken* GetToken(const TLine* line, uint token) const
	{
		Assert(token < line->m_iNumTokens);
		return &m_Tokens[line->m_iFirstToken + token];
	}

	ENF_INLINE TToken* GetToken(const TLine* line, uint token)
	{
		Assert(token < line->m_iNumTokens);
		return &m_Tokens[line->m_iFirstToken + token];
	}
//@}

	TLine* Search(const char *str, int flags, TLine *tl);

	bool Unparse(const TLine* fline, const TLine* lline, char* buff, int size) const;
	uint Unparse(const TToken* tokens, uint numtokens, char* buff, uint size) const;

	//!Clear&free content of parser
	void Clear();
	//!Internal, adds new module (or reuses existing) and parses it from buffer
  ParseError Parse(CStr module, const char* source, int len);
	/*!
	Loads new content from file
	\param fileName File name
	\param clearContent Clears content prior to parsing
	*/
	ParseError Parse(CStr fileName, bool clearContent = true);
	/*!
	Loads new content from stream
	\param instream Input stream
	\param clearContent Clears content prior to parsing
	*/
  ParseError Parse(CInputStream* instream, bool clearContent = true);
	//!Clears and loads new content from string
	ParseError operator <<(const DString& str);
	//!Clears and loads new content from string
	ParseError operator <<(CStr str);
	
	CParser();
   virtual ~CParser();

};

//should be helpful
PUBLIC_API char* Unquote(char *dst, const char *src, int len);

#define TTEqual(a, b) !enf::strcmp((a).Text, b)
#define TTDefEqual(a, b) ((a)->Text == b)
#define ISTOKEN(tp, a) if(!enf::enf_strcmpi((tp).CToken->Text, a))
//@}
}

#endif //ENF_PARSER_H