// Pathname.cpp: implementation of the Pathname class.
//
//////////////////////////////////////////////////////////////////////

#include <El/elementpch.hpp>
#include <malloc.h>
#include <windows.h>
#include <stdio.h>
#include <shlobj.h>
#include <io.h>
#include <assert.h>
#include "Pathname.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#pragma comment(lib,"User32")
#pragma comment(lib,"shell32")

#define t_string _TCHAR *
#define t_char _TCHAR 

static Pathname cwd;
#ifdef _MT_ELES
#include <el/MultiThread/CriticalSection.h>
static MultiThread::MiniCriticalSection cwdLock;
#endif

Pathname::Pathname(const t_string name)
{
  _fullpath=0;
  _path=_filetitle=_extension=_end=_T("");

  if (name==0) SetPathName(_T("*.*"));
  else SetPathName(name);
}

Pathname::Pathname(PathNameNullEnum null)
{
  _fullpath=0;
  _path=_filetitle=_extension=_end=_T("");
}


Pathname::Pathname(const t_string relpath, const Pathname &abspath)
{
  _path=_filetitle=_extension=_end=_T("");
  _fullpath=0;

  t_string part;
  t_string pth=_tcscpy((t_string )alloca(sizeof(*relpath)*(_tcslen(relpath)+1)),relpath);
  part=_tcsrchr(pth,'\\');  
  if (part) part++;else part=0;
  if (part)
  {
    SetFilename(part);
    *part=0;
    SetDirectory(pth);
  }
  else
    SetFilename(pth);
  if (RelativeToFull(abspath)==false) SetPathName(relpath);
}


Pathname::~Pathname()
{
  delete [] _fullpath;
}

const t_string Pathname::GetNameFromPath(const t_string path)
{
  if (path==0) return _T("");
  const t_string c=_tcsrchr(path,'\\');
  if (c!=0) c++;else return path;
  if (c==0) c=_T("");
  return c;
}

const t_string Pathname::GetExtensionFromPath(const t_string path)
{
  if (path==0) return _T("");
  const t_string fname=GetNameFromPath(path);  
  if (fname==0) return _T("");
  const t_string c=_tcsrchr(fname,'.');
  if (c==0) c=_tcsrchr(path,0);
  return c;
}

void Pathname::RebuildData(const t_string path, const t_string filetitle, const t_string extension, int pathlen, int titlelen, int extlen)
{
  int totalsize=(pathlen+titlelen+extlen)*2+10;
  t_string olddata=_fullpath;
  _fullpath=new t_char[totalsize];
  _path=_fullpath+pathlen+titlelen+extlen+1;
  memcpy(_path,path,(pathlen+1)*sizeof(t_char));
  _filetitle=_path+pathlen+1;
  memcpy(_filetitle,filetitle,(titlelen+1)*sizeof(t_char));
  _extension=_filetitle+titlelen+1;
  memcpy(_extension,extension,(extlen+1)*sizeof(t_char));
  _end=_extension+extlen+1;
  RebuildPath();
  delete [] olddata;
}

void Pathname::RebuildPath()
{
  int lp=_tcslen(_path);
  int lf=_tcslen(_filetitle);
  int le=_tcslen(_extension);
  int diff=(lp+lf+le+1)-(_path-_fullpath);
  if (diff>0)
  {
	memmove(_path+diff,_path,lp+le+lf+3);
	_extension+=diff;
	_filetitle+=diff;
	_path+=diff;
  }
  memcpy(_fullpath,_path,lp*sizeof(t_char));
  memcpy(_fullpath+lp,_filetitle,lf*sizeof(t_char));
  memcpy(_fullpath+lp+lf,_extension,(le+1)*sizeof(t_char));
}

void Pathname::SetDrive(const t_char dr)
{
  if (HasDrive())
  {
    if (dr==0)
    {
      _tcscpy(_path,_path+2);
      _tcscpy(_fullpath,_fullpath+2);
    }
    else
    {
      _path[0]=dr;
      _fullpath[0]=dr;
    }
  }
  else if (dr!=0)
  {        
    int np=IsNetworkPath();
    if (np)
    {
      _path[0]=dr;
      _path[1]=':';
      _tcscpy(_path+2,_path+np);
      RebuildPath();
    }
    else
    {
      t_string c=(t_string )alloca((_tcslen(_path)+4)*sizeof(*c));          

      _stprintf(c,_T("%c:%s"),dr,_path);
      SetDirectory(c);
    }
  }
}

