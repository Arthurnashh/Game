#ifdef _MSC_VER
#  pragma once
#endif

/**
    @file   vonPch.hpp
    @brief  Global pre-compiled headers for the "speech" subproject.

    Copyright &copy; 2003 by BIStudio (www.bistudio.com)
    @author PE
    @since  18.1.2003
    @date   26.4.2003
*/

#ifndef _VONPCH_H
#define _VONPCH_H

//-------------------------------------------------------------------------
//  External include files:

#include <Es/essencepch.hpp>

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#if defined _WIN32
#  include <Es/Common/win.h>
#else
#  include <pthread.h>
#  include <semaphore.h>
#  include <unistd.h>
#  include <sys/time.h>
#  include <sys/types.h>
#endif

//-------------------------------------------------------------------------
//  Internal include files:

#include <Es/Types/pointers.hpp>
#include <Es/Containers/array.hpp>
#include <Es/Types/scopeLock.hpp>

#include <Es/Framework/logflags.hpp>
#include <Es/Common/global.hpp>
#include <Es/Threads/pocritical.hpp>
#include <Es/Framework/netlog.hpp>
#include <Es/Algorithms/crc32.h>
#include <Es/Containers/bitmask.hpp>
#include <Es/Containers/maps.hpp>
#include <Es/Framework/potime.hpp>
#include <Es/Threads/pothread.hpp>
#include <Es/Threads/posemaphore.hpp>

#include <El/Common/randomJames.h>

#include <El/Network/netpch.hpp>

#include <El/Speech/vonCodec.hpp>
#include <El/Speech/vonApp.hpp>

#endif
