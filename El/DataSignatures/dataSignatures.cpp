#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <El/elementpch.hpp>
#include "dataSignatures.hpp"

#include <Es/Files/fileContainer.hpp>
#include <Es/Framework/appFrame.hpp>

#include <El/QStream/qbStream.hpp>
#include <El/QStream/serializeBin.hpp>

#if _ENABLE_PBO_PROTECTION
#include <EL/Encryption/PBOHeaderProtection.h>
#endif

#if !defined _DISABLE_SIGNATURES && defined _WIN32
#include <Es/Common/win.h>
#include <Wincrypt.h>

static bool AcquireContext(HCRYPTPROV *provider, bool privateKeyAccess)
{
  DWORD flags = privateKeyAccess ? 0 : CRYPT_VERIFYCONTEXT;
  if (CryptAcquireContext(provider, NULL, NULL, PROV_RSA_FULL, flags)) return true;
  // create a new key container
  if (GetLastError() == NTE_BAD_KEYSET)
  {
    flags |= CRYPT_NEWKEYSET;
    if (CryptAcquireContext(provider, NULL, NULL, PROV_RSA_FULL, flags)) return true;
  }
  return false;
}
#endif

bool DSKey::operator ==(const DSKey &with) const
{
  if (strcmp(_name, with._name) != 0) return false;
  if (_content.Size() != with._content.Size()) return false;
  return memcmp(_content.Data(), with._content.Data(), _content.Size()) == 0;
}

template <typename Type>
void SerializeBin(SerializeBinStream &stream, Temp<Type> &data)
{
  if (stream.IsLoading())
  {
    int size = stream.LoadInt();
    if (size<0) size = 0; // avoid crash when reading behind the EOF
    data.Realloc(size);
    for (int i=0; i<data.Size(); i++)
    {
      stream.Transfer(data.Set(i));
    }
  }
  else
  {
    stream.SaveInt(data.Size());
    // transfer array data
    for (int i=0; i<data.Size(); i++)
    {
      Type val = data.Get(i);
      stream.Transfer(val);
    }
  }
}

void DSKey::SerializeBin(SerializeBinStream &stream)
{
  stream << _name;
  ::SerializeBin(stream, _content);
}

bool DSKey::Load(RString filename)
{
  QIFStream in;
  in.open(filename);
  SerializeBinStream stream(&in);
  SerializeBin(stream);
  return !in.fail();
}

void DSSignature::SerializeBin(SerializeBinStream &stream)
{
  _key.SerializeBin(stream);
  // Both signature versions, sigVer1 & sigVer2 must be stored in the new *.bisign file
  /* NOTE: Version number at the end of the sigVer2 *.bisign file was not sufficient,
      as the old client, who is only SigVer1 aware read it as SigVer1 (it is a feature!), 
      but the hash computed by old server is only the hash of data content,
      so it does not match the hash this signature is signing 
      (data content hash + file list hash + pbo prefix)
  */
  ::SerializeBin(stream, _content1);  // Ver1
  stream.Transfer(_version);
  if (stream.IsLoading())
  {
    if (stream.GetLoadStream()->eof())
    {
      _version = 1;
      return;  // no other data
}
  }
  ::SerializeBin(stream, _content2);  // Ver2 ... signature of hashof(contentHash+fileListHash+pbo prefix)
  ::SerializeBin(stream, _content3);  // Ver2 ... signature of hashof(nonDataHash+fileListHash+pbo prefix)
}

bool DSSignature::Load(RString filename)
{
  QIFStream in;
  in.open(filename);
  SerializeBinStream stream(&in);
  SerializeBin(stream);
  return !in.fail();
}

bool DSSignature::LoadKey(RString filename)
{
  QIFStream in;
  in.open(filename);
  SerializeBinStream stream(&in);
  _key.SerializeBin(stream);
  return !in.fail();
}

struct SelectSignatureContext
{
  DSSignature &_signature;
  const DSKey *_acceptedKeys;
  int _acceptedKeysCount;
  int _reqVersion;

  SelectSignatureContext(DSSignature &signature,
    const DSKey *acceptedKeys, int acceptedKeysCount, int reqVersion=0)
    : _signature(signature), _acceptedKeys(acceptedKeys), _acceptedKeysCount(acceptedKeysCount), _reqVersion(reqVersion)
  {
  }
};

