//-----------------------------------------------------------------------------
// File: enf_strings.h
//
// Desc: String types
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_STRINGS_H
#define ENF_STRINGS_H

#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#ifdef __GCC__
	#include <stdlib.h>
	#include <wchar.h>
#endif
#include "enf_config.h"
#include "common/enf_inttypes.h"
#include "system/enf_memory.h"

//#include "Es\Memory\memAlloc.hpp"
//#include "El\Math\mathDefs.hpp"

namespace enf
{
class StaticBuffer;

/** @defgroup Strings Strings
@ingroup Common
@{*/

extern PUBLIC_API StaticBuffer* g_pStatic;

PUBLIC_API void __vsprintf(char*, const char*, va_list argptr);
PUBLIC_API unsigned int GetCRC(unsigned char* p, int size);

PUBLIC_API int estrlen(const wchar_t *str);
PUBLIC_API void estrcpy(wchar_t *dest, const wchar_t *src);
PUBLIC_API void estrcat(wchar_t *dest, const wchar_t *src);
PUBLIC_API void estrcnv(wchar_t *dest, const char *src);

ENF_INLINEFUNC bool endchar(wchar_t chr)
{
	return (chr == 0);
}

ENF_INLINEFUNC char u2c(wchar_t chr)
{
	return (char)chr;
}

ENF_INLINEFUNC wchar_t c2u (unsigned char chr)
{
	return (wchar_t)chr;
}

PUBLIC_API int enf_strcmpi (const char * dst, const char * src);
PUBLIC_API int enf_strncmpi (const char * dst, const char * src, uint length);
PUBLIC_API int strcmp (const char * dst, const char * src);
PUBLIC_API size_t strlen (const char * src);
PUBLIC_API char* strcat (char * dst, const char * src);
PUBLIC_API char* strcpy(char * dst, const char * src);
PUBLIC_API char* strchr (const char * string, int ch);
PUBLIC_API char* strrchr (const char * string, int ch);
PUBLIC_API uint CountStrings(const char* strings, uint stringPoolSize);
PUBLIC_API void InitStringPool(const char** stringPool, const char* strings, uint count);


/*!
Simple wrapper over const char* strings
to avoid strcmp etc. usage.
*/
class PUBLIC_API CStr
{
private:
	const char* m_pCString;

public:
	CStr(): m_pCString(NULL) {}

	CStr(const char* str): m_pCString(str) {}
	//ENF_INLINE CStr(const DString& str);

	CStr Static() const;

	CStr operator=(const CStr& str)
	{
		m_pCString = str.m_pCString;
		return *this;
	}
	bool IsNull() const
	{
		return (m_pCString == NULL);
	}
	
	bool IsEmpty() const
	{
		return (m_pCString == NULL || m_pCString[0] == 0);
	}

	bool operator==(const char* str) const;
	bool operator!=(const char* str) const;
	bool operator==(const CStr& cstr) const;
	bool operator!=(const CStr& cstr) const;
	bool operator<(const CStr& cstr) const;
	bool operator>(const CStr& cstr) const;
	char operator[](int index) const
	{
		Assert(m_pCString != NULL);
		return m_pCString[index];
	}

	CStr operator=(const char* str)
	{
		m_pCString = str;
		return *this;
	}

	double ToDouble(const double defaultValue = 0.0) const;
	float ToFloat(const float defaultValue = 0.0f) const;
	int ToInt(const int defaultValue = 0) const;

	int cmp(CStr str) const
	{
		if(m_pCString == str.m_pCString) return 0;
		if(m_pCString == NULL) return -1;
		if(str.m_pCString == NULL) return 1;
		return strcmp(m_pCString, str.m_pCString);
	}
	
	int cmpi(CStr str) const
	{
		if(m_pCString == str.m_pCString) return 0;
		if(m_pCString == NULL) return -1;
		if(str.m_pCString == NULL) return 1;
		return enf_strcmpi(m_pCString, str.m_pCString);
	}

	int ncmp(CStr str, size_t len) const;
	const char* strchr(int chr) const;
	const char* strrchr(int chr) const;
	const char* strstr(CStr string) const;
	int scanf(const char* format, ...) const;

	unsigned int Length() const;

	const char* cstr() const
	{
		return m_pCString;
	}

	CStr operator+=(int offset)
	{
		while(m_pCString && *m_pCString && offset--)
			m_pCString++;

		return *this;
	}

