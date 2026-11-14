#ifndef _FILE_MAP_HPP
#define _FILE_MAP_HPP

#include <limits.h>

// memory mapped file access
class FileBufferMapped
{
	HANDLE _fileHandle;
	HANDLE _mapHandle;
	void *_view;
	int _size;


	protected:
	void Open( HANDLE fileHandle, int start, int size );

	public:
	void Open( const char *name, int start=0, int size=INT_MAX );
	void Close();

	FileBufferMapped();
	FileBufferMapped( HANDLE file, int start=0, int size=INT_MAX );
	FileBufferMapped( const char *name, int start=0, int size=INT_MAX );
	~FileBufferMapped();

	char *GetData() const {return (char *)_view;}
	int GetSize() const {return _size;}
	bool GetError() const {return _view==NULL;}
};

#endif