void Pathname::SetDirectory(const t_string dir)
{  
  bool copydrv;       //directory doesn't contain drive, need copy from original
  bool addslash;      //directory doesn't ending by backslash, need add it

  int len=_tcslen(dir);
  copydrv=HasDrive() && !HasDrive(dir); //copy original drive, if exists and directory doesn't contaion drive
  if (_tcsncmp(dir,_T("\\\\"),2)==0) copydrv=false; //network path, don't copy drive
  addslash=len && dir[len-1]!='\\'; //add slash
  if (addslash || copydrv)
  {
    t_string c=(t_string )alloca((len+4)*sizeof(dir[0])); //allocate some space for string
    if (addslash && copydrv)
      _stprintf(c,_T("%c%c%s\\"),_path[0],_path[1],dir);  //add drive and add slash
    else if (addslash)
      _stprintf(c,_T("%s\\"),dir);                        //add slash only
    else 
      _stprintf(c,_T("%c%c%s"),_path[0],_path[1],dir);    //add drive only
    dir=c;                    //this is new path for now
    len=_tcslen(dir);
  }
  if (len<_filetitle-_path)   //there is space for store path
  {_tcscpy(_path,dir); RebuildPath();} //store it and rebuild result
  else    
    RebuildData(dir,_filetitle,_extension,len,_tcslen(_filetitle),_tcslen(_extension));
  //rebuild internal data complettly
}

void Pathname::SetFilename(const t_string filename)
{
  const t_string dot=_tcsrchr(filename,'.');
  if (dot==0)
  {
    SetFiletitle(filename);
    SetExtension(_T(""));
    return;
  }
  int tllen=dot-filename;
  int exlen=_tcslen(dot);
  t_string c=(t_string )alloca((tllen+1)*sizeof(*c));
  memcpy(c,filename,tllen*sizeof(t_char));
  c[tllen]=0;
  if (exlen+tllen+1<_end-_filetitle)
  {
    memcpy(_filetitle,c,(tllen+1)*sizeof(t_char));
    _extension=_filetitle+tllen+1;
    memcpy(_extension,dot,(exlen+1)*sizeof(t_char));
    RebuildPath();
  }
  else
    RebuildData(_path,c,dot,_tcslen(_path),tllen,exlen);
}

void Pathname::SetExtension(const t_string ext)
{
  int len=_tcslen(ext);
  if (ext[0] && ext[0]!='.')
  {
    t_string s=(t_string )alloca((len+2)*sizeof(*s));
    _stprintf(s,_T(".%s"),ext);
    ext=s;
    len++;
  }
  if (len<_end-_extension)
  {
    memcpy(_extension,ext,(len+1)*sizeof(t_char));
    RebuildPath();
  }
  else
  {
    RebuildData(_path,_filetitle,ext,_tcslen(_path),_tcslen(_filetitle),len);
  }
}

void Pathname::SetFiletitle(const t_string title)
{
  int len=_tcslen(title);
  if (len<_extension-_filetitle)
  {
    memcpy(_filetitle,title,(len+1)*sizeof(t_char));
    RebuildPath();
  }
  else
  {
    RebuildData(_path,title,_extension,_tcslen(_path),len,_tcslen(_extension));
  }
}

void Pathname::SetPathName(const t_string pathname)
{
  if (pathname==0 || pathname[0]==0) 
  {
    SetNull();
    return;
  }
  t_string part;
  DWORD needsz=GetFullPathName(pathname,0,0,&part);
  t_string fpth=(t_string )alloca(needsz*sizeof(*fpth));
  GetFullPathName(pathname,needsz,fpth,&part);
  part=_tcsrchr(fpth,'\\');
  if (part) part++;else part=0;
  if (part)
  {
    SetFilename(part);
    *part=0;
  }
  else
    SetFilename(_T(""));
  SetDirectory(fpth);
}

Pathname& Pathname::operator=(const Pathname& other)
{
  if (other.IsNull()) SetNull();
  else RebuildData(other._path,other._filetitle,other._extension,_tcslen(other._path),_tcslen(other._filetitle),_tcslen(other._extension));
  return *this;
}

Pathname::Pathname(const Pathname &other)
{
  _fullpath=0;
  if (other.IsNull()) SetNull();
  else RebuildData(other._path,other._filetitle,other._extension,_tcslen(other._path),_tcslen(other._filetitle),_tcslen(other._extension));  
}

