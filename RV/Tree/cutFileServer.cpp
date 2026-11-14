#include "El/elementpch.hpp"
#include "cutFileServer.hpp"
#ifdef _WIN32
  #include <process.h>
  #include <El/QStream/fileOverlapped.hpp>
#endif
#include "cutFileServerMT.hpp"
//#include "global.hpp"

#if _ENABLE_CHEATS
#define ENABLE_OVERLAPPED_IO 0
#endif

Ref<FileServer> GFileServer;
bool GEnableCaching=true;

FileCache::FileCache( int size, int files )
:_maxSize(size),_maxFiles(files),
_size(0)
{
	RegisterMemoryFreeOnDemand(this);
}

FileCache::~FileCache()
{
}

int FileCache::Find( const char *name )
{
	for( int i=0; i<_cache.Size(); i++ )
	{
		const FileInCache *file=_cache[i];
		if( !strcmp(file->_name,name) ) return i;
	}
	return -1;
}

void FileCache::MoveToFront( int index )
{
	SRef<FileInCache> temp=_cache[index];
	_cache.Delete(index);
	_cache.Insert(0,temp);
}

void FileCache::DeleteLast()
{
	if( _cache.Size()<=0 ) return;
	FileInCache *file = _cache[_cache.Size()-1];
	Assert(file->_data.tellg()==0);
	_size -= file->_data.rest();
	Assert( _size>=0 );
	_cache.Delete(_cache.Size()-1);
}

//! average expected file size is 64 KB
const int FileCacheItemMemSize = 64*1024;

size_t FileCache::FreeOneItem()
{
	if( _cache.Size()<=0 ) return 0;
	size_t sizeBefore = _size;
	DeleteLast();
	size_t sizeAfter = _size;
	return sizeBefore-sizeAfter;
}

float FileCache::Priority()
{
	// estimated CPU time to read typical file
	// due to background loading it is much less that actual time neeed
	const float itemTime = 300000*0.1;
	// estimated time per byte
	return itemTime/FileCacheItemMemSize;
}

int FileCache::Load( const char *name )
{
	// search the cache
	int index=Find(name);
	if( index>=0 )
	{
		if( index!=0 ) MoveToFront(index);
		//Log("File %s cached.",name);
		//_cache[index]->_lastUsed=Glob.time;
		return 0;
	}
	//Log("Loaded file %s",name);
	// not cached yet
	FileInCache *newEntry=new FileInCache(name);
	if( newEntry->_data.fail() )
	{
		delete newEntry;
		RptF("File '%s' not found",name);
		return -1;
	}
	//newEntry->_lastUsed=Glob.time;
	_cache.Insert(0,newEntry);
	Assert(newEntry->_data.tellg()==0);
	// maintain size statistics
	_size+=newEntry->_data.rest();
	Maintain();
	return 0;
}


bool FileCache::IsLoaded(const char *name)
{
	int index=Find(name);
	return( index>=0 );
}

void FileCache::Open( QIFStream &stream, const char *name )
{
	if( !name || !*name )
	{
		Fail("Empty or NULL name not allowed.");
		return;
	}
	// check _done to see if it was requested and satisfied
	// check _queue to see if it was requested

	if (GEnableCaching)
	{
		// note: multiple threads may open file same time
		// make sure file is in cache
		int index=Load(name);
		if( index>=0 )
		{
			// it must be the first file
			Assert( index==0 );
			Assert( !strcmp(name,_cache[index]->_name) );
			// share cached data
			stream=_cache[index]->_data;
		}
		else
		{
			Fail("Cache failure");
		}
	}
	else
	{
		QIFStreamB temp;
		temp.AutoOpen(name);
		stream = temp;
	}
}

void FileCache::Store( QIFStreamB &stream, const char *name )
{
	// search the cache
	int index=Find(name);
	if( index>=0 )
	{
		if( index!=0 ) MoveToFront(index);
		return;
	}
	// not cached yet
	FileInCache *newEntry=new FileInCache(stream,name);
	_cache.Insert(0,newEntry);
	Assert(newEntry->_data.tellg()==0);
	_size+=newEntry->_data.rest();
	Maintain();
}
void FileCache::Maintain()
{
	while( (_size>_maxSize || _cache.Size()>_maxFiles) && _cache.Size()>1 )
	{
		DeleteLast();
	}
}

