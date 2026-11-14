#ifdef _MSC_VER
#pragma once
#endif

#ifndef _PACK_FILES_HPP
#define _PACK_FILES_HPP

#include <Es/Containers/array.hpp>
#include <El/QStream/qStream.hpp>
#include <El/QStream/fileinfo.h>
#include <El/DataSignatures/dataSignatures.hpp>

class SOFStream;

#ifdef Zero
  #undef Zero
#endif

class IFilebankEncryption;

struct FileInfoExt
{
  RString name;
  int compressedMagic;
  int uncompressedSize;
  long startOffset;
  QFileTime time;
  long length;
  int priority;

  void Zero(){memset(this,0,sizeof(*this));}
  const char *GetKey() const {return name;}

  // > 0 gives order of processing
  // small numbers should be included first
  // <= 0 means file should be excluded
  FileInfoExt(){priority=0;}
};

TypeIsMovableZeroed(FileInfoExt)



extern const char *DefFileBankNoCompress[];
extern const char *DefFileBankEncrypt[];

struct QFProperty;

class IBankChecker
{
public:
  virtual void InitChecker(bool reversed) = 0;
};

class QOFStreamChecked : public QOFStream, public IBankChecker
{
protected:
  HashCalculator _calculator;
  bool _reversed;

public:
  bool GetResult(AutoArray<char> &result) {Flush(0); return _calculator.GetResult(result);}
  bool IsReversed() const {return _reversed;}

  virtual void InitChecker(bool reversed);
  virtual void Flush(size_t size);
};

//! utility to create file banks (pbo files)
class FileBankManager
{
protected:
  /// type of header protection
  enum EHeaderProtectionType
  {
    HPNone, ///< none header protection
    HPSetupPrepare, ///< just prepare headers for encrypting for setup, don't encrypt yet
    HPSetupProtect ///< encrypt prepared headers for setup
  };

  AutoArray<FileInfoExt> _files;
  QFileTime _newestFile;
  unsigned int _size;

  public:
  FileBankManager();
  ~FileBankManager();
  //! create a pbo file bank
  //! \param headerProtection if not NULL, then headers are encrypted

  LSError Create
  (
    const char *tgt, const char *src,
    bool compress=false, bool optimize=true,
    const char *logFile=NULL,
    const char **doNotCompress=DefFileBankNoCompress,
    const QFProperty *properties=NULL, int nProperties=0,
    const RString *exclude=NULL, int nExcludes=0,IFilebankEncryption *headerProtection = 0
  );

  //! create a pbo file bank with based on given file list
  LSError Create
    (
    const char *tgt, const char *src,
    bool compress, bool optimize, const AutoArray<FileInfoExt>& files,
    const char **doNotCompress,
    const QFProperty *properties, int nProperties,IFilebankEncryption *headerProtection = 0
    );

  //! create a pbo file bank based on a log file
  //! \param headerProtection if not NULL, then headers are encrypted
  void Create(
    QOStream &out, const char *src, 
    bool compress=false, bool optimize=true,
    const char *logFile=NULL,
    const char **doNotCompress=DefFileBankNoCompress,
    const QFProperty *properties=NULL, int nProperties=0,
    const RString *exclude=NULL, int nExcludes=0,IFilebankEncryption *headerProtection = 0
  );
  //! create encrypted pbo file
  void Create
  (
    QOStream &out, const char *src, 
    IFilebankEncryption *encrypt,
    const QFProperty *properties, int nProperties,
    const RString *exclude=NULL, int nExcludes=0,
    const char **encryptExts=DefFileBankEncrypt
  );

#if _VBS2
   bool EncryptPbo
   (
      RString srcPath,
      RString dstPath,
      const char **encryptExts=DefFileBankEncrypt
   );
#endif

protected:
  // implementation
  void PrepareFileList(
    RString folder, const char *logFile,
    const RString *exclude, int nExcludes
  );
  //! \param headerProtection if not NULL, then headers are encrypted
  void StoreFiles(
    QOStream &out, RString folder,
    bool compress, const char **doNotCompress,
    const QFProperty *properties, int nProperties,IBankChecker *checker, IFilebankEncryption *headerProtection
  );

  void ParseMasks(const char *logFile);
  void SortAndRemove(bool remove);
  /// counts total size of files and adds terminator to file list

  void FinishFileList();
  /// delete files matching to given pattern list
  void Exclude(const RString *exclude, int nExcludes);
  // create bank in memory
  void ScanDir( RString dir, RString rel);

#if _ENABLE_PBO_PROTECTION
  /// excludes dummy file from file list
  void ExcludeDummyPaddingFile();

  /// save protected headers
  /// \param headerProtection encryption class used for headers protection
  /// \return number of bytes written
  QFileSize SaveHeadersOptProtected( QOStream &out, IFilebankEncryption* headerProtection);
#endif //#if _ENABLE_PBO_PROTECTION  

  /// \param headerProtectionType type of protection for headers
  /// \param headerProtection encryption class used for headers protection
  /// \return number of bytes written
  QFileSize SaveHeadersOpt( QOStream &out, EHeaderProtectionType headerProtectionType = HPNone, IFilebankEncryption* headerProtection = 0);

  void SaveHeadersOpt( SOFStream &out);

  void SaveProperties( QOStream &out, const QFProperty *prop, int nProp );
  void SaveProperties( SOFStream &out, const QFProperty *prop, int nProp );
};
// create a pbo file
// tgt - fully qualified target name c:\temp\bank.pbo
// src - path to source directory

#endif
