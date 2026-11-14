#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <time.h>

#include <sys/stat.h>
#include <Es/Files/filenames.hpp>
#include <Es/Files/fileContainer.hpp>
#ifndef _WIN32
#define __cdecl
#include <unistd.h> 
#include <dirent.h> 
#else
#include <io.h>
#endif

#include <Es/Containers/staticArray.hpp>
#include <Es/Algorithms/qsort.hpp>
#include <El/QStream/qStream.hpp>
#include <El/QStream/qbStream.hpp>
#include "packFilesExt.hpp"
#include <El/DataSignatures/dataSignatures.hpp>


#include <stdarg.h>

void FileBankManagerExt::Create
(
 QOStream &out, const char *src,
 const AutoArray<RString> &logFile,
 bool compress, bool optimize,
 const char **doNotCompress,
 const QFProperty *properties, int nProperties,
 const RString *exclude, int nExcludes
)
{
  Assert (optimize);
  Assert(!compress);

  RString folder = src;
  PrepareFileList(folder, logFile, exclude, nExcludes);

  StoreFiles(out, folder, compress, doNotCompress, properties, nProperties, NULL);
  _files.Clear();
}

void FileBankManagerExt::PrepareFileList
 (
   RString folder, const AutoArray<RString> &logFile,
   const RString *exclude, int nExcludes
 )
{
  _newestFile = 0;
  _size = 0;

  _files.Resize(0);
  _files.Realloc(1024);
  ScanDir(folder,"");

  DoAssert(_files.Size()>0);

  if (logFile.Size())
  {
    ParseMasks(logFile);
  }
  if (exclude && nExcludes>0)
  {
    Exclude(exclude,nExcludes);
  }
  SortAndRemove(logFile.Size() != 0);
}

static int MatchMask(const char *mask, const char *string)
{
  bool exclude = false;
  if (*mask=='~')
  {
    exclude = true;
    mask++;
  }
  int ret = Matches(string,mask);
  return exclude ? -ret : ret;
}

void FileBankManagerExt::ParseMasks(const AutoArray<RString> &logFile)
{
  // set priority to all files
  if (!logFile.Size()) return;

  int line = 1;
  for(int k=0; k<logFile.Size(); k++)
  {
    // scan rule
    const char *buf = cc_cast(logFile[k]);
    // buf is now wildcard mask
    // try to match this mask with all files
    for (int i=0; i<_files.Size(); i++)
    {
      FileInfoExt &fi = _files[i];
      int match = MatchMask(buf,fi.name);
      if (match==0) continue;
      if (match>0)
      {
        fi.priority = line;
      }
    }
    line++;
  }
}

AutoArray<FileInfoExt> &FileBankManagerExt::PrepareAndGetFileList
(
 RString folder, const AutoArray<RString> &logFile,
 const RString *exclude, int nExcludes
)
{
  PrepareFileList(folder, logFile, exclude, nExcludes);
  return _files;
}
