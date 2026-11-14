#include <windows.h>
#include "fileMap.hpp"

#include <Es/Framework/debugLog.hpp>
#include <Es/Common/fltopts.hpp>

void FileBufferMapped::Open( HANDLE fileHandle, int start=0, int size=INT_MAX )
{
	int fileSize = ::GetFileSize(fileHandle,NULL);

	saturate(start,0,fileSize);
	saturate(size,0,fileSize-start);

	_mapHandle = ::CreateFileMapping
	(
		fileHandle,NULL,
		PAGE_READONLY,
		0,0, // all file
		NULL
	);
	if (_mapHandle)
	{
		_view = ::MapViewOfFile
		(
			_mapHandle,FILE_MAP_READ,0,start,size
		);
		_size = size;
	}
}

void FileBufferMapped::Open( const char *name, int start, int size )
{
	Close();

	_size = 0;
	_mapHandle = NULL;
	_view = NULL;
	_fileHandle = NULL;

	if (size<=0) return; // zero sized - no data
	_fileHandle=::CreateFile
	(
		name,GENERIC_READ,
		//0, // exclusive access
		FILE_SHARE_READ, // enable reading
    NULL, // security
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL,
    NULL // template
  );
	if( _fileHandle==INVALID_HANDLE_VALUE)
	{
		_fileHandle = NULL;
	}
	else
	{
		Open(_fileHandle,start,size);
	}
}

FileBufferMapped::FileBufferMapped()
{
	_size = 0;
	_mapHandle = NULL;
	_view = NULL;
	_fileHandle = NULL;
}

FileBufferMapped::FileBufferMapped( HANDLE file, int start, int size )
{
	_size = 0;
	_mapHandle = NULL;
	_view = NULL;
	_fileHandle = NULL;
	if (size<=0) return; // zero sized - no data
	
	Open(file,start,size);
}

FileBufferMapped::FileBufferMapped( const char *name, int start, int size )
{
	_size = 0;
	_mapHandle = NULL;
	_view = NULL;
	_fileHandle = NULL;
	Open(name,start,size);
}

void FileBufferMapped::Close()
{
	if (_view) ::UnmapViewOfFile(_view), _view = NULL;
	if (_fileHandle) ::CloseHandle(_fileHandle),_fileHandle = NULL;
	if (_mapHandle) ::CloseHandle(_mapHandle),_mapHandle = NULL;
}

FileBufferMapped::~FileBufferMapped()
{
	Close();
}
