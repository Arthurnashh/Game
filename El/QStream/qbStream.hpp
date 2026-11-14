#ifdef _MSC_VER
#pragma once
#endif

#ifndef _QBSTREAM_HPP
#define _QBSTREAM_HPP

#include "qStream.hpp"

#include <Es/Containers/array.hpp>
#include <Es/Strings/bString.hpp>

#include <Es/Algorithms/qsort.hpp>
#include <Es/Containers/sortedArray.hpp>


class QFBank;
class QFBankHandle;

//! check if given bank/file is accessible in given context
/*!
  In future this will probably be used also to open the file using given access.
*/

class IQFBankContext
{
  public:
  //! check if file can be accessed
  virtual bool IsAccessible(const QFBank *bank) const = 0;
};

class QFBankQueryFunctions
{
  public:
  static QFBank *AutoBank( const char *name );
  //! check timestamp of the file in the bank or outside of it
  static QFileTime TimeStamp( const char *name, IQFBankContext *context=NULL );
  //! check if file exists in the bank or outside of it
  static bool FileExist( const char *name, IQFBankContext *context=NULL );
  //! query file size
  static QFileSize GetFileSize(const char *name);
  // overload FileExists of QIFStream to avoid any confusion
  static bool FileExists( const char *name, IQFBankContext *context=NULL )
  {
    return FileExist(name,context);
  }
  /// check recommended loading order
  static int FileOrder(const char *nameA, const char *nameB);
  /// delete all banks
  static void ClearBanks();
 
  static QFBankHandle GetHandle( const char *name, QFileSize offset=0, QFileSize size=QFileSizeMax); // autoselect bank/file

  // return if given file is really taken from bank (respects patching)
  static bool FileIsTakenFromBank(const char *name);
};

//! stream that is opened from some file bank
class QIFStreamB: public QIFStream //, public QFBankQueryFunctions
{
  public:
  QIFStreamB();

  static Ref<IFileBuffer> OpenSource(const char *name, QFileSize offset=0, QFileSize size=QFileSizeMax);
  static Ref<IFileBuffer> OpenFromBank(const QFBankHandle &handle);
  static Ref<IFileBuffer> OpenFromBank( const QFBank &bank, const char *name, QFileSize offset=0, QFileSize size=QFileSizeMax );

  void AutoOpen( const char *name, QFileSize offset=0, QFileSize size=QFileSizeMax ); // autoselect bank/file
  void open(const QFBank &bank, const char *name);
  void open(const QFBankHandle &handle);
};

class FindBank
{
#ifdef _WIN32
  BString<1024> _wild;
  // check all "pb?" file banks
  void *_info;
  long _handle;

#else
    DIR *dir;
    struct dirent *entry;
#endif
  public:
  FindBank();
  ~FindBank();

  bool First(const char *path);
  bool Next();
  void Close();

  const char *GetName() const;
};

class BankContextBase: public RefCount
{
};

typedef bool (*OpenCallback)(QFBank *bank, BankContextBase *context);

int CmpStartStr( const char *str, const char *start );

#include "fileinfo.h"

#include <Es/Memory/normalNew.hpp>

struct FileInfoO
{
  //char name[NAME_LEN];
  RStringB name;

  unsigned int compressedMagic;
  unsigned int uncompressedSize;
  unsigned long startOffset;
  unsigned long time;
  unsigned long length;
  #if _ENABLE_PATCHING
    bool loadFromFile; // fast patching enabled
  #endif

  FileInfoO()
  {
    #if _ENABLE_PATCHING
    loadFromFile=false;
    #endif
  }

  const char *GetKey() const {return name;}
};

#include <Es/Memory/debugNew.hpp>

TypeIsMovable(FileInfoO);

template <>
struct BinFindArrayKeyTraits<FileInfoO *>
{
  typedef unsigned long KeyType;
  static bool IsEqual(KeyType a, KeyType b) {return a==b;}
  static bool IsLess(KeyType a, KeyType b) {return a<b;}
  static KeyType GetKey(const FileInfoO *a) {return a->startOffset;}
};

typedef MapStringToClass<FileInfoO, AutoArray<FileInfoO> > FileBankType;

typedef void *WINHANDLE;

// interface for creating file log

class IBankLog: public RefCount
{
  public:
  virtual void Init(const char *bankName) = 0;
  virtual void LogFileOp(const char *name) = 0;
  virtual void Flush(const char *bankName) = 0;
};