	CStr operator++(int)
	{
		if(m_pCString && *m_pCString)
			m_pCString++;

		return *this;
	}

	unsigned int GetHashValue() const
	{
		unsigned int hashval = 0;
		if(m_pCString)
		{
			for(const char* s = m_pCString; *s; s++)
				hashval = hashval * 37 + *s;
		}
		return hashval;
	}

};

template<int len = 256> class SString
{
private:
	char		m_Buffer[len];

	const char* _set(const char* str, size_t offset, size_t l)
	{
		Assert((offset + l) < len);
		memcpy(m_Buffer + offset, str, l + 1);

		return m_Buffer;
	}

	int trimleft(char* str)
	{
		if (!str)
			return -1;

		int i = 0, j = 0;
		for (; str[i] < 33; i++)
			;

		if (!i)
			return -1;

		for (; str[i]; i++, j++)
		{
			str[j] = str[i];
		}
		str[j] = 0;

		return j;
	}

	int trimright(char* str, int length = -1)
	{
		if (!str)
			return -1;

		if (length < 0 || str[length] != 0) 
			length = strlen(str);

		for (; length > 0 && str[length - 1] < 33; length--)
			;

		str[length] = 0;

		return length;
	}

	int trim(char* str)
	{
		return trimright(str, trimleft(str));
	}

public:
	SString()
	{
		m_Buffer[0] = '\0';
	}

	SString(CStr str)
	{
		*this = str;
	}

	//!Tells whether is the string empty
	ENF_INLINE bool IsEmpty() const
	{
		return m_Buffer[0] == '\0';
	}

	//!Returns const pointer to the string
	ENF_INLINE const char* cstr() const
	{
		return m_Buffer;
	}

	//!Returns pointer to the first character
	ENF_INLINE const char* First() const { return m_Buffer; }
	//!Returns pointer to the first character
	ENF_INLINE char* First() { return m_Buffer; }
	//!Returns pointer after the last character
	ENF_INLINE const char* Last() const { return m_Buffer + Length(); }
	//!Returns pointer after the last character
	ENF_INLINE char* Last() { return m_Buffer + Length(); }

	//!Returns length of the string
	ENF_INLINE unsigned int Length() const
	{
		return strlen(m_Buffer);
	}

	//!Clears content of the string
	ENF_INLINE void Clear()
	{
		m_Buffer[0] = '\0';
	}

	ENF_INLINE void SetSize(size_t l)
	{
		Assert(l < len);

		m_Buffer[l] = 0;
	}

	ENF_INLINE const char* AppendChar(unsigned int pos, char chr)
	{
		while(pos < len)
		{
			if(m_Buffer[pos] == 0)
			{
				m_Buffer[pos] = chr;
				m_Buffer[pos + 1] = 0;
				return m_Buffer;
			}
			pos++;
		}
		return m_Buffer;
	}

	ENF_INLINE int cmp(CStr str) const
	{
		return strcmp(m_Buffer, str.cstr());
	}

	ENF_INLINE int cmpi(CStr str) const
	{
		return enf_strcmpi(m_Buffer, str.cstr());
	}

	ENF_INLINE int cmpi(const SString& str) const
	{
		return enf_strcmpi(m_Buffer, str.m_Buffer);
	}

	ENF_INLINE const char* operator=(CStr str)
	{
		return _set(str.cstr(), 0, str.Length());
	}

	ENF_INLINE const char* operator=(const SString& str)
	{
		return _set(str.m_Buffer, 0, str.Length());
	}

	ENF_INLINE const char* operator+=(CStr str)
	{
		return _set(str.cstr(), Length(), str.Length());
	}

	ENF_INLINE const char* operator+=(const SString& str)
	{
		return _set(str.m_Buffer, Length(), str.Length());
	}

	ENF_INLINE SString<len> operator+(CStr str) const
	{
		SString<len> result = *this;

		result += str;

		return result;
	}

	ENF_INLINE SString<len> operator+(const SString& str) const
	{
		SString<len> result = *this;

		result += str;

		return result;
	}

	ENF_INLINE bool operator==(const SString& str) const
	{
		return (strcmp(m_Buffer, str.m_Buffer) == 0);
	}

	ENF_INLINE bool operator==(CStr str) const
	{
		return (strcmp(m_Buffer, str.cstr()) == 0);
	}

	ENF_INLINE bool operator!=(const SString& str) const
	{
		return (strcmp(m_Buffer, str.m_Buffer) != 0);
	}

	ENF_INLINE bool operator!=(CStr str) const
	{
		return (strcmp(m_Buffer, str.cstr()) != 0);
	}

	ENF_INLINE char operator[](size_t pos) const
	{
		Assert(pos <= Length());

		return m_Buffer[pos];
	}

	ENF_INLINE long ToLong()
	{
		return atol(m_Buffer);
	}

	ENF_INLINE long ToInt()
	{
		return atoi(m_Buffer);
	}
	
	ENF_INLINE double ToDouble()
	{
		return atof(m_Buffer);
	}

	const char* upper()
	{
		for(unsigned int n = 0; m_Buffer[n] != 0 && n < len; n++)
		{
			m_Buffer[n] = toupper(m_Buffer[n]);
		}
		return m_Buffer;
	}

	const char* lower()
	{
		for(unsigned int n = 0; m_Buffer[n] != 0 && n < len; n++)
		{
			m_Buffer[n] = tolower(m_Buffer[n]);
		}
		return m_Buffer;
	}

	ENF_INLINE size_t Find(const char* chr)
	{
		const char* p = strstr(m_Buffer, chr);

		if(p != NULL)
			return (p - m_Buffer);
		else
			return (size_t)-1;
	}

	ENF_INLINE size_t Find(char chr)
	{
		const char* p = strchr(m_Buffer, (int)chr);

		if(p != NULL)
			return (p - m_Buffer);
		else
			return (size_t)-1;
	}

	ENF_INLINE size_t FindR(char chr)
	{
		const char* p = strrchr(m_Buffer, (int)chr);

		if(p != NULL)
			return (p - m_Buffer);
		else
			return (size_t)-1;
	}

	ENF_INLINE const char* operator=(int num)
	{
		return format("%d", num);
	}

	ENF_INLINE const char* operator=(float num)
	{
		return format("%f", num);
	}

	const char* format(const char *fmt, ...)
	{
		va_list argptr;
		va_start (argptr, fmt);

		__vsprintf(m_Buffer, fmt, argptr);
		
	//		Assert(strlen(m_Buffer) < len);
	//		va_end (argptr);

		//Assert(m_iLength < len);
		va_end(argptr);
		return m_Buffer;
	}

	size_t TrimInPlace()
	{
		int size = trim(m_Buffer);
		return size < 0 ? Length(): size;
	}

	const char* ReplaceExt(const char* ext)
	{
		char* ptr = strrchr(m_Buffer, '.');

		if(ptr)
		{
			strcpy(ptr + 1, ext);
		}
		else
		{
			size_t length = Length();
			m_Buffer[length] = '.';
			strcpy(m_Buffer + length + 1, ext);
		}
		return m_Buffer;
	}

	const char* StripExt()
	{
		int n = 0;
		int ext = -1;
		while(m_Buffer[n])
		{
			if(m_Buffer[n] == '.')
				ext = n;
			n++;
		}

		if(ext != -1)
			m_Buffer[ext] = 0;

		return m_Buffer;
	}

	const char* Copy(const char* str, unsigned int length)
	{
		char* dest  = m_Buffer;
		while(length-- && *str)
		{
			*dest++ = *str++;
		}

		*dest = '\0';
		return m_Buffer;
	}
};

class StaticBuffer;

class CStr;

struct CStrTraits
{
	typedef char CharType;
	typedef unsigned int SizeType;

