#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <delayimp.h>
#include <imagehlp.h>
#include <wintrust.h>

#include "common.h"
#include "COFFSymbolTable.h"
#include "pedump.h"
#include <iostream>


using namespace std;
static bool do_outputs= false;
bool fShowRelocations=false;

void(*export_table_callback)(const char *defn,void *proc);
//============================================================================

// Bitfield values and names for the DllCharacteritics flags
WORD_FLAG_DESCRIPTIONS DllCharacteristics[]=
{
	{ IMAGE_DLLCHARACTERISTICS_NO_BIND,"NO_BIND" },		// Do not bind this image.
	{ IMAGE_DLLCHARACTERISTICS_WDM_DRIVER,	"WDM_DRIVER" },	// Driver uses WDM model
	{ IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE,"TERMINAL_SERVER_AWARE" },
};

#define NUMBER_DLL_CHARACTERISTICS \
    (sizeof(DllCharacteristics) / sizeof(WORD_FLAG_DESCRIPTIONS))

// Names of the data directory elements that are defined
char *ImageDirectoryNames[]={
    "EXPORT",
	"IMPORT",
	"RESOURCE",
	"EXCEPTION",
	"SECURITY",
	"BASERELOC",
    "DEBUG",
	"ARCHITECTURE",
	"GLOBALPTR",
	"TLS",
	"LOAD_CONFIG",
    "BOUND_IMPORT", 		// These two entries added for NT 3.51
	"IAT",
	"DELAY_IMPORT",			// This entry added in NT 5 time frame
	"COM_DESCRPTR" };		// For the .NET runtime (previously called COM+ 2.0)

#define NUMBER_IMAGE_DIRECTORY_ENTRYS \
    (sizeof(ImageDirectoryNames)/sizeof(char *))

void DisplayDataDirectoryEntry(PSTR pszName,IMAGE_DATA_DIRECTORY & dataDirEntry )
{
	if(do_outputs)
		cout<<"  "<<pszName<<endl
			<<" rva: "<<dataDirEntry.VirtualAddress<<endl
			<<"  size: "<<dataDirEntry.Size<<endl;
}


template <class T,class U,class V>
void DumpImportsOfOneModule(	T* pINT,U* pIAT,V * pNTHeader,		// 'T','U'=IMAGE_THUNK_DATA,'v'=IMAGE_NT_HEADERS
								PIMAGE_IMPORT_DESCRIPTOR pImportDesc,
								PBYTE pImageBase )
{
    PIMAGE_IMPORT_BY_NAME pOrdinalName;

	bool bIs64Bit=(pNTHeader->OptionalHeader.Magic==IMAGE_NT_OPTIONAL_HDR64_MAGIC );

	while (1 ) // Loop forever (or until we break out)
	{
		if (pINT->u1.AddressOfData==0 )
			break;

		ULONGLONG ordinal=-1;

		if (bIs64Bit )
		{
			if (IMAGE_SNAP_BY_ORDINAL64(pINT->u1.Ordinal) )
				ordinal=IMAGE_ORDINAL64(pINT->u1.Ordinal);
		}
		else
		{
			if (IMAGE_SNAP_BY_ORDINAL32(pINT->u1.Ordinal) )
				ordinal=IMAGE_ORDINAL32(pINT->u1.Ordinal);			
		}

		if (ordinal != -1 )
		{
			cout<<"  "<<ordinal<<endl;
		}
		else
		{
			// pINT->u1.AddressOfData is theoretically 32 or 64 bits,but in the on-disk representation,
			// we'll assume it's an RVA.  As such,we'll cast it to a DWORD.
			pOrdinalName=(PIMAGE_IMPORT_BY_NAME)GetPtrFromRVA(static_cast<DWORD>(pINT->u1.AddressOfData),pNTHeader,pImageBase);
			
			cout<<"  "<<pOrdinalName->Hint<<"  "<<pOrdinalName->Name<<endl;
		}
		
		// If it looks like the image has been bound,append the
		// bound address
		if (pImportDesc->TimeDateStamp )
			cout<<" Bound to: "<<pIAT->u1.Function<<endl;
		
		cout<<("\n" );
		
		pINT++;       // Advance to next thunk
		pIAT++;       // advance to next thunk
	}	
}

