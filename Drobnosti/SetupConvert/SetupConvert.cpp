// SetupConvert.cpp : Defines the entry point for the console application.
//

#include <El/elementpch.hpp>
#include <Es/Common/win.h>

#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <io.h>
#include <direct.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#include <Es/Framework/debugLog.hpp>
#include <Es/Common/fltOpts.hpp>
#include <Es/Memory/fastAlloc.hpp>
#include <Es/Types/pointers.hpp>
#include <Es/Containers/array.hpp>

#include <El/QStream/fileinfo.h>
#include <El/QStream/qstream.hpp>
#include <El/QStream/qbstream.hpp>
#include <El/QStream/fileMapping.hpp>

#include "packFilesExt.hpp"
#include "setupConvertShare.hpp"
/*
// This part is taken from BankRev.cpp and is intended to be used for reading from pbo
TypeIsSimple(FileInfo);

struct SaveContext
{
  QFBank *bank;
  RString folder;
};

//! create path to given directory
static void CreatePath(char *path)
{
  // string will be changed temporary
  char *end = path;
  while (end = strchr(end, '\\'))
  {
    *end = 0;
    mkdir(path);
    *end = '\\';
    end++;
  }
}

static void SaveFile(const FileInfoO &fi, const FileBankType *files, void *context)
{
  SaveContext *sc = (SaveContext *)context;
  // save given file
  QIFStreamB file;
  file.open(*sc->bank,fi.name);
  // note: file may be compressed - auto handled by bank

  RString outFilename=sc->folder+RString("\\")+RString(fi.name);
  // note file name may contain subfolder
  char folderName[1024];
  strcpy(folderName,outFilename);
  *GetFilenameExt(folderName) = 0;
  if (strlen(folderName)>0)
  {
    if (folderName[strlen(folderName)-1]='\\') folderName[strlen(folderName)-1]=0;
    // FIX: create whole path (not only topmost directory)
    CreatePath(folderName);
    mkdir(folderName);
  }
  // 
  //	FILE *tgt = fopen(outFilename,"wb");
  //	if (!tgt) return;

  //	setvbuf(tgt,NULL,_IOFBF,256*1024);
  QOFStream out(outFilename);
  file.copy(out);
  out.close();
  //  fwrite(file.act(),file.rest(),1,tgt);

  //	fclose(tgt);
}

int main( int argc, const char *argv[] )
{
#define FILE_BUF_SIZE 256*1024
  Temp<char> fileBuf(FILE_BUF_SIZE);

  while( --argc>0 )
  {
    // use bank access functions provided by QIFStreamB 

    // enable bank.pbo as argument
    char folderName[1024];
    strcpy(folderName, *++argv);
    *GetFileExt(folderName) = 0;

    RString folder=folderName;
    RString source=folder;

    QFBank bank;
    // enumerate files in bank

    bank.open(source);

    // create a destination folder
    if (mkdir(source)==0)
    {
      printf("Created %s\n",(const char *)source);
    }

    SaveContext saveContext;
    saveContext.bank = &bank;
    saveContext.folder = folder;
    bank.ForEach(SaveFile,&saveContext);


  }
  return 0;
}
*/

/// Better commandLine LogF
#include "Es/Framework/appFrame.hpp"
class MyAppFrameFunctions : public AppFrameFunctions
{
public:
//   MyAppFrameFunctions() {};
//   virtual ~MyAppFrameFunctions() {};

  void LogF(const char *format, va_list argptr);
};

void MyAppFrameFunctions::LogF(const char *format, va_list argptr)
{
  BString<512> buf;
  vsprintf(buf,format,argptr);
  strcat(buf,"\n");
#ifdef _WIN32
#ifdef UNICODE
  WCHAR wBuf[512];
  MultiByteToWideChar(CP_ACP, 0, buf, -1, wBuf, lenof(wBuf));
  OutputDebugString(wBuf);
#else
  OutputDebugString(buf);
#endif // !UNICODE
  fputs(buf,stderr);
#else
  fputs(buf,stderr);
#endif
}

#if _MSC_VER && !defined INIT_SEG_COMPILER
#pragma warning(disable:4074)
#pragma init_seg(compiler)
#define INIT_SEG_COMPILER
#endif

static MyAppFrameFunctions GAppFrameFunctions INIT_PRIORITY_URGENT;
AppFrameFunctions *CurrentAppFrameFunctions = &GAppFrameFunctions;

///

