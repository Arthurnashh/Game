// patch.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Es/essencepch.hpp>

#include <float.h>
#include <direct.h>

#include <Es/Files/fileNames.hpp>
#include <Es/Strings/rString.hpp>

#include <io.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <Es/Framework/debugLog.hpp>
#include <Es/Common/fltopts.hpp>

#include "fileMap.hpp"
#include "patch.hpp"

#if _DEBUG
#define OUT_TEXT 1
#else
#define OUT_TEXT 0
#endif

#define BUFFER_SIZE 16*1024

char XDelta3TmpFile[MAX_PATH]; //global
const char XDelta3FileName[] = "xdelta3.exe"; //global

class CreatePatch: public IPatchAction
{
  FileMemMapped fileNew;
  FileMemMapped fileOld;

  //int _oldPos,_newPos; // position of identical part
  //int _identical,_oldSkip,_newSkip; // current change state

  int _samePos,_sameSize; // identical prepared to be saved
  int _diffPos,_diffSize; // different prepared to be saved

  FILE *fileDst;
  FILE *fileTxt;

  bool createNew; // based on target version, needed to recognize whether the file is new or not

  public:
  CreatePatch(bool setCreateNew);
  Result PerformByName
  (
    const char *tgtPath, const char *srcPath,const char *dataPath,
    bool removeOnError=true
  );
  Result Perform
  (
    const char *tgtPath, const char *srcPath,const char *dataPath,
    const char *name
  );
  void CleanUp();

  private:
  void SaveFromOld( int oldPos, int oldSize );
  void SaveFromNew( int newPos, int newSize );

  //void SaveIdentical( int identical, int newPos, int oldPos );
  //void SaveChange( int skipOld, int skipNew ); // buffered save

  void Flush();
  void ShowProgress( const char *name, int left );

  bool Delta3Diff(LPCSTR srcFullPath, LPCSTR dstFullPath, LPCSTR dtaFullPath);

  private:
  DWORD lastProgressTime;
  int lastProgressNum;
};

class CreateDelete: public IPatchAction
{
  FileMemMapped fileNew,fileOld;
  FILE *fileDst;

  public:
  CreateDelete();
  Result PerformByName
  (
    const char *tgtFullPath, const char *srcFullPath,const char *dtaFullPath,
    bool removeOnError=true
  );
  Result Perform
  (
    const char *tgtPath, const char *srcPath,const char *dataPath,
    const char *name
  );
  void CleanUp();
};

class ApplyPatch: public IPatchAction
{
  FileMemMapped fileOld,fileDiff;
  RString _errorMessage;
  AutoArray<RString> &_uninstalLog;
  bool _otherTargetDir;

  public:
  ApplyPatch(AutoArray<RString> &uninstalLog, bool otherTargetDir);
  Result PerformByName
  (
    const char *tgtFullPath, const char *srcFullPath,const char *dtaFullPath,
    bool removeOnError=true
  );
  Result Perform
  (
    const char *tgtPath, const char *srcPath,const char *dataPath,
    const char *name
  );
  void CleanUp();
  const char *GetErrorMessage() const {return _errorMessage;}
  Result Delta3Decrypt(LPCSTR in, LPCSTR diff, LPCSTR out);
};

enum // kind of operation
{
  DiffAdd,
  DiffChange,
  DiffRemove
};

CreateDelete::CreateDelete()
{
}

IPatchAction::Result CreateDelete::PerformByName
(
  const char *tgtFullPath, const char *srcFullPath,const char *dtaFullPath,
  bool removeOnError
)
{
  fileNew.Open(tgtFullPath);
  fileOld.Open(srcFullPath);

  if (fileOld.GetError())
  {
    fileDst = fopen(dtaFullPath,"wb");
    if (!fileDst)
    {
      printf("Cannot write %s\n",dtaFullPath);
      return Error;
    }
    // new file does not exist - it was deleted
    char op = DiffRemove;
    fwrite(&op,sizeof(op),1,fileDst);
    fclose(fileDst);
    fileDst = NULL;
    return OK;
  }

  CleanUp();

  return OK;
}

IPatchAction::Result CreateDelete::Perform
(
  const char *tgtPath, const char *srcPath,const char *dataPath,
  const char *name
)
{
  // delete: tgt = new, src = old, data = patch
  char tgtFullPath[1024];
  char srcFullPath[1024];
  char dtaFullPath[1024];
  strcpy(tgtFullPath,tgtPath);
  strcpy(srcFullPath,srcPath);
  strcpy(dtaFullPath,dataPath);
  strcat(tgtFullPath,name);
  strcat(srcFullPath,name);
  strcat(dtaFullPath,name);
  strcat(dtaFullPath,".upd");

  return PerformByName(tgtFullPath,srcFullPath,dtaFullPath);
}

void CreateDelete::CleanUp()
{
  fileNew.Close();
  fileOld.Close();
}

CreatePatch::CreatePatch(bool setCreateNew)
{
  fileDst = NULL;
  fileTxt = NULL;
  createNew = setCreateNew;
}

void CreatePatch::CleanUp()
{
  fileNew.Close();
  fileOld.Close();

  if (fileDst)
  {
    fclose(fileDst);
    fileDst = NULL;
  }
  #if OUT_TEXT
  if (fileTxt)
  {
    fclose(fileTxt);
    fileTxt = NULL;
  }
  #endif
}


