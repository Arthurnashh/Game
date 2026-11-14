// Pathname.h: interface for the Pathname class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_Pathname_H__40F41C23_3AA2_486C_B9E5_33AEE67FB313__INCLUDED_)
#define AFX_Pathname_H__40F41C23_3AA2_486C_B9E5_33AEE67FB313__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>


#ifndef ASSERT
#ifdef _DEBUG
#define ASSERT(x) assert(x)
#else 
#define ASSERT(x)
#endif
#endif



enum PathNameNullEnum {PathNull};

#define PathNameCompare(op) bool operator op (const Pathname &other) const \
{if (IsNull() || other.IsNull()) return false;else return _tcsicmp(_fullpath,other._fullpath) op 0;}\
  bool operator op (const t_string other) const \
{ASSERT(other[0]!=0);\
  if (IsNull() || other==0) return false;else return _tcsicmp(_fullpath,other) op 0;}

#include <TCHAR.H>
#define t_string _TCHAR *
#define t_char _TCHAR 

/** class Pathname simplifying manipulation with pathnames, filenames, general paths, and
also supports convert from absolute path to relative respectively */



class Pathname  
{
  ///object value and data
  /**The implementation of Pathname creates only one buffer for all variables. It can
  increase speed by effective memory use. Strings are stored one after another separated
  by zero byte. Evry time any string changed, implementation recalculate buffer usage, and
  decide, whether it should resize buffer or not. 

  _fullpath also points to string contain full path with filename, 
  it is dominant value of the class
  */
  t_string _fullpath; 
  t_string _path;      ///<current path with drive
  t_string _filetitle;  ///<file title without extension
  t_string _extension;  ///<file extension
  t_string _end;        ///<end of field. Class must know, where buffer ends

public:

  ///Construct Pathname class. 
  /** If name is not provided, current path is used, and as filename, Pathname suppl wildcards
  *.* . That is useful, for searching in directory.<br>
  If name is provided, Pathname will expand it into full name with drive and folder name.
  @param name optional argument to inicialize object
  */
  Pathname(const t_string name=0);

  ///Construct Pathname class
  /** Using this constructor Pathname(PathNull) will create Pathname class with null content is set. 
  If null content is set, all string-query function returns NULL. IsNull function returns true. This state
  is sets until new path is assigned. There is a set of functions invalid called in null state.
  */

  Pathname(PathNameNullEnum null);

  ///Construct Pathname class
  /** 
  @param relpath Relative path or uncomplette path or single filename with extension. 
  Pathname will expand this pathname into full using absolute path provided by the second 
  argument.
  @param abspath Absolute path used as reference to folder - the origin of relative path
  provided in the first argument.
  */
  Pathname(const t_string relpath, const Pathname &abspath);

  ///Construct Pathname as copy of another pathname
  Pathname(const Pathname &other);

  ///Destruct Pathname
  virtual ~Pathname();

  ///Function returns the current drive letter.
  /** Before usage, ensure, that current pathname contain drive.     
  In network path drive letter is missing.
  In this case, result is undefined. To ensure, use HasDrive function
  @return the drive letter of current path.
  */
  t_char GetDrive() const
  {
    if (IsNull()) return 0;
    return _path[0];
  }

  ///Static function determines, if argument contain a drive information. 
  /** 
  @param dir directory to inspect
  @return true, if directory contain drive.<p>
  This function is independed, it don't need any Pathname variable declared.
  */
  static bool HasDrive(const t_string dir) 
  {return (isalpha((unsigned)dir[0]) && dir[1]==':');}

  ///Function determines, if current pathname contain a drive information
  /**
  @return true, if current pathname contain a drive information
  */
  bool HasDrive() const 
  {
    if (IsNull()) return false;
    return HasDrive(_path);
  }


  ///Function returns current folder name
  /**
  if current folder name contain drive, only folder name is returned (without drive).
  In other cases (relative or network drives) returns full path.
  @return folder name or full path. Pointer is valid until any first change in object. 
  Do not invoke release function at pointer!
  */
  const t_string GetDirectory() const
  {
    if (HasDrive()) return _path+3;
    else return _path;
  }

