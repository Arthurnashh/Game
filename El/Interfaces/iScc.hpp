#ifdef _MSC_VER
#pragma once
#endif

#ifndef _I_SCC_HPP
#define _I_SCC_HPP


//! return type
typedef long SCCRTN;
typedef long SCCSTAT;

#ifndef _SCC_STATUS_DEFINED
#define _SCC_STATUS_DEFINED
/**
The SCC_STATUS_xxx macros define the state of a file
*/
enum  SccStatus 
{
  SCC_STATUS_INVALID          = -1L,		///< Status could not be obtained, don't rely on it
  SCC_STATUS_NOTCONTROLLED    = 0x0000L,	///< File is not under source control
  SCC_STATUS_CONTROLLED       = 0x0001L,	///< File is under source code control
  SCC_STATUS_CHECKEDOUT       = 0x0002L,	///< Checked out to current user at local path
  SCC_STATUS_OUTOTHER         = 0x0004L,	///< File is checked out to another user
  SCC_STATUS_OUTEXCLUSIVE     = 0x0008L,	///< File is exclusively check out
  SCC_STATUS_OUTMULTIPLE      = 0x0010L,	///< File is checked out to multiple people
  SCC_STATUS_OUTOFDATE        = 0x0020L,	///< The file is not the most recent
  SCC_STATUS_DELETED          = 0x0040L,	///< File has been deleted from the project
  SCC_STATUS_LOCKED           = 0x0080L,	///< No more versions allowed
  SCC_STATUS_MERGED           = 0x0100L,	///< File has been merged but not yet fixed/verified
  SCC_STATUS_SHARED           = 0x0200L,	///< File is shared between projects
  SCC_STATUS_PINNED           = 0x0400L,	///< File is shared to an explicit version
  SCC_STATUS_MODIFIED         = 0x0800L,	///< File has been modified/broken/violated
  SCC_STATUS_OUTBYUSER        = 0x1000L	///< File is checked out by current user someplace
};
#endif /* _SCC_STATUS_DEFINED */


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
class SccFunctions
{
public:
  virtual ~SccFunctions() {};
  
  /*!
    Initialization function, call as first function.
    @param server_name - the real path to the server (e.g. "\\\\new_server\\vssdatap\\dataP")
    @param proj_name   - the directory in SourceSafe (e.g. "$/")
                  if proj_name is NULL, SS ask you to enter project name
    @param local_path  - local path must correspond with proj_name (e.g. "p:")
                  if local_path is NULL, SS ask you to enter local path name         
    @param hWnd        - a handle that can be used as a parent for any dialog boxes

    @note: Init is obsolete. Use Open for open project
  */
  virtual SCCRTN Init(const char* server_name= NULL, const char* proj_name= NULL, const char* local_path= NULL, HWND hWnd= NULL, const char *user_name=NULL) {return 0;}
  
  /// Opens project from Source Safe
  virtual SCCRTN Open(const char *project_name=NULL, const char *local_path=NULL, const char *server_name=NULL, const char *user_name=NULL)
    {return Init(server_name,project_name, local_path, NULL,user_name);}

  virtual SCCRTN Done() {return 0;}

  //! return true if Scc functions are initialized (a project is opened)
  virtual bool Opened() const {return false;}

  //! Creates and opens a new project in SCC
  virtual bool CreateProject(const char *project_name, const char *local_path, const char *server_name=NULL, const char *user_name=NULL) {return false;}    

  //! open the dialog to select project
  virtual SCCRTN ChooseProject() {return 0;}

  /// open the dialog to select project, you can specify default location
  /** NOTE: Scc.h doesn't support this function */
  virtual SCCRTN ChooseProjectEx(const char *project_name=NULL, const char *local_path=NULL, const char *server_name=NULL, const char *user_name=NULL) {return 0;}

  //! get the last version from SS, local_file_name is local path to the file
  virtual SCCRTN GetLatestVersion(const char* LocalFileName) {return 0;}
  virtual SCCRTN GetLatestVersion(const char** LocalFileNames, int NFile) {return 0;}