	static int Compare( const CharType* szStr1, const CharType* szStr2 )
	{
		return strcmp( szStr1, szStr2 );
	}

	static int CompareI( const CharType* szStr1, const CharType* szStr2 )
	{
		return enf_strcmpi( szStr1, szStr2 );
		//return Compare( szStr1, szStr2 );
	}

	static SizeType Length( const CharType* szString )
	{
		return static_cast< SizeType >( strlen( szString ) );
	}

	static const wchar_t* ToWStr(wchar_t* ret, const CharType* szString, SizeType len )
	{
		for( SizeType i = 0; i < len; ++i )
		{
			ret[i] = static_cast< wchar_t >( szString[i] );
		}

		return ret;
	}
};

struct WStrTraits
{
	typedef wchar_t CharType;
	typedef unsigned int SizeType;

	static int Compare( const CharType* szStr1, const CharType* szStr2 )
	{
		return wcscmp( szStr1, szStr2 );
	}

	static int CompareI( const CharType* szStr1, const CharType* szStr2 )
	{
		return Compare( szStr1, szStr2 ); //TODO:
	}

	static SizeType Length( const CharType* szString )
	{
		return static_cast< SizeType >( wcslen( szString ) );
	}

	static const wchar_t* ToWStr(wchar_t* ret, const CharType* szString, SizeType len )
	{
		return szString;
	}
};

template< class TTraits >
class BaseString
{
public:
	typedef typename TTraits::CharType CharType;
	typedef typename TTraits::SizeType SizeType;

