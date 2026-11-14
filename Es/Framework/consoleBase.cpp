#include "consoleBase.h"
#include <windows.h>
#include <stdio.h>

bool wasError = false;
bool ignoreError = false;

static HANDLE hConsoleOutput,hConsoleInput;

static void ExitCon()
{
	if( wasError && !ignoreError)
	{
		DWORD ret;
		static const char text[]="\n-----------------------\nPress ENTER to continue";
		char buf[80];
		WriteConsole(hConsoleOutput,text,strlen(text),&ret,NULL);
		ReadConsole(hConsoleInput,buf,sizeof(buf),&ret,NULL);
	}
}


void InitConsoleApp()
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	BOOL bLaunched;
	
	// Lets try a trick to determine if we were 'launched' as a seperate
	// screen, or just running from the command line.
	
	hConsoleOutput = GetStdHandle(STD_ERROR_HANDLE);
	hConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
	GetConsoleScreenBufferInfo(hConsoleOutput, &csbi);
	bLaunched = ((csbi.dwCursorPosition.X==0) && (csbi.dwCursorPosition.Y==0));
	if ((csbi.dwSize.X<=0) || (csbi.dwSize.Y <= 0)) bLaunched = FALSE;

	if( bLaunched ) atexit(ExitCon);
}

/*

void __cdecl LogF( const char *format, ... )
{
	char buf[512];
		
	va_list arglist;
	va_start( arglist, format );

	vsprintf(buf,format,arglist);
	strcat(buf,"\n");
	OutputDebugString(buf);

	va_end( arglist );

}

void __cdecl ErrF( const char *format, ... )
{
	char buf[512];
		
	va_list arglist;
	va_start( arglist, format );

	vsprintf(buf,format,arglist);
	strcat(buf,"\n");
	OutputDebugString(buf);

	va_end( arglist );

}
*/

int main( int argc, const char *argv[] )
{
	InitConsoleApp();
	int ret = consoleMain(argc,argv);
	if (ret) wasError = true;
	return ret;

}
