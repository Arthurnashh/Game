#include <Es/essencepch.hpp>
#include <Es/Common/win.h>

#include <stdio.h>

#include "Scc.hpp"
#include <Es/Framework/appFrame.hpp>
#include <Es/Strings/bstring.hpp>


/*!
    Version Flag.  Major is HIWORD, Minor is LOWORD
*/
#define SCC_MAJOR_VER_VAL   1
#define SCC_MINOR_VER_VAL   2
#define SCC_VER_NUM         MAKELONG(SCC_MINOR_VER_VAL, SCC_MAJOR_VER_VAL)
#define SCC_GET_MAJOR_VER(ver)  HIWORD(ver)
#define SCC_GET_MINOR_VER(ver)  LOWORD(ver)


/*!
    Following strings are the keys for accessing the registry to find
    the SCC provider.
*/
#if !defined( _SCC_REG_KEYS )
#define _SCC_REG_KEYS
#define STR_SCC_PROVIDER_REG_LOCATION   "Software\\SourceCodeControlProvider"
#define STR_PROVIDERREGKEY              "ProviderRegKey"
#define STR_SCCPROVIDERPATH             "SCCServerPath"
#define STR_SCCPROVIDERNAME             "SCCServerName"
#define STR_SCC_INI_SECTION             "Source Code Control"
#define STR_SCC_INI_KEY                 "SourceCodeControlProvider"
#define SCC_PROJECTNAME_KEY             "SCC_Project_Name"
#define SCC_PROJECTAUX_KEY              "SCC_Aux_Path"
#define SCC_STATUS_FILE                 "MSSCCPRJ.SCC"
#define SCC_KEY                         "SCC"
#define SCC_FILE_SIGNATURE              "This is a source code control file"
#endif /* _SCC_REG_KEYS */


#ifndef _SCC_DIRSTATUS_DEFINED
#define _SCC_DIRSTATUS_DEFINED
/*!
    The SCC_DIRSTATUS_xxx macros define the state of a directory
*/
enum  SccDirStatus 
{
    SCC_DIRSTATUS_INVALID       = -1L,		// Status could not be obtained, don't rely on it
    SCC_DIRSTATUS_NOTCONTROLLED = 0x0000L,	// Directory is not under source control
											//   i.e. there is no project corresponding to the directory
    SCC_DIRSTATUS_CONTROLLED    = 0x0001L,	// Directory is under source code control
											//   i.e. there exists a project corresponding to the directory
    SCC_DIRSTATUS_EMPTYPROJ     = 0x0002L	// Project corresponding to directory is empty
};
#endif // _SCC_DIRSTATUS_DEFINED 


/*!
    SccOpenProject flags
*/
#define SCC_OP_CREATEIFNEW      0x00000001L
#define SCC_OP_SILENTOPEN       0x00000002L


/*!
    Keep checked out
*/
#define SCC_KEEP_CHECKEDOUT     0x1000


/*!
    Add flags
*/
#define SCC_FILETYPE_AUTO       0x00
#define SCC_FILETYPE_TEXT       0x01
#define SCC_FILETYPE_BINARY     0x02
#define SCC_ADD_STORELATEST     0x04


/*!
  Diff flags.  The SCC_DIFF_QD_xxx flags are mutually exclusive.  If any
	one of the three are specified, then no visual feed back is to be given.
	If one is specified but not supported, then the next best one is chosen.
*/
#define SCC_DIFF_IGNORECASE     0x0002
#define SCC_DIFF_IGNORESPACE    0x0004
#define SCC_DIFF_QD_CONTENTS    0x0010
#define SCC_DIFF_QD_CHECKSUM	  0x0020
#define SCC_DIFF_QD_TIME        0x0040
#define SCC_DIFF_QUICK_DIFF     0x0070		/* Any QD means no display     */


/*!
    Get flags
*/
#define SCC_GET_ALL             0x00000001L
#define SCC_GET_RECURSIVE       0x00000002L


/*!
    PopulateList flags
*/
#define SCC_PL_DIR				0x00000001L


/*!
    Options for SccGetCommandOptions and SccPopulateList
*/
typedef LPVOID LPCMDOPTS;
enum  SCCCOMMAND 
{
	SCC_COMMAND_GET,
	SCC_COMMAND_CHECKOUT,
	SCC_COMMAND_CHECKIN,
	SCC_COMMAND_UNCHECKOUT,
 	SCC_COMMAND_ADD,
	SCC_COMMAND_REMOVE,
	SCC_COMMAND_DIFF,
	SCC_COMMAND_HISTORY,
	SCC_COMMAND_RENAME,
	SCC_COMMAND_PROPERTIES,
	SCC_COMMAND_OPTIONS
};

