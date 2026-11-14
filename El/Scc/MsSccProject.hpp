#pragma once

#include "MsSccLib.hpp"
#include <El/Interfaces/IScc.hpp>

struct SccProviderInfo
{
  const char *callerName; //user defined client name, e.g. application name (can be NULL)
  char sccName[SCC_NAME_LEN]; //SS provider's name (filled by SCC)
  long sccCaps;           //SS provider's capabilities (filled by SCC)
  char sccDllName[SCC_PRJPATH_LEN]; //SS provider's module name (filled by SCC)
  long maxCheckoutCommentLen; //maximum len of checkout comment including '\0' (filled by SCC)
  long maxCommentLen; //maximum len of other comments including '\0' (filled by SCC)
  SCCRTN result;      //result of initializing project (filled by SCC)
};

class MsSccProjectBase;

typedef HWND (*MsSccProjectWindowCallback)(const MsSccProjectBase *project);

class MsSccProjectBase
{
protected:
  MsSccLib &_scc;
  void *_context;
  HWND _defaultWindow;
  char _user[SCC_USER_LEN];
  char _project[SCC_PRJPATH_LEN];
  char _auxProjPath[SCC_PRJPATH_LEN];
  char _localPath[SCC_PRJPATH_LEN];

  bool _enableTextOut;
  bool _projectActive;    

  MsSccProjectWindowCallback GetActiveWindowCallback;

  HWND DefaultWindow() const
    {
      HWND wnd;
      if (GetActiveWindowCallback==0) return _defaultWindow;
      wnd=GetActiveWindowCallback(this);
      if (wnd) return wnd;
      return _defaultWindow;
    }


public:
  ///Constructs MsSccProjectBase, calls Inicialize
  /**
  @param scc reference to loaded MsSccLib singleton class
  @param defWindow handle to window, that will own all folowing dialogs. 
  Each function (which use windows) can override default window by parameter.    
  @return Test succes using IsValid function
  */
  MsSccProjectBase(MsSccLib &scc, SccProviderInfo *sinfo=NULL, HWND defWindow=NULL);
  virtual ~MsSccProjectBase();

  ///Tests, if current project is opened and valid
  bool IsValid() {return _context!=NULL;}

  bool IsProjectOpened() const {return _projectActive;}

  /**
  Opens a project.  This function should never be called with an already open
  project on pContext.  The lpUser, lpProjName, and lpAuxProjPath values
  may be modified by the provider.
  */
  SCCRTN OpenProject(const char *user=NULL,
    const char *projName=NULL,
    const char *localProjPath=NULL,
    const char *database=NULL,
    HWND hWnd=NULL
    );

  ///Called to close a project opened by SccOpenProject.
  SCCRTN CloseProject();

  /**
  Prompts the user for provider project information.  This may include the
  path to a certain project.  The caller must be prepared to accept changes
  to lpUser, lpProjName, lpLocalPath, and lpAuxProjPath.  lpProjName and
  lpAuxProjPath are then used in a call to SccOpenProject.  They should not
  be modified by the caller upon return.  The caller should avoid displaying
  these two parameters upon return, as the provider might use a formatted
  string that is not ready for view.
  */
  SCCRTN OpenProjectAskUser(
    const char *user=NULL,
    const char *projName=NULL,
    const char *localPath=NULL,
    const char *auxProjPath=NULL,
    bool bAllowChangePath=TRUE,
    bool *new_project=NULL,
    HWND hWnd=NULL
    );

  /**
  Retrieves a read only copy of a set of files.  The array is a set of files
  on the local disk.  The paths must be fully qualified.

  @param nFiles number of files to retrieve
  @param fileNames array of files to retrieve
  @param hWnd optional handle to window that overrides defaultWindow
  @param flags Service provider's defined flags
  @param options additional options, descripted in original MSSCCI
  */
  SCCRTN Get(LONG nFiles, LPCSTR* fileNames, HWND hWnd=NULL,
    LONG flags=0,  LPCMDOPTS options=NULL )      
  {return _scc.Get(_context,hWnd?hWnd:DefaultWindow(),nFiles,fileNames,flags,options);}