bool Pathname::FullToRelative(const Pathname &relativeto)
{
  if (relativeto.IsNull() || IsNull()) return false;
  bool h1=HasDrive();
  bool h2=relativeto.HasDrive();
  if (h1!=h2) return false;         //rozdilny zpusob adresace - nelze vytvorit relatvni cestu
  if (h1==true && h2==true && toupper(GetDrive())!=toupper(relativeto.GetDrive()))
    return false;       //ruzne disky, nelze vytvorit relativni cestu
  if (_tcsncmp(_path,_T("\\\\"),2)==0) //sitova cesta
  {
    int slsh=0;           //citac lomitek
    const t_string a=_path;
    const t_string b=relativeto._path;
    while (toupper(*a)==toupper(*b) && *a && slsh<3)  //zacatek sitove cesty musi byt stejny
    {
      if (*a=='\\') slsh++;
      a++;b++;
    }
    if (slsh!=3) return false;      //pokud neni stejny, nelze vytvorit relativni cestu
  }
  int sublevel=0;
  const t_string ps1=_path;
  const t_string ps2=relativeto._path;
  if (h1) 
  {ps1+=2;ps2+=2;}
  const t_string sls=ps2;
  while (toupper(*ps1)==toupper(*ps2) && *ps1) 
  {
    if (*ps2=='\\') sls=ps2+1;
    ps1++;ps2++;
  }
  ps1-=ps2-sls;
  if (sls)
  {    
    while (sls=_tcschr(sls,'\\'))
    {
      sls++;
      sublevel++;
    }
  }
  t_string buff=(t_string )alloca((sublevel*3+_tcslen(ps1)+1)*sizeof(*buff));
  t_string pos=buff;
  for (int i=0;i<sublevel;i++) 
  {_tcscpy(pos,_T("..\\"));pos+=3;}
  _tcscpy(pos,ps1);
  SetDrive(0);
  SetDirectory(buff);
  return true;
}

bool Pathname::RelativeToFull(const Pathname &ref)
{
  if (ref.IsNull() || IsNull()) return false;  
  const t_string beg;
  if (HasDrive())
    if (toupper(GetDrive())!=toupper(ref.GetDrive())) return false;
    else beg=_path+2;
  else beg=_path;
  const t_string end=_tcschr(ref._path,0);  
  if (beg[0]=='\\')
  {
    int np;
    if (ref.HasDrive()) end=ref._path+2;
    else  if (np=ref.IsNetworkPath()) end=ref._path+np;
    else end=ref._path;
  }
  else while (_tcsncmp(beg,_T("..\\"),3)==0 || _tcsncmp(beg,_T(".\\"),2)==0)
  {
    if (beg[1]=='.')
    {
      if (end>ref._path)
      {
        end--;
        while (end>ref._path && end[-1]!='\\') end--;
      }      
      beg+=3;
    }
    else 
      beg+=2;
  }
  int partln=end-ref._path;
  t_string buff=(t_string )alloca((partln+_tcslen(beg)+1)*sizeof(*buff));
  memcpy(buff,ref._path,partln*sizeof(t_char));
  _tcscpy(buff+partln,beg);
  SetDrive(0);
  SetDirectory(buff);
  return true;
}

int Pathname::IsNetworkPath() const
{
  if (_tcsncmp(_path,_T("\\\\"),2)==0) //sitova cesta
  {
    const t_string p=_path+2;
    const t_string c=_tcschr(p,'\\');
    if (c) return c-_path;
  }
  return 0;
}

void Pathname::SetServerName(const t_string server)
{
  if (HasDrive()) SetDrive(0);
  else
  {
    int np=IsNetworkPath();
    if (np) _tcscpy(_path,_path+np); //str
  }
  t_string buff=(t_string )alloca((_tcslen(server)+_tcslen(_path)+5)*sizeof(*buff));
  if (_path[0]!='\\')
    _stprintf(buff,_T("\\\\%s\\%s"),server,_path);
  else
    _stprintf(buff,_T("\\\\%s%s"),server,_path);
  SetDirectory(buff);
}

void Pathname::SetNull()
{
  delete [] _fullpath;
  _fullpath=0;
  _path=_filetitle=_extension=_end=_T("");
}