typedef BOOL (*POPLISTFUNC) (LPVOID pvCallerData, BOOL bAddKeep, LONG nStatus, LPCSTR lpFile);
 
/*!
    The SCC_CAP_xxx flags are used to determine what capabilites a provider
    has.
*/
#define SCC_CAP_REMOVE            0x00000001L   // Supports the SCC_Remove command
#define SCC_CAP_RENAME            0x00000002L   // Supports the SCC_Rename command
#define SCC_CAP_DIFF              0x00000004L   // Supports the SCC_Diff command
#define SCC_CAP_HISTORY           0x00000008L   // Supports the SCC_History command
#define SCC_CAP_PROPERTIES        0x00000010L   // Supports the SCC_Properties command
#define SCC_CAP_RUNSCC            0x00000020L   // Supports the SCC_RunScc command
#define SCC_CAP_GETCOMMANDOPTIONS 0x00000040L   // Supports the SCC_GetCommandOptions command
#define SCC_CAP_QUERYINFO         0x00000080L   // Supports the SCC_QueryInfo command
#define SCC_CAP_GETEVENTS         0x00000100L   // Supports the SCC_GetEvents command
#define SCC_CAP_GETPROJPATH       0x00000200L   // Supports the SCC_GetProjPath command
#define SCC_CAP_ADDFROMSCC        0x00000400L   // Supports the SCC_AddFromScc command
#define SCC_CAP_commentCHECKOUT   0x00000800L   // Supports a comment on Checkout
#define SCC_CAP_commentCHECKIN    0x00001000L   // Supports a comment on Checkin
#define SCC_CAP_commentADD        0x00002000L   // Supports a comment on Add
#define SCC_CAP_commentREMOVE     0x00004000L   // Supports a comment on Remove
#define SCC_CAP_TEXTOUT           0x00008000L   // Writes text to an IDE-provided output function
#define SCC_CAP_CREATESUBPROJECT  0x00010000L   // Supports the SccCreateSubProject command
#define SCC_CAP_GETPARENTPROJECT  0x00020000L   // Supports the SccGetParentProjectPath command
#define SCC_CAP_BATCH             0x00040000L   // Supports the SccBeginBatch and SccEndBatch commands
#define SCC_CAP_DIRECTORYSTATUS   0x00080000L   // Supports the querying of directory status
#define SCC_CAP_DIRECTORYDIFF     0x00100000L   // Supports differencing on directories
#define SCC_CAP_ADD_STORELATEST   0x00200000L   // Supports storing files without deltas
#define SCC_CAP_HISTORY_MULTFILE  0x00400000L   // Multiple file history is supported
#define SCC_CAP_IGNORECASE        0x00800000L   // Supports case insensitive file comparison
#define SCC_CAP_IGNORESPACE       0x01000000L   // Supports file comparison that ignores white space
#define SCC_CAP_POPULATELIST      0x02000000L   // Supports finding extra files
#define SCC_CAP_commentPROJECT    0x04000000L   // Supports comments on create project
#define SCC_CAP_MULTICHECKOUT     0x08000000L   // Supports multiple checkouts on a file
												//   (subject to administrator override)
#define SCC_CAP_DIFFALWAYS        0x10000000L   // Supports diff in all states if under control
#define SCC_CAP_GET_NOUI          0x20000000L	// Provider doesn't support a UI for SccGet,
												//   but IDE may still call SccGet function.
#define SCC_CAP_REENTRANT		      0x40000000L	// Provider is reentrant and thread safe.
#define SCC_CAP_SCCFILE           0x80000000L   // Supports the MSSCCPRJ.SCC file
												//   (subject to user/administrator override)