  //! Get latest version of dircetory. Recursive, if flag 'recursive' is true
  virtual SCCRTN GetLatestVersionDir(const char* local_file_name, bool recursive= false) {return 0;}

  //! Check out, local_file_name is local path to the file
  virtual SCCRTN CheckOut(const char* local_file_name, const char *comment=NULL) {return 0;}
  virtual SCCRTN CheckOut(const char** local_file_names, int n_file, const char *comment=NULL) {return 0;}

  //! Check in, local_file_name is local path to the file
  virtual SCCRTN CheckIn(const char* LocalFileName, const char* Comment= NULL) {return 0;}
  virtual SCCRTN CheckIn(const char** LocalFileNames, int NFile, const char* Comment= NULL) {return 0;}

  //! Unod check out, local_file_name is local path to the file
  virtual SCCRTN UndoCheckOut(const char* LocalFileName) {return 0;}
  virtual SCCRTN UndoCheckOut(const char** LocalFileNames, int NFile) {return 0;}

  //! add file(s) to SS
  virtual SCCRTN Add(const char* LocalFileName, const char* Comment= NULL) {return 0;}
  virtual SCCRTN Add(const char** LocalFileNames, int NFile, const char* Comment= NULL) {return 0;}

  //! remove file(s) from SS
  virtual SCCRTN Remove(const char* LocalFileName, const char* Comment= NULL) {return 0;}
  virtual SCCRTN Remove(const char** LocalFileNames, int NFile, const char* Comment= NULL) {return 0;}

  //show differences between the local copy of file and the version in SS
  virtual SCCRTN Diff(const char* LocalFileName) {return 0;}

  //! show SS history of the file(s) local_file_name
  virtual SCCRTN History(const char* LocalFileName) {return 0;}
  virtual SCCRTN History(const char** LocalFileNames, int NFile) {return 0;}

  //! show properties
  virtual SCCRTN Properties(const char* local_file_name) {return 0;}

  /*!
     return status of 'local_file_name', the values can be ORed together,
     see defined values SCC_STATUS_xxx above
  */
  virtual SCCSTAT Status(const char* local_file_name) {return -1;}
  virtual SCCRTN Status(const char** LocalFileNames, int NFile, SCCSTAT *statusArray) {return -1;}

  //! return true if local_file_name is under SS control
  virtual bool UnderSSControl(const char* local_file_name) {return false;}

  //! return true if local_file_name is checked out by current user
  virtual bool CheckedOut(const char* local_file_name) {return false;}

  //! return true if local_file_name is deleted in the SourceSafe
  virtual bool Deleted(const char* local_file_name) {return false;}

  //! return true if local_file_name is under SS control and not deleted
  virtual bool UnderSSControlNotDeleted(const char* local_file_name) {
      return UnderSSControl(local_file_name) && !Deleted(local_file_name);}

  //! return user name
  virtual const char* GetUserName() const {return NULL;}

  //! return SS project name
  virtual const char* GetProjName() const  {return NULL;}

  //! return local path of the project
  virtual const char* GetLocalPath() const  {return NULL;}

  //! return server name
  virtual const char* GetServerName() const {return NULL;}
  
  //! Clones current project to another. New project can change local path without affecting origin
  /**
  @return pointer to new project initialized as current project. Call delete to destroy this project.
  Function returns NULL, if an error occured (project is not open, too many projects opened or
  function is not supported)
  */
  virtual SccFunctions *Clone() const {return NULL;}

  /// Gets directory status
  /** Default implementation returns true, to emulate this function on all possible directories.
  Applications, that using this function on library, that doesn't support it will try work with
  director normally.  if default implementation would return false, source safe support in application would be blocked.
  PS: Microsoft Visual Source Safe tuhle funkci bohuzel nepodporuje. Snad nejaky jiny MSSCCI provider
  */
  virtual bool UnderSSControlDir(const char* dir_name) {return true;}

  virtual bool Rename(const char *oldName, const char *newName) {return false;}
};


#endif