	// invalid search return value
	static SizeType npos;

	struct Rep
	{
		SizeType Length, Cap;
		CharType Data[1];
	};

protected:
	//! singleton fields
	PUBLIC_API static Rep m_null;

	// actual string data
	Rep* m_pData;

	// helpers
	CharType* GetData() const { return m_pData->Data; }
	SizeType GetCapacity() const { return m_pData->Cap; }
	CharType* First() const { return GetData(); }
	CharType* Last() const { return GetData() + Length(); }
	Rep* Null() const { return &m_null; }

	//! init storage
	void Init( SizeType size, SizeType cap )
	{
		if( cap )
		{
			m_pData = (Rep*)GMemFunctions->New( cap * sizeof(CharType) + sizeof( Rep ), __FILE__, __LINE__ );
			m_pData->Cap = cap;
			m_pData->Length = size;
			m_pData->Data[size] = 0;
		}
		else
		{
			m_pData = Null();
		}
	}

	//! default init
	void Init( SizeType size )
	{
		Init( size, size );
	}

	//! destruct storage
	void Destroy()
	{
		if( m_pData->Cap > 0 )
		{
			GMemFunctions->Delete( reinterpret_cast< void* >( m_pData ) );
		}
	}

public:
	//! default ctor
	BaseString()
		: m_pData( Null() )
	{}
	//! copy ctor
	BaseString( const BaseString& copy )
	{
		SizeType length = copy.Length();
		Init( length );
		::memcpy( First(), copy.GetData(), length * sizeof(CharType) );
	}
	//! from const char*
	BaseString( const CharType* szCopy )
	{
		if( szCopy )
		{
			SizeType length = TTraits::Length( szCopy );
			Init(length);
			::memcpy( First(), szCopy, length * sizeof(CharType) );
		}
		else
			m_pData = Null();
	}
	//! from const char* partial
	BaseString( const CharType* szCopy, SizeType size )
	{
		if( szCopy )
		{
			Init( size );
			::memcpy( First(), szCopy, size * sizeof(CharType));
		}
		else
			m_pData = Null();
	}
	//! from CStr
	BaseString( const CStr& copy )
	{
		if( !copy.IsEmpty() )
		{
			SizeType len = copy.Length();

			// don't really need this, but only other option is some ugly checking we'd have
			// to do for cleanup of temp string, and this is determined at CT so 100% win!
			if( !IsSameType< CharType, char >::value )
			{
				// 'len' is now length of string in characters, need to convert to unicode size
				Init( static_cast< SizeType >( len ) );

				// I am unicode, convert
			//	CStrTraits::ToWStr(First(), copy.cstr(), len );
				wchar_t* temp = ALLOCA(wchar_t, len);
				CStrTraits::ToWStr(temp, copy.cstr(), len );
				::memcpy( First(), temp, len * sizeof(CharType) );
			}
			else
			{
				Init( static_cast< SizeType >( len ) );
				::memcpy( First(), copy.cstr(), len * sizeof(CharType) );
			}
		}
		else
			m_pData = Null();
	}

	//! dtor
	~BaseString()
	{
		Destroy();
	}

	//! swap contents with another BaseString
	void Swap( BaseString& other )
	{
		enf_swap(m_pData, other.m_pData);
	}

	void Flush(unsigned int level)
	{
		if(level == ENF_CLEANUP_FLUSHLEVEL)
		{
			Destroy();
			m_pData = Null();
		}
	}
	void SetSize( SizeType size )
	{
		Assert(size <= m_pData->Cap);
		m_pData->Length = size; m_pData->Data[size] = 0;
	}

