#ifdef _MSC_VER
#pragma once
#endif

#ifndef _EL_DATA_SIGNATURES_HPP
#define _EL_DATA_SIGNATURES_HPP

#ifndef _WIN32
#define USE_HASH_LIB
#endif

#include <Es/Strings/rString.hpp>
#include <Es/Types/pointers.hpp>
#include <Es/Containers/staticArray.hpp>
#include <El/QStream/qbStream.hpp>

#ifdef USE_HASH_LIB
#include "hashwrapper.h"
#include "sha1wrapper.h"
#endif

#define SIGNATURES_TEST_ONLY 0

// BIS_SIGNATURE_VERSION review
// 1 ... old style signature signs hash of data content and there is no version number stored after the serialized data _content
// 2 ... signature signs hash of (data content hash)+(pbo file list hash)+(pbo prefix) and there is version number stored
//   ... moreover  signs hash of (nonData cont hash)+(pbo file list hash)+(pbo prefix) 
#define BIS_SIGNATURE_VERSION 2

class SerializeBinStream;

struct DSKey
{
  RString _name;
  Temp<char> _content;

  bool operator ==(const DSKey &with) const;

  void SerializeBin(SerializeBinStream &stream);
  bool Load(RString filename);
};
TypeIsMovable(DSKey)

struct DSHash
{
  Temp<char> _content;
};

struct DSSignature
{
  DSKey _key;
  int     _version;
  Temp<char> _content1;  // sigVer1
  Temp<char> _content2;  // sigVer2 ... signature of hashof(contentHash+fileListHash+pbo prefix)
  Temp<char> _content3;  // sigVer2 ... signature of hashof(nonDataHash+fileListHash+pbo prefix)

  void SerializeBin(SerializeBinStream &stream);
  bool Load(RString filename);
  int  Version() const { return _version; }
  bool LoadKey(RString filename); //load only key name
};

// Cache of signatures used to find fast whether given file contains given signature 
struct SignaturesCacheItem
{
  RString _filename;               // file
  FindArray<RString> _signatures;  // and its signatures

  const char *GetKey() const {return _filename;}
};
TypeIsMovableZeroed(SignaturesCacheItem);

template <>
struct MapClassTraits<SignaturesCacheItem> : public MapClassTraitsNoCase<SignaturesCacheItem>
{
};

/// SignaturesCache to find signatures fast
typedef MapStringToClass< SignaturesCacheItem, AutoArray<SignaturesCacheItem> > SignaturesCache;

class DataSignatures
{
public:
  // in - filename
  // in - acceptedKeys, acceptedKeysCount
  // out - signature
  // return - found
  static bool FindSignature(
    DSSignature &signature,
    RString filename,
    const DSKey *acceptedKeys, int acceptedKeysCount, int reqVersion=0); //any sigVer is sufficient by default

  // in - filename
  // out - hash
  // return - processed without errors
  static bool GetHash(DSHash &hash, RString filename);

  // in - hash
  // in - signature
  // return - signature is valid
  static bool VerifySignature(const DSHash &hash, const DSSignature &signature, int level=0);

  //@{ Light-weighted verification checking
  static SignaturesCache sigCache;

  // Test whether signature file is present for acceptedKeys list
  static bool PreverifySignature(RString filename, StaticArrayAuto<RString> &acceptedKeys);
  static bool HasSignatureFromList(SignaturesCacheItem &sigItem, StaticArrayAuto<RString> &acceptedKeys);
  //@}

};

class HashCalculator
{
protected:
  void *_provider;
  void *_hash;
  mutable bool _ok;
#ifdef USE_HASH_LIB
  Ref<hashwrapper> shaWrapper;
#endif

public:
  //! Constructor
  HashCalculator();
  //! Destructor
  ~HashCalculator();
  //! initialize
  void Reset();
  //! add memory block
  void Add(const void *data, int len);
  //! add memory block - OStream like interface (useful for QIStream::copy)
  __forceinline void write(const void *data, int len) {Add(data,len);}
  //! add single character
  void Add(char c) {Add(&c, sizeof(char));}
  //! get result of all Add operations since Reset()
  bool GetResult(AutoArray<char> &result) const;
};

class QIStream;
class QFBank;

class BankHashCalculatorAsync
{
protected:
  void *_provider;
  void *_hash;
  bool _ok;
  bool _cancel;
  RString _bankPrefix;
#ifdef USE_HASH_LIB
  Ref<hashwrapper> shaWrapper;
#endif

  int _bankIndex;

  struct BankRange
  {
    BankRange():_offsetBeg(0), _offsetEnd(0)
#if _ENABLE_PBO_PROTECTION
      , _isProtHeader(false) 
#endif
    {}

    int _offsetBeg;
    int _offsetEnd;
#if _ENABLE_PBO_PROTECTION
    bool _isProtHeader; ///< if this range is protected header
#endif
    ClassIsSimple(BankRange)
  };

  AutoArray<BankRange> _ranges;
  int _index;
  int _pos;
  bool _async;


#if _ENABLE_PBO_PROTECTION
  /// encrypted data of protected range
  AutoArray<unsigned char> _protRange;

  /// processes protected range
  bool ProcessProtectedRange();
#endif
public:
  //! Constructor
  BankHashCalculatorAsync(int bankIndex, bool async=true);
  //! Destructor
  ~BankHashCalculatorAsync();
  //! Update function
  /** returns true if done */
  bool Process();
  //! the result of testing
  bool IsValid();

  // Callback functions
  //! Add a range for testing
  void AddRange(int dataBeg, int dataEnd
#if _ENABLE_PBO_PROTECTION
    , bool isProtHeader
#endif
  );
  //! Calculate hash from stream if data are ready
  bool Process(QIStream &in);
};

//! It computes NonDataHash needed for sigVer.2, level=2 signatures checks used for both DataSignatureAnswer and FileSignatureAnswer
class BankSignatureCheckAsync
{
protected:
  int _bankIndex;
  int _sigVer;
  int _level;

  int _index;
  int _curFile;
  int _type;
  int _maxFrames; //maximal count of calls to Process to finish the check
  bool _dummyPaddingFilePresent;
  AutoArray<NonDataFile> _nonDataFiles;
  HashCalculator _calculator;

public:
  BankSignatureCheckAsync(int bankIndex, int sigVer, int level, int type, int maxFrames=20);

  bool GetHash(Temp<char> &hash);
  //! Update function
  // returns true if done
  bool Process();
  int GetBankIndex() const { return _bankIndex; }
};

#endif