void CreatePatch::ShowProgress( const char *name, int left )
{
  if (left>0)
  {
    if (lastProgressNum-left<256*1024)
    {
      DWORD time = GetTickCount();
      if (time-lastProgressTime<1000) return; // nothing to show
    }
  }
  lastProgressTime = GetTickCount();
  lastProgressNum = left;
  printf("%20s %10d KB\r",name,left/1024);
}

/// simulate fwrite
int fwrite(FileMapAccess data, int size, int count, FILE *file)
{
  for (int c=0; c<count; c++)
  {
    for (int s=0; s<size; s++)
    {
      char put = *data;
      data++;
      // in case of error return how much was fully written
      if (_fputc_nolock(put,file)==EOF) return c;
    }
  }
  return count;
}

void CreatePatch::Flush()
{
  if (_sameSize<=0 && _diffSize<=0)
  {
    // nothing to flush
    return;
  }
  //int _diffPos,_diffSize; // different prepared to be saved
  // save identical data

  FileMapAccess newData = fileNew.GetData();
  FileMapAccess oldData = fileOld.GetData();

  int newLen = fileNew.GetSize();
  int oldLen = fileOld.GetSize();

  // save identical information
  #if OUT_TEXT
    fprintf(fileTxt,"Old data %d size %d\r\n",_samePos,_sameSize);
  #endif
  fwrite(&_sameSize,sizeof(_sameSize),1,fileDst);
  if (_sameSize>0)
  {
    fwrite(&_samePos,sizeof(_samePos),1,fileDst);
  }
  // save information about change

  #if OUT_TEXT
    fprintf(fileTxt,"New data %d size %d\r\n",_diffPos,_diffSize);
  #endif
  // save new from newRead to bestN
  fwrite(&_diffSize,sizeof(_diffSize),1,fileDst);
  if (_diffSize>0)
  {
    fwrite(&_diffPos,sizeof(_diffPos),1,fileDst);
    fwrite(newData+_diffPos,_diffSize,1,fileDst);
  }

  _samePos = 0;
  _sameSize = 0;
  _diffPos = 0;
  _diffSize = 0;
}

void CreatePatch::SaveFromOld( int oldPos, int oldSize )
{
  if (oldSize<=0) return;
  // check if "same" section is closed
  if (_diffSize>0)
  {
    // check if different data before us are really different
    int maxCmp = _diffSize;
    saturateMin(maxCmp,oldPos);

    FileMapAccess oldDiffEnd = fileOld.GetData()+oldPos;
    FileMapAccess newDiffEnd = fileNew.GetData()+_diffPos+_diffSize;
    // try to attach as much as possible to our identical section

    while (--maxCmp>=0)
    {
      --oldDiffEnd;
      --newDiffEnd;
      if (*oldDiffEnd!=*newDiffEnd) break;
      // move byte from "diff" section to "same" section
      _diffSize--;
      oldPos--;
      oldSize++;
    }

    if (_diffSize>0)
    {
      Flush();
    }
  }
  // check if we may append to identical information
  if (_samePos+_sameSize==oldPos)
  {
    _sameSize += oldSize;
  }
  else
  {
    Flush();
    _samePos = oldPos;
    _sameSize = oldSize;
  }

}

void CreatePatch::SaveFromNew( int newPos, int newSize )
{
  if (newSize<=0) return;
  // check if may be appended to "diff"
  if (_diffPos+_diffSize==newPos)
  {
    _diffSize += newSize;
  }
  else
  {
    // may be appended to "same" section
    // wrtiting to diff section makes "same" section closed
    if (_diffSize>0)
    {
      Flush();
    }
    _diffPos = newPos;
    _diffSize = newSize;
  }
}

#if _DEBUG
#pragma optimize("gt",on)
#endif

static int CountSame(FileMapAccess a, FileMapAccess b, int maxLen)
{
  int oldLen = maxLen;
  FileMapAccess oldA = a;
  while (--maxLen>=0)
  {
    if (*a!=*b)
    {
      break;
    }
    a++,b++;
  }
  return a-oldA;
}

#if 0 // reference implementation - can be used to verify corectness of the optimised version
/// generic (slow with views) implementation
template <class Access>
static Access findchrSlow( Access beg, Access end, char c )
{
  while (end>beg)
  {
    if (*beg==c) return beg;
    beg++;
  }
  return Access();
}

/// generic (slow with views) implementation
template <class Access>
static Access findrchrSlow( Access beg, Access end, char c )
{
  while (end>beg)
  {
    if (*--end==c) return end;
  }
  return Access();
}
#endif

/// generic (slow with views) implementation
template <class Access>
static Access findrchr( Access beg, Access end, char c )
{
  while (end>beg)
  {
    if (*--end==c) return end;
  }
  return Access();
}

/// generic (slow with views) implementation
template <class Access>
static Access findchr( Access beg, Access end, char c )
{
  while (end>beg)
  {
    if (*beg==c) return beg;
    beg++;
  }
  return Access();
}


/**
Specialization could be possible, using SSE pre-fetch, unrolling and MMX compares
Measurement however shows no gain, at least on Xeon
It is quite likely CPU pre-fetch prediction is good enough to handle a linear pattern like this
*/