	/*!
	Assign a value to this instance.
	*/
	BaseString& Assign( const CharType* szString, SizeType len )
	{
		// do not initialize empty string by empty string
		if( len == 0 && m_pData == Null() )
			return *this;

		SizeType cap = GetCapacity();
		//reallocate if capacity is small, or if capacity is much bigger, but initializer is not empty (it's by purpose)
		if( len > cap || (len != 0 && cap > 3 * (len + 8) ))
		{
			BaseString tmp;
			tmp.Init( len );
			::memcpy( tmp.First(), szString, len * sizeof(CharType));
			Swap( tmp );
		}
		else
		{
			::memmove( First(), szString, len * sizeof(CharType));
			SetSize( len );
		}

		return *this;
	}

	/*!
	Append another string.
	*/
	BaseString& Append( const CharType* szString, SizeType len )
	{
		SizeType newSize = Length() + len;
		Reserve( newSize);
		::memmove( Last(), szString, len * sizeof(CharType));
		SetSize( newSize );

		return *this;
	}

	/*!
	Insert substring in the middle.
	*/
	/*BaseString& Insert( const CharType* szString, SizeType at )
	{
		BaseString tmp = Left( at );
		tmp += szString;
		tmp += Right( Length() - at );
		Swap( tmp );

		return *this;
	}*/

	/*!
	Allocate enough space to hold larger string.
	*/
	void Reserve( SizeType size )
	{
		if( size > GetCapacity() )
		{
			BaseString tmp;
			SizeType length = Length();
			tmp.Init( length, size + 8);
			::memcpy( tmp.First(), GetData(), length * sizeof(CharType) );
			Swap( tmp );
		}
	}

	/*!
	\name Comparison
	*/
	//@{

	int Compare( const CharType* szString, bool bCaseSensitive = true ) const
	{
		if( bCaseSensitive )
		{
			return TTraits::Compare( GetData(), szString );
		}
		else
		{
			return TTraits::CompareI( GetData(), szString );
		}
	}

	int Compare( const BaseString& other, bool bCaseSensitive = true ) const
	{
		if( bCaseSensitive )
		{
			return TTraits::Compare( GetData(), other.GetData() );
		}
		else
		{
			return TTraits::CompareI( GetData(), other.GetData() );
		}
	}

	//@}

	/*!
	\name Analysis
	*/
	//@{

	//! Substring from left
	BaseString Left( SizeType size ) const
	{
		Assert( size <= Length() );
		return BaseString( GetData(), size );
	}

	//! Substring from right
	BaseString Right( SizeType size ) const
	{
		Assert( size <= Length() );
		return BaseString( GetData() + Length() - size, size );
	}

	//! Substring from middle
	BaseString Mid( SizeType from, SizeType count ) const
	{
		Assert( from + count <= Length() );
		return BaseString( GetData() + from, count );
	}

	//! Single character
	CharType At( SizeType at ) const
	{
		Assert( at < Length() );
		return GetData()[at];
	}

	void Lower()
	{
		for(unsigned int n = 0; n < Length(); n++)
		{
			CharType c = GetData()[n];
			//if(c <= 255)
			GetData()[n] = tolower(c);
		}
	}

	//! Find first character occurrence in the string
	SizeType Find( CharType toFind, SizeType start = 0 ) const
	{
		if( start >= Length() ) return npos;

		for( const CharType* c = GetData() + start; *c; ++c )
		{
			if( *c == toFind )
				return static_cast< SizeType >( c - GetData() );
		}

		return npos;
	}

	//! Find last character occurrence in the string
	SizeType FindReverse( CharType toFind, SizeType start = 0 ) const
	{
		if( start >= Length() ) return npos;

		for( const CharType* c = GetData() + Length() - 1; c >= (GetData() + start) && *c; c-- )
		{
			if( *c == toFind )
				return static_cast< SizeType >( c - GetData() );
		}

		return npos;
	}

	//! strip extension from string and return it
	BaseString StripExt()
	{
		SizeType pos = FindReverse('.', 0);
		if(pos != npos)
		{
			BaseString ext = Right(pos + 1);
			*this = Left(pos);
			return ext;
		}

		return "";
	}


