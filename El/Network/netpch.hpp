#ifdef _MSC_VER
#  pragma once
#endif

/**
    @file   netpch.hpp
    @brief  Global pre-compiled headers for the "network" subproject.

    Copyright &copy; 2001-2002 by BIStudio (www.bistudio.com)
    @author PE
    @since  18.11.2001
    @date   30.9.2002
*/

#ifndef _NETPCH_H
#define _NETPCH_H

//-------------------------------------------------------------------------
//  External include files:

#include <Es/essencepch.hpp>

#include "netXboxConfig.hpp"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#if _GAMES_FOR_WINDOWS // using special Winsock library
#  include <Es/Common/win.h>
#elif defined _WIN32
#  include <Es/Common/win.h>
#  include <winsock2.h>
#else
#  include <pthread.h>
#  include <semaphore.h>
#  include <unistd.h>
#  include <sys/time.h>
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <netdb.h>
#endif

//-------------------------------------------------------------------------
//  Internal include files:
//  Include base directories required ("-Ixx" command-line option in UNIX):
//      ../../suma
//      ../../Hierarchy

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

#include <El/Network/netglobal.hpp>
#include <El/Network/netapi.hpp>
#include <El/Network/netmessage.hpp>

#endif