/*!
	The following flags are used for the print call-back that the IDE
	provides on SccInitialize.  
	
	If the IDE supports cancel, it may get one of the Cancel messages.
	In this case, the provider will inform the IDE to show the Cancel
	button with SCC_MSG_STARTCANCEL.  After this, any set of normal
	messages may be sent.  If any of these return SCC_MSG_RTN_CANCEL,
	then the provider will quit the operation and return.  The Provider
	will also poll periodically with SCC_MSG_DOCANCEL to see if the
	user has canceled the operation.  When all operations are done, or
	the user has canceled, SCC_MSG_STOPCANCEL will be sent through.

	The SCC_MSG_INFO, WARNING, and ERROR types are used for messages that
	get displayed in the scrolling list of messages.  SCC_MSG_STATUS is
	a special type that indicates that the text should show up in a 
	status bar or temporary display area.  This message type should not
	remain permanently in the list.
*/
enum
{
	// Return codes
	SCC_MSG_RTN_CANCEL=-1,		// Returned from call-back to indicate cancel
	SCC_MSG_RTN_OK=0,					// Returned from call-back to continue
	// Message types
	SCC_MSG_INFO=1,						// Message is informational
	SCC_MSG_WARNING,					// Message is a warning
	SCC_MSG_ERROR,						// Message is an error
	SCC_MSG_STATUS,						// Message is meant for status bar
	// IDE supports Cancel operation
	SCC_MSG_DOCANCEL,					// No text, IDE returns 0 or SCC_MSG_RTN_CANCEL
	SCC_MSG_STARTCANCEL,			// Start a cancel loop
	SCC_MSG_STOPCANCEL				// Stop the cancel loop
};

#ifndef _LPTEXTOUTPROC_DEFINED
#define _LPTEXTOUTPROC_DEFINED
typedef LONG (*LPTEXTOUTPROC) (LPCSTR, DWORD);
#endif /* _LPTEXTOUTPROC_DEFINED */


/*!
    nOption values for SccSetOption.
*/
#define SCC_OPT_EVENTQUEUE      0x00000001L     // Set status of the event queue
#define SCC_OPT_USERDATA        0x00000002L     // Specify user data for 
												//	SCC_OPT_NAMECHANGEPFN
#define SCC_OPT_HASCANCELMODE	0x00000003L		// The IDE can handle Cancel 
												//	of long running operations
#define SCC_OPT_NAMECHANGEPFN   0x00000004L     // Set a callback for name changes
#define SCC_OPT_SCCCHECKOUTONLY 0x00000005L		// Disable SS explorer checkout, 
												//  and don't set working dir
#define SCC_OPT_SHARESUBPROJ    0x00000006L		// if this is turned on, allow
												// AddFromScc to specify a working
												//  dir, try to share into the assoc
												//  project if direct descendant.


/* SCC_OPT_EVENTQUEUE values */
#define SCC_OPT_EQ_DISABLE      0x00L           // Suspend event queue activity
#define SCC_OPT_EQ_ENABLE       0x01L           // Enable event queue logging

/* SCC_OPT_NAMECHANGEPFN callback typedef */
/*
typedef void (*OPTNAMECHANGEPFN)(LONG pvCallerData, 
                    LPCSTR pszOldName, LPCSTR pszNewName);
*/
/*!
	Values for SCC_OPT_HASCANCELMODE.  By default, it is assumed that the IDE
	will not allow for canceling a long running operation.  The provider must
	handle this on their own in this case.  If the IDE, however, sets this
	option to SCC_OPT_HCM_YES, it means that it will handle canceling the
	operation.  In this case, use the SCC_MSG_xxx flags with the output
	call-back to tell the IDE what messages to display while the operation
	is running.
*/
#define SCC_OPT_HCM_NO			0L				// (Default) Has no cancel mode,
              												//	Provider must supply if desired
#define SCC_OPT_HCM_YES			1L				// IDE handles cancel

/*!
	Values for SCC_OPT_SCCCHECKOUTONLY.  By default, it is assumed that 
	the user may use the gui to get and checkout files from this project,
	and that a working dir should be set,  If this option is explicitly turned on,
	then no working dir is set for the project, and the files may only be gotten
	or checked in or out from scc integration, never from the gui.
*/
#define SCC_OPT_SCO_NO			0L				// (Default) OK to checkout from GUI
							              					//	Working dir is set.
#define SCC_OPT_SCO_YES			1L				// no GUI checkout, no working dir



/*!
	The SCC_PROC_NAMES macro will expand for each function in the API.
	You must define SCC_PROC to do what you want.  For this file, it
	creates an enum (see code just after the table), by concatenating
	SCC_CMD_ with the function name.  This enum is used in the sccuser.cpp
	module as an index for function pointers of the provider.  sccuser.cpp
	also redefines SCC_PROC to expand a table of character string function
	names.  This table, when used with the enum, allows for adding and
	removing of function names from the API will only a recompile required
	to fix up addresses and offsets.  You can #undef SCC_PROC and redefine
	it to do other tables of your own if required.
*/
#define SCC_PROC(nam)   SCC_CMD_ ## nam