	//! Find substring occurrence in the string
	SizeType Find( const BaseString& strSub, SizeType start = 0 ) const
	{
		if( start >= Length() || strSub.Length() < 1 ) return npos;
		
		SizeType at = 0, ret = npos;
		for( const CharType* c = GetData() + start; *c; ++c )
		{
			if( *c == strSub[at] )
			{
				if( at == 0 )
				{
					// first match
					ret = c - GetData();
				}

				if( ++at >= strSub.Length() ) break; // found it!
			}
			else if( at != 0 )
			{
				// reset search
				at = 0;
				ret = npos;
			}
		}

		return ret;
	}

	//! Find substring occurrence in the string (const char* style version)
	SizeType Find( const CharType* szSub, SizeType start = 0 ) const
	{
		SizeType len = TTraits::Length( szSub );
		if( start >= Length() || len < 1 ) return npos;

		SizeType at = 0, ret = npos;
		for( const CharType* c = GetData() + start; *c; ++c )
		{
			if( *c == szSub[at] )
			{
				if( at == 0 )
				{
					// first match
					ret = c - GetData();
				}

				if( ++at >= len ) break; // found it!
			}
			else if( at != 0 )
			{
				// reset search
				at = 0;
				ret = npos;
			}
		}

		return ret;
	}

	//@}

	/*!
	\name Converters
	*/
	//@{

	const CharType* Str() const { return GetData(); }

	const CharType* strrchr(CharType c) const
	{
		uint pos = Length();
		while(pos--)
		{
			if(m_pData->Data[pos] == c)
				return &m_pData->Data[pos];
		}
		return NULL;
	}
	//const char* CStr() const { return 0; /* TODO */ }
	//const wchar_t* WStr() const { return 0; /* TODO */ }

	//@}

	/*!
	Length of the string.
	*/
	SizeType Length() const { return m_pData->Length; }

	/*!
	Check emptiness.
	*/
	bool IsEmpty() const { return (m_pData->Length == 0); }

	/*!
	Clear the string.
	*/
	void Clear()
	{
		Destroy();
		Init( 0 );
	}

	/*!
	\name Operators
	*/
	//@{

	operator CStr() const { return CStr(cstr()); }

	BaseString& operator=( const CharType* szString )
	{
		if( !szString )
		{
			Clear();
			return *this;
		}

		return Assign( szString, TTraits::Length( szString ) );
	}

	BaseString& operator=( const BaseString& other )
	{
		return Assign( other.First(), other.Length() );
	}

	BaseString& Remove(uint from, uint count)
	{
		Assert((from + count) <= Length());
		uint len = Length();
		memmove(m_pData->Data + from, Data() + from + count, sizeof(CharType) * (Length() + 1 - (from + count)));
		m_pData->Length = len - count;
		return *this;
	}

	BaseString& Insert(uint to, CharType chr)
	{
		Assert(to < Length());

		uint len = Length();
		Reserve(len + 1);
		
		memmove(m_pData->Data + to + 1, Data() + to, sizeof(CharType) * (Length() + 1 - to));
		m_pData->Data[to] = chr;
		m_pData->Length = len + 1;
		return *this;
	}
	BaseString& operator=( CStr copy )
	{
		if( !copy.IsNull() )
		{
			SizeType len = copy.Length();

			if( !IsSameType< CharType, char >::value )
			{
				wchar_t* temp = ALLOCA(wchar_t, len);
				CStrTraits::ToWStr(temp, copy.cstr(), len );
				BaseString& s = Assign( (CharType*)temp, len);
				return s;
			}
			else
			{
				return Assign( (CharType*)copy.cstr(), len );	
			}
		}
		else
		{
			Clear();
			return *this;
		}
	}

	BaseString& operator+=( const CharType* szString )
	{
		return Append( szString, TTraits::Length( szString ) );
	}

	BaseString& operator+=( const BaseString& other )
	{
		return Append( other.GetData(), other.Length() );
	}

	BaseString& operator+=( const CStr& other )
	{
		SizeType len = other.Length();

		if( !IsSameType< CharType, char >::value )
		{
			wchar_t* temp = ALLOCA(wchar_t, len);
			CStrTraits::ToWStr(temp, other.cstr(), len );
			BaseString& s = Append( (CharType*)temp, len );
			return s;
		}
		else
		{
			return Append( (CharType*)other.cstr(), len );	
		}
	}

	BaseString& operator+=( CharType c )
	{
		SizeType newSize = Length() + 1;
		Reserve( newSize);
		*Last() = c;
		SetSize( newSize );
		return *this;
	}

