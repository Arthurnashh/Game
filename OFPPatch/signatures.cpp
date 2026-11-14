#include <El/elementpch.hpp>

#include "signatures.hpp"
#include <Es/Strings/rString.hpp>

#include <Es/Common/win.h>
#include <Wincrypt.h>

static bool AcquireContext(HCRYPTPROV *provider)
{
  // FIX: access was not always granted to the private key, but we need only the public key
  if (CryptAcquireContext(provider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) return true;
  // create a new key container
  if (GetLastError() == NTE_BAD_KEYSET)
  {
    if (CryptAcquireContext(provider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_NEWKEYSET)) return true;
  }
  return false;
}

int VerifySignature(const void *data, int size, const DSSignature &signature)
{
  HCRYPTPROV provider = NULL;
  HCRYPTKEY key = NULL;
  HCRYPTHASH hash = NULL;

  if (!AcquireContext(&provider))
  {
    return 3; // error #    
  }

  // import the public key
  if (!CryptImportKey(
    provider, (const BYTE *)signature._key._content.Data(), signature._key._content.Size(), NULL, 0, &key))
  {
    if (provider) CryptReleaseContext(provider, 0);
    return 4; // error #
  }

  // create the hash
  if (!CryptCreateHash(provider, CALG_SHA, NULL, 0, &hash))
  {
    if (key) CryptDestroyKey(key);
    if (provider) CryptReleaseContext(provider, 0);
    return 5; // error #
  }

  // calculate the hash
  if (!CryptHashData(hash, (BYTE *)data, size, 0))
  {
    if (hash) CryptDestroyHash(hash);
    if (key) CryptDestroyKey(key);
    if (provider) CryptReleaseContext(provider, 0);
    return 6; // error #
  }

  // check the signature
  bool ok = CryptVerifySignature(hash, (BYTE *)signature._content1.Data(), signature._content1.Size(), key, NULL, 0) == TRUE; 

  if (hash) CryptDestroyHash(hash);
  if (key) CryptDestroyKey(key);
  if (provider) CryptReleaseContext(provider, 0);
  
  return ok ? 0 : 7; // error #
}
