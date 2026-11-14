#ifdef _MSC_VER
#pragma once
#endif

#ifndef _I_APP_INFO_HPP
#define _I_APP_INFO_HPP

typedef void *INSTANCE_HANDLE;
typedef void *WINDOW_HANDLE;
typedef void *FILE_HANDLE;

//! class of callback functions
class ApplicationInfoFunctions
{
public:
	ApplicationInfoFunctions() {}
	virtual ~ApplicationInfoFunctions() {}

	//! callback function to return version as number
	virtual int GetVersionNumber() {return 0;}
	//! callback function to return version as text
	/**
	Note: only string constant or static string may be used here,
	no memory allocation is allowed
	*/
	virtual const char *GetVersionText() {return "0.00";}
  //! callback function to return build as number
  virtual int GetBuildNumber() {return 0;}

	//! callback function to return handle to instance
	virtual INSTANCE_HANDLE GetAppHInstance() {return NULL;}
	//! callback function to return handle to main application window
	virtual WINDOW_HANDLE GetAppHWnd() {return NULL;}

	//! callback function to write constant header
	virtual bool ConstantHeader(FILE_HANDLE file) {return true;}
	//! callback function to write variant header
	virtual void VariableHeader(FILE_HANDLE file) {}
	//! callback function to flush all pending logs
	virtual void FlushLogs(FILE_HANDLE file) {}
	//! callback function to terminate application
	virtual void DDTerm() {}
};

extern ApplicationInfoFunctions *CurrentAppInfoFunctions;

#endif