//
// Dump the delayed imports table of a PE file
//

template <class T,class U> void DumpDelayedImportsImportNames(PBYTE pImageBase,T* pNTHeader,U* thunk,bool bUsingRVA )	// T=PIMAGE_NT_HEADER,U='IMAGE_THUNK_DATA'
{
    while (1 ) // Loop forever (or until we break out)
    {
        if (thunk->u1.AddressOfData==0 )
            break;

		ULONGLONG ordinalMask;
		if (sizeof(thunk->u1.Ordinal)==sizeof(ULONGLONG) )	// Which ordinal mask should we use?
			ordinalMask=IMAGE_ORDINAL_FLAG64;
		else
			ordinalMask=IMAGE_ORDINAL_FLAG32;

        if (thunk->u1.Ordinal & ordinalMask )
        {
            cout<<"  "<<(thunk->u1.Ordinal & 0xFFFF)<<endl;
        }
        else
        {
            PIMAGE_IMPORT_BY_NAME pOrdinalName=(PIMAGE_IMPORT_BY_NAME)((PBYTE)0 + thunk->u1.AddressOfData);

			pOrdinalName=bUsingRVA
				? (PIMAGE_IMPORT_BY_NAME)GetPtrFromRVA((DWORD)thunk->u1.AddressOfData,pNTHeader,pImageBase)
				: (PIMAGE_IMPORT_BY_NAME)GetPtrFromVA((PVOID)pOrdinalName,pNTHeader,pImageBase);
            
			cout<<"  "<<pOrdinalName->Hint<<"  "<<pOrdinalName->Name<<endl;
        }
        
        cout<<("\n" );

        thunk++;          // Advance to next thunk
    }

	cout<<("\n" );
}