  /**
  Retrieves a read only copy of a one file.  It is specified by location 
  on the local disk.  The path must be fully qualified.

  @param fileName fully qualified pathname into local disky of file to get
  @param hWnd optional handle to window that overrides defaultWindow
  @param dwFlags Service provider's defined flags
  @param pvOptions additional options, descripted in original MSSCCI
  */
  SCCRTN Get(LPCSTR fileName, HWND hWnd=NULL,
    LONG flags=SCC_OPT_SCO_NO,  LPCMDOPTS options=NULL )
  {return _scc.Get(_context,hWnd?hWnd:DefaultWindow(),1,&fileName,flags,options);}

  ///Checks out the array of files.  The array is a set of fully qualified local path names.
  /**
  @param nFiles number of files to check out
  @param fileNames array of files to check out
  @param comment optional comment with check out
  @param hWnd optional handle to window that overrides defaultWindow
  @param flags Service provider's defined flags
  @param options additional options, descripted in original MSSCCI
  */
  SCCRTN Checkout(LONG nFiles, LPCSTR* fileNames, LPCSTR comment=NULL,
    HWND hWnd=NULL, LONG flags=SCC_OPT_SCO_NO, LPCMDOPTS options=NULL)
  {return _scc.Checkout(_context,hWnd?hWnd:DefaultWindow(),nFiles,fileNames,comment?comment:"",flags,options);}

  ///Checks out one file. 
  /**
  @param fileName fully qualified pathname into local disky of file to check out
  @param hWnd optional handle to window that overrides defaultWindow
  @param comment optional comment with check out
  @param dwFlags Service provider's defined flags
  @param pvOptions additional options, descripted in original MSSCCI
  */
  SCCRTN Checkout(LPCSTR fileName, LPCSTR comment=NULL,
    HWND hWnd=NULL, LONG flags=SCC_OPT_SCO_NO, LPCMDOPTS options=NULL)
  {return _scc.Checkout(_context,hWnd?hWnd:DefaultWindow(),1,&fileName,comment?comment:"",flags,options);}


  ///Undo a checkout of an array of files.  The array is a set of fully qualified local path names.
  /**
  @param nFiles number of files to undo check out
  @param fileNames array of files to retrieve
  @param hWnd optional handle to window that overrides defaultWindow
  @param flags Service provider's defined flags
  @param options additional options, descripted in original MSSCCI
  */
  SCCRTN Uncheckout(LONG nFiles,LPCSTR* fileNames, 
    HWND hWnd=NULL, LONG flags=SCC_OPT_SCO_NO, LPCMDOPTS options=NULL)
  {return _scc.Uncheckout(_context,hWnd?hWnd:DefaultWindow(),nFiles,fileNames,flags,options);}


  /**
  ///Undo a checkout a file.

  @param fileName fully qualified pathname into local disky of file to undo check out
  @param hWnd optional handle to window that overrides defaultWindow
  @param dwFlags Service provider's defined flags
  @param pvOptions additional options, descripted in original MSSCCI
  */
  SCCRTN Uncheckout(LPCSTR fileName, HWND hWnd=NULL,
    LONG flags=SCC_OPT_SCO_NO,  LPCMDOPTS options=NULL )
  {return _scc.Uncheckout(_context,hWnd?hWnd:DefaultWindow(),1,&fileName,flags,options);}

  ///Make the modifications the user has made to an array of files permanent. 
  /**The file names must be fully qualified local paths. 
  @param nFiles number of files to check in
  @param fileNames array of files to check in
  @param comment optional comment with check in
  @param hWnd optional handle to window that overrides defaultWindow
  @param flags Service provider's defined flags
  @param options additional options, descripted in original MSSCCI
  */
  SCCRTN Checkin( LONG nFiles,  LPCSTR* fileNames, LPCSTR comments=NULL, 
    HWND hWnd=NULL, LONG flags=SCC_OPT_SCO_NO, LPCMDOPTS options=NULL)
  {return _scc.Checkin(_context,hWnd?hWnd:DefaultWindow(),nFiles,fileNames,comments?comments:"",flags,options);}

  ///Make the modifications the user has made to a file permanent. 
  /**The file name must be fully qualified local path 
  @param fileName name of file to check in
  @param comment optional comment with check in
  @param hWnd optional handle to window that overrides defaultWindow
  @param flags Service provider's defined flags
  @param options additional options, descripted in original MSSCCI
  */
  SCCRTN Checkin(  LPCSTR fileName, LPCSTR comments=NULL, 
    HWND hWnd=NULL, LONG flags=0, LPCMDOPTS options=NULL)
  {return _scc.Checkin(_context,hWnd?hWnd:DefaultWindow(),1,&fileName,comments?comments:"",flags,options);}

