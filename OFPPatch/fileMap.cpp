#include "stdafx.h"
#include <Es/essencepch.hpp>
#include "fileMap.hpp"

#include <Es/Framework/debugLog.hpp>
#include <Es/Common/fltopts.hpp>

void FileMemMapped::MapWholeFile()
{
	if (_mapHandle && !_view)
	{
		_view = ::MapViewOfFile(_mapHandle,FILE_MAP_READ,0,_start,_size);
    if (!_view)
    {
      HRESULT err = GetLastError();
      LogF("Error: %x",err);
      // check current memory
      MEMORYSTATUS mem;
      mem.dwLength = sizeof(mem);
      GlobalMemoryStatus(&mem);
      LogF("Virtal mem. free: %x",mem.dwAvailVirtual);
      // fatal error - unable to apply the patch
      MessageBox(NULL,"Out of memory","Patch error",MB_OK|MB_ICONERROR);
    }
	}

}

/**
We assume view is already aligned as needed
*/
Ref<FileMemView> FileMemMapped::NewView(int index, int start, int size) const
{
  Assert(start%_viewPage==0);
  //LogF("Mapping %x",start);
  void *mem = ::MapViewOfFile(_mapHandle,FILE_MAP_READ,0,start,size);
  if (!mem)
  {
    LogF("Memory mapping error %x",GetLastError());
    return NULL;
  }
  FileMemView *view = new FileMemView(index);
  view->_view = mem;
  view->_size = size;
  return view;
}
void FileMemMapped::CloseView(FileMemView *view) const
{
  ::UnmapViewOfFile(view->_view);
  view->_view = NULL;
  view->_size = 0;
}

void FileMemMapped::LoadSlot( int index ) const
{
  // last page cannot be mapped full
  int start = index<<_viewPageLog2;
  int rest = _size-start;
  _views[index] = NewView(index,index<<_viewPageLog2,rest<_viewPage ? rest : _viewPage);
  _lruViews.Insert(_views[index]);
  
  // if there are too many slots, we may need to discard some
  // we have at max 2 GB of virtual space
  // we want to be able to map two files at once
  // one file can therefore have less than 1 GB
  const int maxMem = 800*1024*1024;
  //const int maxMem = 256*1024*1024;
  //const int maxMem = 1*1024*1024;
  const int maxViews = maxMem>>_viewPageLog2;
  if (_lruViews.Size()>maxViews)
  {
    int index = _lruViews.Last()->_index;
    _lruViews.Delete(_lruViews.Last());
    CloseView(_views[index]);
    _views[index].Free();
  }
}

FileMemView *FileMemMapped::GetView(int &offset) const
{
  // check which page
  int index = offset>>_viewPageLog2;
  // return what the real offset is
  offset = index<<_viewPageLog2;
  FileMemView *slot = _views[index];
  // load the slot now if not loaded yet
  if (!slot)
  {
    LoadSlot(index);

    slot = _views[index];
  }
  return slot;
}

char FileMemMapped::Get(int offset) const
{
  // check which page
  int index = offset>>_viewPageLog2;
  FileMemView *slot = _views[index];
  // load the slot now if not loaded yet
  if (!slot)
  {
    LoadSlot(index);
    
    slot = _views[index];
  }
  char *data = (char *)slot->_view;
  int inPage = offset&(_viewPage-1);
  Assert(inPage<slot->_size);
  return data[inPage];
}


void FileMemMapped::Open( HANDLE fileHandle, int start=0, int size=INT_MAX )
{
	int fileSize = ::GetFileSize(fileHandle,NULL);

	saturate(start,0,fileSize);
	saturate(size,0,fileSize-start);

  _start = start;
	_size = size;

	_mapHandle = ::CreateFileMapping(
		fileHandle,NULL,
		PAGE_READONLY,
		0,0, // all file
		NULL
	);

  SYSTEM_INFO sys;
  GetSystemInfo(&sys);
  _viewPage = sys.dwAllocationGranularity;
  // we want to use quite big pages
  while (_viewPage<256*1024) _viewPage *=2;
  //while (_viewPage<32*1024) _viewPage *=2;

  _viewPageLog2 = log2Floor(_viewPage);
  Assert((1<<_viewPageLog2)==_viewPage);

  int slotCount = (size + _viewPage-1)>>_viewPageLog2;
  _views.Realloc(slotCount);
  _views.Resize(slotCount);
  // log2
  /*
	if (_mapHandle)
	{
    MapWholeFile();
	}
  */
}

void *FileMemMapped::GetRawData()
{
  // create the continuous memory mapping as needed
  MapWholeFile();
  return _view;
}

/*
void *FileMemMapped::GetView(int offset, int size)
{
  if (_view) return _view;
	_tempView = ::MapViewOfFile(_mapHandle,FILE_MAP_READ,0,offset,size);
  _tempViewSize = size;
}

void *FileMemMapped::CloseView(void *view)
{
  // permanent view should never be closed
  if (view==_view) return;
  Assert(view==_tempView);
}
*/

void FileMemMapped::Open( const char *name, int start, int size )
{
	Close();

	_size = 0;
	_mapHandle = NULL;
	_view = NULL;
	_fileHandle = NULL;

	if (size<=0) return; // zero sized - no data
	_fileHandle=::CreateFile(
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

FileMemMapped::FileMemMapped()
{
	_size = 0;
	_mapHandle = NULL;
	_view = NULL;
	_fileHandle = NULL;
}

FileMemMapped::FileMemMapped( HANDLE file, int start, int size )
{
	_size = 0;
	_mapHandle = NULL;
	_view = NULL;
	_fileHandle = NULL;
	if (size<=0) return; // zero sized - no data
	
	Open(file,start,size);
}

FileMemMapped::FileMemMapped( const char *name, int start, int size )
{
	_size = 0;
	_mapHandle = NULL;
	_view = NULL;
	_fileHandle = NULL;
	Open(name,start,size);
}

void FileMemMapped::Close()
{
	if (_view) ::UnmapViewOfFile(_view), _view = NULL;
	if (_fileHandle) ::CloseHandle(_fileHandle),_fileHandle = NULL;
	if (_mapHandle) ::CloseHandle(_mapHandle),_mapHandle = NULL;
}

FileMemMapped::~FileMemMapped()
{
  // unmap all views which are still open
  for (int i=0; i<_views.Size(); i++)
  {
    if (!_views[i]) continue;
    CloseView(_views[i]);
    _views[i].Free();
  }
	Close();
}