/// generic (slow with views) implementation

#if 1
template <>
static FileMapAccessView findchr<FileMapAccessView>( FileMapAccessView beg, FileMapAccessView end, char c )
{
  while (end>beg)
  {
    // process as much in one view as possible
    FileMapAccessMemory view = beg.GetViewFrom();
    FileMapAccessMemory endView = view.GetEnd();
    Assert(endView>view);
    FileMapAccessMemory find = findchr(view,endView,c);
    if (!!find)
    {
      // found something - check if it is withing range and return it
      FileMapAccessView ret = beg+(find-view);
      if (end>ret) return ret;
      return FileMapAccessView();
    }
    // not found - advance to the end of the view
    beg += endView-view;
  }
  return FileMapAccessView();
}
#endif

#if 1
template <>
static FileMapAccessView findrchr<FileMapAccessView>( FileMapAccessView beg, FileMapAccessView end, char c )
{
  while (end>beg)
  {
    // process as much in one view as possible
    FileMapAccessMemory view = end.GetViewTo();
    FileMapAccessMemory viewBeg = view.GetBeg();
    //Assert(endView>view);
    FileMapAccessMemory find = findrchr(viewBeg,view,c);
    if (!!find)
    {
      // found something - return it
      Assert(viewBeg<=find);
      Assert(view>find);
      FileMapAccessView ret = end - (view-find);
      if (beg<=ret) return ret;
      return FileMapAccessView();
    }
    // not found - advance to the end of the view
    end -= view-viewBeg;
  }
  return FileMapAccessView();
}
#endif

#if _DEBUG
#pragma optimize("",on)
#endif

bool CreatePatch::Delta3Diff(LPCSTR srcFullPath, LPCSTR tgtFullPath, LPCSTR dtaFullPath)
{
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  RString cmdLine = Format("%s -f -I 262144 -W 1048576 -B 268435456 -s \"%s\" \"%s\" \"%s\"", 
    XDelta3FileName, srcFullPath, tgtFullPath, dtaFullPath
  );

  ZeroMemory( &si, sizeof(si) );
  si.cb = sizeof(si);
  ZeroMemory( &pi, sizeof(pi) );

  // Start the child process. 
  if( !CreateProcess( NULL,   // No module name (use command line)
    cmdLine.MutableData(),      // Command line
    NULL,           // Process handle not inheritable
    NULL,           // Thread handle not inheritable
    FALSE,          // Set handle inheritance to FALSE
    0,              // No creation flags
    NULL,           // Use parent's environment block
    NULL,           // Use parent's starting directory 
    &si,            // Pointer to STARTUPINFO structure
    &pi )           // Pointer to PROCESS_INFORMATION structure
    ) 
  {
    printf( "CreateProcess failed (%d).\n", GetLastError() );
    return false;
  }

  // Wait until child process exits.
  WaitForSingleObject( pi.hProcess, INFINITE );
  DWORD exitCode;
  if (GetExitCodeProcess(pi.hProcess, &exitCode))
  {
    if (exitCode)
    {
      printf("xdelta3 returned error code: %d\n", exitCode);
      CloseHandle( pi.hProcess );
      CloseHandle( pi.hThread );
      return false;
    }
  }

  // Close process and thread handles. 
  CloseHandle( pi.hProcess );
  CloseHandle( pi.hThread );
  return true;
}

RString ChangePatchFileExt(const char *dtaFullPath, char *ext)
{
  RString outFileName = dtaFullPath;
  char* ext1 = strrchr(outFileName.MutableData(), '.');
  if (ext1 && ext1[1]=='#')
  {
    *ext1 = 0; //clear to find next
    char * ext2 = strrchr(outFileName.MutableData(), '.');
    if (ext2) strcpy(ext2, ext); //we know all extensions have the same size .upd, .new, .del
    *ext1 = '.'; // .#distro extension back
  }
  else strcpy(ext1,ext); //we know all extensions have the same size .upd, .new, .del
  return outFileName;
}

// return 0 if the files are the same
static int CompareFilesBinary(const char *file1, const char *file2)
{
  int fh1 = open( file1, _O_RDONLY | _O_BINARY); // C4996
  if( fh1 == -1 )
  {
    fprintf(stderr, "Cannot open file %s\n", file1);
    return 1;
  }
  int fh2 = open( file2, _O_RDONLY | _O_BINARY);
  if( fh2 == -1 )
  {
    fprintf(stderr, "Cannot open file %s\n", file2);
    close(fh1);
    return 1;
  }
  int retVal = 0; //the same by default
  if (_filelength(fh1) == _filelength(fh2))
  {
    const int LargeBufSize = 1024*1024*20;
    char *buf1 = new char[LargeBufSize];
    char *buf2 = new char[LargeBufSize];
    if (!buf1 || !buf2)
    {
      fprintf(stderr, "Not enough memory - inside CompareFilesBinary\n");
      if (buf2) delete[] buf2;
      if (buf1) delete[] buf1;
      return 1;
    }
    int len1 = 0, len2 = 0;
    do 
    {
      len1 = read(fh1, buf1, LargeBufSize);
      len2 = read(fh2, buf2, LargeBufSize);
      if (len1 != len2) { retVal=1; break; }
      int val = memcmp(buf1, buf2, len1);
      if (val) { retVal = val; break; }
    } while (len1 && len2);
    delete[] buf2;
    delete[] buf1;
  }
  else retVal = 1; //not same
  
  close(fh2);
  close(fh1);

  return retVal;
}

