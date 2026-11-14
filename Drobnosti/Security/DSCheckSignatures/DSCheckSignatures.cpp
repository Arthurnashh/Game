// DSCheckSignatures.cpp : Defines the entry point for the console application.
//

#include <El/elementpch.hpp>

#include <Es/Strings/rString.hpp>
#include <Es/Common/win.h>
#include <Es/Files/fileContainer.hpp>

#include <El/QStream/qbStream.hpp>
#include <El/QStream/serializeBin.hpp>
#include <El/DataSignatures/dataSignatures.hpp>

#include <Wincrypt.h>

static inline void TerminateBy(RString &name, char c)
{
  int n = name.GetLength();
  if (n > 0 && name[n - 1] != c) name = name + RString(&c, 1);
}

static bool LoadKey(const FileItem &file, AutoArray<DSKey> &keys)
{
  int index = keys.Add();
  keys[index].Load(file.path + file.filename);
  return false;
}

struct CheckSignatureContext
{
  DSHash           (&_hashes)[BIS_SIGNATURE_VERSION];
  AutoArray<DSKey> &_keys;
  int &_count;

  CheckSignatureContext(DSHash (&hashes)[BIS_SIGNATURE_VERSION], AutoArray<DSKey> &keys, int &count) : _hashes(hashes), _keys(keys), _count(count) {}
};

static bool CheckSignature(const FileItem &file, CheckSignatureContext &ctx)
{
  ctx._count++;

  DSSignature signature;
  if (signature.Load(file.path + file.filename))
  {
    bool found = false;
    for (int i=0; i<ctx._keys.Size(); i++)
    {
      if (signature._key == ctx._keys[i])
      {
        // accepted key found
        int sigVer = signature.Version();
        if (sigVer>0 && sigVer<=BIS_SIGNATURE_VERSION)
        {
          if (sigVer!=BIS_SIGNATURE_VERSION)
          {
            fprintf(stdout, "Warning: Signature %s is deprecated!\n", cc_cast(file.path + file.filename));
          }
          if (!DataSignatures::VerifySignature(ctx._hashes[sigVer-1], signature))
          {
            fprintf(stdout, "Signature %s is wrong\n", cc_cast(file.path + file.filename));
          }
          found = true;
        }
      }
    }
    if (!found)
    {
      fprintf(stdout, "Key not found for signature %s\n", cc_cast(file.path + file.filename));
    }
  }
  else
  {
    fprintf(stdout, "Signature %s not loaded\n", cc_cast(file.path + file.filename));
  }
  return false;
}

#ifdef USE_HASH_LIB
#include "El/DataSignatures/hashwrapper.h"
#include "El/DataSignatures/sha1wrapper.h"
struct HashLibBankCalculator
{
  Ref<hashwrapper> shaWrapper;
  int _begin;
  int _end;
  int _pos;
  bool &_ok;

  HashLibBankCalculator(bool &ok) : _ok(ok)
  {
    shaWrapper = new sha1wrapper();
    shaWrapper->resetContext();
    _begin = 0;
    _end = 0;
    _pos = 0;
  }
  void Init(int begin, int end)
  {
    //printf("begin = %d, end = %d\n", begin, end);
    _begin = begin;
    _end = end;
    _pos = 0;
  }
  void operator () (const void *buf, int size)
  {
    if (_ok)
    {
      int begin = max(0, _begin - _pos);
      int end = min(size, _end - _pos);

      if (begin < end)
      {
        shaWrapper->updateContext((unsigned char *)buf + begin, end - begin);
      }

      _pos += size;
    }
  }
  void GetHash(char *data)
  {
    shaWrapper->GetHash((unsigned char *)data);
  }
};
#endif

