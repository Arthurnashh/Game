extern void(*export_table_callback)(const char *,void *);
void DumpExeFile( HMODULE hModule,PIMAGE_DOS_HEADER dosHeader );
template <class T> void DumpOptionalHeader(T* pImageOptionalHeader);	// 'T' is IMAGE_OPTIONAL_HEADER32/64
