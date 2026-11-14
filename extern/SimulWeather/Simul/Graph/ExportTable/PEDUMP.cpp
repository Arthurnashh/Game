#include <windows.h>
#include <stdio.h>
#include "exedump.h"
#include "common.h"
#include <iostream>
using namespace std;
// Global variables set here,and used elsewhere in the program
BOOL fShowSymbolTable=FALSE;
BOOL fShowLineNumbers=FALSE;
BOOL fShowIATentries=FALSE;
BOOL fShowPDATA=FALSE;
BOOL fShowResources=FALSE;
PBYTE g_pMappedFileBase=0;
static bool do_outputs= false;

//
// Map a file to memory,and call the appropriate dumping routine
void DumpFile(HMODULE hModule,const char *fn,void(*export_callback)(const char *,void *))
{
	export_table_callback=export_callback;
	LPCSTR filename=fn;
    HANDLE hFile;
    HANDLE hFileMapping;
    PIMAGE_DOS_HEADER dosHeader;
    
    hFile=CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,NULL,
                        OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
                    
    if ( hFile==INVALID_HANDLE_VALUE )
    {
		if(do_outputs)
			cerr<<("Couldn't open file ")<<filename<<(" with CreateFile()\n");
        return;
    }
	/*
	 A memory-mapped file is a file that has been mapped (i.e.,not copied) into
	 virtual memory such that it looks as though it has been loaded into memory.
	 Rather than actually being copied into virtual memory,a range of virtual
	 memory addresses are simply marked off for use by the file.
	 can then access the file as though it were memory,freely changing parts of
	 it without having to make file I/O calls.  Windows transparently loads parts
	 of the file into physical memory as you access them.  You don’t have to concern
	 yourself with which parts are and are not in memory at any given time.
	 */
    hFileMapping=CreateFileMapping(hFile,NULL,PAGE_READONLY,0,0,NULL);
    if ( hFileMapping==0 )
    {
        CloseHandle(hFile);
		if(do_outputs)
			cerr<<"Couldn't open file mapping "<<filename<<" with CreateFileMapping()\n";
        return;
    }

   g_pMappedFileBase=(PBYTE)MapViewOfFile(hFileMapping,FILE_MAP_READ,0,0,0);

    if ( g_pMappedFileBase==0 )
    {
        CloseHandle(hFileMapping);
        CloseHandle(hFile);
		if(do_outputs)
			cerr<<"Couldn't map view of file "<<filename<<" with MapViewOfFile()\n";
        return;
    }
	if(do_outputs)
		cout<<"\nDump of file "<<filename<<"\n";

    dosHeader=(PIMAGE_DOS_HEADER)g_pMappedFileBase;

    if ( dosHeader->e_magic==IMAGE_DOS_SIGNATURE )
    {
        DumpExeFile( hModule,dosHeader );
    }

    UnmapViewOfFile(g_pMappedFileBase);
    CloseHandle(hFileMapping);
    CloseHandle(hFile);
}
