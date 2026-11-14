#ifdef _MSC_VER
#pragma once
#endif

#ifndef _FILE_SERVER_HPP
#define _FILE_SERVER_HPP

#include <El/QStream/QStream.hpp>

// file buffering class, suitable for background (multithreaded) usage

class QFBank;

class FileServer: public RefCount
{

	public:
	// abstract class - single/multi threaded file loading
	// say in advance we will need the file - insert it into the queue
	virtual void Request( const char *name, float time, int from=0, int to=INT_MAX)=NULL;
	virtual void CancelRequest( const char *name, int from=0, int to=INT_MAX )=NULL;
	// say we need the file NOW - load it
	virtual void Open( QIFStream &stream, const char *name )=NULL;

	virtual void Start()=NULL; // start/stop background activity
	virtual void Stop()=NULL;

	virtual void FlushBank(QFBank *bank) = NULL;
    
};

extern Ref<FileServer> GFileServer;
extern bool GEnableCaching;

#endif
