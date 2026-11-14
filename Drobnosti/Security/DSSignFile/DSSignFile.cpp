// DSSignFile.cpp : Defines the entry point for the console application.
//

#include <El/elementpch.hpp>

#include <Es/Strings/rString.hpp>
#include <Es/Common/win.h>

#include <El/QStream/qbStream.hpp>
#include <El/QStream/serializeBin.hpp>
#include <El/DataSignatures/dataSignatures.hpp>

#include <Wincrypt.h>

void PrintErrorMessage()
{
  int errnum = GetLastError();
  //NTE_BAD_KEYSET
  char buf[1024]; *buf=0;
  FormatMessage
    (
    FORMAT_MESSAGE_FROM_SYSTEM,
    NULL, // source
    errnum, // requested message identifier 
    0, // language
    buf,sizeof(buf),
    NULL
    );
  printf("Error description: %x: %s\n", errnum, buf);
}

static char *BiKeyContainer = "Bohemia Interactive Studio";

static bool AcquireContext(HCRYPTPROV *provider)
{
  if (CryptAcquireContext(provider, BiKeyContainer, NULL, PROV_RSA_FULL, 0)) return true;
  // create a new key container
  if (GetLastError() == NTE_BAD_KEYSET)
  {
    if (CryptAcquireContext(provider, BiKeyContainer, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET)) return true;
  }
  PrintErrorMessage();
  return false;
}

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

bool ExportSignature(RString filename, HCRYPTHASH hash3, HCRYPTHASH hash2, HCRYPTHASH hash1, RString keyname, HCRYPTKEY key)
{
  DSSignature signature;
  
  // create public key
  signature._key._name = keyname;
  signature._version = BIS_SIGNATURE_VERSION;

  DWORD size = 0;
  if (!CryptExportKey(key, NULL, PUBLICKEYBLOB, 0, NULL, &size))
  {
    printf("CryptExportKey failed.\n");
    return false;
  }
  signature._key._content.Realloc(size);
  if (!CryptExportKey(key, NULL, PUBLICKEYBLOB, 0, (BYTE *)signature._key._content.Data(), &size))   
  {
    printf("CryptExportKey failed.\n");
    return false;
  }

  { // create Ver1 signature
    size = 0;
    if(!CryptSignHash(hash1, AT_SIGNATURE, NULL, 0, NULL, &size))
    {
      printf("CryptSignHash failed.\n");
      PrintErrorMessage();
      return false;
    }
    signature._content1.Realloc(size);
    if(!CryptSignHash(hash1, AT_SIGNATURE, NULL, 0, (BYTE *)signature._content1.Data(), &size))
    {
      printf("CryptSignHash failed.\n");
      PrintErrorMessage();
      return false;
    }
  }

  { // create Ver2 signature of (datahash+fileListHash+pbo prefix)
    size = 0;
    if(!CryptSignHash(hash2, AT_SIGNATURE, NULL, 0, NULL, &size))
    {
      printf("CryptSignHash failed.\n");
      PrintErrorMessage();
      return false;
    }
    signature._content2.Realloc(size);
    if(!CryptSignHash(hash2, AT_SIGNATURE, NULL, 0, (BYTE *)signature._content2.Data(), &size))
    {
      printf("CryptSignHash failed.\n");
      PrintErrorMessage();
      return false;
    }
  }

  { // create Ver2 signature of (nonDataHash+fileListHash+pbo prefix)
    size = 0;
    if(!CryptSignHash(hash3, AT_SIGNATURE, NULL, 0, NULL, &size))
    {
      printf("CryptSignHash failed.\n");
      PrintErrorMessage();
      return false;
    }
    signature._content3.Realloc(size);
    if(!CryptSignHash(hash3, AT_SIGNATURE, NULL, 0, (BYTE *)signature._content3.Data(), &size))
    {
      printf("CryptSignHash failed.\n");
      PrintErrorMessage();
      return false;
    }
  }

  // save signature
  QOFStream out;
  out.open(filename);
  SerializeBinStream stream(&out);
  signature.SerializeBin(stream);
  out.close();

#if 0
  DSHash result;
  DWORD siz = 20;
  result._content.Realloc(siz);
  CryptGetHashParam(hash, HP_HASHVAL, (BYTE *)result._content.Data(), &siz, 0);
  
  if (!DataSignatures::VerifySignature(result, signature))
  {
    RptF("Error, does not match!");
  }
#endif

  return !out.fail();
}

