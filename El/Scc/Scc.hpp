#ifdef _MSC_VER
#pragma once
#endif

#ifndef _scc_h_
#define _scc_h_


#include <windows.h>
#include <El/Interfaces/iScc.hpp>

#define SCC_NAME_LEN            31      // lpSccName, SCCInitialize
#define SCC_AUXLABEL_LEN        31      // lpAuxPathLabel, SCCInitialize
#define SCC_USER_LEN            31      // lpUser, SCCOpenProject
#define SCC_PRJPATH_LEN         300     // lpAuxProjPath, SCCGetProjPath
#define PATH_LEN                2047

/*! 
    These are the error codes that may be returned from a function.
    All errors are < 0, warnings are > 0, and success is 0.  Use the
    macros provided for quick checking.  
*/
#define IS_SCC_ERROR(rtn) (((rtn) < 0) ? TRUE : FALSE)
#define IS_SCC_SUCCESS(rtn) (((rtn) == SCC_OK) ? TRUE : FALSE)
#define IS_SCC_WARNING(rtn) (((rtn) > 0) ? TRUE : FALSE)


#define SCC_I_SHARESUBPROJOK                    7
#define SCC_I_FILEDIFFERS                       6
#define SCC_I_RELOADFILE                        5
#define SCC_I_FILENOTAFFECTED                   4
#define SCC_I_PROJECTCREATED                    3
#define SCC_I_OPERATIONCANCELED                 2
#define SCC_I_ADV_SUPPORT                       1

#define SCC_OK                                  0

#define SCC_E_INITIALIZEFAILED                  -1
#define SCC_E_UNKNOWNPROJECT                    -2
#define SCC_E_COULDNOTCREATEPROJECT             -3
#define SCC_E_NOTCHECKEDOUT                     -4
#define SCC_E_ALREADYCHECKEDOUT                 -5
#define SCC_E_FILEISLOCKED                      -6
#define SCC_E_FILEOUTEXCLUSIVE                  -7
#define SCC_E_ACCESSFAILURE                     -8
#define SCC_E_CHECKINCONFLICT                   -9
#define SCC_E_FILEALREADYEXISTS                 -10
#define SCC_E_FILENOTCONTROLLED                 -11
#define SCC_E_FILEISCHECKEDOUT                  -12
#define SCC_E_NOSPECIFIEDVERSION                -13
#define SCC_E_OPNOTSUPPORTED                    -14
#define SCC_E_NONSPECIFICERROR                  -15
#define SCC_E_OPNOTPERFORMED                    -16
#define SCC_E_TYPENOTSUPPORTED                  -17
#define SCC_E_VERIFYMERGE                       -18
#define SCC_E_FIXMERGE                          -19
#define SCC_E_SHELLFAILURE                      -20
#define SCC_E_INVALIDUSER                       -21
#define SCC_E_PROJECTALREADYOPEN                -22
#define SCC_E_PROJSYNTAXERR                     -23
#define SCC_E_INVALIDFILEPATH                   -24
#define SCC_E_PROJNOTOPEN                       -25
#define SCC_E_NOTAUTHORIZED                     -26
#define SCC_E_FILESYNTAXERR                     -27
#define SCC_E_FILENOTEXIST                      -28
#define SCC_E_CONNECTIONFAILURE                 -29
#define SCC_E_UNKNOWNERROR                      -30
#define SCC_E_NODLLFOUND                        -31
#define SCC_E_CANNOTLOADDLL                     -32 
#define SCC_E_CANNOTLOADFROMDLL                 -33
#define SCC_E_CANNOTFREEDLL                     -34
#define SCC_E_DDLISNOTOPENED                    -35
#define SCC_E_LONGUSERNAME                      -36
#define SCC_E_LONGPATH                          -37


/*!
  These are the return values of SCCSTAT. These values can be ORed togetger.
*/
#define SCC_STATUS_INVALID          -1		// Status could not be obtained, don't rely on it
#define SCC_STATUS_NOTCONTROLLED    0	    // File is not under source control
#define SCC_STATUS_CONTROLLED       1	    // File is under source code control
#define SCC_STATUS_CHECKEDOUT       2	    // Checked out to current user at local path
#define SCC_STATUS_OUTOTHER         4	    // File is checked out to another user
#define SCC_STATUS_OUTEXCLUSIVE     8	    // File is exclusively check out
#define SCC_STATUS_OUTMULTIPLE      16	  // File is checked out to multiple people
#define SCC_STATUS_OUTOFDATE        32	  // The file is not the most recent
#define SCC_STATUS_DELETED          64	  // File has been deleted from the project
#define SCC_STATUS_LOCKED           128	  // No more versions allowed
#define SCC_STATUS_MERGED           256	  // File has been merged but not yet fixed/verified
#define SCC_STATUS_SHARED           512	  // File is shared between projects
#define SCC_STATUS_PINNED           1024	// File is shared to an explicit version
#define SCC_STATUS_MODIFIED         2048	// File has been modified/broken/violated
#define SCC_STATUS_OUTBYUSER        4096	// File is checked out by current user someplace