  ///Add an array of fully qualified files to the source control system. 
  /** 
  The file names must be fully qualified local paths. 
  @param nFiles number of files to add
  @param fileNames array of files to add
  @param comment optional comment with add
  @param flags The array of flags describe the type of file.  See the SCC_FILETYPE_xxxx flags
  @param hWnd optional handle to window that overrides defaultWindow
  @param options additional options, descripted in original MSSCCI
  */
  SCCRTN Add(LONG nFiles,  LPCSTR* fileNames,  LPCSTR comments=NULL, 
    HWND hWnd=NULL, LONG *flags=NULL, LPCMDOPTS options=NULL)
  {return _scc.Add(_context,hWnd?hWnd:DefaultWindow(),nFiles,fileNames,comments?comments:"",flags,options);}

  ///Add a file to the source control system. 
  /** 
  The file name must be fully qualified local path. 
  @param fileName array of files to add
  @param comment optional comment with add
  @param hWnd optional handle to window that overrides defaultWindow
  @param flags The flags to describe the type of file.  See the SCC_FILETYPE_xxxx flags
  @param options additional options, descripted in original MSSCCI
  */
  SCCRTN Add(LPCSTR fileName,  LPCSTR comments=NULL, 
    HWND hWnd=NULL, LONG flags=0, LPCMDOPTS options=NULL)
  {return _scc.Add(_context,hWnd?hWnd:DefaultWindow(),1,&fileName,comments?comments:"",&flags,options);}

  ///Removes the array of fully qualified files from the source control system.
  /** The files are not removed from the user's disk, unless advanced options
  are set by the user.  Advaned options are defined by the provider.
  @param nFiles number of files to retrieve
  @param fileNames array of files to retrieve
  @param hWnd optional handle to window that overrides defaultWindow
  @param flags Service provider's defined flags
  @param options additional options, descripted in original MSSCCI*/
  SCCRTN Remove( LONG nFiles, LPCSTR* fileNames, LPCSTR comments=NULL,
    HWND hWnd=NULL, LONG flags=0, LPCMDOPTS options=NULL )
  {return _scc.Remove(_context,hWnd?hWnd:DefaultWindow(),nFiles,fileNames,comments?comments:"",flags,options);}

  ///Removes a fully qualified file from the source control system.
  /** The files are not removed from the user's disk, unless advanced options
  are set by the user.  Advaned options are defined by the provider.
  @param fileName fully qualified pathname to remove from the source control system.
  @param hWnd optional handle to window that overrides defaultWindow
  @param flags Service provider's defined flags
  @param options additional options, descripted in original MSSCCI*/
  SCCRTN Remove(LPCSTR* fileName, LPCSTR comments=NULL,
    HWND hWnd=NULL, LONG flags=0, LPCMDOPTS options=NULL )
  {return _scc.Remove(_context,hWnd?hWnd:DefaultWindow(),1,fileName,comments?comments:"",flags,options);}


  SCCRTN BeginBatch() 
  {return _scc.BeginBatch();}
  SCCRTN EndBatch()
  {return _scc.EndBatch();}

  ///Renames the given file to a new name in the source control system. 
  /** The	provider should not attempt to access the file on disk.  It is the
  caller's responsibility to rename the file on disk.*/
  SCCRTN Rename(LPCSTR fileName, LPCSTR newName,HWND hWnd=NULL)
  {return _scc.Rename(_context,hWnd?hWnd:DefaultWindow(),fileName,newName);}

  /**Show the differences between the local users fully qualified file 
  and the version under source control.*/

  SCCRTN Diff(LPCSTR fileName,HWND hWnd=NULL, LONG flags=0, LPCMDOPTS options=NULL)
  {return _scc.Diff(_context,hWnd?hWnd:DefaultWindow(),fileName,flags,options);}

  /**
  Show the differences between the local user's fully qualified directory and
  the project under source control.
  */
  SCCRTN DirDiff(LPCSTR dirName,HWND hWnd=NULL, LONG flags=0, LPCMDOPTS options=NULL)
  {return _scc.DirDiff(_context,hWnd?hWnd:DefaultWindow(),dirName,flags,options);}


