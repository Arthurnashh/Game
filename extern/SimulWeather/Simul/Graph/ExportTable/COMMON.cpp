#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "common.h"
#include "symboltablesupport.h"
#include "pedump.h"
#include <iostream>
using namespace std;

PIMAGE_DEBUG_MISC g_pMiscDebugInfo=0;
PDWORD g_pCVHeader=0;
PIMAGE_COFF_SYMBOLS_HEADER g_pCOFFHeader=0;
COFFSymbolTable * g_pCOFFSymbolTable=0;

/*----------------------------------------------------------------------------*/
//
// Header related stuff
//
/*----------------------------------------------------------------------------*/


// Bitfield values and names for the IMAGE_FILE_HEADER flags
WORD_FLAG_DESCRIPTIONS ImageFileHeaderCharacteristics[]=
{
{ IMAGE_FILE_RELOCS_STRIPPED,"RELOCS_STRIPPED" },
{ IMAGE_FILE_EXECUTABLE_IMAGE,"EXECUTABLE_IMAGE" },
{ IMAGE_FILE_LINE_NUMS_STRIPPED,"LINE_NUMS_STRIPPED" },
{ IMAGE_FILE_LOCAL_SYMS_STRIPPED,"LOCAL_SYMS_STRIPPED" },
{ IMAGE_FILE_AGGRESIVE_WS_TRIM,"AGGRESIVE_WS_TRIM" },
{ IMAGE_FILE_LARGE_ADDRESS_AWARE,"LARGE_ADDRESS_AWARE" },
{ IMAGE_FILE_BYTES_REVERSED_LO,"BYTES_REVERSED_LO" },
{ IMAGE_FILE_32BIT_MACHINE,"32BIT_MACHINE" },
{ IMAGE_FILE_DEBUG_STRIPPED,"DEBUG_STRIPPED" },
{ IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP,"REMOVABLE_RUN_FROM_SWAP" },
{ IMAGE_FILE_NET_RUN_FROM_SWAP,"NET_RUN_FROM_SWAP" },
{ IMAGE_FILE_SYSTEM,"SYSTEM" },
{ IMAGE_FILE_DLL,"DLL" },
{ IMAGE_FILE_UP_SYSTEM_ONLY,"UP_SYSTEM_ONLY" },
{ IMAGE_FILE_BYTES_REVERSED_HI,"BYTES_REVERSED_HI" }
// { IMAGE_FILE_MINIMAL_OBJECT,"MINIMAL_OBJECT" },// Removed in NT 3.5
// { IMAGE_FILE_UPDATE_OBJECT,"UPDATE_OBJECT" },  // Removed in NT 3.5
// { IMAGE_FILE_16BIT_MACHINE,"16BIT_MACHINE" },  // Removed in NT 3.5
// { IMAGE_FILE_PATCH,"PATCH" },
};

#define NUMBER_IMAGE_HEADER_FLAGS \
    (sizeof(ImageFileHeaderCharacteristics) / sizeof(WORD_FLAG_DESCRIPTIONS))

//
// Dump the IMAGE_FILE_HEADER for a PE file or an OBJ
//
void DumpHeader(PIMAGE_FILE_HEADER pImageFileHeader)
{
//    UINT headerFieldWidth=30;
    UINT i;

	char time_str[21];
	time_t timeStamp=pImageFileHeader->TimeDateStamp;
	ctime_s(time_str,20,&timeStamp);
    
    cout<<("File Header\n");

    cout<<"   Machine: "<<
                pImageFileHeader->Machine<<" "<<
                GetMachineTypeName(pImageFileHeader->Machine)<<endl;
    cout<<"   Number of Sections: "<<
                pImageFileHeader->NumberOfSections<<endl;
    cout<<"   TimeDateStamp: "<<
		pImageFileHeader->TimeDateStamp<<" "<<std::string(time_str)<<endl;
    cout<<"   PointerToSymbolTable: "<<
                pImageFileHeader->PointerToSymbolTable<<endl;
    cout<<"   NumberOfSymbols: "<<
                pImageFileHeader->NumberOfSymbols<<endl;
    cout<<"   SizeOfOptionalHeader: "<<
                pImageFileHeader->SizeOfOptionalHeader<<endl;
    cout<<"   Characteristics: "<<
                pImageFileHeader->Characteristics<<endl;
    for( i=0;i < NUMBER_IMAGE_HEADER_FLAGS;i++ )
    {
        if ( pImageFileHeader->Characteristics & 
             ImageFileHeaderCharacteristics[i].flag )
            cout<<"    "<<ImageFileHeaderCharacteristics[i].name<<endl;
    }
}