  const t_string GetDirectoryWithDrive() const
  {
    return _path;
  }

  ///Function returns current filename with extension
  /**
  @return current filename with extension. Pointer is valid until any first change in object. 
  Do not invoke release function at pointer!
  */
  const t_string GetFilename() const
  {
    if (IsNull()) return 0;      
    t_string blk=_tcsrchr(_fullpath,'\\');
    if (blk) blk=blk+1;else blk=_fullpath;
    return blk;
  }

  ///Function returns current extension (with starting dot)
  /**
  @return current extension. Pointer is valid until any first change in object. 
  Do not invoke release function at pointer!
  */
  const t_string GetExtension() const
  {return _extension;}

  ///Function returns current filename without extension (without dot)
  /**
  @return current filename without extension (without dot). Pointer is valid until any first change in object. 
  Do not invoke release function at pointer!
  */
  const t_string GetTitle() const
  {return _filetitle;}

  ///Function changes current drive.
  /**If object contain pathname with drive, then current drive is changed and function returns.
  If object contain network path, then computer name is changed to the drive name.
  If object contain relative path, then whole path is replaced by path on root on drive.
  @param dr new drive letter. This parameter can be set to zero. It means, that current
  driver is deleted, and path is converted to relative path from root. Note: Zero c
  cannot be used with network paths and relative paths, and finnaly has no effect to the object
  */

  void SetDrive(const t_char dr);

  ///Sets new directory for object
  /** if object contain a drive letter and argument dir doesn't, then current drive is remain
  and only directory part is replaced. If current path is network path or relative path,
  then whole path is replaced by new one.
  If argument dir contain drive letter, then whole path is replaced too.
  @param dir contain new pathname. Backslash should be the last character in string
  */
  void SetDirectory(const t_string dir);

  ///Sets new filename for object. 
  /** 
  If filename contain dot, function assumes, that filename is provided with extension.
  Otherwise, current extension remains untouched.
  @param filename new filename for object
  */

  void SetFilename(const t_string filename);

  ///Sets new extension for object.
  /**
  If ext doesn't starting with dot, function adds it.
  @param ext new extension for object
  */
  void SetExtension(const t_string ext);

  ///Sets new file title
  /** Function changes file title, extension remains untouched. 
  if title contains extension (dot inside its name), this extension doesn't change
  current extension. For example, if current extension is ".cpp" and filetitle contain
  "source.h", then result is "source.h.cpp"
  @param title a new title for object.
  */

  void SetFiletitle(const t_string title);

  ///Function returns full pathname.
  /**
  @return current pathname. Pointer is valid until any first change in object. 
  Do not invoke release function at pointer!
  */

  const t_string GetFullPath() const 
  {return _fullpath;} 

  ///Sets pathname
  /** Function has same effect as constructor. But it can be used 
  anytime during object lifetime. It simply replaces current pathname with newer. Pathname
  in argument is expanded to full pathname, current directory is used as reference.
  @param pathname new pathname
  */
  void SetPathName(const t_string pathname);

  Pathname& operator=(const t_string other) 
  {SetPathName(other);return *this;}
  Pathname& operator=(const Pathname& other);

  ///converts object to string
  operator const t_string () const
  {return GetFullPath();}

  ///converts text to singlebyte string (SBS). 
  /**Function uses remaining space of buffer to copy SBS result.
  If buffer is not large enought, function reallocates the buffer to hold result.
  Use this function with any GetXXX function, if you want SBS result
  @oaram text string to convert into SBS
  @result string SBS format;*/
  const char *ToSBS(const t_string text) const;

#ifdef _UNICODE
  operator const char *() const {return ToSBS(GetFullPath());}
#endif

  ///Static function to help getting filename from pathname
  /** Function finds last backslash / and return pointer to first character after it.
  Pointer stays valid until original path is destroyed or until original path is changed
  @param path pathname to inspect as string
  @return pointer to filename
  */
  static const t_string GetNameFromPath(const t_string path);

  ///Static function to help getting extension from pathname
  /** Function finds last dot '.' in filename return pointer to it (extension with dot).
  Pointer stays valid until original path is destroyed or until original path is changed
  @param path pathname to inspect as string
  @return pointer to extension
  */
  static const t_string GetExtensionFromPath(const t_string path);