bool Pathname::GetPartFromPath(const t_string path, int partnum, t_string buff, int bufsize, int mode)
{
  const t_string scan=path;
  while (*scan=='\\') scan++;
  while (partnum && *scan)
  {
    while (*scan!='\\' && *scan) scan++;
    while (*scan=='\\') scan++;
    partnum--;
  }
  if (*scan==0) 
  {
    buff[0]=0;
    return false;
  }
  int pt=0;
  if (mode==-1)
  {
    pt=scan-path;
    if (pt>bufsize)
    {
      buff[0]=0;
      return true;
    }
    else           
      memcpy(buff,path,pt*sizeof(t_char));
  }
  bool nlast=false;
  while (*scan && (mode==1 || !nlast) && pt<bufsize)
  {
    buff[pt]=*scan;
    pt++;
    scan++;
    if (*scan=='\\') nlast=true;
  }
  if (pt==bufsize)
  {
    buff[0]=0;
    return true;
  }
  buff[pt]=0;
  return nlast;
}



bool Pathname::GetDirectoryWithDriveWLBS(t_string buff, size_t size) const
{
  size_t psize=_tcslen(GetDirectoryWithDrive());
  if (psize>size) return false;
  if (psize==0) {buff[0]=0;return true;}
  _tcsncpy(buff,GetDirectoryWithDrive(),psize-1);
  buff[psize-1]=0;
  return true;
}

const t_string Pathname::GetDirectoryWithDriveWLBS() const
{
  size_t size=_tcslen(GetDirectoryWithDrive());
  t_string buff=reinterpret_cast<t_string>(AllocateExtraSpace(size*sizeof(t_char)));
  GetDirectoryWithDriveWLBS(buff,size);
  return buff;
}

const t_string Pathname::GetDirectoryFullPath() const
{
  const t_string dir=GetDirectory();
  int offset=IsNetworkPath();
  dir+=offset;
  if (_tcsicmp(dir,_T("\\"))==0 || dir[0]==0) return GetDirectoryWithDrive();
  else return GetDirectoryWithDriveWLBS();
}

bool Pathname::IsPathValid() const
{
  if (IsNull()) return false;  
  t_string invalidChars=_T("/*?\"<>|");
  const t_string path=GetFullPath();
  if (*path==0) return false;
  while (*path)
  {
    if (_tcschr(invalidChars,*path)!=0) return false;
    path++;
  }
  return true;
}



bool Pathname::SetTempDirectory()
{
  t_char buff[1];
  DWORD size=GetTempPath(1,buff);
  if (size==0) return false;
  size++;
  t_string p=(t_string )alloca(size*sizeof(t_char));
  if (GetTempPath(size,p)==0) return false;
  SetDirectory(p);
  return true;
}

bool Pathname::SetDirectorySpecial(int nSpecCode)
{
  t_char buff[MAX_PATH];
  if (SHGetSpecialFolderPath(GetForegroundWindow(),buff,nSpecCode,FALSE)==FALSE) return false;
  SetDirectory(buff);
  return true;
}

bool Pathname::SetTempFile(const t_string prefix, unsigned int unique)
{
  t_char tempname[MAX_PATH];
  if (GetTempFileName(GetDirectoryWithDrive(),prefix,unique,tempname)==0) return false;
  this->SetPathName(tempname);
  return true;
}

Pathname Pathname::GetExePath(const void *module)
{
  t_char buff[MAX_PATH*4];
  GetModuleFileName(module?*reinterpret_cast<const HMODULE *>(module):0,buff,sizeof(buff));
  return Pathname(buff);
}

const t_string Pathname::FullToRelativeProjectRoot(const t_string full, const t_string projectRoot)
{
  const t_string a=full,*b=projectRoot;
  while (*a && tolower(*a)==tolower(*b)) {a++;b++;};
  if (*b) return full;
  return a;
}

bool Pathname::CreateFolder(void *security_descriptor)
{
  int begpart=-1;
  int len=_tcslen(_fullpath)+1;
  t_string buff=(t_string )alloca(len*sizeof(t_char));
  for (int i=1;GetPart(i,buff,len,-1);i++)
  {
    if (begpart==-1 && _taccess(buff,0)!=0) begpart=i;
    if (begpart!=-1)
    {
      if (begpart==-1) begpart=i;
      BOOL res=CreateDirectory(buff,(LPSECURITY_ATTRIBUTES)security_descriptor);
      if (res==FALSE)
      {
        for (;i>=begpart;i--)
        {
          GetPart(i,buff,len,-1);
          RemoveDirectory(buff);
        }
        return false;
      }
    }
  }
  return true;
}

bool Pathname::CreateFolder(const t_string path, void *security_descriptor)
{
  Pathname pth;
  pth.SetDirectory(path);
  return pth.CreateFolder(security_descriptor);
}

