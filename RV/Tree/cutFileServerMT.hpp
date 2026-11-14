#ifdef _MSC_VER
#pragma once
#endif

#ifndef _FILE_SERVER_MT_HPP
#define _FILE_SERVER_MT_HPP

#include "cutFileServer.hpp"
#include <Es/Memory/fastAlloc.hpp>
#include <Es/Containers/staticArray.hpp>

#include <Es/Memory/normalNew.hpp>

#include <El/QStream/QBStream.hpp>

#include "El/FreeOnDemand/memFreeReq.hpp"

class FileCache: public MemoryFreeOnDemandHelper
{
	class FileInCache //: public RefCount
	{
		public:
//		char _name[64-5*4]; // align FileInCache to 64B
		RString _name;
		QIFStreamB _data;
		//float _lastUsed;

		FileInCache(){} // empty name for and empty entry
		~FileInCache(){} // no name for destructed entry
		FileInCache( const char *name )
		{
//			Assert( strlen(name)<sizeof(_name) );
//			strncpy(_name,name,sizeof(_name));
			_name = name;
			_data.AutoOpen(_name);
		}
		FileInCache( QIFStreamB &in, const char *name )
		{
//			Assert( strlen(name)<sizeof(_name) );
//			strncpy(_name,name,sizeof(_name));
			_name = name;
			_data = in;
		}
		USE_FAST_ALLOCATOR
	};

	int _size;
	int _maxSize;
	int _maxFiles;

	AutoArray< SRef<FileInCache> > _cache;

	protected:
	int Find( const char *name );
	void MoveToFront( int index );
	void DeleteLast();

	public:
	FileCache( int size=8*1024*1024, int files=1024 );
	~FileCache();

	//! open file and cache it as neccessary
	bool IsLoaded(const char *name);
	//! open file and cache it as neccessary
	void Open( QIFStream &stream, const char *name );
	//! we have opened the file and we want to give it caching opportunity
	void Store( QIFStreamB &stream, const char *name );
	//! flush any old data
	void Maintain();
	//! load, do not return result
	int Load( const char *name );

	void FlushBank(QFBank *bank);

	// implementation of MemoryFreeOnDemandHelper abstract interface
	virtual size_t FreeOneItem();
	virtual float Priority();
};

#include <Es/Memory/debugNew.hpp>

//! file request - waiting in queue to
struct FileRequest
{
	//! source file name
	RString _filename;
	//! file region we are interested in
	int _from,_to;
	//! time when we will need the file
	DWORD _timeNeeded;

	//! stream where result should be stored
	QIFStreamB _in;

	//! overlapped operator performed here
	Ref<IFileBuffer> _filebuf;

	FileRequest(){}
	FileRequest(const char *name, DWORD time, int from=0, int to=INT_MAX)
	:_filename(name),_timeNeeded(time),_from(0),_to(INT_MAX)
	{
	}

	bool operator == (const FileRequest &with) const;
	bool Contains(const FileRequest &with) const;
};

TypeIsMovable(FileRequest)

template <>
struct HeapTraits<FileRequest>
{
	static bool IsLess(const FileRequest &a, const FileRequest &b)
	{
		return a._timeNeeded < b._timeNeeded;
	}
	static bool IsLessOrEqual(const FileRequest &a, const FileRequest &b)
	{
		return a._timeNeeded <= b._timeNeeded;
	}
};

//! file server

class FileServerST: public FileServer
{
	FileCache _cache;
	//! list of satisfied request
	FindArray<FileRequest> _done;
	//! heap of waiting requests
	HeapArray<FileRequest> _queue;

	public:
	FileServerST( int cacheSize )
	:_cache(cacheSize)
	{
	}
	
	//! find any matching request
	bool RequestPresent(const FileRequest &req) const;
	int RequestPresentAndDone(const FileRequest &req) const;
	int RequestPresentAndNotDone(const FileRequest &req) const;

	// say in advance we will need the file - insert it into the queue
	void Request(const FileRequest &req);
	void CancelRequest(const FileRequest &req);

	void Request(const char *name, float time, int from, int to);
	void CancelRequest(const char *name, int from, int to);
	// say we need the file NOW - load it
	void Open( QIFStream &stream, const char *name );
	void FlushBank(QFBank *bank);
	void Start(){}
	void Stop(){}

	//! main body of file-server - perform reqular maintenance
	void Update();
};


#endif