static char *UpdExtensions[] = {".upd",".ini",".new",".equ",".del"}; // files are checked in this order
IPatchAction::Result CreatePatch::PerformByName
(
  const char *tgtFullPath, const char *srcFullPath,const char *dtaFullPath,
  bool removeOnError
)
{
  // check if the upd file is up-to-date
  for (int i=0; i<sizeof(UpdExtensions)/sizeof(const char*); i++)
  {
    RString dtaFileName = ChangePatchFileExt(dtaFullPath, UpdExtensions[i]);
    struct _stat64i32 dtaStat;
    if (_stat64i32(cc_cast(dtaFileName), &dtaStat) == 0)
    {
      struct _stat64i32 srcStat;
      if (_stat64i32(srcFullPath, &srcStat) == 0 && dtaStat.st_mtime >= srcStat.st_mtime)
      {
        // new file should always exist (if not, the function will return later)
        struct _stat64i32 tgtStat;
        if (_stat64i32(tgtFullPath, &tgtStat) == 0)
        {
          if (dtaStat.st_mtime >= tgtStat.st_mtime) return OK; // both sources are older than destination
        }
        else return OK; // the only source is older than destination
      }
      // remove the old result
      unlink(cc_cast(dtaFileName));
      break;
    }
  }

  const char *shortName = GetFilenameExt(tgtFullPath);

  DWORD time = ::GetTickCount();

#if USE_DELTA3_BINARY_DIFF
  //determine the diff type .upd, .del, .new
  struct _stat64i32 srcStat;
  bool srcExists = (_stat64i32(srcFullPath, &srcStat) == 0);
  struct _stat64i32 tgtStat;
  bool tgtExists =  (_stat64i32(tgtFullPath, &tgtStat) == 0);
  if (srcExists && tgtExists)
  { //diff
    RString dtaFullPathExt = RString(dtaFullPath);
    if (CompareFilesBinary(tgtFullPath, srcFullPath)==0) //files are the very same
    {
      RString outFileName = ChangePatchFileExt(dtaFullPath, ".equ");
      FILE *out;
      if ( (out = fopen(cc_cast(outFileName), "wb")) != NULL )
      {
        char c = 'E';
        fwrite(&c, 1, 1, out);
        fclose(out);
      }
      else 
      {
        printf("Cannot write %s\n",cc_cast(outFileName));
        return Error;
      }    
    }
    else 
    {
      if (!Delta3Diff(tgtFullPath, srcFullPath, cc_cast(dtaFullPathExt))) //SOURCE and DESTINATIONs have DIFFERENT meaning
      { //some Error
        return Error;
      }
    }
  }
  else if (tgtExists)
  { //remove
    RString outFileName = ChangePatchFileExt(dtaFullPath, ".del");
    FILE *out;
    if ( (out = fopen(cc_cast(outFileName), "wb")) != NULL )
    {
      char c = 'X';
      fwrite(&c, 1, 1, out);
      fclose(out);
    }
    else 
    {
      printf("Cannot write %s\n",cc_cast(outFileName));
      return Error;
    }
  }
  else
  { //new
    Assert(srcExists);
    if (createNew)
    {
      RString outFileName = ChangePatchFileExt(dtaFullPath, ".new");
      if (!CopyFile(srcFullPath, cc_cast(outFileName), FALSE))
      {
        printf("Cannot copy file %s to %s\n",srcFullPath, cc_cast(outFileName));
        return Error;
      }
    }
    else
    {
      RString outFileName = ChangePatchFileExt(dtaFullPath, ".ini");
      FILE *out;
      if ( (out = fopen(cc_cast(outFileName), "wb")) != NULL )
      {
        char c = 'I';
        fwrite(&c, 1, 1, out);
        fclose(out);
      }
      else 
      {
        printf("Cannot write %s\n",cc_cast(outFileName));
        return Error;
      }    
    }
  }
#else
  fileNew.Open(srcFullPath);
  fileOld.Open(tgtFullPath);

  lastProgressNum = fileNew.GetSize()*2; // init
  lastProgressTime = GetTickCount();

  _samePos = 0;
  _sameSize = 0;
  _diffPos = 0;
  _diffSize = 0;
  // compare srcNew and srcOld files
  // store differences to dst file
  fileDst = fopen(dtaFullPath,"wb");
  if (!fileDst)
  {
    printf("Cannot write %s\n",dtaFullPath);
    return Error;
  }
  #if OUT_TEXT
  char difFullPath[1024];
  strcpy(difFullPath,dtaFullPath);
  strcat(difFullPath,".log");
  fileTxt = fopen(difFullPath,"wb");
  if (!fileTxt)
  {
    printf("Cannot write log file\n");
    return Error;
  }
  #endif
  setvbuf(fileDst,NULL,_IOFBF,BUFFER_SIZE);

  if (fileNew.GetError())
  {
    // new file should always exist
    Fail("Error");
    return OK;
  }
  else
  {
    // file added or changed
    char op = DiffChange;
    if (fileOld.GetError())
    {
      op = DiffAdd;
    }
    fwrite(&op,sizeof(op),1,fileDst);

    // calculate file checksums
    int checkSumNew=0;

    int newLen = fileNew.GetSize();
    int oldLen = fileOld.GetSize();
    int newRead = 0;
    int oldRead = 0;

    // write information about old file
    #if OUT_TEXT
    fprintf(fileTxt,"New '%s' - %d B\r\n",srcFullPath,newLen);
    fprintf(fileTxt,"Old '%s' - %d B\r\n",tgtFullPath,oldLen);
    #endif

    //const char *newShortName = GetFilenameExt(newName);
    //const char *oldShortName = GetFilenameExt(oldName);

    //fwrite(newShortName,strlen(newShortName)+1,1,fileDst);
    fwrite(&newLen,sizeof(newLen),1,fileDst);
    //fwrite(oldShortName,strlen(oldShortName)+1,1,fileDst);
    fwrite(&oldLen,sizeof(oldLen),1,fileDst);

    // write information about new file

    // check identical parts

    FileMapAccess newData = fileNew.GetData();
    FileMapAccess oldData = fileOld.GetData();
    FileMapAccess oldEnd = oldData+oldLen;

    float skipSize = 1;
    //int skipSize = 1;

    const int minIdenticalRequired = 8;
    const int minIdenticalWanted = 16;

    while(newRead<newLen)
    {
      ShowProgress(shortName,newLen-newRead);
      {
        // count how much is rest in both files
        int restNew = newLen-newRead;
        int restOld = oldLen-oldRead;

        int rest = restNew;
        saturateMin(rest,restOld);

        // prefer working in smaller pages
        // this enables us to display the progress
        int countSamePage = 256*1024;
        
        int identical = 0;
        FileMapAccess newIdent = newData+newRead;
        FileMapAccess oldIdent = oldData+oldRead;
        while (rest>0)
        {
          int count = intMin(rest,countSamePage);
          int idSeq = CountSame(newIdent,oldIdent,count);
          if (idSeq==0) break;
          identical += idSeq;
          newIdent += idSeq;
          oldIdent += idSeq;
          rest -= idSeq;
          ShowProgress(shortName,newLen-newRead-identical);
        }
        

        // compare if byte is equal in both files
        if (identical>minIdenticalRequired || identical==restNew)
        {
          for (int i=0; i<identical; i++)
          {
            checkSumNew += newData[newRead+i];
          }
          // skip identical part
          newRead+=identical;
          oldRead+=identical;
          skipSize = 1;
          // write info about identical part
          SaveFromOld(oldRead-identical,identical);
          continue;
        }
      }

      // now comes the hard part - try to get both files in sync
      // actually: we want to find data we have in fileNew somewhere in fileOld
      // we have some expectations

      // we maintain oldRead cursor that shows where reading ended in oldFile
      // (1) data will probably lay someplace near
      // (2) we will check all other positions in oldFile is this is not true
      // (3) if we will not find anything, we will skip some data

      int bestLen = 0;
      int bestO = 0;
      // (1)
      const int searchOld = 16*1024;
      const int goodMatch = 1024;

      // search depth depending on skipped data
      float satSkipSize = floatMin(skipSize*256,(float)oldLen);
      int depth = toLargeInt(satSkipSize+0.5f);
      
      // find next occurrence of the searched character
      int actC = newData[newRead];

      int advanceMax = depth;
      int advanceMin = -depth;
      saturateMin(advanceMax,oldLen-oldRead);
      saturateMax(advanceMin,-oldRead);
      FileMapAccess pos = oldData+oldRead;
      FileMapAccess advanceEnd = pos+advanceMax;
      FileMapAccess advanceBeg = pos+advanceMin;

      for(;;)
      {
        //FileMapAccess posV = findchrSlow(pos,advanceEnd,actC);
        //FileMapAccess pos0 = pos;
        
        pos = findchr(pos,advanceEnd,actC);
        //Assert(pos==posV);
        
        if (!pos) break; // no more matches

        int o = pos-oldData;
        // calculate how much is rest in both files
        int restOld = oldLen-o;
        int restNew = newLen-newRead;
        int rest = restOld;
        saturateMin(rest,restNew);

        saturateMin(rest,goodMatch);
        int r = CountSame(newData+newRead,pos,rest);
        Assert(r>0);

        if (r>=minIdenticalWanted && r>=bestLen)
        {
          bestLen = r;
          bestO = o;
          if (r>=goodMatch) break;
        }
        pos++; // skip this occurrence
      }

      // no good match when advancing - try to go back
      // if the match found is near, it may be much shorter - big probability of sync
      if (bestLen<=goodMatch)
      {
        pos = oldData+oldRead;

        for(;;)
        {
          //FileMapAccess pos0 = pos;
          //FileMapAccess posV = findrchrSlow(advanceBeg,pos0,actC);
          pos = findrchr(advanceBeg,pos,actC);
          //Assert(pos==posV);
          if (!pos) break; // no more matches

          int o = pos-oldData;

          // calculate how much is rest in both files
          int restOld = oldLen-o;
          int restNew = newLen-newRead;
          int rest = restOld;
          saturateMin(rest,restNew);

          saturateMin(rest,goodMatch);
          int r = CountSame(newData+newRead,pos,rest);
          Assert(r>0);
          if (r>=minIdenticalWanted && r>=bestLen)
          {
            bestLen = r;
            bestO = o;
            if (r>=goodMatch) break;
          }
          pos--; // skip this occurrence
        }
      }

      if (bestLen<goodMatch)
      {
        // no match
      }

      if (bestLen>0)
      {
        // we have found some old data
        // default loop handling will take care of them
        oldRead = bestO;
        continue;
      }
      else
      {
        // we have to save difference
        float skipSizeSat = floatMin(skipSize,1024*16);
        int diffSize = toLargeInt(skipSizeSat+0.5f);
        saturateMin(diffSize,newLen-newRead);
        SaveFromNew(newRead,diffSize);
        // skip diffSize bytes
        for (int i=0; i<diffSize; i++)
        {
          checkSumNew += newData[newRead+i];
        }
        newRead += diffSize;
        skipSize *= 1.1f;
      }

      // continue comparing
    }
    // check if there is something rest in old or new
    //
    // save change information:
    // oldLen-oldRead characters replaced by newLen-newRead
    int cnLen = newLen-newRead;
    int coLen = oldLen-oldRead;
    if (cnLen>0)
    {
      SaveFromNew(newRead,cnLen);

      // calculate cs of skipped part
      for (int n=newRead; n<newLen; n++)
      {
        checkSumNew += newData[n];
      }
    }

    ShowProgress("",0);

    Flush();

    // save summary information
    #if OUT_TEXT
    fprintf(fileTxt,"New checksum %d\r\n",checkSumNew);
    #endif
    fwrite(&checkSumNew,sizeof(checkSumNew),1,fileDst);

  }
  CleanUp();
#endif //USE_DELTA3_BINARY_DIFF

  LogF("Diff of %s took %d ms",cc_cast(dtaFullPath),::GetTickCount()-time);

  return OK;
}