bool SelectSignature(const FileItem &file, SelectSignatureContext &ctx)
{
  DSSignature signature;
  if (signature.Load(file.path + file.filename))
  {
    for (int i=0; i<ctx._acceptedKeysCount; i++)
    {
      if (  (signature._key == ctx._acceptedKeys[i])   // key is accepted by server
         && (signature.Version() >= ctx._reqVersion) ) // signature version is sufficient
      {
        ctx._signature = signature;
        return true;
      }
    }
  }
  return false;
}

bool DataSignatures::FindSignature(
    DSSignature &signature,
    RString filename,
    const DSKey *acceptedKeys, int acceptedKeysCount, int reqVersion)
{
  RString path, name;
  const char *ptr = strrchr(filename, '\\');
  if (ptr++)
  {
    path = filename.Substring(0, ptr - cc_cast(filename));
    name = ptr;
  }
  else
  {
    name = filename;
  }

  SelectSignatureContext ctx(signature, acceptedKeys, acceptedKeysCount, reqVersion);

  return ForMaskedFile(path, name + RString(".*.bisign"), SelectSignature, ctx);
}

bool StoreSignatureToCache(const FileItem &file, SignaturesCacheItem &cacheItem)
{
  DSSignature signature;
  if (signature.LoadKey(file.path + file.filename))
  {
    cacheItem._signatures.AddUnique(signature._key._name);
  }
  return false; //process all signatures
}

// static signatureCache instanciation
SignaturesCache DataSignatures::sigCache;

bool DataSignatures::PreverifySignature(RString filename, StaticArrayAuto<RString> &acceptedKeys)
{
  RString path, name;
  const char *ptr = strrchr(filename, '\\');
  if (ptr++)
  {
    path = filename.Substring(0, ptr - cc_cast(filename));
    name = ptr;
  }
  else
  {
    name = filename;
  }
  SignaturesCacheItem& sigItem = sigCache[filename];
  if ( sigCache.IsNull(sigItem) )
  {
    SignaturesCacheItem newItem;
    newItem._filename = filename;
    ForMaskedFile(path, name + RString(".*.bisign"), StoreSignatureToCache, newItem);
    sigCache.Add(newItem);
    return HasSignatureFromList(newItem, acceptedKeys);
  }
  return HasSignatureFromList(sigItem, acceptedKeys);
}

bool DataSignatures::HasSignatureFromList(SignaturesCacheItem &sigItem, StaticArrayAuto<RString> &acceptedKeys)
{
  for (int i=0, siz=acceptedKeys.Size(); i<siz; i++)
  {
    for (int j=0, siz2=sigItem._signatures.Size(); j<siz2; j++)
    {
      if (strcmpi(acceptedKeys[i],sigItem._signatures[j])==0)
      {
        return true;
      }
    }
  }
  return false;
}

#if !defined _DISABLE_SIGNATURES && defined _WIN32

struct CalculateHash
{
  HCRYPTHASH _hash;
  bool &_ok;

  CalculateHash(HCRYPTHASH hash, bool &ok) : _hash(hash), _ok(ok) {_ok = true;}

  void operator ()(char *data, int size)
  {
    if (!CryptHashData(_hash, (BYTE *)data, size, 0)) _ok = false;
  }
};

#endif

const int PreloadSize = 256 * 1024;

#ifdef USE_HASH_LIB
struct HashlibCalculator
{
  bool &_ok;
  Ref<hashwrapper> shaWrapper;

  HashlibCalculator(bool &ok) : _ok(ok) 
  {
    shaWrapper = new sha1wrapper();
    shaWrapper->resetContext();
    _ok = true;
  }
  void GetHash(unsigned char *data) { shaWrapper->GetHash(data); }

  void operator ()(char *data, int size)
  {
    shaWrapper->updateContext((unsigned char *)data, size);
  }

};

#include <El/Credits/credits.hpp>
REGISTER_CREDITS("hashlib++", "2007 Benjamin Grüdelbach",STANDARD_DISCLAIMER)

#endif