void FileCache::FlushBank(QFBank *bank)
{
	// release all files from this bank

	for( int i=0; i<_cache.Size(); i++ )
	{
		const FileInCache *file=_cache[i];
		const QIFStreamB &stream = file->_data;
		if (!stream.IsFromBank(bank)) continue;
		_cache.Delete(i);
		i--;
	}
}

DEFINE_FAST_ALLOCATOR(FileCache::FileInCache)

bool FileRequest::operator == (const FileRequest &with) const
{
	return
	(
		!strcmpi(_filename,with._filename) &&
		_from==with._from &&
		_to==with._to
	);
}
bool FileRequest::Contains(const FileRequest &with) const
{
	return
	(
		!strcmpi(_filename,with._filename) &&
		_from<=with._from &&
		_to>=with._to
	);
}

int FileServerST::RequestPresentAndDone(const FileRequest &req) const
{
	for (int i=0; i<_done.Size(); i++)
	{
		if (_done[i].Contains(req))
		{
			return i;
		}
	}
	return -1;
}
int FileServerST::RequestPresentAndNotDone(const FileRequest &req) const
{
	for (int i=0; i<_queue.Size(); i++)
	{
		if (_queue[i].Contains(req))
		{
			return i;
		}
	}
	return -1;
}

bool FileServerST::RequestPresent(const FileRequest &req) const
{
	if (RequestPresentAndNotDone(req)>=0) return true;
	if (RequestPresentAndDone(req)>=0) return true;
	return false;
}

void FileServerST::Request(const char *name, float time, int from, int to)
{
	#if ENABLE_OVERLAPPED_IO
	if (_cache.IsLoaded(name))
	{
		Update();
		return;
	}
	// check if given request is already queued
	DWORD itime = GlobalTickCount() + toInt(1000*time);
	FileRequest request(name,itime,from,to);
	// check 
	if (!RequestPresent(request))
	{
		_queue.HeapInsert(request);
	}
	Update();
	#endif
}

void FileServerST::CancelRequest(const char *name, int from, int to)
{
}

void FileServerST::Update()
{
	// check if current operation is terminated
	// if it is, process next request
	while(_queue.Size()>0)
	{
		// get heap head
		// check if operation already started
		FileRequest &head = _queue[0];
		if (!head._filebuf)
		{
			QFBank *bank = QIFStreamB::AutoBank(head._filename);
			if (bank)
			{
				const char *name = head._filename;
				name += bank->GetPrefix().GetLength();
				head._filebuf = bank->ReadOverlapped(name);
			}
			else
			{
				QIFStreamB temp;
				temp.AutoOpen(head._filename);
				head._filebuf = head._in.GetBuffer();
			}
		}
		// check if operation terminated
		if(!head._filebuf->IsReady())
		{
			break;
		}
		FileRequest req;
		_queue.HeapRemoveFirst(req);
		req._in.OpenBuffer(head._filebuf);
		_done.Add(req);
	}
}

void FileServerST::Open( QIFStream &stream, const char *name )
{
	#if ENABLE_OVERLAPPED_IO
	Update();
	// try to locate it in cache
	if (_cache.IsLoaded(name))
	{
		_cache.Open(stream,name);
		return;
	}
	// try to locate it in _done queue
	FileRequest req(name,0);
	int done = RequestPresentAndDone(req);
	if(done>=0)
	{
		_cache.Store(_done[done]._in,name);
		stream = _done[done]._in;
		return;
	}
	int queue = RequestPresentAndNotDone(req);
	if (queue>=0)
	{
		// wait until it is loaded
		while (RequestPresentAndNotDone(req)>=0)
		{
			Update();
		}
		int done = RequestPresentAndDone(req);
		if(done>=0)
		{
			_cache.Store(_done[done]._in,name);
			stream = _done[done]._in;
			_done.Delete(done);
			return;
		}
		else
		{
			LogF("Request failed: %s",name);
			_cache.Open(stream,name);
			return;
		}
	}
	#endif
	_cache.Open(stream,name);
}

void FileServerST::FlushBank(QFBank *bank)
{
	// scan all files and release those comming this bank
	_cache.FlushBank(bank);
}