IPatchAction::Result CreatePatch::Perform
(
  const char *tgtPath, const char *srcPath,const char *dataPath,
  const char *name
)
{
  // create: tgt = old, src = new, data = patch
  char tgtFullPath[1024];
  char srcFullPath[1024];
  char dtaFullPath[1024];
  strcpy(tgtFullPath,tgtPath);
  strcpy(srcFullPath,srcPath);
  strcpy(dtaFullPath,dataPath);
  strcat(tgtFullPath,name);
  strcat(srcFullPath,name);
  strcat(dtaFullPath,name);
  strcat(dtaFullPath,".upd");

  return PerformByName(tgtFullPath,srcFullPath,dtaFullPath);
}

ApplyPatch::ApplyPatch(AutoArray<RString> &uninstalLog, bool otherTargetDir)
: _uninstalLog(uninstalLog), _otherTargetDir(otherTargetDir)
{
}

template <class Type>
inline Type GetDiffT( FileMapAccess &pos, FileMapAccess end )
{
  if (pos+sizeof(Type)>end) return Type(0); // return error
  Type ret(0);
  
  int shift = 0;
  for (int i=0; i<sizeof(Type); i++)
  {
    ret |= ((unsigned char)(*pos))<<shift;
    shift += 8;
    pos++;
  }
  return ret;
}