  ///Show the history for an array of fully qualified local file names.
  /** The provider may not always support an array of files, in which case only the
  first files history will be shown.
  */
  SCCRTN History(LONG nFiles,LPCSTR* fileNames,
    HWND hWnd=NULL, LONG flags=0, LPCMDOPTS options=NULL )
  {return _scc.History(_context,hWnd?hWnd:DefaultWindow(),nFiles,fileNames,flags,options);}


  ///Show the properties of a fully qualified file.  
  /** The properties are defined
  by the provider and may be different for each one. */

  SCCRTN Properties(LPCSTR fileName,HWND hWnd=NULL)
  {return _scc.Properties(_context,hWnd?hWnd:DefaultWindow(),fileName);}


  ///Examine a list of fully qualified files for their current status.
  /** The return array will be a bitmask of SCC_STATUS_xxxx bits.  A provider may
  not support all of the bit types.  For example, SCC_STATUS_OUTOFDATE may
  be expensive for some provider to provide.  In this case the bit is simply
  not set.*/
  SCCRTN QueryInfo(LONG nFiles, LPCSTR* fileNames, LONG *status)
  {return _scc.QueryInfo(_context,nFiles,fileNames,status);}      


  ///Examine a list of fully qualified dirs for their current status.
  /**The return array will be a bitmask of SCC_DIRSTATUS_xxxx bits.  A provider may
  not support all of the bit types.  For example, SCC_DIRSTATUS_EMPTYPROJ may
  be expensive for some provider to provide.  In this case the bit is simply
  not set.
  */
  SCCRTN DirQueryInfo(LONG nDirs, LPCSTR* dirNames, LONG *status)
  {return _scc.DirQueryInfo(_context,nDirs,dirNames,status);}

  ///Like SccQueryInfo, this function will examine the list of files for their current status.  
  /**In addition, it will use the OnPopulate function to  notify the 
  caller when a file does not match the critera for the nCommand.
  For example, if the command is SCC_COMMAND_CHECKIN, and a file in the list
  is not checked out, then the callback is used to tell the caller this.  
  Finally, the provider may find other files that could be part of the command
  and add them.  This allows a VB user to check out a .bmp file that is used
  by their VB project, but does not appear in the VB makefile.
  */
  SCCRTN PopulateList(enum SCCCOMMAND command,LONG files, LPCSTR* fileNames, 
    LONG *lpStatus,  LONG flags=0);

  ///SccGetEvents runs in the background checking the status of files
  /** that the caller has asked about (via SccQueryInfo).  
  When the status changes, it 
  builds a list of those changes that the caller may exhaust on idle.  This
  function must take virtually no time to run, or the performance of the 
  caller will start to degrade.  For this reason, some providers may choose
  not to implement this function.*/

  SCCRTN GetEvents( LPSTR fileName, LONG *status,LONG *eventsRemaining)
  {return _scc.GetEvents(_context,fileName,status,eventsRemaining);}

  ///This function allows a user to access the full range of features
  /**of the	source control system.  This might involve launching the native front end
  to the product.  Optionally, a list of files are given for the call.  This
  allows the provider to immediately select or subset their list.  If the
  provider does not support this feature, it simply ignores the values.*/

  SCCRTN RunScc( LONG nFiles, LPCSTR* fileNames, HWND hWnd=NULL)
  {return _scc.RunScc(_context,hWnd?hWnd:DefaultWindow(),nFiles,fileNames);}

  ///This function will prompt the user for advaned options for the given command.
  /** Call it once with ppvOptions==NULL to see if the provider
  actually supports the feature.  Call it again when the user wants to see
  the advaned options (usually implemented as an Advaned button on a dialog).
  If a valid *ppvOptions is returned from the second call, then this value
  becomes the pvOptions value for the SccGet, SccCheckout, SccCheckin, etc...
  functions.*/

  SCCRTN GetCommandOptions(enum SCCCOMMAND command,  LPCMDOPTS *options, HWND hWnd=NULL)
  {return _scc.GetCommandOptions(_context,hWnd?hWnd:DefaultWindow(),command,options);}

  ///This function allows the user to browse for files that are already in the
  /**source control system and then make those files part of the current project.
  This is handy, for example, to get a common header file into the current
  project without having to copy the file.  The return array of files
  (fileNames) contains the list of files that the user wants added to
  the current makefile/project.*/