bool DataSignatures::GetHash(DSHash &hash, RString filename)
{
#ifdef _DISABLE_SIGNATURES
  return false;
#elif defined USE_HASH_LIB
  bool done = false;
  bool ok;
  HashlibCalculator func(ok);

  QIFStream file;
  file.open(filename);
  // file.PreReadSequential(PreloadSize);
  file.Process(func);
  file.close();

  if (!ok || (file.fail() && !file.eof()))
  {
    LogF("CryptHashData failed");
  }
  else
  {
    DWORD size = SHA1HashSize;
    hash._content.Realloc(size);
    func.GetHash((unsigned char *)unconst_cast<char *>(hash._content.Data()));
    done = true;
  }
  return done;

#else
  bool done = false;

  HCRYPTPROV provider = NULL;
  HCRYPTHASH handle = NULL;

  if (!AcquireContext(&provider, false)) 
  {
    LogF("AcquireContext failed");
  }
  else if (!CryptCreateHash(provider, CALG_SHA, NULL, 0, &handle))
  {
    LogF("CryptCreateHash failed");
  }
  else
  {
    bool ok;
    CalculateHash func(handle, ok);

    QIFStream file;
    file.open(filename);
    // file.PreReadSequential(PreloadSize);
    file.Process(func);
    file.close();

    if (!ok || (file.fail() && !file.eof()))
    {
      LogF("CryptHashData failed");
    }
    else
    {
      DWORD size = 0;
      if (!CryptGetHashParam(handle, HP_HASHVAL, NULL, &size, 0)) 
      {
        LogF("CryptGetHashParam failed");
      }
      else
      {
        hash._content.Realloc(size);
        if (!CryptGetHashParam(handle, HP_HASHVAL, (BYTE *)hash._content.Data(), &size, 0))
        {
          LogF("CryptGetHashParam failed");
        }
        else
        {
          // hash finally received
          done = true;
        }
      }
    }
  }

  if (handle) CryptDestroyHash(handle);
  if (provider) CryptReleaseContext(provider, 0);
  return done;
#endif
}

#ifdef USE_HASH_LIB
  #ifndef _WIN32
  typedef unsigned int ALG_ID;
  typedef struct _PUBLICKEYSTRUC {  BYTE bType;  BYTE bVersion;  WORD reserved;  ALG_ID aiKeyAlg; } BLOBHEADER;
  typedef struct _RSAPUBKEY {  DWORD magic;  DWORD bitlen;  DWORD pubexp; } RSAPUBKEY;
  #endif
#include <El/CDKey/serial.hpp>
#endif
bool DataSignatures::VerifySignature(const DSHash &hash, const DSSignature &signature, int level)
{
#ifdef _DISABLE_SIGNATURES
  return false;
#elif defined USE_HASH_LIB
  int skipBytes = sizeof(BLOBHEADER) + sizeof(RSAPUBKEY);
  const unsigned char *keyData = (const unsigned char *)signature._key._content.Data() + (skipBytes - sizeof(DWORD));
  int keySize = signature._key._content.Size() - skipBytes;
  Temp<char> calculatedHash;
  calculatedHash.Realloc(keySize);
  // Beware! Hash computed by RSADecrypt is in different endian.
  switch (signature.Version())
  {
  case 1:
  RSADecrypt((unsigned char *)calculatedHash.Data(), 
      (const unsigned char *)signature._content1.Data(), signature._content1.Size(),
             keyData, keySize
             );
    break;
  case 2:
    {
      switch (level)
      {
      case 2:
        RSADecrypt((unsigned char *)calculatedHash.Data(), 
          (const unsigned char *)signature._content3.Data(), signature._content3.Size(),
          keyData, keySize
          );
        break;
      default:
        RSADecrypt((unsigned char *)calculatedHash.Data(), 
          (const unsigned char *)signature._content2.Data(), signature._content2.Size(),
          keyData, keySize
          );
        break;
      }
    }
    break;
  }
  Temp<char> revHash; revHash.Realloc(hash._content.Size());
  for (int i=0, siz=hash._content.Size()-1; i<=siz; i++)
  {
    revHash.Set(i) = calculatedHash.Get(siz-i);
  }
  bool ok = (memcmp(revHash.Data(),hash._content.Data(),hash._content.Size())==0);
  return ok;
#else
  bool ok = false;

  HCRYPTPROV provider = NULL;
  HCRYPTKEY key = NULL;
  HCRYPTHASH handle = NULL;

  if (!AcquireContext(&provider, false))
  {
    LogF("AcquireContext failed");
  }
  else if (!CryptImportKey(
    provider, (const BYTE *)signature._key._content.Data(), signature._key._content.Size(), NULL, 0, &key))
  {
    LogF("CryptImportKey failed");
  }
  else if (!CryptCreateHash(provider, CALG_SHA, NULL, 0, &handle))
  {
    LogF("CryptCreateHash failed");
  }
  else if (!CryptSetHashParam(handle, HP_HASHVAL, (BYTE *)hash._content.Data(), 0))
  {
    LogF("CryptSetHashParam failed");
  }
  else
  {
    switch (signature.Version())
    {
    case 1:
      ok = CryptVerifySignature(handle, (BYTE *)signature._content1.Data(), signature._content1.Size(), key, NULL, 0) == TRUE;
      break;
    case 2:
      {
        switch (level)
        {
        case 2:
          ok = CryptVerifySignature(handle, (BYTE *)signature._content3.Data(), signature._content2.Size(), key, NULL, 0) == TRUE;
          break;
        default:
          ok = CryptVerifySignature(handle, (BYTE *)signature._content2.Data(), signature._content2.Size(), key, NULL, 0) == TRUE;
          break;
        }
      }
      break;
  }
  }

  if (handle) CryptDestroyHash(handle);
  if (key) CryptDestroyKey(key);
  if (provider) CryptReleaseContext(provider, 0);
  return ok;
#endif
}