class FileBufferMapped;

//! any bank may have several properties attached
struct QFProperty
{
  RString name;
  RString value;
};

TypeIsMovableZeroed(QFProperty)

//! virtual read-only file system
/*!
Access to files in QFBank is much faster and
they are stored more efficiently than when using OS services.
*/

class QFBank;

//! filebank compression/encryption interface
/*!
Instance of IFilebankEncryption already should contain any information
required to encrypt/decrypt data
*/

class IFilebankEncryption: public RefCount
{
  public:
#if _VBS2
  // should never be called, directly here!
  virtual bool Decode(char *dst,long uncompressedSize,QIStream &inBuf,unsigned char initVec[16] ){ DoAssert(false); return false; };
  virtual void Encode(QOStream &out, QIStream &in,unsigned char intVec[16]){ DoAssert(false); };
  virtual void Encode( QOStream &out, const char *dst, long lensb,unsigned char intVec[16] )
  {
    QIStrStream in(dst,lensb);
    Encode(out,in,intVec);
  }
#endif

  //! decode block of data
  /*!
  \param dst pointer to data destination
  \param lensb destination length
  \param in encoded data stored in stream
  */
  virtual bool Decode( char *dst, long lensb, QIStream &in ) = 0;
  //! encode block of data
  /*!
  \param out stream into which encoding is done
  \param dst pointer to source data
  \param lensb length of source data
  */
  virtual void Encode( QOStream &out, const char *dst, long lensb )
  {
    QIStrStream in(dst,lensb);
    Encode(out,in);
  }
  //! encode block of data
  /*!
  \param out stream into which encoding is done
  \param in input stream
  */
  virtual void Encode( QOStream &out, QIStream &in) = 0;
};

//! create encryptor that is able to encrypt/decrypt given bank
Ref<IFilebankEncryption> CreateFilebankEncryption(const char *name, const void *context);

//! register another encryptor
void RegisterFilebankEncryption
(
  const char *name, IFilebankEncryption *(*createFunction)(const void *context)
);

//! interface - using this interface you can quickly open file
class QFBankPointer: public RefCount
{
  public:
  virtual Ref<IFileBuffer> OpenSource() = 0;
  virtual RString GetDebugName() const = 0;
  virtual Ref<QFBankPointer> Partial(QFileSize offset, QFileSize size) const = 0;
};

//! encapsulation of Ref<QFBankPointer> - hiding implementation from the user

class QFBankHandle
{
  Ref<QFBankPointer> _ref;
  friend class QFBank;
  //friend class QIFStreamB;
  friend class QFBankQueryFunctions;

  QFBankHandle(QFBankPointer *ref):_ref(ref){}

  public:
  QFBankHandle(){}
  QFBankHandle(const QFBankHandle &src, QFileSize offset, QFileSize size)
  {
    _ref = src._ref->Partial(offset,size);
  }
  //! open, default file position
  Ref<IFileBuffer> OpenSource() const
  {
    Assert(_ref);
    if (_ref) return _ref->OpenSource();
    return NULL;
  }
  bool IsNull() const {return _ref==NULL;}
  RString GetDebugName() const
  {
    return _ref ? _ref->GetDebugName() : "<null>";
  }
};

struct AddFileName
{
  AutoArray<RString> &_fileNames;

  AddFileName(AutoArray<RString> &fileNames) :_fileNames(fileNames) {}    

  bool operator ()(const FileInfoO &info, const FileBankType *container)
  {
    if (info.length>0)
    {
      RString lowerName = info.name; lowerName.Lower();
      _fileNames.Add(lowerName);
    }
    return false;
  }
};

//@{ NonDataFileHash calculation
struct NonDataFile
{
  unsigned long startOffset;
  unsigned long length;
  NonDataFile() :startOffset(0), length(0) {}
  NonDataFile(unsigned long offset, unsigned long len) :startOffset(offset), length(len) {}

  ClassIsMovable(NonDataFile);
};

struct AddNonDataFile
{
  AutoArray<NonDataFile> &_nonDataFiles;
  bool &_dummyPaddingFilePresent;

  AddNonDataFile(AutoArray<NonDataFile> &nonDataFiles, bool &dummyPFP) :_nonDataFiles(nonDataFiles), _dummyPaddingFilePresent(dummyPFP) {}    