class SetupFolder
{
  RString path;
public:
  SetupFolder() {};
  bool Init(RString pathToSetup);
  bool FileExist(RString filename);
  const char *GetPath() const {return cc_cast(path);}
  RString GetFilePath(const char *filename) const {return path + "\\" + filename;}
  int GetHeadLen() const { return path.GetLength()+1; }
};

bool SetupFolder::Init(RString pathToSetup)
{
  for (int i=pathToSetup.GetLength()-1; i>0; i--)
  {
    if (isspace(pathToSetup[i])) continue;
    if (pathToSetup[i]=='\\') {char *data = pathToSetup.MutableData(); data[i]=0; break;}
    if (i<pathToSetup.GetLength()-1) {char *data = pathToSetup.MutableData(); data[i+1]=0;}
    break;
  }
  path = pathToSetup;
  WIN32_FIND_DATA info;
  HANDLE h = FindFirstFile(cc_cast(path), &info);
  if (h != INVALID_HANDLE_VALUE &&
     (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) return true;
  return false;
}

bool SetupFolder::FileExist(RString filename)
{
  RString filePath = path + "\\" + filename;
  int h = open(filePath,O_RDONLY|O_BINARY);
  if (h<0) return false;
  close(h);
  return true;
}

void DisplayErrorMessage()
{
  LPVOID lpMsgBuf;
  FormatMessage( 
    FORMAT_MESSAGE_ALLOCATE_BUFFER | 
    FORMAT_MESSAGE_FROM_SYSTEM | 
    FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,
    GetLastError(),
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
    (LPTSTR) &lpMsgBuf,
    0,
    NULL 
    );
  // Display the string.
  fprintf( stderr, "%s\n", (const char *)lpMsgBuf);
  // Free the buffer.
  LocalFree( lpMsgBuf );
}

extern bool wasError;
void FileToResource(int headlen,const char *name, HANDLE handle, bool useFileExtension)
{
  LPCSTR resType = RT_RCDATA;
  const char *txtInfo = "";
  if (useFileExtension)
  {
    if (stricmp(GetFileExt(name),".bmp")==0) { resType=RT_BITMAP; txtInfo = "(Bitmap)"; }
  }
  LogF("Add file %s %s",name+headlen, txtInfo);

  char nameRes[1024]; *nameRes=0;
  int size = 0;
  BOOL ok = FALSE;
  FILE *in = fopen(name, "rb");
  if (in!=NULL)
  {
    size = QIFileFunctions::GetFileSize(name);
    char *buffer = new char[size];
    if (!buffer)
    {
      fprintf(stderr,"Error: not enough memory to save file \"%s\" into resource.\n",name+headlen);
      fclose(in);
      return;
    }
    fread(buffer, sizeof(char), size, in);
    fclose(in);
    strcpy(nameRes,name+headlen);
    strupr(nameRes);
    ok = UpdateResource
      (
      handle, resType, nameRes,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
      buffer, size
      );
    delete buffer;
  }

  if (!ok)
  {
    // some file 
    HRESULT hr = GetLastError();

    RString errMsg="";
    LPVOID lpMsgBuf;
    if (FormatMessage( 
      FORMAT_MESSAGE_ALLOCATE_BUFFER | 
      FORMAT_MESSAGE_FROM_SYSTEM | 
      FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      GetLastError(),
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
      (LPTSTR) &lpMsgBuf,
      0,
      NULL ))
    {
      // Free the buffer.
      errMsg = (char *)lpMsgBuf;
      LocalFree( lpMsgBuf );
    }

    fprintf(stderr,"Error (%x) \"%s\" updating %s\n\tfirst package maybe too big\n",hr,cc_cast(errMsg), nameRes);
    exit(15);
  }
}

void FilesToResource(int headlen, const char *name, HANDLE handle, bool useFileExtension)
{
  char buffer[1024];
  sprintf(buffer, "%s\\*.*", name);

  WIN32_FIND_DATA info;
  HANDLE h = FindFirstFile(buffer, &info);
  if (h != INVALID_HANDLE_VALUE)
  {
    do
    {
      if ((info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
      {
        // FIX - do not delete only current and parent directory
        if (strcmp(info.cFileName, ".") && strcmp(info.cFileName, ".."))
        {
          sprintf(buffer, "%s\\%s", name, info.cFileName);
          FilesToResource(headlen,buffer, handle, useFileExtension);
        }
      }
      else
      {
        sprintf(buffer, "%s\\%s", name, info.cFileName);
        // Copy file to resource
        FileToResource(headlen,buffer,handle, useFileExtension);
      }
    }
    while (FindNextFile(h, &info));
    FindClose(h);
  }
}

static void MemoryToResource
(
 const void *buffer, size_t size, const char *name, HANDLE handle
)
{
  LogF("Add memory file %s",name);

  char nameRes[1024];
  strcpy(nameRes,name);
  strupr(nameRes);

  UpdateResource
    (
    handle, RT_RCDATA, nameRes,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
    (void *)buffer, size
    );
}

#define TEST_BANK_FROM_MEMORY 0
RString GetTempDirectory(RString appName)
{
  char buff[1];
  DWORD size=GetTempPath(1,buff);
  if (size==0) return RString();
  size++;
  RString out; out.CreateBuffer(size);
  if (GetTempPath(size,out.MutableData())==0) return RString("");
  return (out + appName + RString(" - Installer"));
}

class FileBufferFromMemory: public IFileBuffer
{
protected:
  const char *_data;
  int size;

public:
  FileBufferFromMemory(const char *dt, int sz) : _data(dt), size(sz) {}

  const char *GetData() const {return _data;}
  // non-virtual writable access
  //char *GetWritableData() {return _data;}
  bool PreloadBuffer(
    RequestableObject *obj, RequestableObject::RequestContext *ctx,
    FileRequestPriority priority, QFileSize pos, QFileSize size
    ) const
  {
    // data are always ready
    return true;
  }

  int GetSize() const {return size;}
  bool GetError() const {return false;}
  bool IsFromBank(QFBank *bank) const {return false;}
  bool IsReady() const {return true;}
};

struct SaveContext
{
  QFBank *bank;
  RString folder;
};

//! create path to given directory
static void CreatePath(char *path)
{
  // string will be changed temporary
  char *end = path;
  while (end = strchr(end, '\\'))
  {
    *end = 0;
    mkdir(path);
    *end = '\\';
    end++;
  }
}

static void SaveFile(const FileInfoO &fi, const FileBankType *files, void *context)
{
  SaveContext *sc = (SaveContext *)context;
  // save given file
  QIFStreamB file;
  file.open(*sc->bank,fi.name);
  // note: file may be compressed - auto handled by bank

  RString outFilename=sc->folder+RString("\\")+RString(fi.name);
  // note file name may contain subfolder
  char folderName[1024];
  strcpy(folderName,outFilename);
  *GetFilenameExt(folderName) = 0;
  if (strlen(folderName)>0)
  {
    if (folderName[strlen(folderName)-1]='\\') folderName[strlen(folderName)-1]=0;
    // FIX: create whole path (not only topmost directory)
    CreatePath(folderName);
    mkdir(folderName);
  }
  // 
  //	FILE *tgt = fopen(outFilename,"wb");
  //	if (!tgt) return;

  //	setvbuf(tgt,NULL,_IOFBF,256*1024);
  QOFStream out(outFilename);
  file.copy(out);
  out.close();
  //  fwrite(file.act(),file.rest(),1,tgt);

  //	fclose(tgt);
}

#include <extern/libpng/include/zlib.h>
int GetGzFileLength(RString pathRoot, FileInfoExt &finfo)
{
//If it is necessary to insert right unpacked sizes into filelist, change the COMPUTE_GZLEN
#define COMPUTE_GZLEN 0
#if COMPUTE_GZLEN
  const char *ext = GetFileExt(cc_cast(finfo.name));
  if (stricmp(ext, ".gz")==0)
  {
    RString sSrcFile = pathRoot + finfo.name;
    gzFile Source;
    if ( (Source = gzopen(sSrcFile, "rb"))==NULL)
    {
      ErrF("Error: cannot open gzipped file: %s", sSrcFile);
      return finfo.length;
    }
    int   nBytes;
    const int COPY_BUFF_SIZE = 256*1024;
    TCHAR buff[COPY_BUFF_SIZE];
    // get the output file size
    DWORD	nLen = 0;
    bool copyFinished = false;
    while (!copyFinished)
    {
      nBytes = gzread(Source, buff, COPY_BUFF_SIZE);
      if (nBytes>0) nLen += nBytes;
      else copyFinished = true;
    };
    gzclose(Source);
    return nLen;
  }
#endif
  return finfo.length; //TODO
}

void ExitWithUsage()
{
  printf("Usage: SetupConvert.exe [-one] path"
    "\n  Parameter 'path' is the path to the folder containg at least setup.dat and setup.exe."
    "\n  -one  can be used to produce only one large exe file"
    "\n  -size=XXXX max size in MB of each package"
    "\n  -first=XXXX max size in MB of first(exe) package"
    "\n  -name=name of output file \n\n");

  exit(1);
}

int main( int argc, const char *argv[] )
{
  if (argc<2) ExitWithUsage();
  SetupFolder inputSetupFolder;
  bool setupParOne = false; //true when only one large exe and no setupNN.bin should be generated
  bool folderInitDone = false;


  int setupMaxSize = 2000; // max size in MB
  RString setupOutputName = "setup";
  int setupFirstMaxSize = -1; // max size in MB
  RString forceFileInFirst;

  for (int i=1; i<argc; i++)
  {
    if (stricmp(argv[i],"-one")==0) setupParOne = true;
    else if ((strnicmp(argv[i],"-name=",6)==0) && strlen(argv[i])>6)
    {
      setupOutputName = (argv[i]+6);
    }
    else if ((strnicmp(argv[i],"-size=",6)==0) && strlen(argv[i])>6)
    {
      setupMaxSize = atoi(argv[i]+6);
    }
    else if ((strnicmp(argv[i],"-size",5)==0) && strlen(argv[i])>5)
    {
      setupMaxSize = atoi(argv[i]+5);
    }
    else if ((strnicmp(argv[i],"-first=",7)==0) && strlen(argv[i])>7) // better use "-first=100" then "-first100"
    {
      setupFirstMaxSize = atoi(argv[i]+7);
    }
    else if ((strnicmp(argv[i],"-first",6)==0) && strlen(argv[i])>6)
    {
      setupFirstMaxSize = atoi(argv[i]+6);
    }
    else if ((strnicmp(argv[i],"-forceFirst=",12)==0) && strlen(argv[i])>12)
    {
      forceFileInFirst = (argv[i]+12);
    }
    else 
    {
      if (!folderInitDone) 
      {
        inputSetupFolder.Init(argv[i]);
        folderInitDone = true;
      }
      else ExitWithUsage();
    }
  }
  if (setupFirstMaxSize == -1) setupFirstMaxSize = setupMaxSize;

  if (!folderInitDone) ExitWithUsage();
  //Test setup.exe and setup.dat presence inside inputSetupFolder
  const char _errMissingFileFmt[]="There is no file \"%s\" in the \"%s\" folder!\n\n";
  if (!inputSetupFolder.FileExist("setup.dat"))
  {
    printf(_errMissingFileFmt, "setup.dat", inputSetupFolder.GetPath());
    exit(3);
  }
  if (!inputSetupFolder.FileExist("setup.exe"))
  {
    printf(_errMissingFileFmt, "setup.exe", inputSetupFolder.GetPath());
    exit(4);
  }
  //Create output folder and copy setup.exe into output folder
  if (mkdir("setupConverted")==-1 && errno!=EEXIST)
  {
    printf("Error: cannot create setupConvert directory.\n\n");
    exit(2);
  }
  SetupFolder outputSetupFolder;
  outputSetupFolder.Init("setupConverted");
  CopyFile( inputSetupFolder.GetFilePath("setup.exe"),outputSetupFolder.GetFilePath(setupOutputName+".exe"),false);
  int initSize = QIFileFunctions::GetFileSize(inputSetupFolder.GetFilePath("setup.exe"));
  //insert setup.dat into setup.exe as resource
  HANDLE handle = BeginUpdateResource(outputSetupFolder.GetFilePath(setupOutputName+".exe"), FALSE);
  if (!handle)
  {
    DisplayErrorMessage();
    return 5;
  }
  //STAGE 1: insert setup.dat and setup.csv into setup.exe resources
  FileToResource(inputSetupFolder.GetHeadLen(), inputSetupFolder.GetFilePath("setup.dat"), handle, false);
  if (inputSetupFolder.FileExist("setup.dat.bisign")) FileToResource(inputSetupFolder.GetHeadLen(), inputSetupFolder.GetFilePath("setup.dat.bisign"), handle, false);
  FileToResource(inputSetupFolder.GetHeadLen(), inputSetupFolder.GetFilePath("setup.csv"), handle, false);
  if (inputSetupFolder.FileExist("setup.dat.bisign"))
    FileToResource(inputSetupFolder.GetHeadLen(), inputSetupFolder.GetFilePath("setup.dat.bisign"), handle, false);

  //STAGE 2: save all files from DATA into install_data.pbo
  //STAGE 3: add DirectX and openAL stuff into install_data.pbo and save this file into setup.exe RCDATE resources
  //FilesToResource(inputSetupFolder.GetHeadLen(), inputSetupFolder.GetFilePath("data"), handle, false);
  AutoArray<RString> logFile;
  logFile.Add(RString("Data\\*"));
  logFile.Add(RString("setup.bmp"));
  logFile.Add(RString("setup.crc"));
  if (!forceFileInFirst.IsEmpty())
  {
    logFile.Add(forceFileInFirst);
  }
  //logFile.Add(RString("DirectX\\*")); //this is too large, we will add it into separate folder
  QOStrStream out;
  FileBankManagerExt mgr;
  mgr.Create(out, cc_cast(inputSetupFolder.GetFilePath("")), logFile);
  MemoryToResource(out.str(), out.pcount(), "SETUP_3RD_PARTY_AND_DATA", handle);
  initSize += out.pcount();
#if TEST_BANK_FROM_MEMORY
  //test: unpack SETUP_3RD_PARTY_AND_DATA bank into temporary directory
  RString tmpDir = GetTempDirectory("Setup - install");
  QFBank testBank;
  Ref<FileBufferFromMemory> fileBuf = new FileBufferFromMemory(out.str(), out.pcount());
  SaveContext ctx;
  ctx.bank = &testBank;
  ctx.folder = tmpDir;
  testBank.open(fileBuf);
  testBank.ForEach(SaveFile,&ctx);
  testBank.close();
#endif
/*
  //test: save created pbo into file
  QOFStream testOut;
  testOut.open("testSetupBank.pbo");
  testOut.write(out.str(),out.pcount());
  testOut.close();
*/
  out.close();
  //directX
  {
    AutoArray<RString> logFile;
    //logFile.Add(RString("Data\\*"));
    logFile.Add(RString("DirectX\\*")); //this is too large, we will add it into separate folder
    //logFile.Add(RString("OpenALwEAX.exe"));
    QOStrStream out;
    FileBankManagerExt mgr;
    mgr.Create(out, cc_cast(inputSetupFolder.GetFilePath("")), logFile);
    MemoryToResource(out.str(), out.pcount(), "DIRECTX_INSTALLER", handle);
    initSize += out.pcount();
    out.close();
  }
  //STAGE 4: copy all setup files into exe resources and setupNN.bin files
  //FIRST - generate fileList, which would be written into resources too
  logFile.Clear();
  logFile.Add(RString("*"));
  AutoArray<RString> excludes(2);
  excludes.AddFast("Data\\*");
  excludes.AddFast("DirectX\\*");
  if (!forceFileInFirst.IsEmpty())
  {
    excludes.Add(forceFileInFirst);
  }
  //excludes.AddFast("openALwEAX.exe");
  FileBankManagerExt flistMgr;
  AutoArray<FileInfoExt> &finfolist = flistMgr.PrepareAndGetFileList
    (
      cc_cast(inputSetupFolder.GetFilePath("")), logFile, 
      excludes.Data(), excludes.Size()
    );
  AutoArray<ResFileItem> flist(finfolist.Size());
  const int MAX_FILE_NUM = 50;

  int MAX_FILE_SIZE = min(setupMaxSize*1024*1024,1900000000);
  int MAX_FIRST_FILE_SIZE = min(setupFirstMaxSize*1024*1024,1700000000);
                                 
  const int MAX_RES_FILE_LEN = /*20000000*/200000000;
  __int64 initLen[MAX_FILE_NUM+1]; for (int i=0; i<MAX_FILE_NUM+1; i++) initLen[i]=0;
  initLen[0] = initSize + 10000000; // init size + same space for filelist
  for (int i=0; i<finfolist.Size(); i++)
  {
    if (finfolist[i].name.IsEmpty())
      continue;
    int kam = 0;
    if (setupParOne) kam = 1;
    else
    { //first fit
      if ( (initLen[0] + finfolist[i].length <= MAX_FIRST_FILE_SIZE ) 
        && (finfolist[i].length <= MAX_RES_FILE_LEN) 
      ) kam = 0;
      else
      {
        kam = 1;
        while (initLen[kam] + finfolist[i].length > MAX_FILE_SIZE && kam < MAX_FILE_NUM) kam++;
      }
    }
    initLen[kam] += finfolist[i].length;
    int flen = GetGzFileLength(inputSetupFolder.GetFilePath(""), finfolist[i]);
    flist.Add(ResFileItem(finfolist[i].name, flen, kam));
  }
  char kam=0;
  for (int i=0; i<=MAX_FILE_NUM; i++)
  {
    if (initLen[i]==0) 
    {
      kam = i-1;
      break; 
    }
  }

  QOStrStream fileListOut;
  SerializeBinStream stream(&fileListOut);
  stream << flist;
  MemoryToResource(fileListOut.str(), fileListOut.pcount(), "SETUP_FILE_LIST", handle);
  //SECOND: save setup files into setup.exe resources or additional bank
  //a) save something into resources
#define MULTI_SETUP_RESOURCES 0
#if MULTI_SETUP_RESOURCES
  char *tmpbuf = new char[MAX_RES_FILE_LEN];
  if (!tmpbuf) 
  {
    printf("ERROR: not enough memory to create continuous %d block!\n\n",MAX_RES_FILE_LEN);
    exit(10);
  }
#endif
  for (int i=0; i<flist.Size(); i++)
  {
    if (flist[i].whereSaved==0) //this is scheduled to be saved into resources
    {
#if MULTI_SETUP_RESOURCES
      if (flist[i].len>MAX_RES_FILE_LEN)
      {
        char nameRes[1024];
        strcpy(nameRes,cc_cast(flist[i].name));
        strupr(nameRes);
        FILE *in = fopen(inputSetupFolder.GetFilePath(cc_cast(flist[i].name)),"rb");
        if (in)
        {
          int len=0;
          int curPart = 0;
          while ((len=fread(tmpbuf,sizeof(char),MAX_RES_FILE_LEN,in))>0)
          {
            RString resName;
            if (!curPart) resName = nameRes;
            else resName=Format("%s__PART_%02d", nameRes, curPart);
            MemoryToResource(tmpbuf,len,resName,handle);
            curPart++;
          }
          fclose(in);
        }
        else
        {
          printf("ERROR: Cannot open file %s!\n\n",cc_cast(inputSetupFolder.GetFilePath(cc_cast(flist[i].name))));
          exit(10);
        }
      }
      else 
#endif
      FileToResource(inputSetupFolder.GetHeadLen(), inputSetupFolder.GetFilePath(cc_cast(flist[i].name)), handle, false);
    }
  }
#if MULTI_SETUP_RESOURCES
  delete tmpbuf;
#endif
  if (handle)
  {
    LogF("*** Starting resource updating ***");
    BOOL ok = EndUpdateResource(handle, FALSE);
    LogF("*** Resources updated ***");
    handle = NULL;
    if (!ok)
    {
      HRESULT hr = GetLastError();
      fprintf(stderr,"Error %x updating resources.\n",hr);
      return 1;
    }
  }

  //b) save other into additional banks
  BankInfo bankInfo(kam); //used only for setupParOne
  for (int bankIx=1; bankIx<=kam; bankIx++)
  {
    AutoArray<RString> logFile;
    for (int i=0; i<flist.Size(); i++)
    {
      if (flist[i].whereSaved==bankIx)
      {
        logFile.Add(flist[i].name);
      }
    }
    if (setupParOne)
    { //we should append binFile to setup.exe
      RString setupExeOutFile = outputSetupFolder.GetFilePath("setup.exe");
      QFileSize oldSize = QIFileFunctions::GetFileSize(setupExeOutFile);
      QOFStream out;
      out.openForAppend(setupExeOutFile);
      FileBankManagerExt mgr;
      mgr.Create(out, cc_cast(inputSetupFolder.GetFilePath("")), logFile);
      out.close();
      QFileSize newSize = QIFileFunctions::GetFileSize(setupExeOutFile);
      QFileSize addSize = Convert64ToQFileSize(QFileSize64(newSize)-QFileSize64(oldSize));
      bankInfo.Add(addSize);
    }
    else
    {
      RString binFileName = Format(setupOutputName+"%02d.bin", bankIx);
      QOFStream out(outputSetupFolder.GetFilePath(cc_cast(binFileName)));
      FileBankManagerExt mgr;
      mgr.Create(out, cc_cast(inputSetupFolder.GetFilePath("")), logFile);
      out.close();
    }
  }
  if (setupParOne && kam>=1)
  {
    RString setupExeOutFile = outputSetupFolder.GetFilePath("setup.exe");
    QOFStream out;
    out.openForAppend(setupExeOutFile);
    LogF("Setup Convert %s\n", out.fail()? "FAILED" : "Processed");
    QOStrStream outStr;
    SerializeBinStream stream(&outStr);
    stream << bankInfo;
    int count = outStr.pcount();
    out.write(outStr.str(), count);
    out.write(&count, sizeof(int));
    out.close();
  }

  return 0;
}
