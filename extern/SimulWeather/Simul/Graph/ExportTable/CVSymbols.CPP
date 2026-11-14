#include <windows.h>
#include <stdio.h>
#include <imagehlp.h>
#include <string.h>
#include "common.h"
#include "cvexefmt.h"
#include "CVInfoNew.h"	// Our own definitions for "newer" CV info
#include <iostream>
using namespace std;

void DecodeDataSym32( DATASYM32 * pDataSym32,PSTR szOutBuffer );
void DecodeDataSym32_New( DATASYM32_NEW *,PSTR szOutBuffer,size_t bufsize );
PSTR GetSubsectionTypeName( unsigned short sst );
PSTR GetSymRecordName( SYM_ENUM_e symrectype );



void DisplaySSTModule( PBYTE pRawData,DWORD cb )
{
	cb;
	OMFModule * pOMFModule=(OMFModule*)pRawData;
	
	// Pointer math below!!!
	PBYTE pcbName=(PBYTE)(&pOMFModule->SegInfo + pOMFModule->cSeg);

	cout<<"  Name: "<<*pcbName<<" "<<pcbName + 1<<endl;
		
    cout<<"  ovlNumber: "<<pOMFModule->ovlNumber
		<<"  iLib: "<<pOMFModule->iLib
		<<"  cSeg: "<<pOMFModule->cSeg
		<<"  Style: "<<pOMFModule->Style[0]
		<<"  "<<pOMFModule->Style[1]<<endl;
	
	for( unsigned i=0;i < pOMFModule->cSeg;i++ )
	{
	    cout<<"  SegInfo["<<i
			<<"] Seg: "<<pOMFModule->SegInfo[i].Seg
			<<"  pad: "<<pOMFModule->SegInfo[i].pad
			<<"  Off: "<<pOMFModule->SegInfo[i].Off
			<<"  cbSeg: "<<pOMFModule->SegInfo[i].cbSeg <<endl;
	}
}

void DisplaySSTGlobalPub( PBYTE pRawData,DWORD cb )
{
	cb;
	OMFSymHash * pOMFSymHash=(OMFSymHash*)pRawData;

    cout<<"symhash: "<<pOMFSymHash->symhash<<endl;
    cout<<"addrhash: "<<pOMFSymHash->addrhash<<endl;
    cout<<"cbSymbol: "<<pOMFSymHash->cbSymbol<<endl;
    cout<<"cbHSym: "<<pOMFSymHash->cbHSym<<endl;
    cout<<"cbHAddr: "<<pOMFSymHash->cbHAddr<<endl;

	unsigned long bytesLeft=pOMFSymHash->cbSymbol;
	SYMTYPE * pSymbol=(SYMTYPE*)(pRawData + sizeof(OMFSymHash));
	
	while ( bytesLeft )
	{
		char szSymDetail[512];
		
		switch( pSymbol->rectyp )
		{
			case S_LDATA32:
			case S_GDATA32:
			case S_PUB32:
				DecodeDataSym32( (DATASYM32 *)pSymbol,szSymDetail );
				break;
			case S_PUBSYM32_NEW:
				DecodeDataSym32_New( (DATASYM32_NEW *)pSymbol,szSymDetail,512 );
				break;
			default:
				szSymDetail[0]=0;
		}
		 
		cout<<"  "<<GetSymRecordName((SYM_ENUM_e)pSymbol->rectyp)<<" "<<
				szSymDetail<<endl;
		
		bytesLeft -= (pSymbol->reclen + sizeof(unsigned short));
		pSymbol=NextSym( pSymbol );
	}	
}

void DisplaySSTSegMap( PBYTE pRawData,DWORD cb )
{
	cb;
	OMFSegMap * pOMFSegMap=(OMFSegMap*)pRawData;

	cout<<"cSeg: "<<pOMFSegMap->cSeg<<endl;
	cout<<"cSegLog: "<<pOMFSegMap->cSegLog<<endl;

	OMFSegMapDesc * pOMFSegMapDesc=pOMFSegMap->rgDesc;
	
	for( unsigned i=0;i < pOMFSegMap->cSeg;i++ )
	{
		cout<<endl;
    	cout<<"  flags: "<<pOMFSegMapDesc->flags<<endl;
    	cout<<"  ovl: "<<pOMFSegMapDesc->ovl<<endl;
    	cout<<"  group: "<<pOMFSegMapDesc->group<<endl;
    	cout<<"  frame: "<<pOMFSegMapDesc->frame<<endl;
    	cout<<"  iSegName: "<<pOMFSegMapDesc->iSegName<<endl;
    	cout<<"  iClassName: "<<pOMFSegMapDesc->iClassName<<endl;
    	cout<<"  offset: "<<pOMFSegMapDesc->offset<<endl;
    	cout<<"  cbSeg: "<<pOMFSegMapDesc->cbSeg<<endl;
		cout<<endl;
	
		pOMFSegMapDesc++;
	}
}