/*!
  The class MsSccFunctions offer you functions to work with SourceSafe:
  Get Latest Version, Check Out, Check In, Undo Check Out, Add, Remove, Differences, History.
  First call the initialization function Init().
  Example:
  --------
  MsSccFunctions SccFcs;
  SccFcs.Init("\\\\new_server\\vssdatap\\dataP", "$/", "p:");
  SccFcs.GetLatestVersion("p:\\Ofp2\\Weapons\\Rifles\\M4\\data\\m4_acog.rvmat");
  SccFcs.CheckOut("p:\\Ofp2\\Weapons\\Rifles\\M4\\data\\m4_acog.rvmat");
  SccFcs.CheckIn("p:\\Ofp2\\Weapons\\Rifles\\M4\\data\\m4_acog.rvmat");
  SccFcs.Diff("p:\\Ofp2\\Weapons\\Rifles\\M4\\data\\m4_acog.rvmat");
*/
class MsSccFunctions : public SccFunctions
{
private:
  HMODULE _scc_dll;
  char _scc_dll_name[PATH_LEN+1];
  void* _context;
  HWND _hWnd;
  //initialize
  char _scc_name[SCC_NAME_LEN+1];
  LONG _scc_caps;
  char _aux_path_label[SCC_AUXLABEL_LEN+1];
  LONG _checkout_comment_len;
  LONG _comment_len;
  //open project
  char _user_name[SCC_USER_LEN+1];
  char _proj_name[PATH_LEN+1];
  char _local_path[PATH_LEN+1];
  char _aux_proj_path[PATH_LEN+1];
  BOOL _new_proj;
  BOOL _opened;

  SCCRTN Initialize();
  BOOL SetSccDllPath();
  SCCRTN ModifyProjName();
  SCCRTN LoadScc(HWND hWnd= NULL);
  SCCRTN FreeScc(); 
  SCCRTN OpenProject(const char* user_name= NULL, const char* proj_name= NULL,
                     const char* local_path= NULL, const char* server_name= NULL);
  SCCRTN CloseProject();

public:
  //! constructor load all scc functions from dll
  MsSccFunctions();
  ~MsSccFunctions();

  /*!
    Initialization function, call as first function.
    @param server_name - the real path to the server (e.g. "\\\\new_server\\vssdatap\\dataP")
                         if server_name is NULL, SS ask you to open a database from SS
    @param proj_name   - the directory in SourceSafe (e.g. "$/")
                         if proj_name is NULL, SS ask you to enter project name
    @param local_path  - local path must correspond with proj_name (e.g. "p:")
                         if local_path is NULL, SS ask you to enter local path name         
    @param hWnd        - a handle that can be used as a parent for any dialog boxes
    @param user_name   - optional username to be set with current instance
  */
  virtual SCCRTN Init(const char* server_name= NULL, const char* proj_name= NULL, const char* local_path= NULL, HWND hWnd= NULL, const char *user_name=NULL);
  virtual SCCRTN Done();

  //! return TRUE if Scc functions are initialized (a project is opened)
  virtual bool Opened() const;

  //! open the dialog to select project
  virtual SCCRTN ChooseProject();

  //! get the last version from SS, local_file_name is local path to the file
  virtual SCCRTN GetLatestVersion(const char* local_file_name);
  /*!
    If you want to get last version of more than one file together.
    @param local_file_names - the array of the local paths
    @param n_files          - the number of files
  */
  virtual SCCRTN GetLatestVersion(const char** local_file_names, int n_file);

  //! Get latest version of dircetory. Recursive, if flag 'recursive' is TRUE
  virtual SCCRTN GetLatestVersionDir(const char* local_file_name, BOOL recursive= FALSE);

  //! Check out, local_file_name is local path to the file
  virtual SCCRTN CheckOut(const char* local_file_name, const char *comment=NULL);
  virtual SCCRTN CheckOut(const char** local_file_names, int n_file, const char *comment=NULL);

  //! Check in, local_file_name is local path to the file
  virtual SCCRTN CheckIn(const char* local_file_name, const char* comment= NULL);
  virtual SCCRTN CheckIn(const char** local_file_names, int n_file, const char* comment= NULL);

  //! Unod check out, local_file_name is local path to the file
  virtual SCCRTN UndoCheckOut(const char* local_file_name);
  virtual SCCRTN UndoCheckOut(const char** local_file_names, int n_file);

  //! add file(s) to SS
  virtual SCCRTN Add(const char* local_file_name, const char* comment= NULL);
  virtual SCCRTN Add(const char** local_file_names, int n_file, const char* comment= NULL);

  //! remove file(s) from SS
  virtual SCCRTN Remove(const char* local_file_name, const char* comment= NULL);
  virtual SCCRTN Remove(const char** local_file_names, int n_file, const char* comment= NULL);

  //! show differences between the local copy of file and the version in SS
  virtual SCCRTN Diff(const char* local_file_name);

  //! show SS history of the file(s) local_file_name
  virtual SCCRTN History(const char* local_file_name);
  virtual SCCRTN History(const char** local_file_names, int n_file);

  //! show properties
  virtual SCCRTN Properties(const char* local_file_name);

  /*!
     return status of 'local_file_name', the values can be ORed together,
     see defined values SCC_STATUS_xxx above
  */
  virtual SCCSTAT Status(const char* local_file_name);

  //! return TRUE if local_file_name is under SS control
  virtual bool UnderSSControl(const char* local_file_name);

  //! return TRUE if local_file_name is checked out by current user
  virtual bool CheckedOut(const char* local_file_name);

  //! return TRUE if local_file_name is deleted in the SourceSafe
  virtual bool Deleted(const char* local_file_name);

  //! return user name
  virtual const char* GetUserName() const;

  //! return SS project name
  virtual const char* GetProjName() const;

  //! return local path of the project
  virtual const char* GetLocalPath() const;

  //! return server name
  virtual const char* GetServerName() const;
};


#endif       // _scc_h_