#ifndef USE_HASH_LIB
#define HASH ((HCRYPTHASH)_hash)
#define PROVIDER ((HCRYPTPROV)_provider)
#define PHASH ((HCRYPTHASH*)&_hash)
#define PPROVIDER ((HCRYPTPROV*)&_provider)
#endif

HashCalculator::HashCalculator()
{
  _provider = NULL;
  _hash = NULL;

#ifdef _DISABLE_SIGNATURES
  _ok = false;
#elif defined USE_HASH_LIB
  _ok = true;
  shaWrapper = new sha1wrapper();
  Reset();
#else
  _ok = true;

  if (!AcquireContext(PPROVIDER, false)) 
  {
    LogF("AcquireContext failed");
    _provider = NULL;
    _ok = false;
  }
  else
  {
    Reset();
  }
#endif
}

HashCalculator::~HashCalculator()
{
#ifdef _DISABLE_SIGNATURES
  return;
#elif ! defined USE_HASH_LIB
  if (_hash) CryptDestroyHash(HASH);
  if (_provider) CryptReleaseContext(PROVIDER, 0);
  _hash = NULL;
#endif
}

void HashCalculator::Reset()
{
#ifdef _DISABLE_SIGNATURES
  return;
#elif defined USE_HASH_LIB
  shaWrapper->resetContext();
  _ok = true;
#else
  if (!_provider) return;
  if (_hash)
  {
    CryptDestroyHash(HASH);
    _hash = NULL;
  }
  _ok = true;
  if (!CryptCreateHash(PROVIDER, CALG_SHA, NULL, 0, PHASH))
  {
    LogF("CryptCreateHash failed");
    _hash = NULL;
    _ok = false;
  }
#endif
}

void HashCalculator::Add(const void *data, int len)
{
#ifdef _DISABLE_SIGNATURES
  return;
#elif defined USE_HASH_LIB
  shaWrapper->updateContext((unsigned char *)data, len);
#else
  if (_ok)
  {
    if (!CryptHashData(HASH, (BYTE *)data, len, 0))
    {
      LogF("CryptHashData failed");
      _ok = false;
    }
  }
#endif
}

bool HashCalculator::GetResult(AutoArray<char> &result) const
{
#ifndef _DISABLE_SIGNATURES
#if defined USE_HASH_LIB
   if (_ok)
   {
     result.Realloc(SHA1HashSize);
     result.Resize(SHA1HashSize);
     shaWrapper->GetHash((unsigned char *)result.Data());
   }
#else
  if (_ok)
  {
    DWORD size = 0;
    if (!CryptGetHashParam(HASH, HP_HASHVAL, NULL, &size, 0)) 
    {
      LogF("CryptGetHashParam failed");
      _ok = false;
    }
    else
    {
      result.Realloc(size);
      result.Resize(size);
      if (!CryptGetHashParam(HASH, HP_HASHVAL, (BYTE *)result.Data(), &size, 0))
      {
        LogF("CryptGetHashParam failed");
        _ok = false;
      }
    }
  }
#endif
#endif
  return _ok;
}

