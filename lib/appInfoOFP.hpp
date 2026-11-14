#ifdef _MSC_VER
#pragma once
#endif

#ifndef _APP_INFO_OFP_HPP
#define _APP_INFO_OFP_HPP

#ifdef _WIN32
#include "Es/Common/win.h"
#endif
#include <El/Interfaces/iAppInfo.hpp>
#include "versionNo.h"

extern void PrintMissionInfo(HANDLE file);
extern bool PrintConfigInfo(HANDLE file);
extern void PrintNetworkInfo(HANDLE file);
extern void DDTerm();

extern INSTANCE_HANDLE	hInstApp;
extern WINDOW_HANDLE		hwndApp;

//! class of callback functions
class OFPApplicationInfoFunctions : public ApplicationInfoFunctions
{
public:
	//! callback function to return version as number
	virtual int GetVersionNumber() {return APP_VERSION_NUM;}
	//! callback function to return version as text
	virtual const char *GetVersionText() {return APP_VERSION_TEXT;}
  //! callback function to return build as number
  virtual int GetBuildNumber() {return BUILD_NO;}

	//! callback function to return handle to instance
	virtual INSTANCE_HANDLE GetAppHInstance() {return hInstApp;}
	//! callback function to return handle to main application window
	virtual WINDOW_HANDLE GetAppHWnd() {return hwndApp;}

	//! callback function to write constant header
	virtual bool ConstantHeader(HANDLE file) {return PrintConfigInfo(file);}
	//! callback function to write variant header
	virtual void VariableHeader(HANDLE file) {PrintMissionInfo(file);}
	//! callback function to flush all pending logs
	virtual void FlushLogs(HANDLE file) {PrintNetworkInfo(file);}
	//! callback function to terminate application
	virtual void DDTerm() {::DDTerm();}
};

#endif
