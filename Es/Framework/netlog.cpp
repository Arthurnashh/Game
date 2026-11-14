/**
  @file   netlog.cpp
  @brief  NetLog - General purpose logging object.

  Copyright &copy; 2002-2003 by BIStudio (www.bistudio.com)
  @author PE
  @since  12.3.2002
  @date   6.11.2003
*/

#include <Es/essencepch.hpp>
#include <time.h>
#include "netlog.hpp"
#include "Es/Framework/potime.hpp"
#include "Es/Strings/bString.hpp"

//------------------------------------------------------------
//  Logging (thread-safe):

bool netLogValid = false;

#if !defined(_WIN32) || defined NET_LOG

NetLogger::NetLogger ( unsigned _period )
    : LockInit(cs,"NetLogger",true)
{
    period = 0;
    while ( _period ) {
        _period >>= 1;
        period += period + 1;
        }
    cs.enter();
    f = NULL;
    startSystemTime();
    startTime = getSystemTime();
    netLogValid = true;
    f = fopen("nolog.txt","rt");
    if ( f ) {
        fclose(f);
        f = NULL;
        netLogValid = false;
        }
    cs.leave();
}

void NetLogger::open ()
{
    if ( f ) return;
    cs.enter();
    int i = 1;
    char buf[32];
#ifdef NET_LOG_BRIEF
    f = fopen("net.log","a+t");
#else
    do
    {
      sprintf(buf,"net%03d.log",i++);
      f = fopen(buf,"rt");
      if ( !f ) break;
      fclose(f);
    } while ( true );
    f = fopen(buf,"wt");
#endif
    counter = 0;
        // initial message:
    time_t timet;
    time(&timet);
    timet -= (time_t)((getSystemTime()-startTime)/1000000);
    strcpy(buf,ctime(&timet));
    buf[strlen(buf)-1] = (char)0;           // remove trailing '\n'
#ifdef NET_LOG_BRIEF
    fprintf(f,"%9.3f: NetLogger: start - %s\n",0.0,buf);
    fprintf(f,"%9.3f: Clk(%u)\n",0.0,getClockFrequency());
#else
    fprintf(f,"%10.5f: NetLogger: start - %s\n",0.0,buf);
    fprintf(f,"%10.5f: Clock frequency = %u Hz\n",0.0,getClockFrequency());
#endif
    cs.leave();
}

NetLogger::~NetLogger ()
{
    if ( !netLogValid || !f ) return;
    char buf[32];
    time_t timet;
    time(&timet);
    strcpy(buf,ctime(&timet));
    buf[strlen(buf)-1] = (char)0;           // remove trailing '\n'
    netLog("NetLogger: stop - %s",buf);
    netLogValid = false;
    cs.enter();
    if ( f ) {
        fclose(f);
        f = NULL;
        }
    cs.leave();
}

double NetLogger::getTime () const
{
    return( 1.e-6 * (getSystemTime() - startTime) );
}

void NetLogger::log ( const char *format, va_list argptr )
{
    cs.enter();
    if ( netLogValid ) {
        if ( !f ) open();                   // deferred file open
        Assert( f );
        BString<2048> buf;
#ifdef NET_LOG_BRIEF
        sprintf(buf,"%9.3f: ",getTime());
#else
        sprintf(buf,"%10.5f: ",getTime());
#endif
        LString pbuf = buf+strlen(buf);
        vsprintf(pbuf,format,argptr);

        strcat(buf,"\n");
        OutputDebugString(buf);
        fputs(buf,f);
        if ( !(++counter & period) )
            fflush(f);
        }
    cs.leave();
}

void NetLogger::flush ()
{
  cs.enter();
  if ( f ) fflush(f);
  cs.leave();
}

#ifdef _WIN32

#ifdef EXTERN_NET_LOG
  extern
#endif
NetLogger netLogger;

#else

NetLogger netLogger INIT_PRIORITY_URGENT;

#endif

void netLog ( const char *format, ... )
{
    if ( !netLogValid ) return;
    va_list arglist;
    va_start(arglist,format);
    netLogger.log(format,arglist);
    va_end(arglist);
}

double getLogTime ()
{
    if ( !netLogValid ) return 0.0;
    return netLogger.getTime();
}

void netLogFlush ()
{
  if ( !netLogValid ) return;
  netLogger.flush();
}

#else

double getLogTime ()
{
    return 0.0;
}

#endif
