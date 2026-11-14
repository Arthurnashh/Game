/**
    @file   memPosix.cpp
    @brief  Memory manager for plain POSIX systems.

    Copyright &copy; 2002 by BIStudio (www.bistudio.com)
    @author PE
    @since  7.10.2002
    @date   9.10.2002
*/

#include "Es/essencepch.hpp"
#include "El/FreeOnDemand/memFreeReq.hpp"

void MemoryInit ()
{
}

void MemoryCleanUp ()
{
}

void MemoryDone ()
{
}

MemoryFreeOnDemandList GFreeOnDemand INIT_PRIORITY_URGENT; 

size_t MemoryFreeOnDemand ( size_t size )
{
    return GFreeOnDemand.Free(size);
}

void RegisterMemoryFreeOnDemand ( IMemoryFreeOnDemand *object )
{
    GFreeOnDemand.Register(object);
}
 
#if !_SUPER_RELEASE && !defined _XBOX

#  include "Es/essencepch.hpp"
#  if defined(NET_LOG) && defined(EXTERN_NET_LOG)
NetLogger netLogger INIT_PRIORITY_URGENT;
#  endif

#endif