  bool operator ()(const FileInfoO &info, const FileBankType *container)
  {
    if (info.length>0)
    {
      static const char* skipFileList[] = {"p3d","paa","tga","rvmat","lip","ogg","wss","png","jpg","rtm","pac","fxy","wrp"};
      for (int i=0; i<sizeof(skipFileList)/sizeof(*skipFileList); i++)
      {
        const char *ext = GetFileExt(info.name);
        if (ext && *ext=='.' && stricmp(ext+1, skipFileList[i])==0)
        {
          return false; //continue
        }
      }
      if (!stricmp(info.name,"___dummypadding___")) _dummyPaddingFilePresent = true;
      // file extension is not listed, so add it
      _nonDataFiles.Add(NonDataFile(info.startOffset, info.length));
    }
    return false;
  }
};
//@} NonDataFileHash calculation

#define MT_SAFE 0
class QFBank: public RefCount
{
#if _VBS2 
  friend class FileBankManager;
#endif
  #if MT_SAFE
  mutable CriticalSection _lock;
  #endif
  #if !_RELASE
    mutable bool _serialize; // help finding bugs
  #endif
  Ref<IBankLog> _log;

  friend class QIFStream;
  private:

  // remember parameters necessary for opening
  //! name provided by open call
  RString _openName;
  //! callback provided by open call
  OpenCallback _openBeforeOpenCallback;
  //! context provided by open call
  Ref<BankContextBase> _openContext;

  //! time of last open - can be used for automated unloading of unused banks
  //DWORD _lastOpen;
  //! during some operations (file from bank being used) unload is not possible
  //int _openCount;
  // ! lock bank - it cannot be unloaded while locked
  mutable bool _locked;
  //! only lockable bank can be locked/unlocked
  bool _lockable;
  /// mark if the bank contain any config file
  bool _verifySignature;
#if _ENABLE_PATCHING
  /// some file is patched
  bool _patched;
#endif
  RString _prefix;
  FileBankType _files;
  
#if _ENABLE_PERFLOG
  BinFindArrayKey<FileInfoO *> _filesByOffset;
#endif

  AutoArray<QFProperty> _properties;

  //int _handle;
  Ref<IFileBuffer> _fileAccess;

  //! set true once Load was sucessfull
  bool _loaded;
  //! set true when attempt to open (DoOpen) failed
  bool _error;

#if _ENABLE_PBO_PROTECTION
  // starting offset of the header
  QFileSize _headerStart;
#endif //#if _ENABLE_PBO_PROTECTION

  /// init file access
  bool InitFileAccess();
  /// load what needs to be loaded during open
  bool Init();
  void ScanPatchFiles(RString prefix, RString subdir);
  
  public:
  QFBank();
  ~QFBank();
  const RString &GetPrefix() const {return _prefix;}
  void SetPrefix(RString prefix);
  const RString &GetOpenName() const {return _openName;}
  bool VerifySignature() const {return _verifySignature;}
  void SetVerifySignature() {_verifySignature = true;}
  bool IsPatched() const
  {
#if _ENABLE_PATCHING
    return _patched;
#else
    return false;
#endif
  }
  /// access to properties
  int NProperties() const {return _properties.Size();}
  /// access to properties
  const QFProperty &GetProperty(int index) const {return _properties[index];}

  //! open bank from file
  bool open
    (
    RString name, OpenCallback beforeOpen=NULL,
    BankContextBase *context=NULL
    );
  //! open bank from memory
  bool open
  (
    IFileBuffer *buffer,
    OpenCallback beforeOpen=NULL, BankContextBase *context=NULL
  );
  //! open bank from file, not adding ".pbo" at the end of name
  bool openFromFile
    (
    RString name, OpenCallback beforeOpen=NULL,
    BankContextBase *context=NULL,
    QFileSize start=0, QFileSize size=0
    );
  //! load bank - physically performs action
  bool Load();
  //! open bank and mark is as locked (cannot be un-opened)
  void Lock() const;
  //! set lockable starus
  void SetLockable(bool lockable) {_lockable=lockable;}
  //! get lockable starus
  bool GetLockable() const {return _lockable;}
  //! mark bank as unlocked and unload if possible
  void Unlock() const;
  //! check if it can be unloaded
  bool IsLocked() const {return _locked;}
  //! check if it can be unloaded (if it is used, it cannot be unloaded)
  bool CanBeUnloaded() const;
  //! load bank - enable modification of necessary fields
  bool Load() const
  {
    return const_cast<QFBank *>(this)->Load();
  }
  /// no files from the bank should be flushed on alt-tab, no need to check for modification
  void MakeHandlePermanent();
  //! unload bank
  void Unload();
  //! unload bank - enable modification of necessary fields
  void Unload() const
  {
    if (!_loaded) return;
    const_cast<QFBank *>(this)->Unload();
  }
  void Clear(); // release all files
  void close() {Clear();}