struct BankHashCalculator
{
  HashCalculator &_calculator;
  int _begin;
  int _end;
  int _pos;
  QFBank &_bank;

  BankHashCalculator(HashCalculator &calculator, QFBank &bank) : _calculator(calculator), _bank(bank)
  {
    _begin = 0;
    _end = 0;
    _pos = 0;
  }
  void Init(int begin, int end)
  {
    _begin = begin;
    _end = end;
    _pos = 0;
  }
  void operator () (const void *buf, int size)
  {
    int begin = max(0, _begin - _pos);
    int end = min(size, _end - _pos);

    if (begin < end)
    {
      _calculator.Add((BYTE *)buf + begin, end - begin);
    }

    _pos += size;
  }
  void GetResult(AutoArray<char> &hash) { _calculator.GetResult(hash); }

  void AddRange(int dataBeg, int dataEnd
#if _ENABLE_PBO_PROTECTION
    , bool isProtHeader
#endif //#if _ENABLE_PBO_PROTECTION
  );

};


void BankHashCalculator::AddRange(int dataBeg, int dataEnd
#if _ENABLE_PBO_PROTECTION
, bool isProtHeader
#endif //#if _ENABLE_PBO_PROTECTION
)
{
  _bank.CalculateRangeHash(_calculator, dataBeg, dataEnd
#if _ENABLE_PBO_PROTECTION
     ,isProtHeader
#endif //#if _ENABLE_PBO_PROTECTION
  );
}

bool GetHash(RString filename,
             HCRYPTHASH NDtFlPxHash/*ver2, NonData+FileList+Prefix*/, 
             HCRYPTHASH DtFlPxHash /*ver2, Data+FileList+Prefix*/, 
             HCRYPTHASH dataHash   /*ver1*/)
{
  QFBank bank;

  RString bankName = filename;
  if (stricmp(cc_cast(filename) + filename.GetLength() - 4, ".pbo") == 0)
  {
    bankName = filename.Substring(0, filename.GetLength() - 4);
  }
  
  if (bank.open(bankName) && bank.Load() && bank.NFiles() > 0)
  {
    Temp<char> storedHash;
    if (!bank.GetHash(storedHash)) return false;

    HashCalculator calculator;
    BankHashCalculator bankcalculator(calculator, bank);
    if (!bank.CalculateInit(bankcalculator)) return false;
    AutoArray<char> calculatedHash;
    calculator.GetResult(calculatedHash);

    // Get the SigVer1 hash
    if ( !CryptSetHashParam(dataHash, HP_HASHVAL, (const BYTE *)calculatedHash.Data(), 0) )
      return false;

    if (storedHash.Size() != calculatedHash.Size()) goto HashDontMatch;
    for (int i=0; i<storedHash.Size(); i++)
    {
      if (storedHash[i] != calculatedHash[i]) goto HashDontMatch;
    }

    {
      // combine content hash with hash of pbo file list and pbo prefix
      // the DtFlPxHash will be: HashOf(storedHash+fileListHash+pboPrefix)
      AutoArray<char> fileListHash;
      if (!bank.GetFileListHash<HashCalculator>(fileListHash)) 
        return false;
      // and hash it finally
      if (!CryptHashData(DtFlPxHash, (const BYTE *)calculatedHash.Data(), calculatedHash.Size(), 0)) return false;
      if (!CryptHashData(DtFlPxHash, (const BYTE *)fileListHash.Data(), fileListHash.Size(), 0)) return false;
      if (!CryptHashData(DtFlPxHash, (const BYTE *)bank.GetPrefix().Data(), bank.GetPrefix().GetLength(), 0)) return false;
      /////////////////////////////////////////////////////////////////////////////////////////////////////////
      // get the NonDataHash
      AutoArray<char> nonDataHash;
      if (!bank.GetNonDataHash<HashCalculator>(nonDataHash))
        return false;
      // combine NonDataHash with hash of pbo file list and pbo prefix
      // the NDtFlPxHash will be: HashOf(nonDataHash+fileListHash+pboPrefix)
      // and hash it finally
      if (!CryptHashData(NDtFlPxHash, (const BYTE *)nonDataHash.Data(), nonDataHash.Size(), 0)) return false;
      if (!CryptHashData(NDtFlPxHash, (const BYTE *)fileListHash.Data(), fileListHash.Size(), 0)) return false;
      if (!CryptHashData(NDtFlPxHash, (const BYTE *)bank.GetPrefix().Data(), bank.GetPrefix().GetLength(), 0)) return false;
    }

    return true;

  HashDontMatch:
    fprintf(stderr, "Error: Stored and calculated hash don't match\n");
    return false;
  }
  else
  {
    // common file
    QIFStream file;
    file.open(filename);

    bool ok;
    CalculateHash func(DtFlPxHash, ok);
    file.Process(func);

    // SigVer1 hash is the same as SigVer2
    Temp<char> hashData;
    if (ok)
    {
      DWORD size = 0;
      if (!CryptGetHashParam(DtFlPxHash, HP_HASHVAL, NULL, &size, 0)) 
      {
        ok = false;
      }
      else
      {
        hashData.Realloc(size);
        if (!CryptGetHashParam(DtFlPxHash, HP_HASHVAL, (BYTE *)hashData.Data(), &size, 0))
        {
          ok = false;
        }
      }
      if (ok)
      {
        if ( !CryptSetHashParam(dataHash, HP_HASHVAL, (const BYTE *)hashData.Data(), 0) )
          ok = false;
        if ( !CryptSetHashParam(NDtFlPxHash, HP_HASHVAL, (const BYTE *)hashData.Data(), 0) )
          ok = false;
      }
    }

    file.close();
    return (ok && (!file.fail() || file.eof()));
  }
}