WORD_VALUE_NAMES g_arMachines[]=
{
{ IMAGE_FILE_MACHINE_UNKNOWN,"UNKNOWN" },
{ IMAGE_FILE_MACHINE_I386,"I386" },
{ IMAGE_FILE_MACHINE_R3000,"R3000" },
{ IMAGE_FILE_MACHINE_R4000,"R4000" },
{ IMAGE_FILE_MACHINE_R10000,"R10000" },
{ IMAGE_FILE_MACHINE_WCEMIPSV2,"WCEMIPSV2" },
{ IMAGE_FILE_MACHINE_ALPHA,"ALPHA" },
{ IMAGE_FILE_MACHINE_SH3,"SH3" },
{ IMAGE_FILE_MACHINE_SH3DSP,"SH3DSP" },
{ IMAGE_FILE_MACHINE_SH3E,"SH3E" },
{ IMAGE_FILE_MACHINE_SH4,"SH4" },
{ IMAGE_FILE_MACHINE_SH5,"SH5" },
{ IMAGE_FILE_MACHINE_ARM,"ARM" },
{ IMAGE_FILE_MACHINE_THUMB,"THUMB" },
{ IMAGE_FILE_MACHINE_AM33,"AM33" },
{ IMAGE_FILE_MACHINE_POWERPC,"POWERPC" },
{ IMAGE_FILE_MACHINE_POWERPCFP,"POWERPCFP" },
{ IMAGE_FILE_MACHINE_IA64,"IA64" },
{ IMAGE_FILE_MACHINE_MIPS16,"MIPS16" },
{ IMAGE_FILE_MACHINE_ALPHA64,"ALPHA64" },
{ IMAGE_FILE_MACHINE_MIPSFPU,"MIPSFPU" },
{ IMAGE_FILE_MACHINE_MIPSFPU16,"MIPSFPU16" },
{ IMAGE_FILE_MACHINE_TRICORE,"TRICORE" },
{ IMAGE_FILE_MACHINE_CEF,"CEF" },
{ IMAGE_FILE_MACHINE_EBC,"EBC" },
{ IMAGE_FILE_MACHINE_AMD64,"AMD64" },
{ IMAGE_FILE_MACHINE_M32R,"M32R" },
{ IMAGE_FILE_MACHINE_CEE,"CEE" },
};


PSTR GetMachineTypeName( WORD wMachineType )
{
	for( unsigned i=0;i < ARRAY_SIZE(g_arMachines);i++ )
		if ( wMachineType==g_arMachines[i].wValue )
			return g_arMachines[i].pszName;

	return "unknown";
}

bool IsValidMachineType( WORD wMachineType )
{
	for( unsigned i=0;i < ARRAY_SIZE(g_arMachines);i++ )
		if ( wMachineType==g_arMachines[i].wValue )
			return true;

	return false;
}

/*----------------------------------------------------------------------------*/
//
// Section related stuff
//
/*----------------------------------------------------------------------------*/

//
// Given a section name,look it up in the section table and return a
// pointer to the start of its raw data area.
//
LPVOID GetSectionPtr(PSTR name,PIMAGE_NT_HEADERS pNTHeader,PBYTE imageBase)
{
    PIMAGE_SECTION_HEADER section=IMAGE_FIRST_SECTION(pNTHeader);
    unsigned i;
    
    for( i=0;i < pNTHeader->FileHeader.NumberOfSections;i++,section++ )
    {
        if (strncmp((char *)section->Name,name,IMAGE_SIZEOF_SHORT_NAME)==0)
            return (LPVOID)(section->PointerToRawData + imageBase);
    }
    
    return 0;
}


PIMAGE_COFF_SYMBOLS_HEADER PCOFFDebugInfo=0;

char *SzDebugFormats[]={
"UNKNOWN/BORLAND","COFF","CODEVIEW","FPO","MISC","EXCEPTION","FIXUP",
"OMAP_TO_SRC","OMAP_FROM_SRC"};

//
// Dump the debug directory array
//
void DumpDebugDirectory(PIMAGE_DEBUG_DIRECTORY debugDir,DWORD size,PBYTE pImageBase)
{
    DWORD cDebugFormats=size / sizeof(IMAGE_DEBUG_DIRECTORY);
    PSTR szDebugFormat;
    unsigned i;
    
    if ( cDebugFormats==0 )
        return;
    
    cout<<(
    "Debug Formats in File\n"
    "  Type            Size     Address  FilePtr  Charactr TimeDate Version\n"
    "  --------------- -------- -------- -------- -------- -------- --------\n"
    );
    
    for( i=0;i < cDebugFormats;i++ )
    {
        szDebugFormat=(debugDir->Type <= IMAGE_DEBUG_TYPE_OMAP_FROM_SRC )
                        ? SzDebugFormats[debugDir->Type] : "???";

        cout<<("  %-15s %08X %08X %08X %08X %08X %u.%02u\n",
            szDebugFormat,debugDir->SizeOfData,debugDir->AddressOfRawData,
            debugDir->PointerToRawData,debugDir->Characteristics,
            debugDir->TimeDateStamp,debugDir->MajorVersion,
            debugDir->MinorVersion);

		switch( debugDir->Type )
		{
        	case IMAGE_DEBUG_TYPE_COFF:
	            g_pCOFFHeader =
                (PIMAGE_COFF_SYMBOLS_HEADER)(pImageBase+ debugDir->PointerToRawData);
				break;

			case IMAGE_DEBUG_TYPE_MISC:
				g_pMiscDebugInfo =
				(PIMAGE_DEBUG_MISC)(pImageBase + debugDir->PointerToRawData);
				break;

			case IMAGE_DEBUG_TYPE_CODEVIEW:
				g_pCVHeader=(PDWORD)(pImageBase + debugDir->PointerToRawData);
				break;
		}

        debugDir++;
    }
}