template <class Type>
inline Type GetDiffT( const char *pos, const char *end )
{
  if (pos+sizeof(Type)>end) return Type(0); // return error
  Type *ret = (Type *)pos;
  pos += sizeof(Type);
  return *ret;
}

static void CCALL ToLogF(const char* format, ...)
{
  char buffer[1024];
  
  va_list argptr;      
  va_start(argptr, format);
  wvsprintf(buffer, format, argptr);
  va_end(argptr);

  FILE *logFile = fopen("patch.log", "a+");
  if (!logFile)
  {
    printf("Cannot write to log file\n");
    return;
  }
  fwrite(buffer, sizeof(char), strlen(buffer), logFile);
  fclose(logFile);
}

IPatchAction::Result ApplyPatch::Delta3Decrypt(LPCSTR in, LPCSTR diff, LPCSTR out)
{
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  RString cmdLine = Format("%s -f -d -s \"%s\" \"%s\" \"%s\"", 
    XDelta3TmpFile, in, diff, out
  );

  ZeroMemory( &si, sizeof(si) );
  si.cb = sizeof(si);
  ZeroMemory( &pi, sizeof(pi) );

  // Start the child process. 
  if( !CreateProcess( NULL,   // No module name (use command line)
    cmdLine.MutableData(),      // Command line
    NULL,           // Process handle not inheritable
    NULL,           // Thread handle not inheritable
    FALSE,          // Set handle inheritance to FALSE
    CREATE_NO_WINDOW, // We do not want to see the console window during patching!
    NULL,           // Use parent's environment block
    NULL,           // Use parent's starting directory 
    &si,            // Pointer to STARTUPINFO structure
    &pi )           // Pointer to PROCESS_INFORMATION structure
    ) 
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
    RString basicInfo = Format("CreateProcess %s failed.", XDelta3TmpFile);
    _errorMessage = Format("%s %s",(LPCTSTR)lpMsgBuf, cc_cast(basicInfo));
    printf("CreateProcess %s failed.\n", XDelta3TmpFile);
    ToLogF("CreateProcess %s failed.\n", XDelta3TmpFile);
    return Error;
  }

  // Wait until child process exits.
  WaitForSingleObject( pi.hProcess, INFINITE );