BankHashCalculatorAsync::BankHashCalculatorAsync(int bankIndex, bool async)
{
  _provider = NULL;
  _hash = NULL;
  _ok = true;

  _bankIndex = bankIndex;
  _index = -1;
  _pos = 0;
  _async = async;

#ifdef _DISABLE_SIGNATURES
  _cancel = true;
#else
#if defined USE_HASH_LIB
  shaWrapper = new sha1wrapper();
  shaWrapper->resetContext();
  _cancel = false;
#else
  _cancel = false;
  if (!AcquireContext(PPROVIDER, false)) 
  {
    LogF("AcquireContext failed");
    _provider = NULL;
    _ok = false;
  }
  else if (!CryptCreateHash(PROVIDER, CALG_SHA, NULL, 0, PHASH))
  {
    LogF("CryptCreateHash failed");
    _hash = NULL;
    _ok = false;
  }
#endif
  BankList::ReadAccess banks(GFileBanks);
  if (_bankIndex >= banks.Size())
  {
    _cancel = true;
  }
  else
  {
    QFBank &bank = banks[_bankIndex];
    bank.CalculateInit(*this);
    _bankPrefix = bank.GetPrefix();
  }
#endif
}

BankHashCalculatorAsync::~BankHashCalculatorAsync()
{
#ifndef _DISABLE_SIGNATURES
#if !defined USE_HASH_LIB
  if (_hash) CryptDestroyHash(HASH);
  if (_provider) CryptReleaseContext(PROVIDER, 0);
#endif
#endif
}

bool BankHashCalculatorAsync::Process()
{
#ifdef _DISABLE_SIGNATURES
  return true;
#else
  if (_cancel || !_ok) return true;
  BankList::ReadAccess banks(GFileBanks);
  if (_bankIndex >= banks.Size())
  {
    _cancel = true;
    return true;
  }
  else
  {
    QFBank &bank = banks[_bankIndex];
    if (_bankPrefix != bank.GetPrefix())
    {
      _cancel = true;
      return true;
    }
    bool done = bank.CalculateProcess(*this);
#if SIGNATURES_TEST_ONLY
    if (_index >= 0 && _index < _ranges.Size())
    {
      float factor = (float)(_pos - _ranges[_index]._offsetBeg) / (float)(_ranges[_index]._offsetEnd - _ranges[_index]._offsetBeg);
      DIAG_MESSAGE(500, Format("File: %s - %.0f%% done", cc_cast(bank.GetOpenName()), 100.0f * factor));
    }
#endif
    return done;
  }
#endif
}

bool BankHashCalculatorAsync::IsValid()
{
#ifdef _DISABLE_SIGNATURES
  return true;
#else
  if (_cancel) return true;
  if (!_ok) return false;

  // calculated hash
#ifdef USE_HASH_LIB
  Temp<char> calculatedHash;
  calculatedHash.Realloc(SHA1HashSize);
  shaWrapper->GetHash((unsigned char *)calculatedHash.Data());
#else
  DWORD size = 0;
  if (!CryptGetHashParam(HASH, HP_HASHVAL, NULL, &size, 0)) 
  {
    LogF("CryptGetHashParam failed");
    return false;
  }
  Temp<char> calculatedHash;
  calculatedHash.Realloc(size);
  if (!CryptGetHashParam(HASH, HP_HASHVAL, (BYTE *)calculatedHash.Data(), &size, 0))
  {
    LogF("CryptGetHashParam failed");
    return false;
  }
#endif

  BankList::ReadAccess banks(GFileBanks);
  // stored hash
  Temp<char> storedHash;
  if (_bankIndex >= banks.Size())
  {
    _cancel = true;
    return true;
  }
  else
  {
    QFBank &bank = banks[_bankIndex];
    if (_bankPrefix != bank.GetPrefix())
    {
      _cancel = true;
      return true;
    }
    if (!bank.GetHash(storedHash)) return false;
  }

  // compare
  if (calculatedHash.Size() != storedHash.Size()) return false;
  for (int i=0; i<calculatedHash.Size(); i++)
  {
    if (calculatedHash[i] != storedHash[i]) return false;
  }

  return true;
#endif
}

void BankHashCalculatorAsync::AddRange(int dataBeg, int dataEnd
#if _ENABLE_PBO_PROTECTION
, bool isProtHeader
#endif //#if _ENABLE_PBO_PROTECTION
)
{
  int index = _ranges.Add();
  _ranges[index]._offsetBeg = dataBeg;
  _ranges[index]._offsetEnd = dataEnd;
#if _ENABLE_PBO_PROTECTION
  _ranges[index]._isProtHeader = isProtHeader;
#endif
}