	BaseString operator+( const CharType* szString ) const
	{
		BaseString res = *this;
		res.Append( szString, TTraits::Length( szString ) );
		return res;
	}

	BaseString operator+( const BaseString& other ) const
	{
		BaseString res = *this;
		res.Append( other.GetData(), other.Length() );
		return res;
	}

	bool operator==( const CharType* szString ) const
	{
		return (Compare( szString ) == 0);
	}

	bool operator==( const BaseString& other ) const
	{
		return (Compare( other ) == 0);
	}

	bool operator!=( const CharType* szString ) const
	{
		return (Compare( szString ) != 0);
	}

	bool operator!=( const BaseString& other ) const
	{
		return (Compare( other ) != 0);
	}

	CharType operator[]( const SizeType pos ) const
	{
		return At( pos );
	}

	BaseString operator>>( SizeType count ) const
	{
		// substring from left
		return Left( Length() - count );
	}

	BaseString operator<<( SizeType count ) const
	{
		// substring from right
		return Right( Length() - count );
	}

	bool operator>( const BaseString& other ) const
	{
		return (Compare( other ) > 0);
	}

	bool operator<( const BaseString& other ) const
	{
		return (Compare( other ) < 0);
	}
	//@}

	/*!
	\name Utils
	*/
	//@{

	unsigned int GetHashValue() const
	{
		unsigned int hash = 0;
		for( const CharType* c = GetData(); *c; ++c )
		{
			hash = hash * 37 + *c; //???
		}
		return hash;
	}

	//@}

	/*!
	DString compatibility API
	*/
	//@{

	BaseString& setConstant( const CharType* copy )
	{
		return Assign( copy, TTraits::Length( copy ) );
	}

	const CharType* c_str() const { return Str(); }
	const CharType* cstr() const { return Str(); }
	const CharType* Data() const { return Str(); }

	int cmp( const BaseString& other ) const { return Compare( other ); }
	int cmpi( const BaseString& other ) const { return Compare( other, false ); }

	SizeType Capacity() const { return m_pData->Cap; }

	//@}
};

// operators
template< class T >
BaseString< T > operator+( const typename BaseString< T >::CharType* str1, const BaseString< T >& str2 )
{
	BaseString< T > ret( str1 );
	ret += str2;
	return ret;
}

// static fields
//template< class T >
//typename BaseString< T >::Rep BaseString< T >::m_null = { 0, 0, '\0' }; // default char
/*
template<>
typename BaseString< CStrTraits >::Rep BaseString< CStrTraits >::m_null = { 0, 0, '\0' };
template<>
typename BaseString< WStrTraits >::Rep BaseString< WStrTraits >::m_null = { 0, 0, L'\0' };
*/
template< class T >
typename BaseString< T >::SizeType BaseString< T >::npos = -1;

// typedefs
typedef BaseString< CStrTraits > DString;
typedef BaseString< WStrTraits > WString;

// hash func
template< class TTraits >
struct StringHashFuncCS
{
	unsigned int operator()( const BaseString< TTraits >& str ) const
	{
		return str.GetHashValue();
	}

	int compare( const BaseString< TTraits >& str1, const BaseString< TTraits >& str2 ) const
	{
		return str1.cmp( str2 );
	}
};
typedef StringHashFuncCS< CStrTraits > DStringHashFuncCS;
typedef StringHashFuncCS< WStrTraits > WStringHashFuncCS;

struct CStrHashFuncCS
{
	unsigned int operator()(const CStr& str) const
	{
		return str.GetHashValue();
	}

	int compare(const CStr& str1, const CStr& str2) const
	{
		return str1.cmp(str2);
	}
};

struct CStrHashFunc
{
	unsigned int operator()(const CStr& str) const
	{
		unsigned int hashval = 0;
		for(const char* s = str.cstr(); *s; s++)
			hashval = hashval * 37 + ::tolower(*s);

		return hashval;
	}

	int compare(const CStr& str1, const CStr& str2) const
	{
		return str1.cmpi(str2);
	}
};

//ENF_INLINE CStr::CStr(const DString& str) : m_pCString( str.c_str() ) {}

//#pragma warning( disable : 4996 )
//#pragma warning( default : 4996 )
//@}
}

#endif //ENF_STRINGS
