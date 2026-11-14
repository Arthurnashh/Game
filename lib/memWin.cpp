// global redefiniton of new, delete operators
#include "wpch.hpp"
#include "winpch.hpp"

#include "Es/Memory/normalNew.hpp"

// standard memory allocation
// dummy implemenation

void InitMemory( int size )
{
}
void MemoryFast()
{
}
void MemoryGood()
{
}

bool MemoryCheck()
{
	return true;
}


#if _MSC_VER
#define MEM_CONV __cdecl
#else
#define MEM_CONV
#endif

void * operator new ( size_t size, const char *file, int line )
{
	return operator new(size);
}