#if _ENABLE_PBO_PROTECTION
bool BankHashCalculatorAsync::ProcessProtectedRange()
{
  if (_protRange.Size() == 0)
    return false;

  BankList::ReadAccess access(GFileBanks);
  if (_bankIndex < 0 || _bankIndex >= access.Size())
    return false;

  QFBank& bank = access[_bankIndex];
  SRef<IFilebankEncryption> headerProtection(bank.CreateSerialHeaderProtection());
  if (headerProtection.IsNull())
  {
    LogF("Creation of bank header protection class failed.");
    return false;
  }

  // decrypt header
  QIStrStream encrypted;
  encrypted.init(_protRange.Data(), _protRange.Size());

  QOStrStream decrypted;

  PBOHeaderProtection::DecryptedHeaderInfo info;
  if (!PBOHeaderProtection::DecryptHeaders(decrypted, encrypted, headerProtection, &info))
  {
    LogF("Decryption of range failed.");
    return false;
  }

  if (decrypted.pcount() != _protRange.Size())
  {
    LogF("Decryption of range failed - invalid decrypted size");
    return false;
  }

#ifdef USE_HASH_LIB
  shaWrapper->updateContext((unsigned char *)decrypted.str(), decrypted.pcount());
#else
  if (!CryptHashData(HASH, (BYTE *)decrypted.str(), decrypted.pcount(), 0))
  {
    LogF("CryptHashData failed");
    return false;
  }
#endif

  return true;
}
#endif //#if _ENABLE_PBO_PROTECTION

bool BankHashCalculatorAsync::Process(QIStream &in)
{
#ifdef _DISABLE_SIGNATURES
  return true;
#else
  const int granularity = 64 * 1024;

  if (_index < 0 || _pos >= _ranges[_index]._offsetEnd)
  {
    if (++_index >= _ranges.Size()) return true;
    _pos = granularity * (_ranges[_index]._offsetBeg / granularity);
#if _ENABLE_PBO_PROTECTION
    if (_ranges[_index]._isProtHeader)
    {
      _protRange.Resize(_ranges[_index]._offsetEnd - _ranges[_index]._offsetBeg);
  }
#endif //#if _ENABLE_PBO_PROTECTION
  }

  while (_pos < _ranges[_index]._offsetEnd)
  {
    if (_async && !in.PreRead(FileRequestPriority(20000), _pos, granularity)) return false;
    in.seekg(_pos);
    char buffer[granularity];
    int size = in.read(buffer, sizeof(buffer));

    // while processing current buffer, read upcoming in the background
    if (!_async) in.PreRead(FileRequestPriority(20000), _pos+granularity, granularity);

    int begin = std::max(0, _ranges[_index]._offsetBeg - _pos);
    int end = std::min(size, _ranges[_index]._offsetEnd - _pos);
    
#if _ENABLE_PBO_PROTECTION
    if (_ranges[_index]._isProtHeader)
    {
      // this is encrypted range, just copy it to _protRange and check later
      int rangeOffset = std::max(_pos - _ranges[_index]._offsetBeg, 0);
      int dataSize = end-begin;

      if ((rangeOffset >= 0) && (rangeOffset < _protRange.Size()) && (rangeOffset + dataSize <= _protRange.Size()))
      {
        memcpy(_protRange.Data() + rangeOffset, buffer + begin, dataSize);
      }
      else
      {
        Assert(!"Values out of range.");
        LogF("Values out of range while processing encrypted range.");
        _ok = false;
        return true;
      }
    }
    else
#endif //#if _ENABLE_PBO_PROTECTION
    {
#ifdef USE_HASH_LIB
    shaWrapper->updateContext((unsigned char *)(buffer + begin), end - begin);
#else
    if (!CryptHashData(HASH, (BYTE *)(buffer + begin), end - begin, 0))
    {
      LogF("CryptHashData failed");
      _ok = false;
      return true;
    }
#endif
    }

    _pos += size;
    if (!_async) return false; // always process only one item (to prevent memory exhaustion)
  }

#if _ENABLE_PBO_PROTECTION
  if (_ranges[_index]._isProtHeader)
  {
    if (!ProcessProtectedRange())
    {
      LogF("Error while processing protected range.");
      _ok = false;
      return true;
    }
  }
#endif
  return false;
#endif
}
