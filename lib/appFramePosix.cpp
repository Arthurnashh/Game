#include "Es/essencepch.hpp"
#include "Es/Framework/appFrame.hpp"

#include "Es/Containers/typeDefines.hpp"
#include "Es/Types/pointers.hpp"
#include "Es/Strings/bString.hpp"
#include "Es/Common/global.hpp"
#include <time.h>
#include "El/Debugging/debugTrap.hpp"

class LinuxFrameFunctions : public AppFrameFunctions
{
protected:

#if _ENABLE_REPORT
    FILE *f;
#endif

    unsigned64 startTime;

public:

    LinuxFrameFunctions ()
    {
#if _ENABLE_REPORT
        f = NULL;
#endif
        startTime = getSystemTime();
    }

#if _ENABLE_REPORT
    virtual void LogF ( const char *format, va_list argptr );
    virtual void LstF ( const char *format, va_list argptr );
    virtual void ErrF ( const char *format, va_list argptr );
#endif

    virtual void ErrorMessage ( const char *format, va_list argptr );
    virtual void ErrorMessage ( ErrorMessageLevel level, const char *format, va_list argptr );
    virtual void WarningMessage ( const char *format, va_list argptr );

    virtual DWORD TickCount ()
    {
        return( (DWORD)((getSystemTime() - startTime + 500) / 1000) ); // 1 ms resolution
    }

#if _ENABLE_REPORT
    virtual ~LinuxFrameFunctions ();
#endif

};

static LinuxFrameFunctions GAppFrameFunctions INIT_PRIORITY_URGENT;
AppFrameFunctions *CurrentAppFrameFunctions = &GAppFrameFunctions;

#if _ENABLE_REPORT

void LinuxFrameFunctions::LogF ( const char *format, va_list argptr )
{
    BString<512> buf;
    vsprintf(buf,format,argptr);
    strcat(buf,"\n");
        // always write to debug log:
    if ( !f ) {
        f = fopen("debug.log","a+t");
        if ( f ) {
                // initial message:
            time_t timet;
            time(&timet);
            timet -= (time_t)((getSystemTime()-startTime)/1000000);
            fprintf(f,"---------------------------------------------------\n"
                      "%8.3f: Log-file start: %s",0.0,ctime(&timet));
            }
        }
    if ( f ) 
        fprintf(f,"%8.3f: %s",TickCount()*0.001,buf.cstr());
}

void LinuxFrameFunctions::ErrF ( const char *format, va_list argptr )
{
    BString<512> buf;
    vsprintf(buf,format,argptr);
    ::LogF("Err: %s",(const char *)buf);
    fputs(buf,stderr);
    fputc('\n',stderr);
}

void LinuxFrameFunctions::LstF ( const char *format, va_list argptr )
{
    BString<512> buf;
    vsprintf(buf,format,argptr);
    ::LogF("Lst: %s",(const char *)buf);
    fputs(buf,stderr);
    fputc('\n',stderr);
}

LinuxFrameFunctions::~LinuxFrameFunctions ()
{
    if ( f ) {
            // final message:
        time_t timet;
        time(&timet);
        timet -= (time_t)((getSystemTime()-startTime)/1000000);
        fprintf(f,"%8.3f: Log-file end: %s",TickCount()*0.001,ctime(&timet));
	    // close the log-file:
        fclose(f);
	f = NULL;
	}
}

#endif

extern void DDTerm ();
extern void WarningMessageLevel ( ErrorMessageLevel level, const char *format, va_list argptr );

void LinuxFrameFunctions::ErrorMessage ( const char *format, va_list argptr )
{
    static int avoidRecursion = 0;
    if( avoidRecursion++ != 0 ) return;
    GDebugger.NextAliveExpected(15*60*1000);
    BString<256> buf;
    vsprintf( buf, format, argptr );
    #if _ENABLE_REPORT
      ::RptF("ErrorMessage: %s",(const char *)buf);
    #endif
    DDTerm();
    fputs(buf,stderr);
    fputc('\n',stderr);
    #if _ENABLE_REPORT
      ::LogF("Exit Error");
    #endif
    exit(1);
}

void LinuxFrameFunctions::ErrorMessage ( ErrorMessageLevel level, const char *format, va_list argptr )
{
    if ( level <= EMError )
    {
        WarningMessageLevel(level,format,argptr);
    }
    else
    {
        ErrorMessage(format,argptr);
    }
}

void LinuxFrameFunctions::WarningMessage ( const char *format, va_list argptr )
{
    WarningMessageLevel(EMWarning,format,argptr);
}