  ///Function sets server name for network path
  /** If current path is network path, then changes server name to newer. Otherwise
  it remove drive letter, and insert server name before remain path
  @param server server name without slashes
  */
  void SetServerName(const t_string server);

  ///Function inspects current path and returns, whether contain server name
  /**@return zero, if current path is not valid network path. Nonzero if path contain
  server name. Then value returned count characters containing server name with precedent 
  slashes.
  */
  int IsNetworkPath() const;

  ///Function converts current relative path into absolute path
  /** 
  If current path is not relative, function do nothing.
  @param ref reference to path, against which path is relative. 
  @return true if path has been converted, or false, if conversion is impossible
  */
  bool RelativeToFull(const Pathname &ref);

  ///Function converts current absolute path into relative path
  /** 
  If current path is not relative, function do nothing. Both paths must be on the same
  drive or network computer.

  @param ref reference to path, against which path should be relative. 
  @return true if path has been converted, or false, if conversion is impossible
  */
  bool FullToRelative(const Pathname &relativeto);

  Pathname& operator+=(const t_string relativePath) 
  {*this=Pathname(relativePath,*this);return *this;}

  Pathname operator+(const t_string relativePath)
  {Pathname out(relativePath,*this);return out;}

  bool IsNull() const {return _fullpath==0;}

  void SetNull();

  PathNameCompare(<)
    PathNameCompare(>)
    PathNameCompare(==)
    PathNameCompare(>=)
    PathNameCompare(<=)
    PathNameCompare(!=)


    ///Function gets part of pathname
    /** 
    @param path subject of examine
    @param partnum zero-base index of part of pathname. Index 0 mostly contain drive or server, in case of
    relative path, there is the name of the first folder or dots.
    @param buff buffer for store result
    @param bufsize count characters in buffer;
    @param mode mode=0, gets only name of part. 
    mode=1, get current part and remain parts of path.
    mode=-1, gets all parts till current
    @return Function returns true, if it was succesful, and it was not last part. Function returns
    false, if it was succesful, and it was last part. Function returns false and sets buffer empty,
    if an error occured. Function returns true and sets buffer empty, if buffer is too small to hold data
    */
    static bool GetPartFromPath(const t_string path, int partnum, t_string buff, int bufsize, int mode=0);

  ///Function gets part of object
  /** 
  @param partnum zero-base index of part of pathname. Index 0 mostly contain drive or server, in case of
  relative path, there is the name of the first folder or dots.
  @param buff buffer for store result
  @param bufsize count characters in buffer;
  @param mode mode=0, gets only name of part. 
  mode=1, get current part and remain parts of path.
  mode=-1, gets all parts till current
  @return Function returns true, if it was succesful, and it was not last part. Function returns
  false, if it was succesful, and it was last part. Function returns false and sets buffer empty,
  if an error occured. Function returns true and sets buffer empty, if buffer is too small to hold data
  */
  bool GetPart(int partnum, t_string buff, int bufsize,int mode=0) const
  {
    return GetPartFromPath(this->_fullpath,partnum,buff,bufsize,mode);
  }

  /// Get Directory With Drive Without Last Back Slash
  /** Retrieves into buffer directory with drive and removes last backslash
  @param buff buffer that retrieves path
  @param size size of buffer
  @return true, if success, failed if buffer is too small*/


  bool GetDirectoryWithDriveWLBS(t_string buff, size_t size) const;


  /// Get Directory With Drive Without Last Back Slash
  /** Function uses internal buffer to store the string
  @return pointer to directory name without last back slash. Pointer is valid, until
  object destroyed.
  */
  const t_string GetDirectoryWithDriveWLBS() const;


  /// Function retrieves full pathname name of the directory
  /**
   * Compare with GetDirectoryWithDrive and with GetDirectoryWithDriveWLBS.
   * This function returns any directory as GetDirectoryWithDriveWLBS, but when root directory
   * is specified, it returns GetDirectoryWithDrive (with backslash). This function is
   * useful when directory name should be passed into OS system functions, where system
   * handles root name in special way.
   *
   * Function also supports UNC names.
   */
  const t_string GetDirectoryFullPath() const;