  bool error() const;

  const RString &GetProperty(const RString &name) const;

  const FileInfoO &FindFileInfo( const char *name ) const; // seek to beginning of some file
  bool FileExists( const char *name ) const; // check if file exists
  bool Contains(const char *path) const; // check if file exists
  /// get OS timestamp of the pbo file
  QFileTime TimeStamp() const;
  // low level access to bank
  //int AlignedRead( char *buf, long size ) const; // read from bank
  //int AlignSize() const {return _align;}
  //int InSectorOffset() const {return _wantPos&(_align-1);}
  void Read( QFileSize startOffset, void *buf, long size, const char *name ) const; // read from bank

  //! prepare handle so that file can be opened quickly
  QFBankHandle GetHandle(const char *fullName, const char *filename, QFileSize offset, QFileSize size);

  //! read file - uncompress if necessary
  Ref<IFileBuffer> Read( const char *file, QFileSize offset=0, QFileSize size=QFileSizeMax ) const;
  //! get file order in the bank
  int GetFileOrder(const char *file);

  void ForEach
  (
    void (*Func)(const FileInfoO &fi, const FileBankType *files, void *context),
    void *context
  ) const; // call Func for all files
  static bool IsNull(const FileInfoO &value) {return FileBankType::IsNull(value);}
  static bool NotNull(const FileInfoO &value) {return FileBankType::NotNull(value);}
  static FileInfoO &Null() {return FileBankType::Null();}

  bool BufferOwned(const FileBufferMapped *buffer) const; // check if file belongs to this bank
  bool HandleOwned(const FileServerHandle &handle) const; // check if file belongs to this bank

  /// check which file resides on given offset
  RString FileOnOffset(QFileSize offsetBeg, QFileSize offsetEnd) const;

  /// check which file corresponds to given handle and offset
  static RString FileOnOffset(const FileServerHandle &handle, QFileSize offsetBeg, QFileSize offsetEnd);

  static bool FreeUnusedBanks(size_t sizeNeeded);