template <class T> void DumpDelayedImportsSection(PBYTE pImageBase,T* pNTHeader,bool bIs64Bit )	// 'T'=PIMAGE_NT_HEADERS 
{
	DWORD delayImportStartRVA;
    PCImgDelayDescr pDelayDesc;

    // Look up where the delay imports section is (normally in the .didat
	/// section) but not necessarily so.
 	delayImportStartRVA=GetImgDirEntryRVA(pNTHeader,IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT );
    if (!delayImportStartRVA )
        return;

	// This code is more complicated than it needs to be,thanks to Microsoft.  When the
	// ImgDelayDescr was originally created for Win32,portability to Win64 wasn't
	// considered.  As such,MS used pointers,rather than RVAs in the data structures.
	// Finally,MS issued a new DELAYIMP.H,which issued a flag indicating whether the
	// field values are RVAs or VAs.  Unfortunately,Microsoft has been rather slow to
	// get this header file out into general distibution.  Currently,you can get it as
	// part of the Win64 headers,or as part of VC7.  In the meanwhile,we'll use some
	// preprocessor trickery so that we can use the new field names,while still compiling
	// with the original DELAYIMP.H.

	#if _DELAY_IMP_VER<2
	#define rvaDLLName		szName
	#define rvaHmod			phmod
	#define rvaIAT			pIAT
	#define rvaINT			pINT
	#define rvaBoundIAT		pBoundIAT
	#define rvaUnloadIAT	pUnloadIAT
	#endif

    pDelayDesc=(PCImgDelayDescr)GetPtrFromRVA(delayImportStartRVA,pNTHeader,pImageBase);
	if (!pDelayDesc )
		return;
            
    cout<<("Delay Imports Table:\n");
	
	while (pDelayDesc->rvaDLLName )
	{
		PSTR pszDLLName;
		
		// from more recent DELAYIMP.H:
		// enum DLAttr {                   // Delay Load Attributes
		//    dlattrRva=0x1,               // RVAs are used instead of pointers
		//    };
		bool bUsingRVA=pDelayDesc->grAttrs & 1;

		DWORD dllNameRVA=(DWORD)pDelayDesc->rvaDLLName;
		PVOID dllNameVA=(PBYTE)0+(DWORD)pDelayDesc->rvaDLLName;

		pszDLLName=bUsingRVA
			? (PSTR)GetPtrFromRVA(dllNameRVA,pNTHeader,pImageBase)
			: (PSTR)GetPtrFromVA(dllNameVA,pNTHeader,pImageBase);

	    cout<< "  %s\n"<<pszDLLName<<endl;
		cout<< "  Attributes:                 "<<pDelayDesc->grAttrs<<endl;
		if (pDelayDesc->grAttrs & 1 )
			cout<<("  dlattrRva" );
		cout<<"\n" ;
		cout<<"  Name R(VA):                 "<<pDelayDesc->rvaDLLName<<endl;
	    cout<<"  HMODULE R(VA):              "<<pDelayDesc->rvaHmod<<endl;
	    cout<<"  Import Address Table R(VA): "<<pDelayDesc->rvaIAT<<endl;
	    cout<<"  Import Names Table R(VA):   "<<pDelayDesc->rvaINT<<endl;
	    cout<<"  Bound IAT R(VA):            "<<pDelayDesc->rvaBoundIAT<<endl;
	    cout<<"  Unload IAT R(VA):           "<<pDelayDesc->rvaUnloadIAT<<endl;	    
		cout<<"  TimeDateStamp:              "<<pDelayDesc->dwTimeStamp<<endl;

		if (pDelayDesc->dwTimeStamp )
		{
			time_t timeStamp=pDelayDesc->dwTimeStamp;
			cout<<" -> "<<ctime(&timeStamp)<<endl;
		}
		else
			cout<<("\n" );

		//
		// Display the Import Names Table.
		PVOID pvThunk;
		
		PVOID thunkVA=(PBYTE)0 + (DWORD)pDelayDesc->rvaINT;

		if (pDelayDesc->grAttrs & 1 )	// see above comment on dlatttrRva being==1
			pvThunk=GetPtrFromRVA((DWORD)pDelayDesc->rvaINT,pNTHeader,pImageBase );
		else
			pvThunk=GetPtrFromVA(thunkVA,pNTHeader,pImageBase );
    
        cout<<("  Ordn  Name\n");
        
		bIs64Bit
			? DumpDelayedImportsImportNames(pImageBase,pNTHeader,(PIMAGE_THUNK_DATA64)pvThunk,bUsingRVA )
			: DumpDelayedImportsImportNames(pImageBase,pNTHeader,(PIMAGE_THUNK_DATA32)pvThunk,bUsingRVA );

		pDelayDesc++;	// Pointer math.  Advance to next delay import desc.
	}

    cout<<("\n");

#if _DELAY_IMP_VER<2 // Remove the alias names from the namespace
#undef szName
#undef phmod
#undef pIAT
#undef pINT
#undef pBoundIAT
#undef pUnloadIAT
#endif
}