  SCCRTN AddFromScc(LONG *files,LPCSTR** fileNames,HWND hWnd=NULL)
  {return _scc.AddFromScc(_context,hWnd?hWnd:DefaultWindow(),files,fileNames);}

  ///SccSetOption is a generic function used to set a wide variety of options.
  /* Each option starts with SCC_OPT_xxx and has its own defined set of values.*/
  SCCRTN SetOption(LONG option, LONG val)
  {return _scc.SetOption(_context,option,val);}
  ///Checks if multiple checkouts on a file are allowed.
  /**@return function returns SCC_OK if feature is enabled, or returns SCC_CE_DISABLED,
  if feature is disabled. If an error occured, it returns an error from set of SCC_E_XXXX
  errors.*/
  SCCRTN IsMultiCheckoutEnabled()
  {
    BOOL enable;
    SCCRTN ret=_scc.IsMultiCheckoutEnabled(_context,&enable);
    if (ret==SCC_OK)
      if ( enable) return SCC_OK;
      else return SCC_CE_DISABLED;
    else
      return ret;
  }


  MsSccLib &GetProvider() const {return _scc;}

  MsSccProjectWindowCallback GetWindowCallback() const {return GetActiveWindowCallback;}
  void SetWindowCallback(MsSccProjectWindowCallback callback) {GetActiveWindowCallback=callback;}

 

  ///Overrides:
public:

  ///Called everytime that Scc wants send to client a message
  /**
  @param message message text sent
  @msgtype type of message (MsSccLib.hpp SCC_MSG_XXXX values)
  @return function must return SCC_MSG_RTN_OK.           
  */
  static SCCRTN (*OnTextOut)(const char *message, long msgtype);

  virtual bool OnPopulate(bool AddKeep,long status, const char *filename)
  {return true;}
  };

  ///Interface functions

class MsSccProject: public SccFunctions, public MsSccProjectBase
{
public:
  MsSccProject(MsSccLib &scc, SccProviderInfo *sinfo=NULL, HWND defWindow=NULL):
      MsSccProjectBase(scc,sinfo,defWindow) {}

  virtual SCCRTN Open(const char *project_name=NULL, const char *local_path=NULL, const char *server_name=NULL, const char *user_name=NULL)
    {return MsSccProjectBase::OpenProject(user_name,project_name,local_path,server_name);}
  virtual SCCRTN ChooseProjectEx(const char *project_name=NULL, const char *local_path=NULL, const char *server_name=NULL, const char *user_name=NULL)
    {return MsSccProjectBase::OpenProjectAskUser(user_name,project_name,local_path,server_name);}
  virtual SCCRTN Done()
    {return CloseProject();}
  //! return TRUE if Scc functions are initialized (a project is opened)
  virtual bool Opened() const 
    {return IsProjectOpened();}

  //! open the dialog to select project
  virtual SCCRTN ChooseProject()
    {return OpenProjectAskUser();}

  //! get the last version from SS, local_file_name is local path to the file
  virtual SCCRTN GetLatestVersion(const char* LocalFileName)
    {return Get(LocalFileName);}  
  virtual SCCRTN GetLatestVersion(const char** LocalFileNames, int NFile)
    {return Get(NFile,LocalFileNames);}

  //! Get latest version of dircetory. Recursive, if flag 'recursive' is TRUE
  virtual SCCRTN GetLatestVersionDir(const char* local_file_name, bool recursive= FALSE);

  //! Check out, local_file_name is local path to the file
  virtual SCCRTN CheckOut(const char* local_file_name, const char *comment=NULL)
    {return Checkout(local_file_name,comment);}
  virtual SCCRTN CheckOut(const char** local_file_names, int n_file, const char *comment=NULL) 
    {return Checkout(n_file,local_file_names,comment);}

  //! Check in, local_file_name is local path to the file
  virtual SCCRTN CheckIn(const char* LocalFileName, const char* Comment= NULL) 
    {return Checkin(LocalFileName,Comment);}
  virtual SCCRTN CheckIn(const char** LocalFileNames, int NFile, const char* Comment= NULL) 
    {return Checkin(NFile,LocalFileNames,Comment);}

  //! Unod check out, local_file_name is local path to the file
  virtual SCCRTN UndoCheckOut(const char* LocalFileName)
    {return Uncheckout(LocalFileName);}
  virtual SCCRTN UndoCheckOut(const char** LocalFileNames, int NFile) 
    {return Uncheckout(NFile,LocalFileNames);}