#define SCC_PROC_NAMES \
    SCC_PROC(SccInitialize), \
    SCC_PROC(SccUninitialize), \
    SCC_PROC(SccOpenProject), \
    SCC_PROC(SccCloseProject), \
    SCC_PROC(SccGet), \
    SCC_PROC(SccCheckout), \
    SCC_PROC(SccUncheckout), \
    SCC_PROC(SccCheckin), \
    SCC_PROC(SccAdd), \
    SCC_PROC(SccRemove), \
    SCC_PROC(SccRename), \
    SCC_PROC(SccDiff), \
    SCC_PROC(SccDirDiff), \
    SCC_PROC(SccHistory), \
    SCC_PROC(SccProperties), \
    SCC_PROC(SccRunScc), \
    SCC_PROC(SccGetCommandOptions), \
    SCC_PROC(SccQueryInfo), \
    SCC_PROC(SccDirQueryInfo), \
    SCC_PROC(SccGetEvents), \
    SCC_PROC(SccGetProjPath), \
    SCC_PROC(SccPopulateList), \
    SCC_PROC(SccAddFromScc), \
    SCC_PROC(SccSetOption), \
    SCC_PROC(SccGetVersion), \
    SCC_PROC(SccCreateSubProject), \
    SCC_PROC(SccGetParentProjectPath), \
    SCC_PROC(SccBeginBatch), \
    SCC_PROC(SccEndBatch), \
    SCC_PROC(SccIsMultiCheckoutEnabled), \
    SCC_PROC(SccWillCreateSccFile)

enum 
{
    SCC_PROC_NAMES
};
#define SCC_CMD_COUNT           (SCC_CMD_SccWillCreateSccFile + 1)


/*!
    Scc functions from dll
*/
typedef SCCRTN (*TSccInitialize)(LPVOID*, HWND, LPCSTR, LPSTR, LPLONG, LPSTR, LPLONG, LPLONG);
TSccInitialize SccInitialize;

typedef SCCRTN (*TSccUninitialize)(LPVOID);
TSccUninitialize SccUninitialize;

typedef SCCRTN (*TSccOpenProject)(LPVOID, HWND, LPSTR, LPCSTR, LPCSTR, LPSTR, LPCSTR, LPTEXTOUTPROC, LONG);
TSccOpenProject SccOpenProject;

typedef SCCRTN (*TSccCloseProject)(LPVOID);
TSccCloseProject SccCloseProject;

typedef SCCRTN (*TSccGetProjPath)(LPVOID, HWND, LPSTR, LPSTR, LPSTR, LPSTR, BOOL, LPBOOL);
TSccGetProjPath SccGetProjPath;

typedef SCCRTN (*TSccGet)(LPVOID, HWND, LONG, LPCSTR*, LONG, LPCMDOPTS);
TSccGet SccGet;

typedef SCCRTN (*TSccCheckout)(LPVOID, HWND, LONG, LPCSTR*,LPCSTR, LONG, LPCMDOPTS);
TSccCheckout SccCheckout;

typedef SCCRTN (*TSccCheckin)(LPVOID, HWND, LONG,	LPCSTR*, LPCSTR, LONG, LPCMDOPTS);
TSccCheckin SccCheckin;

typedef SCCRTN (*TSccUncheckout)(LPVOID, HWND, LONG,	LPCSTR*, LONG, LPCMDOPTS);
TSccUncheckout SccUncheckout;

typedef SCCRTN (*TSccAdd)(LPVOID,	HWND, LONG,	LPCSTR*, LPCSTR, LONG*,	LPCMDOPTS);
TSccAdd SccAdd;

typedef SCCRTN (*TSccRemove)(LPVOID, HWND, LONG, LPCSTR*,	LPCSTR, LONG,	LPCMDOPTS);
TSccRemove SccRemove;

typedef SCCRTN (*TSccDiff)(LPVOID, HWND, LPCSTR, LONG, LPCMDOPTS);
TSccDiff SccDiff;

typedef SCCRTN (*TSccHistory)(LPVOID,	HWND,	LONG,	LPCSTR*, LONG, LPCMDOPTS);
TSccHistory SccHistory;

typedef SCCRTN (*TSccProperties)(LPVOID, HWND, LPCSTR);
TSccProperties SccProperties;

typedef SCCRTN (*TSccQueryInfo)(LPVOID, LONG, LPCSTR*, LPLONG);
TSccQueryInfo SccQueryInfo;


