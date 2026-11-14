// basic memory types

#ifndef _MEMTYPE_H
#define _MEMTYPE_H

typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef unsigned int UINT;
typedef long LONG;

typedef unsigned short word;
typedef unsigned long dword;
typedef unsigned char byte;

#ifndef FALSE
	typedef int BOOL;
	#define FALSE 0
	#define TRUE 1
#endif

#ifndef NULL
	#define NULL 0
#endif

#endif


