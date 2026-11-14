#ifdef _MSC_VER
#pragma once
#endif

#ifndef _EL_IFILE_MAKE_HPP
#define _EL_IFILE_MAKE_HPP

#include <Es/Framework/appFrame.hpp>

// Declare platform independent representant of HANDLE
#if defined(POSIX_FILES) && !defined(_WIN32)
typedef int BinMakeHandle;
#else
typedef struct _binMakeHandle{} *BinMakeHandle;
#endif

/// file making interface
class FileMakeFunctions
{
public:
	//! virtual destructor
	virtual ~FileMakeFunctions() {}
	/// make target file from a source file
	/**
	@param tgt target file
	@param src source file
	@param rules rule set identification
	@return <0 on error, >=0 on success
	*/
	virtual int Make(const char *src, const char *tgt, const char *rules, BinMakeHandle *handle)
	{
	  // by default we are unable to convert any files
	  ErrorMessage(
	    EMError,"Make not available (requested by '%s')",src
	  );
	  return -1;
	}
};

#endif
