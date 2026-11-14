// DSCreateKey.cpp : Defines the entry point for the console application.
//

#include <El/elementpch.hpp>

#include <Es/Strings/rString.hpp>
#include <Es/Common/win.h>

#include <El/QStream/qStream.hpp>
#include <El/QStream/serializeBin.hpp>
#include <El/DataSignatures/dataSignatures.hpp>

#include <Wincrypt.h>

static bool AcquireContext(HCRYPTPROV *provider)
{
  if (CryptAcquireContext(provider, NULL, NULL, PROV_RSA_FULL, 0)) return true;
  // create a new key container
  if (GetLastError() == NTE_BAD_KEYSET)
  {
    if (CryptAcquireContext(provider, NULL, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET)) return true;
  }
  return false;
}

int main(int argc, char* argv[])
{
  // the first argument is exe filename
  if (argc != 2)
  {
    fprintf(stderr, "Usage: dsCreateKey authority_name");
    return 1;
  }
  
  RString name = argv[1];

  HCRYPTPROV provider = NULL;
  HCRYPTKEY key = NULL;

  if (!AcquireContext(&provider)) 
  {
    fprintf(stderr, "CPAcquireContext failed");
    return 1;
  }

  if (!CryptGenKey(provider, AT_SIGNATURE, CRYPT_EXPORTABLE, &key))
  {
    fprintf(stderr, "CPGenKey failed");
    if (provider) CryptReleaseContext(provider, 0);
    return 1;
  }

  DSKey privateKey; privateKey._name = name;
  DSKey publicKey; publicKey._name = name;

  int ret = 0;

  DWORD size = 0;
  if (!CryptExportKey(key, NULL, PRIVATEKEYBLOB, 0, NULL, &size))
  {
    fprintf(stderr, "CryptExportKey failed");
    ret = 1;
  }
  else
  {
    privateKey._content.Realloc(size);
    if (!CryptExportKey(key, NULL, PRIVATEKEYBLOB, 0, (BYTE *)privateKey._content.Data(), &size))   
    {
      fprintf(stderr, "CryptExportKey failed");
      ret = 1;
    }
    else
    {
      QOFStream out;
      out.open(name + RString(".biprivatekey"));
      SerializeBinStream stream(&out);
      privateKey.SerializeBin(stream);
      out.close();
    }
  }

  if (!CryptExportKey(key, NULL, PUBLICKEYBLOB, 0, NULL, &size))
  {
    fprintf(stderr, "CryptExportKey failed");
    ret = 1;
  }
  else
  {
    publicKey._content.Realloc(size);
    if (!CryptExportKey(key, NULL, PUBLICKEYBLOB, 0, (BYTE *)publicKey._content.Data(), &size))   
    {
      fprintf(stderr, "CryptExportKey failed");
      ret = 1;
    }
    else
    {
      QOFStream out;
      out.open(name + RString(".bikey"));
      SerializeBinStream stream(&out);
      publicKey.SerializeBin(stream);
      out.close();
    }
  }

  if (key) CryptDestroyKey(key);
  if (provider) CryptReleaseContext(provider, 0);

  return ret;
}