/*!
    Scc private functions
*/
SCCRTN MsSccFunctions::Initialize()
{
  char caller_name[256];

  strcpy(caller_name, "");
  strcpy(_scc_name, "");
  strcpy(_aux_path_label, "");

  return SccInitialize((LPVOID*)&_context, _hWnd, (LPCSTR)caller_name, (LPSTR)_scc_name,
           &_scc_caps, (LPSTR)_aux_path_label, &_checkout_comment_len, &_comment_len);
}


BOOL MsSccFunctions::SetSccDllPath()
{
  HKEY key;
  char scc_provider[1024];
  //find provider
  if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\SourceCodeControlProvider", 0, KEY_QUERY_VALUE, &key)
       == ERROR_SUCCESS )
  {
    int size = 256;

    LONG ret = RegQueryValueEx(key, "ProviderRegKey", NULL, NULL, (LPBYTE)scc_provider, (LPDWORD)&size);
    if ( ret == ERROR_SUCCESS )
      RegCloseKey(key);
    else        
      return FALSE;    
  }
  else
    return FALSE;

  //find Scc dll name
  if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE, scc_provider, 0, KEY_QUERY_VALUE, &key)
       == ERROR_SUCCESS )
  {
    int size = PATH_LEN;
    if ( RegQueryValueEx(key, "SCCServerPath", NULL, NULL, (LPBYTE)_scc_dll_name, (LPDWORD)&size)
         == ERROR_SUCCESS )
    {
      RegCloseKey(key);
      return TRUE;
    }
  }
  return FALSE;
}


//modify '_proj_name' if necessary
SCCRTN MsSccFunctions::ModifyProjName()
{
  BString<SCC_PRJPATH_LEN+1> name;

  if ( strlen(_proj_name) < 2 )
  {
    return SCC_E_PROJSYNTAXERR;
  }
  
  strcpy(name, _proj_name);
  if ( _proj_name[0] != '\"' )
    if ( sprintf(name, "\"%s", _proj_name) == -1 )
      return SCC_E_LONGPATH;
  strcpy(_proj_name, name);
  if ( _proj_name[strlen(_proj_name)-1] != '\"' )
    if ( sprintf(name, "%s\"", _proj_name) == -1 )
      return SCC_E_LONGPATH;
  strcpy(_proj_name, name);
  if (  _proj_name[1] == '\\'  ||  _proj_name[1] == '/'  )
    if ( sprintf(name, "\"%s", (_proj_name+2)) == -1 )
      return SCC_E_LONGPATH;
  strcpy(_proj_name, name);
  if (  _proj_name[1] != '$'  &&  _proj_name[2] != '/'  )
    if ( sprintf(name, "\"$/%s", (_proj_name+1)) == -1 )
      return SCC_E_LONGPATH;
  strcpy(_proj_name, name);

  return SCC_OK;
}


SCCRTN MsSccFunctions::LoadScc(HWND hWnd)
{
  //load dll library
  if ( !SetSccDllPath() )
    return SCC_E_NODLLFOUND;
	_scc_dll= LoadLibrary(_scc_dll_name);
	if ( !_scc_dll )
	{
    MessageBox(NULL, "The dynamic library can not load.", "Error", MB_ICONERROR);
		return SCC_E_CANNOTLOADDLL;
	}

  //get scc_dll functions
  SccInitialize  = (TSccInitialize)  GetProcAddress(_scc_dll, "SccInitialize");
  SccOpenProject = (TSccOpenProject) GetProcAddress(_scc_dll, "SccOpenProject");
  SccUninitialize= (TSccUninitialize)GetProcAddress(_scc_dll, "SccUninitialize");
  SccCloseProject= (TSccCloseProject)GetProcAddress(_scc_dll, "SccCloseProject");
  SccGetProjPath = (TSccGetProjPath) GetProcAddress(_scc_dll, "SccGetProjPath");
	SccGet         = (TSccGet)         GetProcAddress(_scc_dll, "SccGet");
  SccCheckout    = (TSccCheckout)    GetProcAddress(_scc_dll, "SccCheckout");
  SccCheckin     = (TSccCheckin)     GetProcAddress(_scc_dll, "SccCheckin");
  SccUncheckout  = (TSccUncheckout)  GetProcAddress(_scc_dll, "SccUncheckout");
  SccAdd         = (TSccAdd)         GetProcAddress(_scc_dll, "SccAdd");
  SccRemove      = (TSccRemove)      GetProcAddress(_scc_dll, "SccRemove");
  SccDiff        = (TSccDiff)        GetProcAddress(_scc_dll, "SccDiff");
  SccHistory     = (TSccHistory)     GetProcAddress(_scc_dll, "SccHistory");
  SccProperties  = (TSccProperties)  GetProcAddress(_scc_dll, "SccProperties");
  SccQueryInfo   = (TSccQueryInfo)   GetProcAddress(_scc_dll, "SccQueryInfo");

  if ( !SccInitialize   ||
       !SccUninitialize ||
       !SccOpenProject  || 
       !SccCloseProject ||
       !SccGetProjPath  ||
       !SccGet          ||
       !SccCheckout     ||
       !SccCheckin      ||
       !SccUncheckout   ||
       !SccAdd          ||
       !SccRemove       ||
       !SccDiff         ||
       !SccHistory      ||
       !SccProperties   ||
       !SccQueryInfo
     )
  {
    MessageBox(NULL, "Some functions can not load from dll library.", "Error", MB_ICONERROR);
    FreeLibrary(_scc_dll);
    return SCC_E_CANNOTLOADFROMDLL;
  }

  //initialize
  _hWnd= hWnd;
  return Initialize();
}


