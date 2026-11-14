#ifndef SIMUL_META_MEMORYINTERFACE_H
#define SIMUL_META_MEMORYINTERFACE_H
#include <stdio.h>
#include <cstdlib>
namespace simul
{
	namespace graph
	{
		namespace meta
		{
			//! A virtual interface class for classes that can allocate and de-allocate memory.

			//! Inherit from MemoryInterface to take control of memory allocation for any class that uses
			//! this interface as a resource.
			class MemoryInterface
			{
			public:
				//! Allocate \param nbytes bytes of memory and return a pointer to them.
				virtual void* Allocate(size_t nbytes)=0;
				//! De-allocate the memory at \param address (requires that this memory was allocated with Allocate()).
				virtual void Deallocate(void* address)=0;
			};
		}
	}
}

inline void* operator new(size_t nbytes,simul::graph::meta::MemoryInterface *mi)
{
	if(!mi)
		return malloc(nbytes);
	return mi->Allocate(nbytes);
}

inline void* operator new[](size_t nbytes,simul::graph::meta::MemoryInterface *mi)
{
	if(!mi)
		return malloc(nbytes);
	return mi->Allocate(nbytes);
}

inline void operator delete[](void *p,simul::graph::meta::MemoryInterface *mi)
{
	if(!mi)
		free(p);
	mi->Deallocate(p);
}

inline void operator delete(void *p,simul::graph::meta::MemoryInterface *mi)
{
	if(!mi)
		free(p);
	mi->Deallocate(p);
}

#endif