/*
  while (MsgWaitForMultipleObjects(1, &pi.hProcess, FALSE, 1000, QS_ALLEVENTS)!=WAIT_OBJECT_0)
  {
    extern void ProcessPatchMessages();
    ProcessPatchMessages(); //what happens when application is closed by user here?
  }
*/

  DWORD exitCode;
  if (GetExitCodeProcess(pi.hProcess, &exitCode))
  {
    if (exitCode)
    {
      printf("xdelta3 returned error code: %d\n", exitCode);
      ToLogF("xdelta3 returned error code: %d\n", exitCode);
      _errorMessage = Format("xdelta3 returned error code: %d", exitCode);
      return Error;
    }
  }

  // Close process and thread handles. 
  CloseHandle( pi.hProcess );
  CloseHandle( pi.hThread );
  return OK;
}

IPatchAction::Result ApplyPatch::PerformByName
(
  const char *tgtFullPath, const char *srcFullPath,const char *dtaFullPath,
  bool removeOnError
)
{
#if USE_DELTA3_BINARY_DIFF
  RString srcPath = srcFullPath;
  char *ext = GetFileExt(srcPath.MutableData());
  if (ext && ext[1]=='#')
  {
    *ext = 0;
    ext = GetFileExt(srcPath.MutableData());
  }
  if (stricmp(ext, ".upd")==0)
  {
    if (_otherTargetDir) _uninstalLog.Add(RString("F:")+tgtFullPath);
    return Delta3Decrypt(dtaFullPath/*in*/, srcFullPath/*diff*/, tgtFullPath/*out*/);
  }
  else if (stricmp(ext, ".new")==0)
  {
    if (!CopyFile(srcFullPath, tgtFullPath, FALSE))
    {
      printf("Cannot copy file %s to %s\n",srcFullPath, tgtFullPath);
      ToLogF("Cannot copy file %s to %s\n",srcFullPath, tgtFullPath);
      _errorMessage = Format("Cannot copy file %s to %s",srcFullPath, tgtFullPath);
      return Error;
    }
    _uninstalLog.Add(RString("F:")+tgtFullPath);
  }
  else if (stricmp(ext, ".del")==0)
  {
    RString tgtFullPathExt = ChangePatchFileExt(tgtFullPath, ""); //clear .tmp extension
    if (remove(tgtFullPathExt))
    {
      printf("Cannot remove file %s\n", cc_cast(tgtFullPathExt));
      ToLogF("Cannot remove file %s\n", cc_cast(tgtFullPathExt));
      _errorMessage = Format("Cannot remove file %s", cc_cast(tgtFullPathExt));
      return Error;
    }
    return OKRemoved;
  }
  else if (stricmp(ext, ".ini")==0)
  {
    return OKInPlace;
  }
  else if (stricmp(ext, ".equ")==0)
  {
    return OKInPlace;
  }
  else
  {
    printf("Unknown patch extension: \"%s\"\n",ext);
    ToLogF("Unknown patch extension: \"%s\"\n",ext);
    _errorMessage = Format("Unknown patch extension: \"%s\"",ext);
    return Error;
  }
  return OK;
#else
  fileOld.Open(dtaFullPath);
  fileDiff.Open(srcFullPath);

  FileMapAccess diffPos = fileDiff.GetData();
  FileMapAccess diffEnd = diffPos+fileDiff.GetSize();

  if (fileDiff.GetError())
  {
    printf("Cannot read %s\n",srcFullPath);
    ToLogF("Cannot read %s\n", srcFullPath);
    _errorMessage = Format("Cannot open %s", srcFullPath);
    return Error;
  }
  char op = GetDiffT<char>(diffPos,diffEnd);
  switch (op)
  {
    case DiffRemove:
      // target file should be removed
      remove(tgtFullPath);
      return OKInPlace;
    case DiffChange:
      // old file should exist
      if (fileOld.GetError())
      {
        printf("Warning: Cannot read %s\n",dtaFullPath);
        ToLogF("Cannot read %s\n", dtaFullPath);
        _errorMessage = Format("Cannot open %s, DiffChange operation", dtaFullPath);
        return Error;
      }
      break;
    case DiffAdd:
      break;
    default:
      // unknown operation - ignore
      _errorMessage = Format("Unknown update operation %d", (int)op);
      return Error;
  }
  // read old file
  // read changes
  // read old & new sizes
  int oldLen,newLen;

  newLen = GetDiffT<int>(diffPos,diffEnd);
  oldLen = GetDiffT<int>(diffPos,diffEnd);

  // Fix: for DiffAdd operation, I need not to check the fileOld - it will not be used at all
  if (op == DiffChange && oldLen != fileOld.GetSize())
  {
    printf("Incompatible patch version.\n");
    ToLogF("File %s: bad size: %d (old=%d, new=%d)\n", dtaFullPath, fileOld.GetSize(), oldLen, newLen);
    _errorMessage = Format("Unexpected %s size: %d != %d", dtaFullPath, fileOld.GetSize(), oldLen);
    return Error;
  }

  // check for a special case - old file left intact
  if (newLen==oldLen && diffEnd-diffPos==4*sizeof(int))
  {
    // special case possible
    // only one change, and that is the whole file
    // read change description
    FileMapAccess check = diffPos;

    int oldSize = GetDiffT<int>(check,diffEnd);
    int oldPos = GetDiffT<int>(check,diffEnd);
    int newSize = GetDiffT<int>(check,diffEnd);
    int newPos = GetDiffT<int>(check,diffEnd);
    
    if (oldSize==oldLen && oldPos==0 && newSize==0)
    {
      Log("  no change, skipping");
      return OKInPlace;
    }
  }

  FILE *dstFile = fopen(tgtFullPath,"wb");
  if (!dstFile)
  {
    printf("Cannot write %s\n",tgtFullPath);
    ToLogF("Cannot write %s\n", tgtFullPath);
    _errorMessage = Format("Cannot write to %s", tgtFullPath);
    return Error;
  }
  setvbuf(dstFile,NULL,_IOFBF,BUFFER_SIZE);

  // read change description
  int done =0;
  int newCheckSum = 0;

  const int maxWrite = 64*1024;

  while (done<newLen)
  {
    if (diffEnd<=diffPos)
    {
      _errorMessage = RString("Unexpected file structure");
      return Error; // error
    }

    int oldSize = GetDiffT<int>(diffPos,diffEnd);
    if (oldSize>0)
    {
      int oldPos = GetDiffT<int>(diffPos,diffEnd);
      // copy identical data from old file
      while (oldSize > 0)
      {
        // calculate checksum and write 
        char c = fileOld.GetData()[oldPos];
        newCheckSum += c;
        _fputc_nolock(c,dstFile);
        done++;
        oldPos++;
        oldSize--;
      }
    }
    int newSize = GetDiffT<int>(diffPos,diffEnd);
    if (newSize>0)
    {
      int newPos = GetDiffT<int>(diffPos,diffEnd);
      
      while (newSize > 0)
      {
        //fwrite(diffPos,maxWrite,1,dstFile);
        // calculate checksum
        char c = *diffPos;
        newCheckSum += c;
        _fputc_nolock(c,dstFile);
        
        done++;
        newPos++;
        newSize--;
        diffPos++;
      }
      //fwrite(diffPos,newSize,1,dstFile);
      // calculate checksum
// 			for (int i=0; i<newSize; i++)
// 			{
// 				newCheckSum += diffPos[i];
// 			}
// 			diffPos += newSize;
// 			done += newSize;
    }
  }

  fclose(dstFile);
  dstFile = NULL;
  // read checksums from file
  int newCheckSumF = GetDiffT<int>(diffPos,diffEnd);

  if (newCheckSum != newCheckSumF)
  {
    printf("Incompatible patch version.\n");
    ToLogF("File %s: bad checksum: %d (expected=%d)\n", tgtFullPath, newCheckSum, newCheckSumF);
    _errorMessage = Format("Invalid checksum of %s, %d != %d", tgtFullPath, newCheckSum, newCheckSumF);
    if (removeOnError)
    {
      remove(tgtFullPath);
    }
    return Error;
  }

  if (done != newLen)
  {
    printf("Incompatible patch version.\n");
    ToLogF("File %s: bad size: %d (expected=%d)\n", tgtFullPath, done, newLen);
    _errorMessage = Format("Invalid size of %s, %d != %d", tgtFullPath, done, newLen);
    if (removeOnError)
    {
      remove(tgtFullPath);
    }
    return Error;
  }

  fileDiff.Close();
  fileOld.Close();
#endif
  return OK;
}

