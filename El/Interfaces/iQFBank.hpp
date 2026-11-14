#ifdef _MSC_VER
#pragma once
#endif

#ifndef _EL_I_QFBANK_HPP
#define _EL_I_QFBANK_HPP

#ifndef _XBOX
class FileBufferMapped;
#endif
class QFBank;

//! class of callback functions
class QFBankFunctions
{
public:
	virtual ~QFBankFunctions() {}

  /// if possible, flush the bank handle so that the file can be deleted/written
  virtual bool FlushBankHandle(const char *filename){return true;}
  #ifndef _XBOX
	/// check if buffer is owned by given bank
	virtual bool BufferOwned(const QFBank *bank, const FileBufferMapped *buffer) {return false;}
  #endif
};

#endif
