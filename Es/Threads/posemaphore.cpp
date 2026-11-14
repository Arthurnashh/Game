/**
    @file   posemaphore.cpp
    @brief  Portable semaphore objects.

    Copyright (C) 2001-2002 by BIStudio (www.bistudio.com)
    @author PE
    @since 29.11.2001
    @date  19.9.2002
*/

#include <Es/essencepch.hpp>
#include "posemaphore.hpp"

//------------------------------------------------------------
//  Portable simple semaphore:

PoSemaphore::PoSemaphore ( long init, long maxCount )
{
    LockRegister(lock,"PoSemaphore");
    if ( init < 0L ) init = 0L;
#ifdef _WIN32
    handle = CreateSemaphore(NULL,init,maxCount,NULL);
    error = (handle == NULL);
#else
    error = (sem_init(&sem,0,(unsigned)init) != 0);
        // maxCount has to be ignored in pthreads :C
#endif
}

void PoSemaphore::wait ()
{
#ifdef _WIN32
    if ( handle != NULL )
        WaitForSingleObject(handle,INFINITE);
#else
    sem_wait(&sem);
#endif
}

bool PoSemaphore::tryWait ()
{
#ifdef _WIN32
    if ( handle == NULL ) return false;
    return( WaitForSingleObject(handle,0L) == WAIT_OBJECT_0 );
#else
    return( sem_trywait(&sem) == 0 );
#endif
}

void PoSemaphore::signal ( long count )
{
    if ( count < 1L ) count = 1L;
#ifdef _WIN32
    if ( handle == NULL ) {
        error = true;
        return;
        }
    error = (ReleaseSemaphore(handle,count,NULL) != 0);
#else
    error = false;
    while ( !error && count > 0L ) {
        error = (sem_post(&sem) != 0);
        count--;
        }
#endif
}

long PoSemaphore::getValue ()
{
#ifdef _WIN32
    error = true;       // !!! not implemented in Win32 ?
    return 0L;
#else
#  ifdef __CYGWIN__
    error = true;
    return 0L;
#  else
    int val = 0;
    error = (sem_getvalue(&sem,&val) != 0);
    return (long)val;
#  endif
#endif
}

PoSemaphore::~PoSemaphore ()
{
#ifdef _WIN32
    if ( handle != NULL ) {
        CloseHandle(handle);
        handle = NULL;
        }
#else
    sem_destroy(&sem);
#endif
}
