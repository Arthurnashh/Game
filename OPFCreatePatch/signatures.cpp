#include <El/elementpch.hpp>

#include "signatures.hpp"
#include <Es/Strings/rString.hpp>

#include <Es/Common/win.h>
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

bool CalculateSignature(DSSignature &signature, const char *data, int size, const DSKey &privateKey)
{
  HCRYPTPROV provider = NULL;
  HCRYPTKEY key = NULL;
  HCRYPTHASH hash = NULL;

  if (!AcquireContext(&provider)) 
  {
    fprintf(stderr, "CPAcquireContext failed\n");
    return false;
  }

  // import the private key
  if (!CryptImportKey(provider, (const BYTE *)privateKey._content.Data(), privateKey._content.Size(), NULL, 0, &key))   
  {
    fprintf(stderr, "CryptImportKey failed\n");
    if (provider) CryptReleaseContext(provider, 0);
    return false;
  }

  // create the hash
  if (!CryptCreateHash(provider, CALG_SHA, NULL, 0, &hash))
  {
    fprintf(stderr, "CryptCreateHash failed\n");
    if (key) CryptDestroyKey(key);
    if (provider) CryptReleaseContext(provider, 0);
    return false;
  }

  // calculate the hash
  if (!CryptHashData(hash, (BYTE *)data, size, 0))
  {
    fprintf(stderr, "CryptCreateHash failed\n");
    if (hash) CryptDestroyHash(hash);
    if (key) CryptDestroyKey(key);
    if (provider) CryptReleaseContext(provider, 0);
    return false;
  }

  // calculate the size of the signature
  DWORD hashSize = 0;
  if (!CryptSignHash(hash, AT_SIGNATURE, NULL, 0, NULL, &hashSize))
  {
    fprintf(stderr, "CryptSignHash failed\n");
    if (hash) CryptDestroyHash(hash);
    if (key) CryptDestroyKey(key);
    if (provider) CryptReleaseContext(provider, 0);
    return false;
  }

  // create the signature
  signature._content1.Realloc(hashSize);
  if(!CryptSignHash(hash, AT_SIGNATURE, NULL, 0, (BYTE *)signature._content1.Data(), &hashSize))
  {
    fprintf(stderr, "CryptSignHash failed\n");
    if (hash) CryptDestroyHash(hash);
    if (key) CryptDestroyKey(key);
    if (provider) CryptReleaseContext(provider, 0);
    return false;
  }

  if (hash) CryptDestroyHash(hash);
  if (key) CryptDestroyKey(key);
  if (provider) CryptReleaseContext(provider, 0);
  return true;
}