  /// function checks, if path is valid and returns true, if does.
  bool IsPathValid() const;

  /// Sets special directory. 
  /**
  @param bSpecCode this value may be operation-system
  depend. Windows implementation using CSIDL_XXXX constants, which is described in SHGetSpecialFolderLocation function
  description
  @return true, if function were successful
  */
  bool SetDirectorySpecial(int nSpecCode);

  ///Sets temporaly directory. 
  bool SetTempDirectory();

  ///Guess temporaly file name
  /**
  @param prefix prefix string for name
  @param unique if unique is non-zero, it is used for new temporaly file. If unique is zero, function guess own unique
	  value.
  @return true if function were successful
  */
  bool SetTempFile(const t_string prefix=_T("tmp"), unsigned int unique=0);

  ///Returns path of current executable. 
  /**It useful, when accessing folder, from when current module has been executed 
  @param module Pointer to module descriptor, which path we needed. Default value
  gets path of EXE module. Use HINSTANCE or HMODULE in Windows DLL modules
  @note parameter is pointer to descriptor. Use GetExePath(&hInstance);
  */
  static Pathname GetExePath(const void *module=0);

  ///Solves most used conversion from fullpath to path relative to project root
  /**
  @param full full pathname with drive
  @param projectRoot project root path
  @return function returns pointer to full path, where starts relative part. If
    fullpath doesn't contain project root path, it returns pointer to full.
  @example FullToProjectRoot("x:\\project\\data\\example.txt","x:\\project"); //result is "data\\example.txt"
  */

  static const t_string FullToRelativeProjectRoot(const t_string full, const t_string projectRoot);

  ///Changes root path for the path
  /**
   * Calculates new pathname with changed root. 
   * @param oldRoot old root path, function will check, whether changing from this rood is possible
   * @param newRoot new root path that will replace old root.
   * @retval true, successfully done
   * @retval false, impossible, pathname doesn't contain root in its path.
   * @note if oldRoot is empty, function adds newRoot to current directory. If newRoot is empty, old root is
   * removed from the current directory.
   */
  bool ChangeRoot(const t_string oldRoot, const t_string newRoot);

  ///Creates folder from path
  static bool CreateFolder(const t_string path, void *security_descriptor=0);
  
  ///Creates all folders from path stored in object
  /**
  Function creates path stored in object. Function creates whole path, when it doesn't
  exists. 
  @param security_descriptor pointer for additional information about security on new folder
    if this parameter is NULL, default descriptor is used. This parameter is platform
    depended.
  @return if path has been created, returns true. In case of error, returns false
    and no changes are made on disk (Function rollbacks any changes)
    */
  bool CreateFolder(void *security_descriptor=0);
  

  enum DeleteFolderFlags {
    DFSimple=0, //only deletes latest folder
    DFPath=1, //deletes whole path, if there are no files
    DFFile=2, //deletes file specified in object. You can use wildcards
    DFRecursive=4, //also deletes all folders inside the path
/*    DFRecycleBin=8, //move all deleted files or folders into recycle bin
    DFShowProgress=16, //enables progress bar during deleting*/
  };

  ///Deletes folder stored in object
  /**
  @param dfFlags combination of flags.
  @return function returns true, when no error occured. Function return false,
    when error occured. 
    */
  bool DeleteFolder(int dfFlags);

  enum TestFlags
  {
    TFExistence = 0,
    TFCanWrite = 02,
    TFCanRead = 04,
    TFCanReadWrite = 06
  };
  ///Returns true, if file exists 
  bool TestFile(int testFlags=TFExistence) const;
 ///Returns true, if file exists 
  static bool TestFile(const t_string name, int testFlags=TFExistence);
  ///Retrieves informations about file  
  bool GetFileStat(struct _stat &result) const;
  ///Retrieves informations about file
  bool GetFileStat64(struct __stat64 &result) const;