  /// number of files in bank
  int NFiles() const {return _files.NItems();}
  /// get the stored hash
  bool GetHash(Temp<char> &hash) const;
  /// get the hash to be signed using the specified signature version
  template <class HashCalculator>
  bool GetHash(Temp<char> &hash, int version, int level) const
  {
#define PROFILE_GET_HASH 0
#if _ENABLE_PERFLOG && PROFILE_GET_HASH
    int time1;
    int time2;
    int time3;
    LogF("GetHashProfiling START: %s (level=%d)", cc_cast(GetPrefix()), level);
#endif

#if _ENABLE_PERFLOG
    PROFILE_SCOPE_EX(GHash,*);
    if (PROFILE_SCOPE_NAME(GHash).IsActive())
    {
      PROFILE_SCOPE_NAME(GHash).AddMoreInfo(Format("%s level=%d, sigVer=%d", cc_cast(GetPrefix()), level, version));
    }
#endif
    switch (version)
    {
    case 1: 
      return GetHash(hash); 
      //break;
    case 2:
      {
        switch (level)
        {
        case 0:
        case 1:
          {
            // combine content hash with hash of pbo file list and pbo prefix
            // the final hash will be: HashOf(storedHash+fileListHash+pboPrefix)
            Temp<char> dataHash;
            if (!GetHash(dataHash))
              return false;
#if _ENABLE_PERFLOG && PROFILE_GET_HASH
            time1 = PROFILE_SCOPE_NAME(GHash).TimeSpentNorm();
#endif
            AutoArray<char> fileListHash;
            if (!GetFileListHash<HashCalculator>(fileListHash)) 
              return false;
#if _ENABLE_PERFLOG && PROFILE_GET_HASH
            time2 = PROFILE_SCOPE_NAME(GHash).TimeSpentNorm();
#endif
            // and hash it finally
            HashCalculator calculator;
            calculator.Add(dataHash.Data(), dataHash.Size());
            calculator.Add(fileListHash.Data(), fileListHash.Size());
            calculator.Add(GetPrefix().Data(), GetPrefix().GetLength());
            AutoArray<char> finalHash;
            calculator.GetResult(finalHash);
            hash.Realloc(finalHash.Data(), finalHash.Size()); //Ver2 hash
#if _ENABLE_PERFLOG && PROFILE_GET_HASH
            time3 = PROFILE_SCOPE_NAME(GHash).TimeSpentNorm();
            LogF("    %.2f ms spent, 1 NMT=%d", time1/100.0f);
            LogF("    %.2f ms spent, 2 NMT=%d", (time2-time1)/100.0f);
            LogF("    %.2f ms spent, 3 NMT=%d", (time3-time2)/100.0f);
#endif
            return true;
          }
          //break; //unreachable code
        case 2:
          {
            // the final hash will be: HashOf(nonDataHash+fileListHash+pboPrefix)
            AutoArray<char> nonDataHash;
            if (!GetNonDataHash<HashCalculator>(nonDataHash))
              return false;
#if _ENABLE_PERFLOG && PROFILE_GET_HASH
            time1 = PROFILE_SCOPE_NAME(GHash).TimeSpentNorm();
#endif
            AutoArray<char> fileListHash;
            if (!GetFileListHash<HashCalculator>(fileListHash)) 
              return false;
#if _ENABLE_PERFLOG && PROFILE_GET_HASH
            time2 = PROFILE_SCOPE_NAME(GHash).TimeSpentNorm();
#endif
            // and hash it finally
            HashCalculator calculator;
            calculator.Add(nonDataHash.Data(), nonDataHash.Size());
            calculator.Add(fileListHash.Data(), fileListHash.Size());
            calculator.Add(GetPrefix().Data(), GetPrefix().GetLength());
            AutoArray<char> finalHash;
            calculator.GetResult(finalHash);
            hash.Realloc(finalHash.Data(), finalHash.Size()); //Ver2 hash
#if _ENABLE_PERFLOG && PROFILE_GET_HASH
            time3 = PROFILE_SCOPE_NAME(GHash).TimeSpentNorm();
            LogF("    %.2f ms spent, 1 NMT=%d", time1/100.0f);
            LogF("    %.2f ms spent, 2 NMT=%d", (time2-time1)/100.0f);
            LogF("    %.2f ms spent, 3 NMT=%d", (time3-time2)/100.0f);
#endif
            return true;
          }
          //break; //unreachable code
        }
      }
    }
    return false;
  }

  static int CmpFileNames(const RString *a, const RString *b)
  {
    return strcmp(*a,*b);
  }

  template <class Calculator>
  bool GetFileListHash(AutoArray<char> &hash) const
  {
    AutoArray<RString> fileNames;
    fileNames.Realloc(_files.NItems());
    _files.ForEachF(AddFileName(fileNames));
    // QSort
    QSort(fileNames.Data(), fileNames.Size(), QFBank::CmpFileNames);
    // hash it
    Calculator calculator;
    for (int i=0; i<fileNames.Size(); i++) calculator.Add(fileNames[i].Data(), fileNames[i].GetLength());
    return calculator.GetResult(hash);
  }

  //@{ NonDataFileHash calculation
  static int CmpNonDataFiles(const NonDataFile *a, const NonDataFile *b)
  {
    return a->startOffset-b->startOffset;
  }

  template <class Calculator>
  void AddFileHash(Calculator &calculator, QIStream &in, int startOffset, int length) const
  {
    in.seekg(startOffset);
    {
      const int BufSiz = 16*1024;
      char buf[BufSiz];
      for(int processedBytes=0; processedBytes<length; )
      {
        int rdsiz = (length-processedBytes > BufSiz) ? BufSiz : (length-processedBytes);
        size_t rd = in.read(buf,rdsiz);
        calculator.Add(buf,rd);
        if (in.eof() || in.fail()) break;
        processedBytes += rd;
      }
    }
  }

  /// Returns true when __dummypadding__ file is present
  bool GetNonDataFileList(AutoArray<NonDataFile> &nonDataFiles) const
  {
    nonDataFiles.Realloc(_files.NItems());
    bool dummyFPP = false;
    _files.ForEachF(AddNonDataFile(nonDataFiles,dummyFPP));
    // QSort
    QSort(nonDataFiles.Data(), nonDataFiles.Size(), CmpNonDataFiles);
    return dummyFPP;
  }

#if _ENABLE_PBO_PROTECTION
  QFileSize EncryptHeadersSetup(QOStrStream &out, QOStrStream &in) const;
#endif