static bool DoDeepCheck = false;
static bool CheckSignatures(const FileItem &file, AutoArray<DSKey> &keys)
{
  // remove .pbo extension
  RString bankname = file.path + file.filename;
  const char *ext = strrchr(bankname, '.');
  if (!ext || stricmp(ext, ".pbo") != 0)
  {
    fprintf(stdout, "Wrong bank name %s\n", cc_cast(file.path + file.filename));
    return false;
  }
  bankname = bankname.Substring(0, ext - cc_cast(bankname));

  // retrieve the hash value of the bank
  QFBank bank;
  bank.open(bankname);
  bank.Load();
  Temp<char> dataHash;
  if (!bank.GetHash(dataHash))
  {
    fprintf(stdout, "Hash of %s not loaded\n", cc_cast(file.path + file.filename));
    return false;
  }
  if (DoDeepCheck) fprintf(stdout, "Checking %s\n", cc_cast(file.path + file.filename));
#ifdef GENERATE_HASH_FILE
  RString hashFile = bankname + RString(".hash.Wincrypt");
  FILE *out = fopen(hashFile,"wb");
  fwrite(dataHash.Data(), sizeof(char), dataHash.Size(), out);
  fclose(out);
#endif

#ifdef USE_HASH_LIB
#ifndef GENERATE_HASH_FILE
  if (DoDeepCheck) {
#endif
    bool ok = true;
    HashLibBankCalculator calc(ok);
    bank.CalculateHash(calc);
    Temp<char> hashlibHash;
    hashlibHash.Realloc(SHA1HashSize);
    calc.GetHash(unconst_cast<char *>(hashlibHash.Data()));
    if (DoDeepCheck)
    {
      if (memcmp(hashlibHash.Data(), dataHash.Data(), SHA1HashSize)!=0)
      {
        fprintf(stdout, "Hash inside %s does not match the hash calculated from data\n", cc_cast(file.path + file.filename));
      }
    }
#ifdef GENERATE_HASH_FILE
    hashFile = bankname + RString(".hash.Hashlib");
    out = fopen(hashFile,"wb");
    fwrite(hashlibHash.Data(), sizeof(char), SHA1HashSize, out);
    fclose(out);
#else
  }
#endif
#endif

  DSHash hashes[BIS_SIGNATURE_VERSION];
  hashes[0]._content.Realloc(dataHash.Data(), dataHash.Size());   //Ver1 hash

  // combine content hash with hash of pbo file list and pbo prefix
  // the final hash will be: HashOf(storedHash+fileListHash+pboPrefix)
  AutoArray<char> fileListHash;
  if (!bank.GetFileListHash<HashCalculator>(fileListHash)) 
    return false;
  // and hash it finally
  HashCalculator fincalculator;
  fincalculator.Add(dataHash.Data(), dataHash.Size());
  fincalculator.Add(fileListHash.Data(), fileListHash.Size());
  fincalculator.Add(bank.GetPrefix().Data(), bank.GetPrefix().GetLength());
  AutoArray<char> finalHash;
  fincalculator.GetResult(finalHash);
  hashes[1]._content.Realloc(finalHash.Data(), finalHash.Size()); //Ver2 hash

  // check all signatures for this bank
  int count = 0;
  CheckSignatureContext ctx(hashes, keys, count);
  ForMaskedFile(file.path, file.filename + RString(".*.bisign"), CheckSignature, ctx);
  if (count == 0)
  {
    fprintf(stdout, "No signature found for %s\n", cc_cast(file.path + file.filename));
  }

  return false;
}

int main(int argc, char* argv[])
{
  // the first argument is exe filename
  int offset = 0;
  if (argc == 4)
  {
    // Note: -deep argument works only when USE_HASH_LIB is used (it was used for debugging first)
    if ( stricmp(argv[1], "-deep")==0 ) // -deep parameter forces hash recalculation
    {
      offset = 1; DoDeepCheck = true;
    }
  }
  if (argc != 3 && !offset)
  {
#ifdef USE_HASH_LIB
    fprintf(stderr, "Usage: dsCheckSignatures [-deep] checked_dir keys_dir\n");
#else
    // Note: -deep argument works only when USE_HASH_LIB is used (it was used for debugging first)
    fprintf(stderr, "Usage: dsCheckSignatures checked_dir keys_dir\n");
#endif
    return 1;
  }

  RString checkedDir = argv[1+offset];
  TerminateBy(checkedDir, '\\');
  RString keysDir = argv[2+offset];
  TerminateBy(keysDir, '\\');

  // load the keys
  AutoArray<DSKey> keys;
  ForMaskedFile(keysDir, "*.bikey", LoadKey, keys);

  // check the signatures
  ForMaskedFile(checkedDir, "*.pbo", CheckSignatures, keys);

	return 0;
}