  /**
  @see GetDirectory;
  */
  const char *SBS_GetDirectory() const {return ToSBS(GetDirectory());}
  /**
  @see GetDirectoryWithDrive;
  */
  const char *SBS_GetDirectoryWithDrive() const {return ToSBS(GetDirectoryWithDrive());}
  /**
  @see GetDirectoryWithDriveWLBS;
  */
  const char *SBS_GetDirectoryWithDriveWLBS() const;
  /**
  @see GetFilename;
  */
  const char *SBS_GetFilename() const {return ToSBS(GetFilename());}
  /**
  @see GetExtension;
  */
  const char *SBS_GetExtension() const {return ToSBS(GetExtension());}
  /**
  @see GetTitle;
  */
  const char *SBS_GetTitle() const {return ToSBS(GetTitle());}
  /**
  @see GetFullPath;
  */
  const char *SBS_GetFullPath() const {return ToSBS(GetFullPath());}

  class SearchTestExistence
  {
  public:
    bool operator() (const t_string file) const;
  };


  /// Searches directory structure from the bottom to the top for given file
  /** Function takes starting folder and filename from "this"
  @param test Reference to functor used for testing filename. 
      Functor returns true, if file is acceptable.
  @return true, when file were found, or false, if not. In case of true,
    "this" contains found pathname. In case of false, "this" content is
    destroyed and unusable.
  */

  template<class Functor>
  bool BottomTopSearch(const Functor &test)
  {
    t_string z=0;
    do
    {
      if (test(*this)) {return true;}
      if (z) z[0]=0;
      z=_tcsrchr(_path,'\\');
      if (z) {z[1]=0;RebuildPath();}
    }
    while (z);
    return false;    
  }

  ///Searches directory structure from the bottom to the top for given file
  /**Function tests existence of each file. 
  @see BottomTopSearch
  */
  bool BottomTopSearchExist()
  {return BottomTopSearch(SearchTestExistence());}

  ///Searches paths for given filename. Result is stored into object
  bool SearchPaths(const t_string path, const t_string filename);

  ///Changes current path to point to the user home folder.
  bool SetDirHome();
  ///Changes current path to point to the program folder.
  bool SetDirBin();
  ///Changes current path to point to the user application profile data
  bool SetDirAppData();
  ///Changes current path to point to the user local application profile data
  bool SetDirLocalAppData();
  ///Changes current path to point to the shared application data
  bool SetDirAllUsersAppData();
  ///Changes current path to point to the shared home folder
  bool SetDirAllUsersHome();

  bool SetDirAllApps();
  ///Simulates CD DOS command on object
  /**
   * @param dirPath relative or absolute path. Current path is used to calculate new path
   * @retval true succes
   * @retval false fail
   */
  bool ChangeDir(const t_string dirPath);

  static void SetCWD(const Pathname &pathname);
#ifdef _MT_ELES
  static Pathname GetCWD();
#else
  static const Pathname& GetCWD();
#endif

protected:
  ///Function rebuild buffer with new values
  /** This protected function recalculates space for buffer, allocated it, and rebuild its
  content with data supplied by arguments. Function doesn't assumes, that all strings are 
  terminated by zero by default. "pathlen, titlelen and  extlen" must contain correct values. 
  Terminating zero is not included, but function excepting it.
  Valid using is: RebuildData(a,b,c,_tcslen(a),_tcslen(b),_tcslen(c));
  All pointers returned by Get functions can be used and stays valid, until this function returns.
  */
  void RebuildData(const t_string path, const t_string filetitle, const t_string extension, int pathlen, int titlelen, int extlen);
  ///Function only rebuild _fullpath string.
  /** It doesn't check space for string! This function is used, when length of path is excepted
  the same or smaller, then current.
  */
  void RebuildPath();

  ///Alocates extra space for some results. 
  /* Space is allocated behind buffer in extra block.
  If there is enought space in extra block, no allocation is performed
  */
  void *AllocateExtraSpace(size_t sz) const
  {
    return const_cast<Pathname *>(this)->AllocateExtraSpace(sz);
  }
  void *AllocateExtraSpace(size_t size);


};


#undef PathNameCompare
#undef t_string 
#undef t_char

#endif // !defined(AFX_Pathname_H__40F41C23_3AA2_486C_B9E5_33AEE67FB313__INCLUDED_)