std::string Undecorate(const std::string &decorated)
{
	std::string undecorated;
	static char dec[1000];
	static char undec[1000];
	strcpy(dec,decorated.c_str());
    DWORD result=UnDecorateSymbolName(dec,
		undec,
		sizeof(undec),
        UNDNAME_COMPLETE);
	DWORD err=0;
	if(!result)
	{
		err=GetLastError();
			LPTSTR s;
		if(FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            err,
            0,
            (LPTSTR)&s,
			256,
            NULL)!=0)
		{
			s[255]=0;
			string txt=s;
			cerr<<txt<<endl;
			LocalFree(s);
		}
		else
		{
			// failed
		// Unknown error code %08x (%d)
			cerr<<"Unknown error "<<err<<endl;
		}
		undecorated="";	
	}
	else
	{
		undecorated=undec;
	}
	return undecorated;
}
//
// Dump the exports table (usually the .edata section) of a PE file
//
template <class T> void DumpExportsSection(HMODULE hModule,PBYTE pImageBase,T * pNTHeader)	// 'T'=PIMAGE_NT_HEADERS 
{
    PIMAGE_EXPORT_DIRECTORY export_dir;
    PIMAGE_SECTION_HEADER header;
    INT delta;
    PSTR filename;
    DWORD i;
    PDWORD functions;
    PWORD ordinals;
    DWORD *func_names;
    DWORD exportsStartRVA,exportsEndRVA;
    
    exportsStartRVA=GetImgDirEntryRVA(pNTHeader,IMAGE_DIRECTORY_ENTRY_EXPORT);
    exportsEndRVA=exportsStartRVA +
	   				GetImgDirEntrySize(pNTHeader,IMAGE_DIRECTORY_ENTRY_EXPORT);

    // Get the IMAGE_SECTION_HEADER that contains the exports.  This is
    // usually the .edata section,but doesn't have to be.
    header=GetEnclosingSectionHeader(exportsStartRVA,pNTHeader );
    if (!header )
        return;

    delta=(INT)(header->VirtualAddress - header->PointerToRawData);
        
    export_dir=(PIMAGE_EXPORT_DIRECTORY)GetPtrFromRVA(exportsStartRVA,pNTHeader,pImageBase);

	if(do_outputs)
		cout<<"exports table:\n\n"<<endl;
	if(!export_dir->Name)
		return;
	
	filename=(PSTR)GetPtrFromRVA(export_dir->Name,pNTHeader,pImageBase );
	cout<<"Export table for file: "<< filename<<endl;
	
    functions=(PDWORD)GetPtrFromRVA(export_dir->AddressOfFunctions,pNTHeader,pImageBase );
    ordinals=(PWORD)GetPtrFromRVA(export_dir->AddressOfNameOrdinals,pNTHeader,pImageBase );
    func_names=(DWORD *)GetPtrFromRVA(export_dir->AddressOfNames,pNTHeader,pImageBase );

    for(i=0;i<export_dir->NumberOfFunctions;i++,functions++)
    {
        DWORD entryPointRVA=*functions;

        if (entryPointRVA==0 )   // Skip over gaps in exported function
            continue;             // ordinals (the entrypoint is 0 for
                                    // these functions).
		unsigned *test=(unsigned *)(GetPtrFromRVA(entryPointRVA,pNTHeader,pImageBase ));
		test=(unsigned *)((unsigned)test-(unsigned)pImageBase+(unsigned)0x400000);

		bool name_found=false;
        // See if this function has an associated name exported for it.
        for(unsigned j=0;j<export_dir->NumberOfNames;j++ )
		{
			if (ordinals[j]==i )
			{
				const char *txt=(const char *)GetPtrFromRVA(func_names[j],pNTHeader,pImageBase);
				string ud2=Undecorate(txt);
				FARPROC f=GetProcAddress(
					hModule,
					txt
					);
				if(export_table_callback)
					(*export_table_callback)(ud2.c_str(),(void*)f);
				name_found=true;
			}
		}
		if(!name_found)
		{
			cerr<<"No export found for "<<i<<"."<<endl;
		}
    }
}

// The names of the available base relocations
char *SzRelocTypes[]={
{"ABSOLUTE" },			   // 0
{"HIGH" },                // 1
{"LOW" },                 // 2
{"HIGHLOW" },             // 3
{"HIGHADJ" },             // 4
{"MIPS_JMPADDR" },        // 5
{ "???6" },
{ "???7" },
{ "???8" },
{"IA64_IMM64" },          // 9
{"DIR64" },               // 10
};