  template <class Calculator>
  bool GetNonDataHash(AutoArray<char> &hash) const
  {
    AutoArray<NonDataFile> nonDataFiles;
    bool dummyPaddingUsed = GetNonDataFileList(nonDataFiles);
    // hash it
    Calculator calculator;
    if (nonDataFiles.Size()>0)
    {
      QIStreamDataSource in;
      in.AttachSource(_fileAccess);
      int fileOld = in.tellg(); // store original position
      for (int i=0; i<nonDataFiles.Size(); i++) 
      {
#if !_ENABLE_PBO_PROTECTION
        (void)dummyPaddingUsed;
        AddFileHash(calculator, in, nonDataFiles[i].startOffset, nonDataFiles[i].length);
#else
        bool hashAdded = false;
        if (dummyPaddingUsed)
        {
          dummyPaddingUsed = false; //only once
          // get the original content of ___dummypadding___ file by encrypting pbo header using SETUP key
          BankHeaderProtectionInfo info;
          GetBankHeaderProtectionInfo(info);
          if (info.isProtected)
          {
            QOStrStream decryptedHeader;
            if (DecryptHeaders(decryptedHeader, in, info.protHeaderStart, info.protHeaderStart+info.protHeaderSize))
            {
              QOStrStream setupEncryptedHeader;
              QFileSize encrSize = EncryptHeadersSetup(setupEncryptedHeader, decryptedHeader);
              if (encrSize)
              {
                QIStrStream setupHeader(setupEncryptedHeader.str(), setupEncryptedHeader.pcount());
                AddFileHash(calculator, setupHeader,  nonDataFiles[i].startOffset-info.protHeaderStart, nonDataFiles[i].length);
                hashAdded = true;
              }
            }
          }
          else
          { // we need to Sign the unprotected files containing __dummypadding__ using its Setup K1 encrypted content too
            // get size of the header 
            int headerSize = nonDataFiles[i].startOffset - GetHeaderStartOffset();
            if (headerSize > 0)
            {
              // size of the header including padding
              int headerSizeIncPadding = headerSize + nonDataFiles[i].length;
              in.seekg(GetHeaderStartOffset());
              // read header
              Temp<unsigned char> headerData(headerSize);
              int rd = in.read(headerData.Data(), headerData.Size());
              if ( rd == headerData.Size() && !in.fail() )
              {
                // write header to new stream
                QOStrStream normalHeader;
                normalHeader.write(headerData.Data(), headerData.Size());
                QOStrStream setupEncryptedHeader;
                QFileSize encrSize = EncryptHeadersSetup(setupEncryptedHeader, normalHeader);
                if (encrSize==headerSizeIncPadding)
                {
                  QIStrStream setupHeader(setupEncryptedHeader.str(), setupEncryptedHeader.pcount());
                  AddFileHash(calculator, setupHeader,  nonDataFiles[i].startOffset-GetHeaderStartOffset(), nonDataFiles[i].length);
                  hashAdded = true;
                }
              }
            }
          }
        }
        if ( !hashAdded )
        {
          AddFileHash(calculator, in, nonDataFiles[i].startOffset, nonDataFiles[i].length);
        }
#endif
      }
      in.seekg(fileOld); // move read pointer to original position
    }
    else
    {
      static const char *dummyDataFiles = "nothing";
      calculator.Add(dummyDataFiles, 7); //hash dummy content
    }
    return calculator.GetResult(hash);
  }
  //@} NonDataFileHash calculation

  /// calculate hash from data
  template <typename Calculator>
  bool CalculateHash(Calculator &calculator) const
  {
    int dataBeg, dataEnd;
    if (!FindData(dataBeg, dataEnd)) return false;

    QIStreamDataSource in;
    in.AttachSource(_fileAccess);
    int fileOld = in.tellg(); // store original position

    bool ok = _fileAccess->GetSize() - (dataEnd + sizeof(bool)) > 0;
    if (ok)
    {
      in.seekg(dataEnd);
      bool reversed;
      in.read(&reversed, sizeof(bool));
      if (reversed)
      {
        calculator.Init(dataBeg, dataEnd);
        in.seekg(0);
        in.Process(calculator);
        calculator.Init(0, dataBeg);
        in.seekg(0);
        in.Process(calculator);
      }
      else
      {
        calculator.Init(0, dataEnd);
        in.seekg(0);
        in.Process(calculator);
      }
    }

    in.seekg(fileOld); // move read pointer to original position
    return ok;
  }

#if _ENABLE_PBO_PROTECTION
  bool DecryptHeaders(QOStream &out, QIStream &in, int rangeBeg, int rangeEnd) const;
#endif