SCCRTN MsSccFunctions::FreeScc()
{
  if ( _scc_dll )
  {
    SCCRTN ret= SccUninitialize(_context);
    if ( FreeLibrary(_scc_dll) )
      return ret;
    else
      return SCC_E_CANNOTFREEDLL;
  }
  return SCC_OK;
}


SCCRTN MsSccFunctions::OpenProject(const char* user_name, const char* proj_name,
                                   const char* local_path, const char* server_name)
{
  BOOL ask_to_path= FALSE;
  BOOL allow_change_path= FALSE;
  _new_proj= FALSE;

  //user_name
  if ( user_name )
  {
    if ( strlen(user_name) > SCC_NAME_LEN )
      return SCC_E_LONGUSERNAME;
    strcpy(_user_name, user_name);                               //toto se objevi v prihlasovacim okne
  }
  else
    strcpy(_user_name, "");
  //proj_name
  if ( proj_name )
  {
    if ( strlen(proj_name) > PATH_LEN )
      return SCC_E_LONGPATH;
    strcpy(_proj_name, proj_name);                               //vhodne nastavit
    SCCRTN ret= ModifyProjName();
      if ( !IS_SCC_SUCCESS(ret) )
        return ret;
  }
  else
  {
    strcpy(_proj_name, "");                                     //vhodne nastavit
    ask_to_path= TRUE;
  }
  //local_path
  if ( local_path )
  {
    if ( strlen(local_path) > PATH_LEN )
      return SCC_E_LONGPATH;
    strcpy(_local_path, local_path);                            //nutne nastavit
  }
  else
  {
    strcpy(_local_path, "");                                   //nutne nastavit    
    allow_change_path= TRUE;
    ask_to_path= TRUE;
  }
  //server_name
  if ( server_name )
  {
    if ( strlen(server_name) > PATH_LEN )
      return SCC_E_LONGPATH;
    strcpy(_aux_proj_path, server_name );
  }
  else
  {
    strcpy(_aux_proj_path, "");
    ask_to_path= TRUE;
  }

  //ask to the paths if necessary
  SCCRTN ret= SCC_OK;
  if ( ask_to_path )
  {
    strcpy(_aux_proj_path, "");                                //nastavi SccGetProjPath
    ret= SccGetProjPath(_context, _hWnd, _user_name, _proj_name,
           _local_path, _aux_proj_path, allow_change_path, &_new_proj);
  }
  else
    strcpy(_aux_proj_path, server_name);

  //open project
  char comment[256];
  LPTEXTOUTPROC text_out_proc= NULL;
  LONG flags= SCC_OP_SILENTOPEN;
  strcpy(comment, "");
  if ( IS_SCC_SUCCESS(ret) )
    ret= SccOpenProject(_context, _hWnd, _user_name, _proj_name,
                        _local_path, _aux_proj_path, comment, text_out_proc, flags);
  if ( IS_SCC_SUCCESS(ret) )
    _opened= TRUE;
  
  return ret;
}


SCCRTN MsSccFunctions::CloseProject()
{
  return SccCloseProject(_context);
}


/*!
    Scc public functions
*/

MsSccFunctions::MsSccFunctions()
{
  _scc_dll= NULL;
  _hWnd= NULL;
  _context= NULL;
  _opened= FALSE;
}


MsSccFunctions::~MsSccFunctions()
{
  Done();
}


SCCRTN MsSccFunctions::Init(const char* server_name, const char* proj_name, const char* local_path, HWND hWnd, const char *user_name)
{
  if ( _opened )
    Done();

  SCCRTN ret= LoadScc(hWnd);
  if ( IS_SCC_SUCCESS(ret) )
  {
    ret= OpenProject(user_name, proj_name, local_path, server_name);
    if ( IS_SCC_SUCCESS(ret) )
      _opened= TRUE;
  }

  return ret;
}