  //! add file(s) to SS
  virtual SCCRTN Add(const char* LocalFileName, const char* Comment= NULL)
    {return MsSccProjectBase::Add(LocalFileName,Comment);}
  virtual SCCRTN Add(const char** LocalFileNames, int NFile, const char* Comment= NULL)
    {return MsSccProjectBase::Add(NFile,LocalFileNames,Comment);}

  //! remove file(s) from SS
  virtual SCCRTN Remove(const char * LocalFileName, const char* Comment= NULL) 
    {return MsSccProjectBase::Remove(1,&LocalFileName,Comment);}
  virtual SCCRTN Remove(const char** LocalFileNames, int NFile, const char* Comment= NULL)
    {return MsSccProjectBase::Remove(LocalFileNames,Comment);}

  //show differences between the local copy of file and the version in SS
  virtual SCCRTN Diff(const char* LocalFileName) 
    {return MsSccProjectBase::Diff(LocalFileName);}

  //! show SS history of the file(s) local_file_name
  virtual SCCRTN History(const char* LocalFileName)
    {return MsSccProjectBase::History(1,&LocalFileName);}
  virtual SCCRTN History(const char** LocalFileNames, int NFile) 
    {return MsSccProjectBase::History(NFile,LocalFileNames);}

  //! show properties
  virtual SCCRTN Properties(const char* local_file_name) 
    {return MsSccProjectBase::Properties(local_file_name);}

  /*!
     return status of 'local_file_name', the values can be ORed together,
     see defined values SCC_STATUS_xxx above
  */
  virtual SCCSTAT Status(const char* local_file_name) 
    {
    SCCSTAT stat=SCC_STATUS_INVALID;
    MsSccProjectBase::QueryInfo(1,&local_file_name,&stat);
    return stat;
    }
  virtual SCCRTN Status(const char** LocalFileNames, int NFile, SCCSTAT *statusArray)
  {
    return MsSccProjectBase::QueryInfo(NFile,LocalFileNames,statusArray);
  }

  //! return TRUE if local_file_name is under SS control
  virtual bool UnderSSControl(const char* local_file_name)
  {
    SCCSTAT status = Status(local_file_name);
    return (status != SCC_STATUS_INVALID && (status & SCC_STATUS_CONTROLLED));
  }

  virtual bool UnderSSControlNotDeleted(const char* local_file_name)
  {
    SCCSTAT status = Status(local_file_name);
    return (status != SCC_STATUS_INVALID && (status & SCC_STATUS_CONTROLLED)  &&
        !(status & SCC_STATUS_DELETED));
  }

  virtual bool UnderSSControlDir(const char* dir_name)
  {
    if (_scc.DirQueryInfo==NULL) return true;
    SCCSTAT status;
    MsSccProjectBase::DirQueryInfo(1,&dir_name,&status);
    return (status != SCC_STATUS_INVALID && (status & SCC_STATUS_CONTROLLED));
  }


  //! return TRUE if local_file_name is checked out by current user
  virtual bool CheckedOut(const char* local_file_name) 
  {
    SCCSTAT status = Status(local_file_name);
    return (status != SCC_STATUS_INVALID && (status & SCC_STATUS_OUTBYUSER));
  }

  virtual bool Deleted(const char* local_file_name) 
  {
    SCCSTAT status = Status(local_file_name);
    return status != SCC_STATUS_INVALID && (Status(local_file_name) & SCC_STATUS_DELETED);
  }

  virtual const char* GetUserName() const {return MsSccProjectBase::_user;}

  virtual const char* GetProjName() const  {return MsSccProjectBase::_project;}

  virtual const char* GetLocalPath() const  {return MsSccProjectBase::_localPath;}

  virtual const char* GetServerName() const {return MsSccProjectBase::_auxProjPath;}

  virtual SccFunctions *Clone() const;

  virtual bool Rename(const char *oldName, const char *newName)
  {
    return MsSccProjectBase::Rename(oldName,newName,0)==SCC_OK;
  }

  virtual bool CreateProject(const char *project_name, const char *local_path, const char *server_name=NULL, const char *user_name=NULL)
  {
    bool nww=true;
    return MsSccProjectBase::OpenProjectAskUser(user_name,project_name,local_path,server_name,TRUE,&nww)!=0;
  }

  
};