IPatchAction::Result ApplyPatch::Perform
(
  const char *tgtPath, const char *srcPath,const char *dataPath,
  const char *name
)
{
  const char *ext = GetFileExt(name);
  if (strcmpi(ext,".upd")) return OKInPlace; // ignore unknown file types

  // apply: tgt = new, src = patch, data = old
  // remove .upd from name
  char dtaFullPath[1024];
  char tgtFullPath[1024];
  char srcFullPath[1024];
  strcpy(dtaFullPath,dataPath);
  strcpy(tgtFullPath,tgtPath);
  strcpy(srcFullPath,srcPath);
  strcat(dtaFullPath,name);
  strcat(tgtFullPath,name);
  strcat(srcFullPath,name);

   // remove .upd extension from tgt and dta
  strcpy(GetFileExt(tgtFullPath),"");
  strcpy(GetFileExt(dtaFullPath),"");

  return PerformByName(tgtFullPath,srcFullPath,dtaFullPath);
}

void ApplyPatch::CleanUp()
{
  fileDiff.Close();
  fileOld.Close();
}


IPatchAction *CreatePatchActionCreate(bool createNew)
{
  return new CreatePatch(createNew);
}

IPatchAction *CreatePatchActionDelete()
{
  return new CreateDelete;
}

IPatchAction *CreatePatchActionApply(AutoArray<RString> &uninstalLog, bool otherTargetDir)
{
  return new ApplyPatch(uninstalLog, otherTargetDir);
}