SCCRTN MsSccFunctions::Done()
{
  SCCRTN ret= SCC_OK;
  if ( _opened )
  {
    ret= CloseProject();
    ret= FreeScc();
  }
  if ( IS_SCC_SUCCESS(ret) )
    _opened= FALSE;
  return ret;
}


bool MsSccFunctions::Opened() const
{
  return _opened!=FALSE;
}


SCCRTN MsSccFunctions::ChooseProject()
{
  if ( Opened() )
    Done();

  return Init();
}


SCCRTN MsSccFunctions::GetLatestVersion(const char* local_file_name)
{
  if ( Opened() )
	  return SccGet(_context, _hWnd, 1, (LPCSTR*)&local_file_name, 0, NULL);
  else
    return SCC_E_DDLISNOTOPENED;
}


SCCRTN MsSccFunctions::GetLatestVersion(const char** local_file_names, int n_file)
{
  if ( Opened() )
  	return SccGet(_context, _hWnd, n_file, (LPCSTR*)local_file_names, 0, NULL);
  else
    return SCC_E_DDLISNOTOPENED;
}


SCCRTN MsSccFunctions::GetLatestVersionDir(const char* local_file_name, BOOL recursive)
{
  if ( Opened() )
  {
    if ( recursive )
	    return SccGet(_context, _hWnd, 1, (LPCSTR*)&local_file_name, SCC_GET_ALL|SCC_GET_RECURSIVE, NULL);
    else
	    return SccGet(_context, _hWnd, 1, (LPCSTR*)&local_file_name, SCC_GET_ALL, NULL);
  }
  else
    return SCC_E_DDLISNOTOPENED;
}


SCCRTN MsSccFunctions::CheckOut(const char* local_file_name, const char *comment)
{
  if ( Opened() )
	  return SccCheckout(_context, _hWnd, 1, (LPCSTR*)&local_file_name, comment?comment:"", SCC_OPT_SCO_NO, NULL);
  else
    return SCC_E_DDLISNOTOPENED;
}


SCCRTN MsSccFunctions::CheckOut(const char** local_file_names, int n_file,const char *comment)
{
  if ( Opened() )
    return SccCheckout(_context, _hWnd, n_file, (LPCSTR*)local_file_names, comment?comment:"", SCC_OPT_SCO_NO, NULL);
	  //parametr SCC_OPT_SCO_YES nefunguje, vraci SCC_E_NONSPECIFICERROR
      //Dodatek Bredy: Zrejme jeden chybejici parametr tuto chybu zpusobuje
  else
    return SCC_E_DDLISNOTOPENED;
}


SCCRTN MsSccFunctions::CheckIn(const char* local_file_name, const char* comment)
{
  if ( Opened() )
  {
    if ( comment )
      return SccCheckin(_context, _hWnd, 1, (LPCSTR*)&local_file_name, comment, SCC_OPT_SCO_NO, NULL);
    else
      return SccCheckin(_context, _hWnd, 1, (LPCSTR*)&local_file_name, "", SCC_OPT_SCO_NO, NULL);
  }
  else
    return SCC_E_DDLISNOTOPENED;
}


SCCRTN MsSccFunctions::CheckIn(const char** local_file_names, int n_file, const char* comment)
{
  if ( Opened() )
  {
    if ( comment )
      return SccCheckin(_context, _hWnd, n_file, (LPCSTR*)local_file_names, comment, SCC_OPT_SCO_NO, NULL);
    else
      return SccCheckin(_context, _hWnd, n_file, (LPCSTR*)local_file_names, "", SCC_OPT_SCO_NO, NULL);
  }
  else
    return SCC_E_DDLISNOTOPENED;
}


SCCRTN MsSccFunctions::UndoCheckOut(const char* local_file_name)
{
  if ( Opened() )
    return SccUncheckout(_context, _hWnd, 1, (LPCSTR*)&local_file_name, SCC_OPT_SCO_NO, NULL);
  else
    return SCC_E_DDLISNOTOPENED;
}


SCCRTN MsSccFunctions::UndoCheckOut(const char** local_file_names, int n_file)
{
  if ( Opened() )
    return SccUncheckout(_context, _hWnd, n_file, (LPCSTR*)local_file_names, SCC_OPT_SCO_NO, NULL);
  else
    return SCC_E_DDLISNOTOPENED;
}


