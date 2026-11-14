#ifdef _MSC_VER
#pragma once
#endif

#ifndef _ES__DEB_ALLOC_HPP
#define _ES__DEB_ALLOC_HPP

/*!
\file
Allocator for arrays used for debugging.
*/

/// similiar to MemAllocD, but always using system level allocation

class DebugAlloc
{
  public:
	#if ALLOC_DEBUGGER
	static void *Alloc( size_t &size, const char *file, int line, const char *postfix );
	#else
	static void *Alloc( size_t &size );
	#endif
	static void Free( void *mem, size_t size );
	static void *Realloc(void *mem, size_t oldSize, size_t size);
	static void Unlink( void *mem );
  static inline int MinGrow() {return 32;}

};

#endif
