#ifdef _MSC_VER
#  pragma once
#endif

#ifndef _ES_FILENAMES_HPP
#define _ES_FILENAMES_HPP

#include <string.h>

inline const char *GetFilenameExt( const char *w )
{	// short name with extension
  const char *nam = strrchr(w,'\\');
  if( nam ) return nam+1;
  if( w[0]!=0 && w[1]==':' ) return w+2;
  return w;
}
inline char *GetFilenameExt( char *w )
{	// short name with extension
  char *nam = strrchr(w,'\\');
  if( nam ) return nam+1;
  if( w[0]!=0 && w[1]==':' ) return w+2;
  return w;
}

inline const char *GetFileExt( const char *n )
{ // extension including leading point
  const char *nam=strrchr(GetFilenameExt(n),'.');  // c:\adresar.ext\file = 0 ne .ext\file
  if( nam ) return nam;
  return n+strlen(n);
}
inline char *GetFileExt( char *n )
{ // extension including leading point
  char *nam=strrchr(GetFilenameExt(n),'.');
  if( nam ) return nam;
  return n+strlen(n);
}

inline void TerminateBy( char *s, int c )
{ // if c is not last character, add it
  char *e=s+strlen(s);
  if( e<=s || e[-1]!=c ) *e++=c,*e=0;
}

inline void TerminateBy( wchar_t *s, int c )
{ // if c is not last character, add it
  wchar_t *e=s+wcslen(s);
  if( e<=s || e[-1]!=c ) *e++=c,*e=0;
}

inline void GetFilename( char *dst, const char *sname )
{ // short name, no extension
	const char *name = strrchr(sname,'\\');
	if( name ) name++;
	else name=sname;
	strcpy(dst,name);
	char *ext=strchr(dst,'.');
	if( ext ) *ext=0;
}

inline bool IsRelativePath(const char *path)
{
	return (*path!='\\' && *path!='/' && !strstr(path,"..") && !strchr(path,':'));
}

/// Check if the path is the fully specified (contains drive specification)
bool IsPathAbsolute(const char *name);

//! This function determines wheter the specified mask matches the specified string
bool Matches(const char *s, const char *m, bool caseSens=false);

/// converts path to lowercase, changes all separators to '\', removes duplicate separators, removes last separator (if there is one)
void NormalizePath(char* path);

#endif