SCCRTN MsSccFunctions::Add(const char* local_file_name, const char* comment)
{
  if ( Opened() )
  {
    LONG pfOptions= SCC_FILETYPE_AUTO;
    if ( comment )
      return SccAdd(_context, _hWnd, 1, (LPCSTR*)&local_file_name, comment, &pfOptions, NULL);
    else
      return SccAdd(_context, _hWnd, 1, (LPCSTR*)&local_file_name, "", &pfOptions, NULL);
  }
  else
    return SCC_E_DDLISNOTOPENED;
}


SCCRTN MsSccFunctions::Add(const char** local_file_names, int n_file, const char* comment)
{
  if ( Opened() )
  {
    LONG pfOptions= SCC_FILETYPE_AUTO;
    if ( comment )
      return SccAdd(_context, _hWnd, n_file, (LPCSTR*)local_file_names, comment, &pfOptions, NULL);
    else
      return SccAdd(_context, _hWnd, n_file, (LPCSTR*)local_file_names, "", &pfOptions, NULL);
  }
  else
    return SCC_E_DDLISNOTOPENED;
}


SCCRTN MsSccFunctions::Remove(const char* local_file_name, const char* comment)
{
  if ( Opened() )
  {
    if ( comment )
      return SccRemove(_context, _hWnd, 1, (LPCSTR*)&local_file_name, comment, SCC_OPT_SCO_NO, NULL);
    else
      return SccRemove(_context, _hWnd, 1, (LPCSTR*)&local_file_name, "", SCC_OPT_SCO_NO, NULL);
  }
  else
    return SCC_E_DDLISNOTOPENED;
}


SCCRTN MsSccFunctions::Remove(const char** local_file_names, int n_file, const char* comment)
{
  if ( Opened() )
  {
    if ( comment )
      return SccRemove(_context, _hWnd, n_file, (LPCSTR*)local_file_names, comment, SCC_OPT_SCO_NO, NULL);
    else
      return SccRemove(_context, _hWnd, n_file, (LPCSTR*)local_file_names, "", SCC_OPT_SCO_NO, NULL);
  }
  else
    return SCC_E_DDLISNOTOPENED;
}


SCCRTN MsSccFunctions::Diff(const char* local_file_name)
{
  if ( Opened() )
    return SccDiff(_context, _hWnd, local_file_name, 0, NULL);
  else
    return SCC_E_DDLISNOTOPENED;
}


SCCRTN MsSccFunctions::History(const char* local_file_name)
{
  if ( Opened() )
    return SccHistory(_context, _hWnd, 1, (LPCSTR*)&local_file_name, 0, NULL);
  else
    return SCC_E_DDLISNOTOPENED;
}


SCCRTN MsSccFunctions::History(const char** local_file_names, int n_file)
{
  if ( Opened() )
    return SccHistory(_context, _hWnd, n_file, (LPCSTR*)local_file_names, 0, NULL);
  else
    return SCC_E_DDLISNOTOPENED;
}


SCCRTN MsSccFunctions::Properties(const char* local_file_name)
{
  if ( Opened() )
    return SccProperties(_context, _hWnd, local_file_name);
  else
    return SCC_E_DDLISNOTOPENED;
}



SCCSTAT MsSccFunctions::Status(const char* local_file_name)
{
  if ( Opened() )
  {
    SCCSTAT status;
    SccQueryInfo(_context, 1, (LPCSTR*)&local_file_name, &status);
    return status;
  }
  else
    return SCC_STATUS_INVALID;
}


bool MsSccFunctions::UnderSSControl(const char* local_file_name)
{
  SCCSTAT status = Status(local_file_name);
  return (status != SCC_STATUS_INVALID && (status & SCC_STATUS_CONTROLLED));
}


bool MsSccFunctions::CheckedOut(const char* local_file_name)
{
  SCCSTAT status = Status(local_file_name);
  return (status != SCC_STATUS_INVALID && (status & SCC_STATUS_OUTBYUSER));
}


bool MsSccFunctions::Deleted(const char* local_file_name)
{
  SCCSTAT status = Status(local_file_name);
  return status != SCC_STATUS_INVALID && (Status(local_file_name) & SCC_STATUS_DELETED);
}


const char* MsSccFunctions::GetUserName() const 
{
  return _user_name;
}


const char* MsSccFunctions::GetProjName() const
{
  return _proj_name;
}


const char* MsSccFunctions::GetLocalPath() const
{
  return _local_path;
}


const char* MsSccFunctions::GetServerName() const
{
  return _aux_proj_path;
}