  /// calculate hash from data
  template <typename Calculator>
  bool CalculateRangeHash(Calculator &calculator, int rangeBeg, int rangeEnd
#if _ENABLE_PBO_PROTECTION
    , bool isProtHeader
#endif //#if _ENABLE_PBO_PROTECTION
  ) const
  {
    QIStreamDataSource in;
    in.AttachSource(_fileAccess);
    int fileOld = in.tellg(); // store original position

    in.seekg(rangeBeg);
#if _ENABLE_PBO_PROTECTION
    if (isProtHeader)
    {
      QOStrStream decrypted;
      if (!DecryptHeaders(decrypted,in,rangeBeg,rangeEnd))
        return false;
      calculator.Add(decrypted.str(), decrypted.pcount());
    }
    else
#endif //#if _ENABLE_PBO_PROTECTION
    {
      const int BufSiz = 4*1024;
      char buf[BufSiz];
      for(int pos=rangeBeg; pos<rangeEnd; pos+=BufSiz)
      {
        int rdsiz = rangeEnd-pos;
        if (rdsiz>BufSiz) rdsiz=BufSiz;
        size_t rd = in.read(buf,rdsiz);
        calculator.Add(buf,rd);
        if (in.eof() || in.fail()) break;
      }
    }

    in.seekg(fileOld); // move read pointer to original position
    return true;
  }

  void AttachSource(QIStreamDataSource &in)
  {
    in.AttachSource(_fileAccess);
  }

  /// calculation of hash - initialization (used with engine BankHashCalculatorAsync and BankHashCalculator in 'Security solution DSSignFile tool')
  template <typename Calculator>
  bool CalculateInit(Calculator &calculator) const
  {
    int dataBeg, dataEnd;
    if (!FindData(dataBeg, dataEnd)) return false;

#if _ENABLE_PBO_PROTECTION
    // get protection info
    BankHeaderProtectionInfo protInfo;
    GetBankHeaderProtectionInfo(protInfo);

    if (protInfo.isProtected)
    {
      if (protInfo.protHeaderStart + protInfo.headerSize != dataBeg)
      {
        Assert(false);
        RptF("Bad values in header protection info.");
      }

      // change dataBeg - skip padding
      dataBeg = protInfo.protHeaderStart + protInfo.protHeaderSize;
      Assert(dataBeg <= dataEnd);
    }
#endif //#if _ENABLE_PBO_PROTECTION


    QIStreamDataSource in;
    in.AttachSource(_fileAccess);
    int fileOld = in.tellg(); // store original position

    bool ok = _fileAccess->GetSize() - (dataEnd + sizeof(bool)) > 0;
    if (ok)
    {
      in.seekg(dataEnd);
      bool reversed;
      in.read(&reversed, sizeof(bool));
      if (reversed)
      {
#if _ENABLE_PBO_PROTECTION
        if (protInfo.isProtected)
        {
          calculator.AddRange(dataBeg, dataEnd, false);
          calculator.AddRange(0, protInfo.protHeaderStart, false);
          calculator.AddRange(protInfo.protHeaderStart, dataBeg, true);
        }
        else
        {
          calculator.AddRange(dataBeg, dataEnd, false);
          calculator.AddRange(0, dataBeg, false);
        }
#else //#if _ENABLE_PBO_PROTECTION
        calculator.AddRange(dataBeg, dataEnd);
        calculator.AddRange(0, dataBeg);
#endif //#if _ENABLE_PBO_PROTECTION else
      }
      else
      {
#if _ENABLE_PBO_PROTECTION
        if (protInfo.isProtected)
        {
          calculator.AddRange(0, protInfo.protHeaderStart, false);
          calculator.AddRange(protInfo.protHeaderStart, dataBeg, true);
          calculator.AddRange(dataBeg, dataEnd, false);        
        }
        else
        {
          calculator.AddRange(0, dataEnd, false);
        }
#else //#if _ENABLE_PBO_PROTECTION
        calculator.AddRange(0, dataEnd);
#endif //#if _ENABLE_PBO_PROTECTION else

      }
    }

    in.seekg(fileOld); // move read pointer to original position
    return ok;
  }

