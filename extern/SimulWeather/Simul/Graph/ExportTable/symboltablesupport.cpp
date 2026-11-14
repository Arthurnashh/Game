#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "common.h"
#include "COFFSymbolTable.h"
#include "pedump.h"
#include "cv_dbg.h"
#include "cvsymbols.h"
#include <iostream>
using namespace std;


//
// Dump the COFF debug information header
//
void DumpCOFFHeader(PIMAGE_COFF_SYMBOLS_HEADER pDbgInfo)
{
    cout<<"COFF Debug Info Header\n";
    cout<<"  NumberOfSymbols:      "<< pDbgInfo->NumberOfSymbols<<endl;
    cout<<"  LvaToFirstSymbol:     "<< pDbgInfo->LvaToFirstSymbol<<endl;
    cout<<"  NumberOfLinenumbers:  "<< pDbgInfo->NumberOfLinenumbers<<endl;
    cout<<"  LvaToFirstLinenumber: "<< pDbgInfo->LvaToFirstLinenumber<<endl;
    cout<<"  RvaToFirstByteOfCode: "<< pDbgInfo->RvaToFirstByteOfCode<<endl;
    cout<<"  RvaToLastByteOfCode:  "<< pDbgInfo->RvaToLastByteOfCode<<endl;
    cout<<"  RvaToFirstByteOfData: "<< pDbgInfo->RvaToFirstByteOfData<<endl;
    cout<<"  RvaToLastByteOfData:  "<< pDbgInfo->RvaToLastByteOfData<<endl;
}


//
// Given a COFF symbol table index,look up its name.  This function assumes
// that the COFFSymbolCount and PCOFFSymbolTable variables have been already
// set up.
//
BOOL LookupSymbolName(DWORD index,PSTR buffer,UINT length)
{
	if ( !g_pCOFFSymbolTable )
		return FALSE;

	PCOFFSymbol pSymbol=g_pCOFFSymbolTable->GetSymbolFromIndex( index );

	if ( !pSymbol )
		return FALSE;

	lstrcpyn( buffer,pSymbol->GetName(),length );

	delete pSymbol;

    return TRUE;
}

//
// Dump a range of line numbers from the COFF debug information
//
void DumpLineNumbers(PIMAGE_LINENUMBER pln,DWORD count)
{
    char buffer[64];
    DWORD i;
    
    cout<<("Line Numbers\n");
    
    for(i=0;i < count;i++)
    {
        if ( pln->Linenumber==0 ) // A symbol table index
        {
            buffer[0]=0;
            LookupSymbolName(pln->Type.SymbolTableIndex,buffer,
                            sizeof(buffer));
            cout<<"SymIndex: "<<pln->Type.SymbolTableIndex
				<<" ("<<buffer<<")"<<endl;
        }
        else        // A regular line number
            cout<<" Addr: "<<pln->Type.VirtualAddress<<
				"  Line: "<<pln->Linenumber<<endl;
        pln++;
    }
}


//
// Used by the DumpSymbolTable() routine.  It purpose is to filter out
// the non-normal section numbers and give them meaningful names.
//
void GetSectionName(WORD section,PSTR buffer,unsigned cbBuffer)
{
    char tempbuffer[10];
    
    switch ( (SHORT)section )
    {
        case IMAGE_SYM_UNDEFINED: strcpy(tempbuffer,"UNDEF");break;
        case IMAGE_SYM_ABSOLUTE:  strcpy(tempbuffer,"ABS");break;
        case IMAGE_SYM_DEBUG:     strcpy(tempbuffer,"DEBUG");break;
        default: sprintf(tempbuffer,"%X",section);
    }
    
    strncpy_s(buffer,10,tempbuffer,cbBuffer-1);
}

//
// Dumps a COFF symbol table from an EXE or OBJ
//
void DumpCOFFSymbolTable( PCOFFSymbolTable pSymTab )
{
    cout<<"Symbol Table - "<<pSymTab->GetNumberOfSymbols() <<" entries  (*=auxillary symbol)\n";


    cout<<(
    "Indx Sectn Value    Type  Storage  Name\n"
    "---- ----- -------- ----- -------  --------\n");

	PCOFFSymbol pSymbol=pSymTab->GetNextSymbol( 0 );

	while ( pSymbol )
	{
	    char szSection[10];
        GetSectionName(pSymbol->GetSectionNumber(),szSection,sizeof(szSection));

        cout<< pSymbol->GetIndex()<<" "<< szSection<<" "<< pSymbol->GetValue()<<" "<<
        		pSymbol->GetTypeName()<<" "<< pSymbol->GetStorageClassName()<<" "<<
        		pSymbol->GetName()<<endl;

		if ( pSymbol->GetNumberOfAuxSymbols() )
		{
			char szAuxSymbol[1024];
			if (pSymbol->GetAuxSymbolAsString(szAuxSymbol,sizeof(szAuxSymbol)))
				cout<<"     * "<<szAuxSymbol<<endl;			
		}
		
		pSymbol=pSymTab->GetNextSymbol( pSymbol );

	}
}

void DumpMiscDebugInfo( PIMAGE_DEBUG_MISC pMiscDebugInfo )
{
	if ( IMAGE_DEBUG_MISC_EXENAME != pMiscDebugInfo->DataType )
	{
		cout<<"Unknown Miscellaneous Debug Information type: "<< 
				pMiscDebugInfo->DataType<<endl;
		return;
	}

	cout<<"Miscellaneous Debug Information\n";
	cout<<"  "<<pMiscDebugInfo->Data<<endl;
}

void DumpCVDebugInfoRecord( PDWORD pCVHeader )
{
	PPDB_INFO pPDBInfo;

	cout<<"CodeView Signature: "<<(*pCVHeader )<<endl;

	if ( 'SDSR'==(UINT32)*pCVHeader )
	{
		cout<<( "New format Visual Studio .NET (or later) debug information\n" );
		return;
	}

	if ( 'BN' != (unsigned short)*pCVHeader )
	{
		cout<<"Unhandled CodeView Information format "<<pCVHeader<<endl;
		return;
	}

	if ( '01BN'==(unsigned short)*pCVHeader )	// NB10==PDB file info
	{
		pPDBInfo=(PPDB_INFO)pCVHeader;

		cout<<"  Offset: "<<pPDBInfo->Offset<<"  Signature: "<<pPDBInfo->sig<<"  Age: "<<pPDBInfo->age<<
			endl;
		cout<<"  File: "<<pPDBInfo->PdbName<<endl;
	}
	else	// Should be NB09 or NB11
	{
		DumpCVSymbolTable( (PBYTE)pCVHeader,g_pMappedFileBase );
	}
}