//
// Dump the base relocation table of a PE file
//
template <class T> void DumpBaseRelocationsSection(PBYTE pImageBase,T * pNTHeader)
{
	DWORD dwBaseRelocRVA;
    PIMAGE_BASE_RELOCATION baseReloc;

	dwBaseRelocRVA =
		GetImgDirEntryRVA(pNTHeader,IMAGE_DIRECTORY_ENTRY_BASERELOC );
    if (!dwBaseRelocRVA )
        return;

    baseReloc=(PIMAGE_BASE_RELOCATION)
    				GetPtrFromRVA(dwBaseRelocRVA,pNTHeader,pImageBase );
	if (!baseReloc )
		return;

    cout<<("base relocations:\n\n");

    while (baseReloc->SizeOfBlock != 0 )
    {
        unsigned i,cEntries;
        PWORD pEntry;
        char *szRelocType;
        WORD relocType;

		// Sanity check to make sure the data looks OK.
		if (0==baseReloc->VirtualAddress )
			break;
		if (baseReloc->SizeOfBlock<sizeof(*baseReloc) )
			break;
		
        cEntries=(baseReloc->SizeOfBlock-sizeof(*baseReloc))/sizeof(WORD);
        pEntry=MakePtr(PWORD,baseReloc,sizeof(*baseReloc) );
        
        cout<<"Virtual Address: "
			<<baseReloc->VirtualAddress<<"  size: "<<baseReloc->SizeOfBlock<<endl;
            
        for(i=0;i<cEntries;i++ )
        {
            // Extract the top 4 bits of the relocation entry.  Turn those 4
            // bits into an appropriate descriptive string (szRelocType)
            relocType=(*pEntry & 0xF000) >> 12;
            szRelocType=relocType <= IMAGE_REL_BASED_DIR64 ? SzRelocTypes[relocType] : "unknown";
            
            cout<<"  "<<
                    (*pEntry & 0x0FFF) + baseReloc->VirtualAddress<<" "<<
                    szRelocType<<endl;

			if (IMAGE_REL_BASED_HIGHADJ==relocType )
			{
				pEntry++;
				cEntries--;
				cout<<" ("<<*pEntry<<")"<<endl;
			}

			cout<<("\n" );
            pEntry++; // Advance to next relocation entry
        }
        
        baseReloc=MakePtr(PIMAGE_BASE_RELOCATION,baseReloc,
                             baseReloc->SizeOfBlock);
    }
}

//
// Dump out the new IMAGE_BOUND_IMPORT_DESCRIPTOR that NT 3.51 added
//
template <class T> void DumpBoundImportDescriptors(PBYTE pImageBase,T* pNTHeader )	// 'T'=PIMATE_NT_HEADERS
{
    DWORD bidRVA; // Bound import descriptors RVA
    PIMAGE_BOUND_IMPORT_DESCRIPTOR pibid;

    bidRVA=GetImgDirEntryRVA(pNTHeader,IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT);
    if (!bidRVA )
        return;
    
    pibid=MakePtr(PIMAGE_BOUND_IMPORT_DESCRIPTOR,pImageBase,bidRVA );
    
    cout<<("Bound import descriptors:\n\n" );
    cout<<("  Module        TimeDate\n" );
    cout<<("  ------------  --------\n" );
    
    while (pibid->TimeDateStamp )
    {
        unsigned i;
        PIMAGE_BOUND_FORWARDER_REF pibfr;

        time_t timeStamp=pibid->TimeDateStamp;

        cout<<"  "<<pImageBase + bidRVA + pibid->OffsetModuleName<<
                "  "<<pibid->TimeDateStamp<<
                " -> "<<ctime(&timeStamp)<<endl;
                            
        pibfr=MakePtr(PIMAGE_BOUND_FORWARDER_REF,pibid,
                            sizeof(IMAGE_BOUND_IMPORT_DESCRIPTOR));

        for(i=0;i<pibid->NumberOfModuleForwarderRefs;i++ )
        {
			timeStamp=pibfr->TimeDateStamp;

            cout<<"    forwarder:  "<<
                            pImageBase + bidRVA + pibfr->OffsetModuleName<<
                            "  "<<pibfr->TimeDateStamp<<
                            " -> "<<ctime(&timeStamp)<<endl;
            pibfr++;  // advance to next forwarder ref
                
            // Keep the outer loop pointer up to date too!
            pibid=MakePtr(PIMAGE_BOUND_IMPORT_DESCRIPTOR,pibid,
                             sizeof(IMAGE_BOUND_FORWARDER_REF ) );
        }

        pibid++;  // Advance to next pibid;
    }

    cout<<("\n" );
}