  /// asynchronous calculation of hash - processing
  template <typename Calculator>
  bool CalculateProcess(Calculator &calculator) const
  {
    QIStreamDataSource in;
    in.AttachSource(_fileAccess);
    int fileOld = in.tellg(); // store original position
    bool done = calculator.Process(in);
    in.seekg(fileOld); // move read pointer to original position
    return done;
  }

#if _ENABLE_PBO_PROTECTION
  /// protection info
  struct BankHeaderProtectionInfo
  {
    BankHeaderProtectionInfo() 
      :isProtected(false), protHeaderStart(0), protHeaderSize(0), headerSize(0)
    {}

    /// if the PBO id protected using headers encryption
    bool isProtected;
    /// start offset of protected header
    QFileSize protHeaderStart;
    /// size of protected header
    QFileSize protHeaderSize;
    /// size of decrypted header
    QFileSize headerSize;
  };

  // gets protection info
  void GetBankHeaderProtectionInfo(BankHeaderProtectionInfo& info) const;
  // create serial header protection class
  IFilebankEncryption* CreateSerialHeaderProtection() const;
  /// returns header starting offset (from the beginning of the file)
  QFileSize GetHeaderStartOffset() const {return _headerStart;}
#endif
protected:
  // implementation
  // search for data area in bank
  bool FindData(int &dataBeg, int &dataEnd) const;
};
TypeIsMovable(QFBank)

/// list of all active file banks
class BankList
{
  RefArray<QFBank> _banks;
  ReadersWriterLock _lock;

  bool _orderDirty;
  
  typedef void (*FlushCallback)(QFBank &bank);
  
  AutoArray<FlushCallback> _flushCallback;
  
  int Add(QFBank *newBank);

  public: 

  WriterLock &GetWriterLock(){return _lock;}
  ReadersWriterLock &GetReaderLock(){return _lock;}

  BankList();
  
  struct ReadAccess
  {
    BankList &_list;
    ScopeLock<ReadersWriterLock> _lock;

    ReadAccess(BankList &list):_list(list),_lock(list.GetReaderLock())
    {
    }

    int Size() const {return _list._banks.Size();}  
    QFBank &operator [] (int i) {return *_list._banks.Set(i);}
    QFBank &Get(int i) {return *_list._banks.Get(i);}
  };
  struct WriteAccess
  {
    BankList &_list;
    ScopeLock<WriterLock> _lock;

    WriteAccess(BankList &list):_list(list),_lock(list.GetWriterLock())
    {
    }

    int Size() const {return _list._banks.Size();}  
    QFBank &operator [] (int i) {return *_list._banks.Set(i);}
    QFBank &Set(int i) {return *_list._banks.Set(i);}
    QFBank &Get(int i) {return *_list._banks.Get(i);}
    void Clear() {return _list._banks.Clear();}
    int Add(QFBank *newBank){return _list.Add(newBank);}

  };

  friend struct ReadAccess;
  friend struct WriteAccess;

//   //@{ emulate AutoArray<QFBank> using RefArray<QFBank>
//   int Size() const {return base::Size();}  
//   QFBank &operator [] (int i) {return *base::Set(i);}
//   QFBank &Set(int i) {return *base::Set(i);}
//   const QFBank &Get(int i) {return *base::Get(i);}
//   void Clear() {return base::Clear();}
  
  /// load a new bank into the list
  QFBank *Load(
    const RString &path,
    const RString &bankPrefix,const RString &bName, bool emptyPrefix,
    OpenCallback beforeOpen=NULL, OpenCallback afterOpen=NULL,
    BankContextBase *context=NULL
  );
  void Unload(const RString & bankPrefix,const RString &bName, bool emptyPrefix);
  void Lock(const RString &prefix);
  void Unlock(const RString &prefix);
  void SetLockable(const RString &prefix, bool lockable);

  /// Destroy file bank with any prefix starting with a given string
  void Remove(const char *prefix);
  
  void Remove(QFBank &bank);

  //! unload all unused bank files
  bool UnloadUnused();
  
  void RegisterUnloadCallback(FlushCallback flush){_flushCallback.Add(flush);}

  QFBank* FindBank(const char* path);
  int BankIndex(const QFBank &bank) const;
  RString LoadBank(RString filename, RString prefix);
};

extern BankList GFileBanks;


//! determine if files in the banks can be "patched"
extern bool GEnablePatching;
//! determine if file banks can be used
extern bool GUseFileBanks;
//! determine if file access should be logged
extern bool GLogFileOps;

#endif