static bool RemoveDirectoryFull(const Pathname &p, int part, t_string buff, int bufsize)
{  
  if (p.GetPart(part+1,buff,bufsize,-1))
    if (RemoveDirectoryFull(p,part+1,buff,bufsize)==false) return false;
  p.GetPart(part,buff,bufsize, -1);
  return RemoveDirectory(buff)!=FALSE;
}

static bool RemoveDirRecursive(const t_string dir, const t_string mask)
{
  if (dir==0 || dir[0]==0) return false;
  if (_tcscmp(dir,_T("\\"))==0) return false;
  bool res=true;
  Pathname newp;
  newp.SetDirectory(dir);
  if (mask)
  {
    WIN32_FIND_DATA fnd;
    HANDLE h;
    newp.SetFilename(mask);
    h=FindFirstFile(newp,&fnd);
    if (h) 
    {
      do
      {
        if (!(fnd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
          newp.SetFilename(fnd.cFileName);
          if (DeleteFile(newp)==FALSE) res=false;
        }
      }while (FindNextFile(h,&fnd));
      CloseHandle(h);
    }
  }
  {
    WIN32_FIND_DATA fnd;
    HANDLE h;
    newp.SetFilename(_T("*.*"));
    h=FindFirstFile(newp,&fnd);
    if (h) 
    {
      do
      {
        if ((fnd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && _tcscmp(fnd.cFileName,_T("."))!=0 && _tcscmp(fnd.cFileName,_T(".."))!=0)
        {
          newp.SetFilename(fnd.cFileName);
          if (RemoveDirRecursive(newp,mask)==false) res=false;
          else DeleteFile(newp);
        }
      }while (FindNextFile(h,&fnd));
      CloseHandle(h);
    }
  }
  return res;

}

bool Pathname::DeleteFolder(int dfFlags)
{
  int bufflen=_tcslen(_fullpath)+1;
  t_string buff=(t_string )alloca(bufflen*sizeof(t_char));
/*  if (dfFlags & DFRecycleBin)
  {
    GetDirectoryWithDriveWLBS(buff,bufflen);
    SHFILEOPSTRUCT delinfo;
    delinfo.hwnd=0;
    delinfo.wFunc=FO_DELETE;
    delinfo.pFrom=GetFullPath();
    delinfo.pTo=0;
    delinfo.fFlags=FOF_ALLOWUNDO|FOF_NOCONFIRMATION|FOF_NOERRORUI|((dfFlags & DFRecursive)?0:FOF_NORECURSION)|
      ((dfFlags & DFShowProgress)?0:FOF_SILENT);
    delinfo.fAnyOperationsAborted=0;
    delinfo.hNameMappings=0;
    delinfo.lpszProgressTitle=0;

  }
  else*/
  {
    if (dfFlags & DFRecursive)
    {
      bool res=RemoveDirRecursive(GetDirectoryWithDrive(),dfFlags & DFFile?GetFilename():0);
      if (res==false) return false;
    }
    if (dfFlags & DFPath)
    {
      if (GetPart(1,buff,bufflen,-1)==false) return false;
      return RemoveDirectoryFull(*this, 1,buff,bufflen);
    }
    else
    {
      GetDirectoryWithDriveWLBS(buff,bufflen);
      return RemoveDirectory(buff)!=FALSE;
    }    

  }
}

bool Pathname::TestFile(int testFlags) const
{
  return _taccess(GetFullPath(),testFlags)==0;
}

bool Pathname::TestFile(const t_string name, int testFlags)
{
  return _taccess(name,testFlags)==0;
}


bool Pathname::GetFileStat(struct _stat &result) const
{
  return _tstat(GetFullPath(),&result)==0;
}


#if _MSC_VER>1200
bool Pathname::GetFileStat64(struct __stat64 &result) const
{
  return _tstat64(GetFullPath(),&result)==0;
}
#endif

#ifdef _UNICODE
const char *Pathname::ToSBS(const t_string text) const
{
  if (IsNull()) return 0;
  if (text==0) return 0;
  t_string cpy=_tcscpy((t_string )alloca(sizeof(t_char)*(_tcslen(text)+1)),text);
  size_t size=_tcslen(text)+1;
  char *buff=reinterpret_cast<char *>(AllocateExtraSpace(size*sizeof(char)));
  WideCharToMultiByte(CP_THREAD_ACP,0,cpy,size,buff,size,NULL,NULL);
  return buff;
}
#else
const char *Pathname::ToSBS(const t_string text) const
{
  return text;
}
#endif


bool Pathname::SearchTestExistence::operator() (const t_string file) const
  {return _taccess(file,0)==0;}

void *Pathname::AllocateExtraSpace(size_t size)
{
  t_char *textend=_tcschr(_extension,0)+1;
  size_t remain=(_end-textend)*sizeof(*_end);
  size_t cursize=(textend-_fullpath)*sizeof(*_end);
  if (remain<size)
  {
    t_char *nw=new t_char[(cursize+size+sizeof(t_char)-1)/sizeof(t_char)];
    memcpy(nw,_fullpath,cursize);
    size_t offset=nw-_fullpath;;
    _path+=offset;
    _filetitle+=offset;
    _extension+=offset;
    textend+=offset;
    _end=nw+(cursize+size+1)/sizeof(t_char);
    delete [] _fullpath;
    _fullpath=nw;    
  }
  return textend;
}

bool Pathname::SearchPaths(const t_string path, const t_string filename)
{
  t_char buffer[5000];
  LPTSTR part;
  
  if (::SearchPath(path,filename,0,5000,buffer,&part)==0) return false;
  else
  {
    operator=(buffer);
    return true;
  }

}

#ifndef CSIDL_PROGRAM_FILES 
#define CSIDL_PROGRAM_FILES 0x0026
#endif

#ifndef CSIDL_LOCAL_APPDATA
#define CSIDL_LOCAL_APPDATA 0x001c
#endif

#ifndef CSIDL_COMMON_APPDATA
#define CSIDL_COMMON_APPDATA 0x0023
#endif

#ifndef CSIDL_COMMON_DOCUMENTS
#define CSIDL_COMMON_DOCUMENTS 0x002e
#endif

bool Pathname::SetDirHome()
{
  return SetDirectorySpecial(CSIDL_PERSONAL);
}
bool Pathname::SetDirBin()
{
  return SetDirectorySpecial(CSIDL_PROGRAM_FILES);
}
bool Pathname::SetDirAppData()
{
  return SetDirectorySpecial(CSIDL_APPDATA);
}
bool Pathname::SetDirLocalAppData()
{
  return SetDirectorySpecial(CSIDL_LOCAL_APPDATA);
}
bool Pathname::SetDirAllUsersAppData()
{
  return SetDirectorySpecial(CSIDL_COMMON_APPDATA);
}
bool Pathname::SetDirAllUsersHome()
{
  return SetDirectorySpecial(CSIDL_COMMON_DOCUMENTS);
}
bool Pathname::SetDirAllApps()
{
  return SetDirectorySpecial(CSIDL_STARTMENU);
}
bool Pathname::ChangeDir(const t_string dirPath)
{
  Pathname nw(*this);
  nw.SetDirectory(dirPath);
  nw.RelativeToFull(*this);
    *this=nw;
    return true;
}

#ifdef _MT_ELES
class CWDLock
{
public:
  CWDLock() {cwdLock.Lock();}
  ~CWDLock() {cwdLock.Unlock();}
};
#else
class CWDLock
{
public:
  CWDLock() {}
  ~CWDLock() {}
};
#endif

void Pathname::SetCWD(const Pathname &pathname)
{
  CWDLock lk;
  cwd=pathname;
}
#ifdef _MT_ELES
Pathname Pathname::GetCWD()
{
  CWDLock lk;
  return cwd;
}
#else
const Pathname& Pathname::GetCWD()
{
  return cwd;
}
#endif

bool Pathname::ChangeRoot(const t_string oldRoot, const t_string newRoot)
{
  if (oldRoot==0 || newRoot==0) return false;
  if (oldRoot[0])
  {
    const t_string removeold=FullToRelativeProjectRoot(*this,oldRoot);
    if (removeold==*this) return false;
    SetFilename(_tcscpy((t_string)alloca(sizeof(t_char)*(_tcslen(removeold)+1)),removeold));
    SetDirectory(newRoot);
    SetPathName(GetFullPath());
    return true;    
  }
  else
  {
    const t_string removeold=*this;
    SetFilename(_tcscpy((t_string)alloca(sizeof(t_char)*(_tcslen(removeold)+1)),removeold));
    SetDirectory(newRoot);
    SetPathName(GetFullPath());
    return true;    
  }
}


const char *Pathname::SBS_GetDirectoryWithDriveWLBS() const
{
  return ToSBS(GetDirectoryWithDriveWLBS());
}
