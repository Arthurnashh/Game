#ifdef _MSC_VER
#  pragma once
#endif

/**
    @file   netlog.hpp
    @brief  NetLogger - General purpose logging object.

    Copyright &copy; 2002-2003 by BIStudio (www.bistudio.com)
    @author PE
    @since 13.3.2002
    @date  18.6.2003
*/

#ifndef _NETLOG_H
#define _NETLOG_H

#include "Es/Threads/pocritical.hpp"
#include "Es/Common/global.hpp"
#include "logflags.hpp"

//-------------------------------------------------------------------------
//  NetLogger object:

#if !defined(_WIN32) || defined NET_LOG

/**
    General-purpose logger.
    Writes into "netNNN.log" text files (NNN is 001, 002, ..).
*/
class NetLogger {

protected:

    FILE *f;

    unsigned counter;                       ///< for fflush.

    unsigned period;                        ///< fflush period (must be in form of 2^k-1).

    PoCriticalSection cs;                   ///< for exclusivity.

    unsigned64 startTime;                   ///< in getSystemTime() format (in microseconds).

    void open ();

public:

    NetLogger ( unsigned _period =0x1ff );

    ~NetLogger ();

    void log ( const char *format, va_list argptr );

    double getTime () const;

    void flush ();

    };

#endif  // NET_LOG

#endif
