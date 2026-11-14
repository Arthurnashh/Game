#ifndef _PACK_FILES_EXT_HPP
#define _PACK_FILES_EXT_HPP

#include <El/elementpch.hpp>
#include <El/QStream/packFiles.hpp>

class FileBankManagerExt : public FileBankManager
{
public:
  FileBankManagerExt() {}

  void Create
    (
    QOStream &out, const char *src,
    const AutoArray<RString> &logFile,
    bool compress=false, bool optimize=true,
    const char **doNotCompress=DefFileBankNoCompress,
    const QFProperty *properties=NULL, int nProperties=0,
    const RString *exclude=NULL, int nExcludes=0
    );
  AutoArray<FileInfoExt> &PrepareAndGetFileList
    (
    RString folder, const AutoArray<RString> &logFile,
    const RString *exclude, int nExcludes
    );

protected:
  void PrepareFileList(
    RString folder, const AutoArray<RString> &logFile,
    const RString *exclude, int nExcludes
    );

  void ParseMasks(const AutoArray<RString> &logfile);
};

#endif
