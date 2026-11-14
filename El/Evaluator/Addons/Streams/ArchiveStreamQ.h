//0x ArchiveStreamQ.h: interface for the ArchiveStreamQ class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ARCHIVESTREAMQ_H__ECA98D62_B211_4A74_8339_C4E7E934D826__INCLUDED_)
#define AFX_ARCHIVESTREAMQ_H__ECA98D62_B211_4A74_8339_C4E7E934D826__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/* 
BEFORE INCLUDE THIS HEADER
==========================
Define memory allocator: Currently #define MFC_NEW
Include "el\QStream\QStream.cpp" into your project
Include "El\QStream\qstreamUseFServerDefault.cpp" into your project
Include "es\framework\appFrame.cpp" into your project
Include "es\framework\useAppFrameDefault.cpp" into your project
Include "es\memory\fastAlloc.cpp" into your project
Include "es\strings\rstring.cpp" into your project
Include "El\QStream\fileMapping.cpp" into your project
Include "El\QStream\qstreamUseBankDefault.cpp" into your project

*/


#include "ArchiveStream.h"
#include <El\QStream\QStream.hpp>

class ArchiveStreamQIn : public ArchiveStream  
{
  QIStream *_stream;
  bool _autoclose;
  int _err;
public:
	ArchiveStreamQIn(QIStream *stream,bool autoclose);
	virtual ~ArchiveStreamQIn();
    virtual bool IsStoring() {return false;}
    virtual int DataExchange(void *buffer, int maxsize);
    virtual int IsError() {return _err;}
    virtual void Reset() {_err=0;}
    virtual void SetError(int err) {_err=err;}
    virtual __int64 Tell();
    virtual void Seek(__int64 lOff, SeekOp seekop);

};

class ArchiveStreamQOut : public ArchiveStream  
{
  QOStream *_stream;
  bool _autoclose;
  int _err;
public:
	ArchiveStreamQOut(QOStream *stream,bool autoclose);
	virtual ~ArchiveStreamQOut();
    virtual bool IsStoring() {return true;}
    virtual int DataExchange(void *buffer, int maxsize);
    virtual int IsError() {return _err;}
    virtual void Reset() {_err=0;}
    virtual void SetError(int err) {_err=err;}
    virtual __int64 Tell();
    virtual void Seek(__int64 lOff, SeekOp seekop);

};
#endif // !defined(AFX_ARCHIVESTREAMQ_H__ECA98D62_B211_4A74_8339_C4E7E934D826__INCLUDED_)