//
// Dump the TLS data
//
template <class T,class U> void DumpTLSDirectory(PBYTE pImageBase,T* pNTHeader,U * pTLSDir )	// 'T'=IMAGE_NT_HEADERS,U='IMAGE_TLS_DIRECTORY'
{
    DWORD tlsRVA; // TLS dirs RVA

    tlsRVA=GetImgDirEntryRVA(pNTHeader,IMAGE_DIRECTORY_ENTRY_TLS);
    if (!tlsRVA )
        return;

    pTLSDir=(U*)GetPtrFromRVA(tlsRVA,pNTHeader,pImageBase );

    cout<<("TLS directory:\n");
    cout<< "  StartAddressOfRawData:"<<pTLSDir->StartAddressOfRawData<<endl;
    cout<< "  EndAddressOfRawData:  "<<pTLSDir->EndAddressOfRawData<<endl;
    cout<< "  AddressOfIndex:       "<<pTLSDir->AddressOfIndex<<endl;
    cout<< "  AddressOfCallBacks:   "<<pTLSDir->AddressOfCallBacks<<endl;
    cout<< "  SizeOfZeroFill:       "<<pTLSDir->SizeOfZeroFill<<endl;
    cout<< "  Characteristics:      "<<pTLSDir->Characteristics<<endl;

	cout<< "\n" ;
}

template <class T> void DumpCOR20Header(PBYTE pImageBase,T* pNTHeader )	// T=PIMAGE_NT_HEADERS
{
    DWORD cor20HdrRVA; // COR20_HEADER RVA

    cor20HdrRVA=GetImgDirEntryRVA(pNTHeader,IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR );
    if (!cor20HdrRVA )
        return;

    PIMAGE_COR20_HEADER pCor20Hdr=(PIMAGE_COR20_HEADER)GetPtrFromRVA(cor20HdrRVA,pNTHeader,pImageBase );
	
	cout<<(".NET Runtime Header:\n" );

	cout<<"  Size:       "<<pCor20Hdr->cb<<endl;
	cout<<"  Version:    "<<pCor20Hdr->MajorRuntimeVersion<<"."<<pCor20Hdr->MinorRuntimeVersion<<endl;
	cout<<"  Flags:      "<<pCor20Hdr->Flags<<endl;
	if (pCor20Hdr->Flags & COMIMAGE_FLAGS_ILONLY ) cout<<("    ILONLY\n" );
	if (pCor20Hdr->Flags & COMIMAGE_FLAGS_32BITREQUIRED ) cout<<("    32BITREQUIRED\n" );
	if (pCor20Hdr->Flags & COMIMAGE_FLAGS_IL_LIBRARY ) cout<<("    IL_LIBRARY\n" );
	if (pCor20Hdr->Flags & 8 ) cout<<("    STRONGNAMESIGNED\n" );		// At this moment,WINNT.H and CorHdr.H are out of sync...
	if (pCor20Hdr->Flags & COMIMAGE_FLAGS_TRACKDEBUGDATA ) cout<<("    TRACKDEBUGDATA\n" );

	DisplayDataDirectoryEntry("MetaData",pCor20Hdr->MetaData );
    DisplayDataDirectoryEntry("Resources",pCor20Hdr->Resources );
    DisplayDataDirectoryEntry("StrongNameSig",pCor20Hdr->StrongNameSignature );
    DisplayDataDirectoryEntry("CodeManagerTable",pCor20Hdr->CodeManagerTable );
    DisplayDataDirectoryEntry("VTableFixups",pCor20Hdr->VTableFixups );
    DisplayDataDirectoryEntry("ExprtAddrTblJmps",pCor20Hdr->ExportAddressTableJumps );
    DisplayDataDirectoryEntry("ManagedNativeHdr",pCor20Hdr->ManagedNativeHeader );

	cout<<("\n" );
}