void DisplayCVSubsection( unsigned short sst,PBYTE pRawData,DWORD cb,WORD dirIndex )
{
	cout<<"DirectoryEntry: "<<dirIndex<<" "<<GetSubsectionTypeName(sst)<<endl;
	
	switch ( sst )
	{
		case sstModule: DisplaySSTModule( pRawData,cb );break;
		case sstGlobalPub: DisplaySSTGlobalPub( pRawData,cb );break;
		case sstSegMap: DisplaySSTSegMap( pRawData,cb );break;
	}
}

BOOL DumpCVSymbolTable( PBYTE pRawCVSymbolData,PBYTE pFileBase )
{
	cout<<"== CodeView Info at offset "<<
		pRawCVSymbolData - pFileBase<<"==\n";

	OMFSignature * pOMFSignature=(OMFSignature*)pRawCVSymbolData;
	
	cout<<"Signature: "<<
			pOMFSignature->Signature[0]<<pOMFSignature->Signature[1]<<
			pOMFSignature->Signature[2]<<pOMFSignature->Signature[3]<<endl;
	cout<<( "lfoDirectory: %08X\n",pOMFSignature->filepos);

	cout<<( "\n" );
	
//	PBYTE lfaBase=pRawCVSymbolData;
	
	OMFDirHeader * pOMFDirHeader =
		(OMFDirHeader *)(pRawCVSymbolData + pOMFSignature->filepos);

	cout<<( "OMFDirHeader:\n" );

    cout<<"  cbDirHeader: "<<pOMFDirHeader->cbDirHeader<<endl;
    cout<<"  cbDirEntry: "<<pOMFDirHeader->cbDirEntry<<endl;
    cout<<"  cDir: "<<pOMFDirHeader->cDir<<endl;
    cout<<"  lfoNextDir: "<<pOMFDirHeader->lfoNextDir<<endl;
    cout<<"  flags: "<<pOMFDirHeader->flags<<endl;

	cout<<"\n"<<endl;
	cout<<"OMFDirEntries:\n"<<endl;
	
	OMFDirEntry * pOMFDirEntry=(OMFDirEntry *)(pOMFDirHeader+1);

	for( unsigned i=0;i < pOMFDirHeader->cDir;i++ )
	{
		cout<<( "\n" );

	    cout<<"  SubSection: "<<pOMFDirEntry->SubSection<<" "<<
			GetSubsectionTypeName(pOMFDirEntry->SubSection)<<"\n";
	    cout<<"  iMod: "<<pOMFDirEntry->iMod<<"\n";
	    cout<<"  lfo: "<<pOMFDirEntry->lfo<<"\n";
	    cout<<"  cb: "<<pOMFDirEntry->cb<<"\n";
		
		pOMFDirEntry++;
	}

	pOMFDirEntry=(OMFDirEntry *)(pOMFDirHeader+1);

	for(unsigned i=0;i < pOMFDirHeader->cDir;i++ )
	{
		cout<<( "\n" );

		DisplayCVSubsection( pOMFDirEntry->SubSection,
							 pRawCVSymbolData + pOMFDirEntry->lfo,
							 pOMFDirEntry->cb,
							 (WORD)(i+1) );
		pOMFDirEntry++;
	}
		
	return TRUE;
}

void DecodeDataSym32( DATASYM32 * pDataSym32,PSTR szOutBuffer )
{
    char szSymbolName[512];

    PCSTR pszDecoratedName=(PCSTR)&pDataSym32->name[1];

    if ( '?'==pDataSym32->name[1] )
    {// RVK: this said "pszUndecoratedName" but according to the definition the first param is decorated!!!
        UnDecorateSymbolName(   pszDecoratedName,
                                szSymbolName,
                                sizeof(szSymbolName),
                                UNDNAME_NO_LEADING_UNDERSCORES   |
                                UNDNAME_NO_MS_KEYWORDS           |
                                UNDNAME_NO_FUNCTION_RETURNS      |
                                UNDNAME_NO_ALLOCATION_MODEL      |
                                UNDNAME_NO_ALLOCATION_LANGUAGE   |
                                UNDNAME_NO_MS_THISTYPE           |
                                UNDNAME_NO_CV_THISTYPE           |
                                UNDNAME_NO_THISTYPE              |
                                UNDNAME_NO_ACCESS_SPECIFIERS     |
                                UNDNAME_NO_THROW_SIGNATURES      |
                                UNDNAME_NO_MEMBER_TYPE           |
                                UNDNAME_NO_RETURN_UDT_MODEL      |
                                UNDNAME_32_BIT_DECODE );

        if ( '?'==szSymbolName[0] )
        {
            const char * pszFirstAt;
			const char * pszTwoAts;
            pszFirstAt=strchr( pszDecoratedName,'@' );
            pszTwoAts=strstr( pszDecoratedName,"@@" );
            
            if ( pszFirstAt && pszTwoAts )
            {
                sprintf( szSymbolName,"%.*s::%.*s(%s)",
                        pszTwoAts - pszFirstAt - 1,
                        pszFirstAt + 1,
                        pszFirstAt - pszDecoratedName - 1,
                        pszDecoratedName + 1,
                        pszTwoAts + 2 );
            }
        }
    }
    else    // Just copy the length prefixed name into the szSymbolName variable
    {
        strncpy_s( szSymbolName,512,(char *)&pDataSym32->name[1],pDataSym32->name[0] );
        szSymbolName[pDataSym32->name[0]]=0; // Null terminate it
    }

	sprintf_s( szOutBuffer,512,"%04X:%08X type:%X %s",
		pDataSym32->seg,pDataSym32->off,pDataSym32->typind,szSymbolName );
}

