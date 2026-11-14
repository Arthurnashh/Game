/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2006 Erwin Coumans  http://continuousphysics.com/Bullet/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include "btAlignedAllocator.h"

int gNumAlignedAllocs = 0;
int gNumAlignedFree = 0;
int gTotalBytesAlignedAllocs = 0;//detect memory leaks

static void *btAllocDefault(size_t size, int line, const char* file)
{
	return malloc(size);
}

static void btFreeDefault(void *ptr)
{
	free(ptr);
}

static btAllocFunc *sAllocFunc = btAllocDefault;
static btFreeFunc *sFreeFunc = btFreeDefault;



#if defined (BT_HAS_ALIGNED_ALLOCATOR)
#include <malloc.h>
static void *btAlignedAllocDefault(size_t size, int alignment, int line, const char* file)
{
	return _aligned_malloc(size, (size_t)alignment);
}

static void btAlignedFreeDefault(void *ptr)
{
	_aligned_free(ptr);
}
/*
#elif defined(__CELLOS_LV2__)
#include <stdlib.h>

static inline void *btAlignedAllocDefault(size_t size, int alignment, int line, const char* file)
{
	return memalign(alignment, size);
}

static inline void btAlignedFreeDefault(void *ptr)
{
	free(ptr);
}
*/
#else
static inline void *btAlignedAllocDefault(size_t size, int alignment, int line, const char* file)
{
  void *ret;
  char *real;
  unsigned long offset;

  real = (char *)sAllocFunc(size + sizeof(void *) + (alignment-1), line, file);
  if (real) {
    offset = (alignment - (unsigned long)(real + sizeof(void *))) & (alignment-1);
    ret = (void *)((real + sizeof(void *)) + offset);
    *((void **)(ret)-1) = (void *)(real);
  } else {
    ret = (void *)(real);
  }
  return (ret);
}

static inline void btAlignedFreeDefault(void *ptr)
{
  void* real;

  if (ptr) {
    real = *((void **)(ptr)-1);
    sFreeFunc(real);
  }
}
#endif


static btAlignedAllocFunc *sAlignedAllocFunc = btAlignedAllocDefault;
static btAlignedFreeFunc *sAlignedFreeFunc = btAlignedFreeDefault;

void btAlignedAllocSetCustomAligned(btAlignedAllocFunc *allocFunc, btAlignedFreeFunc *freeFunc)
{
  sAlignedAllocFunc = allocFunc ? allocFunc : btAlignedAllocDefault;
  sAlignedFreeFunc = freeFunc ? freeFunc : btAlignedFreeDefault;
}

void btAlignedAllocSetCustom(btAllocFunc *allocFunc, btFreeFunc *freeFunc)
{
  sAllocFunc = allocFunc ? allocFunc : btAllocDefault;
  sFreeFunc = freeFunc ? freeFunc : btFreeDefault;
}

void*	btAlignedAllocInternal	(size_t size, int alignment, int line, const char* file)
{
	gNumAlignedAllocs++;
	void* ptr;
	ptr = sAlignedAllocFunc(size, alignment, line, file);
//	printf("btAlignedAllocInternal %d, %x\n",size,ptr);
	return ptr;
}

void	btAlignedFreeInternal	(void* ptr)
{
	if (!ptr)
	{
		return;
	}

	gNumAlignedFree++;
//	printf("btAlignedFreeInternal %x\n",ptr);
	sAlignedFreeFunc(ptr);
}


