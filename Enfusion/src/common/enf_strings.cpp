//-----------------------------------------------------------------------------
// File: enf_strings.cpp
//
// Desc: Strings library
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
#include "common/enf_strings.h"
#include "common/enf_staticbuffer.h"
#include <stdlib.h>
#include <stdio.h>


namespace enf
{
StaticBuffer* g_pStatic = NULL;

template<> PUBLIC_API BaseString< CStrTraits >::Rep BaseString< CStrTraits >::m_null = { 0, 0, '\0' };
template<> PUBLIC_API BaseString< WStrTraits >::Rep BaseString< WStrTraits >::m_null = { 0, 0, L'\0' };

void __vsprintf(char* buff, const char* fmt, va_list argptr)
{
	vsprintf(buff, fmt, argptr);
}

//-----------------------------------------------------------------------------
CStr CStr::Static() const
{
	if(IsNull())
		return NULL;

	Assert2(g_pStatic != NULL, "You must allocate global static buffer prior to this!");
	return CStr(g_pStatic->AddString(cstr()));
}

//-----------------------------------------------------------------------------
uint CountStrings(const char* strings, uint stringPoolSize)
{
	uint count = 0;
	while(stringPoolSize--)
	{
		if(*strings++ == 0)
			count++;
	}
	return count;
}

//-----------------------------------------------------------------------------
void InitStringPool(const char** stringPool, const char* strings, uint count)
{
	for(uint n = 0; n < count; n++)
	{
		stringPool[n] = strings;
		while(*strings++);
	}
}

//-----------------------------------------------------------------------------
char * strchr (const char * string, int ch)
{
	Assert(string != NULL);

	while (*string && *string != (char)ch)
		string++;

	if (*string == (char)ch)
		return((char *)string);

	return(NULL);
}

//-----------------------------------------------------------------------------
char * strrchr (const char * string, int ch)
{
	Assert(string != NULL);

	char *start = (char *)string;
	const char *rchr = NULL;

	while (*string)	// find end of string
	{
		if(*string == ch)
			rchr = string; //remember last occurence
		string++;
	}
	
	return (char*)(rchr);
}

//-----------------------------------------------------------------------------
int enf_strcmpi (const char * dst, const char * src)
{
	Assert(dst != NULL && src != NULL);

	int f, l;

	do
	{
		if ( ((f = (unsigned char)(*(dst++))) >= 'A') && (f <= 'Z') )
			f -= 'A' - 'a';
		if ( ((l = (unsigned char)(*(src++))) >= 'A') && (l <= 'Z') )
			l -= 'A' - 'a';
	} while ( f && (f == l) );

	return(f - l);
}

//-----------------------------------------------------------------------------
int enf_strncmpi (const char * dst, const char * src, uint len)
{
	Assert(dst != NULL && src != NULL);

	int f = 0, l = 0;

	do
	{
		if(len == 0)
			break;
		len--;
		if ( ((f = (unsigned char)(*(dst++))) >= 'A') && (f <= 'Z') )
			f -= 'A' - 'a';
		if ( ((l = (unsigned char)(*(src++))) >= 'A') && (l <= 'Z') )
			l -= 'A' - 'a';
	} while ( f && (f == l) );

	return(f - l);
}

//-----------------------------------------------------------------------------
int strcmp (const char * src, const char * dst)
{
	Assert(dst != NULL && src != NULL);

	int ret = 0 ;

	while( ! (ret = *(unsigned char *)src - *(unsigned char *)dst) && *dst)
		++src, ++dst;

	if ( ret < 0 )
		ret = -1 ;
	else if ( ret > 0 )
		ret = 1 ;

	return( ret );
}

//-----------------------------------------------------------------------------
size_t strlen (const char * str)
{
	Assert(str != NULL);

	const char *eos = str;

	while( *eos++ ) ;

	return( (int)(eos - str - 1) );
}

//-----------------------------------------------------------------------------
char* strcat (char * dst, const char * src)
{
	Assert(dst != NULL && src != NULL);

	char * cp = dst;

	while( *cp )
				cp++;                   /* find end of dst */

	while( (*cp++ = *src++) ) ;       /* Copy src to end of dst */

	return( dst );                  /* return dst */
}

//-----------------------------------------------------------------------------
char* strcpy(char * dst, const char * src)
{
	Assert(dst != NULL && src != NULL);

	char * cp = dst;

	while( (*cp++ = *src++) ); /* Copy src over dst */

	return( dst );
}

//int __cdecl _input(FILE *, const unsigned char *, va_list);

//-----------------------------------------------------------------------------
int CStr::scanf(const char* format, ...) const
{
	Assert(!"Not implemented");

/*	va_list arglist;

	FILE str;
	register FILE *infile = &str;
	register int retval;

	va_start (arglist, format);
	
	Assert(m_pCString != NULL);
	Assert(format != NULL);

	infile->_flag = _IOREAD|_IOSTRG|_IOMYBUF;
	infile->_ptr = infile->_base = (char *) m_pCString;
	infile->_cnt = ((int)strlen(m_pCString))*sizeof(char);
	
	retval = _input(infile, (const unsigned char*)format,arglist);

   return(retval);
*/
	return 0;
}

//-----------------------------------------------------------------------------
double CStr::ToDouble(const double defaultValue) const
{
	if(m_pCString == NULL)
		return defaultValue;

	return ::atof(m_pCString);
}

//-----------------------------------------------------------------------------
float CStr::ToFloat(const float defaultValue) const
{
	if(m_pCString == NULL)
		return defaultValue;

	return (float)::atof(m_pCString);
}

//-----------------------------------------------------------------------------
int CStr::ToInt(const int defaultValue) const
{
	if(m_pCString == NULL)
		return defaultValue;

	return ::atol(m_pCString);
}

//-----------------------------------------------------------------------------
int CStr::ncmp(CStr str, size_t length) const
{
	if(m_pCString == str.m_pCString)
		return 0;

	if(m_pCString == NULL)
		return -1;

	if(str.m_pCString == NULL)
		return 1;

	return strncmp(m_pCString, str.m_pCString, length);
}

//-----------------------------------------------------------------------------
const char* CStr::strstr(CStr str) const
{
	if(m_pCString == NULL)
		return NULL;
	return ::strstr(m_pCString, str.cstr());
}

//-----------------------------------------------------------------------------
const char* CStr::strchr(int chr) const
{
	if(m_pCString == NULL)
		return NULL;
	return ::strchr(m_pCString, chr);
}

//-----------------------------------------------------------------------------
const char* CStr::strrchr(int chr) const
{
	if(m_pCString == NULL)
		return NULL;
	return ::strrchr(m_pCString, chr);
}

//-----------------------------------------------------------------------------
uint CStr::Length() const
{
	if(m_pCString == NULL || *m_pCString == 0)
		return 0;
	return strlen(m_pCString);
}

//-----------------------------------------------------------------------------
bool CStr::operator==(const char* str) const
{
	//allow optimization for constant strings from the same StaticBuffer
	//as sideeffect, we can test it on NULL string
	if(m_pCString == str)
		return true;

	if(str == NULL || m_pCString == NULL)
	{
		return false;
	}

	return m_pCString == str || strcmp(m_pCString, str) == 0;
}

//-----------------------------------------------------------------------------
bool CStr::operator!=(const char* str) const
{
	if(str == NULL || m_pCString == NULL)
	{
		return (m_pCString != str);
	}

	return strcmp(m_pCString, str) != 0;
}

//-----------------------------------------------------------------------------
bool CStr::operator==(const CStr& str) const
{
	return *this == str.m_pCString;
}

//-----------------------------------------------------------------------------
bool CStr::operator!=(const CStr& str) const
{
	return *this != str.m_pCString;
}

//-----------------------------------------------------------------------------
bool CStr::operator<(const CStr& cstr) const
{
	if(m_pCString == NULL || cstr.m_pCString == NULL)
	{
		if(m_pCString == NULL && cstr.m_pCString != NULL)
			return true;
		return false;
	}
	return (strcmp(m_pCString, cstr.m_pCString) < 0);
}

//-----------------------------------------------------------------------------
bool CStr::operator>(const CStr& cstr) const
{
	if(cstr.m_pCString == NULL || m_pCString == NULL)
	{
		if(cstr.m_pCString == NULL && m_pCString != NULL)
			return true;
		return false;
	}
	return (strcmp(m_pCString, cstr.m_pCString) > 0);
}

//-----------------------------------------------------------------------
void estrcnv(wchar_t *dest, const char *src)
{
	while(true)
	{
		*dest++ = c2u(*src);

		if(endchar(*src))
			break;

		src++;
	}
}

//-----------------------------------------------------------------------
int estrlen(const wchar_t *str)
{
	int len = 0;

	while(str[len] != 0)
		len++;

	return len;
}

//-----------------------------------------------------------------------
void estrcpy(wchar_t *dest, const wchar_t *src)
{
	while(true)
	{
		*dest++ = *src;

		if(endchar(*src))
			break;

		src++;
	}
}

//-----------------------------------------------------------------------
void estrcat(wchar_t *dest, const wchar_t *src)
{
	while(!endchar(*dest))
		dest++;

	estrcpy(dest, src);

}

}