template <class T,class U> void DumpLoadConfigDirectory(PBYTE pImageBase,T* pNTHeader,U * pLCD )	// T=PIMAGE_NT_HEADERS,U=PIMAGE_LOAD_CONFIG_DIRECTORY 
{
    DWORD loadConfigDirRVA;

    loadConfigDirRVA=GetImgDirEntryRVA(pNTHeader,IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG );
    if (!loadConfigDirRVA )
        return;

	pLCD=(U*)GetPtrFromRVA(loadConfigDirRVA,pNTHeader,pImageBase );

	cout<<"Image Load Configuration Directory:\n"<<endl;

    cout<<"  Size:                      "<<pLCD->Size<<endl;
    cout<<"  TimeDateStamp:             "<<pLCD->TimeDateStamp<<endl;
    cout<<"  Version:                       "<<pLCD->MajorVersion<<"."<<pLCD->MinorVersion<<endl;
    cout<<"  GlobalFlagsClear:              "<<pLCD->GlobalFlagsClear<<endl;
    cout<<"  GlobalFlagsSet:                "<<pLCD->GlobalFlagsSet<<endl;
    cout<<"  CriticalSectionDefaultTimeout: "<<pLCD->CriticalSectionDefaultTimeout<<endl;
    cout<<"  DeCommitFreeBlockThreshold:    "<<pLCD->DeCommitFreeBlockThreshold<<endl;
    cout<<"  DeCommitTotalFreeThreshold:    "<<pLCD->DeCommitTotalFreeThreshold<<endl;
	cout<<"  LockPrefixTable:        "<<pLCD->LockPrefixTable<<endl;
    cout<<"  MaximumAllocationSize:  "<<pLCD->MaximumAllocationSize<<endl;
    cout<<"  VirtualMemoryThreshold: "<<pLCD->VirtualMemoryThreshold<<endl;
    cout<<"  ProcessHeapFlags:       "<<pLCD->ProcessHeapFlags<<endl;
    cout<<"  ProcessAffinityMask:    "<<pLCD->ProcessAffinityMask<<endl;
    cout<<"  CSDVersion:             "<<pLCD->CSDVersion<<endl;
    cout<<"  Reserved1:              "<<pLCD->Reserved1<<endl;
	cout<<"  EditList:               "<<pLCD->EditList<<endl;

	cout<<"\n"<<endl;
}
//
// top level routine called from PEDUMP.CPP to dump the components of a PE file
//
void DumpExeFile(HMODULE hModule,PIMAGE_DOS_HEADER dosHeader )
{
    PIMAGE_NT_HEADERS pNTHeader;
	PIMAGE_NT_HEADERS64 pNTHeader64;
    PBYTE pImageBase=(PBYTE)dosHeader;
    
	// Make pointers to 32 and 64 bit versions of the header.
    pNTHeader=MakePtr(PIMAGE_NT_HEADERS,dosHeader,dosHeader->e_lfanew );

	pNTHeader64=(PIMAGE_NT_HEADERS64)pNTHeader;

    // First,verify that the e_lfanew field gave us a reasonable
    // pointer,then verify the PE signature.
	if (IsBadReadPtr(pNTHeader,sizeof(pNTHeader->Signature) ) )
	{
		cout<<("File is not Portable Executable\n");
		return;
	}

    if (pNTHeader->Signature != IMAGE_NT_SIGNATURE )
    {
        cout<<("File is not Portable Executable\n");
        return;
    }

	bool bIs64Bit=(pNTHeader->OptionalHeader.Magic==IMAGE_NT_OPTIONAL_HDR64_MAGIC );

    if (pNTHeader->FileHeader.PointerToSymbolTable==0 )
        g_pCOFFHeader=0;// Doesn't really exist!

	bIs64Bit
	    ? DumpExportsSection(hModule,pImageBase,pNTHeader64 )
		: DumpExportsSection(hModule,pImageBase,pNTHeader );
	//=========================================================================
	//
	// If we have COFF symbols,create a symbol table now
	//
	//=========================================================================

	if (g_pCOFFHeader )				// Did we see a COFF symbols header while looking
	{								// through the debug directory?
		g_pCOFFSymbolTable=new COFFSymbolTable(
				pImageBase+ pNTHeader->FileHeader.PointerToSymbolTable,
				pNTHeader->FileHeader.NumberOfSymbols );
	}
    if (fShowRelocations )
    {
        bIs64Bit
			? DumpBaseRelocationsSection(pImageBase,pNTHeader64 )
			: DumpBaseRelocationsSection(pImageBase,pNTHeader );
        cout<<("\n");
    } 
    if (fShowSymbolTable && g_pCOFFHeader )
    {
        DumpCOFFHeader(g_pCOFFHeader );
        cout<<("\n");
    }
	if (g_pCOFFSymbolTable)
		delete g_pCOFFSymbolTable;
}