int main(int argc, char* argv[])
{
  // the first argument is exe filename
  if (argc != 3)
  {
    fprintf(stderr, "Usage: dsSignFile private_key_filename file_to_sign_filename\n");
    return 1;
  }

  RString keyname = argv[1];
  RString filename = argv[2];

  // import key from file
  DSKey privateKey;
  QIFStream in;
  in.open(keyname);
  SerializeBinStream stream(&in);
  privateKey.SerializeBin(stream);
  in.close();
  if (in.fail())
  {
    fprintf(stderr, "Wrong private key file\n");
    return 1;
  }

  HCRYPTPROV provider = NULL;
  HCRYPTKEY key = NULL;
  HCRYPTHASH hash1 = NULL;
  HCRYPTHASH hash2 = NULL;
  HCRYPTHASH hash3 = NULL;

  if (!AcquireContext(&provider)) 
  {
    fprintf(stderr, "CPAcquireContext failed\n");
    return 1;
  }

  // import key
  if (!CryptImportKey(provider, (const BYTE *)privateKey._content.Data(), privateKey._content.Size(), NULL, 0, &key))   
  {
    fprintf(stderr, "CryptImportKey failed\n");
    if (provider) CryptReleaseContext(provider, 0);
    return 1;
  }

  int ret = 1;
  if (   CryptCreateHash(provider, CALG_SHA, NULL, 0, &hash1) // get both sigVer 1&2
      && CryptCreateHash(provider, CALG_SHA, NULL, 0, &hash2)
      && CryptCreateHash(provider, CALG_SHA, NULL, 0, &hash3) )
  {
    if (GetHash(filename, hash3, hash2, hash1))
    {
      RString signname = filename + RString(".") + privateKey._name + RString(".bisign");
      if (ExportSignature(signname, hash3, hash2, hash1, privateKey._name, key))
        ret = 0;
    }
    else
    {
      fprintf(stderr, "Error: Cannot get the hash of %s\n", cc_cast(filename));
    }
  }

  if (hash3) CryptDestroyHash(hash3);
  if (hash2) CryptDestroyHash(hash2);
  if (hash1) CryptDestroyHash(hash1);
  if (key) CryptDestroyKey(key);
  if (provider) CryptReleaseContext(provider, 0);

  return ret;
}

