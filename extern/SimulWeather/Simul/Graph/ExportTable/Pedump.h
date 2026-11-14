#pragma once
#include <string>
#include "./Export.h"

#pragma comment(lib,"Dbghelp.lib")

extern BOOL fShowRawSectionData;
extern BOOL fShowSymbolTable;
extern BOOL fShowLineNumbers;
extern BOOL fShowIATentries;
extern BOOL fShowPDATA;
extern BOOL fShowResources;

extern PIMAGE_COFF_SYMBOLS_HEADER g_pCOFFHeader;
extern PIMAGE_DEBUG_MISC g_pMiscDebugInfo;
extern PDWORD g_pCVHeader;

class COFFSymbolTable;
extern COFFSymbolTable * g_pCOFFSymbolTable;

extern PBYTE g_pMappedFileBase;
extern void EXPORTTABLE_EXPORT DumpFile(HMODULE hModule,const char *filename,void(*export_callback)(const char *,void *)=NULL);
extern std::string Undecorate(const std::string &decorated);