void DecodeDataSym32_New( DATASYM32_NEW * pDataSym32_New,PSTR szOutBuffer,size_t bufsize)
{
	sprintf_s( szOutBuffer,bufsize,"%04X:%08X type:%X %.*s",
		pDataSym32_New->seg,pDataSym32_New->off,pDataSym32_New->typind,
		pDataSym32_New->name[0],&pDataSym32_New->name[1] );
}

PSTR GetSubsectionTypeName( unsigned short sst )
{
	#define CASE_STRING( x ) case x: s=#x;break;

	PSTR s="<unknown>";

	switch( sst )
	{
		CASE_STRING( sstModule )
		CASE_STRING( sstTypes )
		CASE_STRING( sstPublic )
		CASE_STRING( sstPublicSym )
		CASE_STRING( sstSymbols )
		CASE_STRING( sstAlignSym )
		CASE_STRING( sstSrcLnSeg )
		CASE_STRING( sstSrcModule )
		CASE_STRING( sstLibraries )
		CASE_STRING( sstGlobalSym )
		CASE_STRING( sstGlobalPub )
		CASE_STRING( sstGlobalTypes )
		CASE_STRING( sstMPC )
		CASE_STRING( sstSegMap )
		CASE_STRING( sstSegName )
		CASE_STRING( sstPreComp )
		CASE_STRING( sstPreCompMap )
		CASE_STRING( sstOffsetMap16 )
		CASE_STRING( sstOffsetMap32 )
		CASE_STRING( sstFileIndex )
		CASE_STRING( sstStaticSym )
	}
	
	return s;
}
#pragma warning(push)
#pragma warning(disable:4063)	// added new definition,not part of original enum.

PSTR GetSymRecordName( SYM_ENUM_e symrectype )
{
	PSTR s="<unknown>";

	switch( symrectype )
	{
    	CASE_STRING( S_COMPILE )
    	CASE_STRING( S_REGISTER )
    	CASE_STRING( S_CONSTANT )
    	CASE_STRING( S_UDT )
    	CASE_STRING( S_SSEARCH )
    	CASE_STRING( S_END )
    	CASE_STRING( S_SKIP )
    	CASE_STRING( S_CVRESERVE )
    	CASE_STRING( S_OBJNAME )
    	CASE_STRING( S_ENDARG )
    	CASE_STRING( S_COBOLUDT )
    	CASE_STRING( S_MANYREG )
    	CASE_STRING( S_RETURN )
    	CASE_STRING( S_ENTRYTHIS )
    	CASE_STRING( S_BPREL16 )
    	CASE_STRING( S_LDATA16 )
    	CASE_STRING( S_GDATA16 )
    	CASE_STRING( S_PUB16 )
    	CASE_STRING( S_LPROC16 )
    	CASE_STRING( S_GPROC16 )
    	CASE_STRING( S_THUNK16 )
    	CASE_STRING( S_BLOCK16 )
    	CASE_STRING( S_WITH16 )
    	CASE_STRING( S_LABEL16 )
    	CASE_STRING( S_CEXMODEL16 )
    	CASE_STRING( S_VFTABLE16 )
    	CASE_STRING( S_REGREL16 )
    	CASE_STRING( S_BPREL32 )
    	CASE_STRING( S_LDATA32 )
    	CASE_STRING( S_GDATA32 )
    	CASE_STRING( S_PUB32 )
    	CASE_STRING( S_LPROC32 )
    	CASE_STRING( S_GPROC32 )
    	CASE_STRING( S_THUNK32 )
    	CASE_STRING( S_BLOCK32 )
    	CASE_STRING( S_WITH32 )
    	CASE_STRING( S_LABEL32 )
    	CASE_STRING( S_CEXMODEL32 )
    	CASE_STRING( S_VFTABLE32 )
    	CASE_STRING( S_REGREL32 )
    	CASE_STRING( S_LTHREAD32 )
    	CASE_STRING( S_GTHREAD32 )
    	CASE_STRING( S_SLINK32 )
    	CASE_STRING( S_LPROCMIPS )
    	CASE_STRING( S_GPROCMIPS )
    	CASE_STRING( S_PROCREF )
    	CASE_STRING( S_DATAREF )
    	CASE_STRING( S_ALIGN )
    	CASE_STRING( S_LPROCREF )
		CASE_STRING( S_PUBSYM32_NEW )
		default:
		{
			static char szRetBuffer[128];
			
			sprintf( szRetBuffer,"<Unknown:%X>",symrectype );
			s=szRetBuffer;
		}
	}

	return s;
}
#pragma warning(pop)