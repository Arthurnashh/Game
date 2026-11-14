#include <El/elementpch.hpp>
#include "AESEncryption.h"

#if _ENABLE_PBO_PROTECTION

unsigned char FilebankEncryptionAES::Blank[FilebankEncryptionAES::MaxKeySizeBytes] = { 0x0 };

//=======================================================================
FilebankEncryptionAES::FilebankEncryptionAES()
:_keySizeBytes(0)
{  
}

//=======================================================================
FilebankEncryptionAES::~FilebankEncryptionAES()
{
}

//=======================================================================
bool FilebankEncryptionAES::Init(unsigned char* key, unsigned int keySizeBytes, unsigned char *iv, unsigned char *hash)
{
  Assert(key);
  if (!key)
    return false;

  Assert(iv);
  if (!iv)
    return false;

  // check key size
  if (keySizeBytes != 16 && keySizeBytes != 24 && keySizeBytes != 32)
  {
    Assert(!"Invalid key size");
    RptF("Invalid key size used for AES algorithm: key size length must be 16B, 24B or 32B");
    return false;
  }

  _keySizeBytes = keySizeBytes;

  // copy key
  _key.Clear();
  _key.Resize(_keySizeBytes);
  _key.Copy(key, keySizeBytes);

  // copy IV
  _iv.Clear();
  _iv.Resize(IVLengthBytes);
  _iv.Copy(iv, IVLengthBytes);

  // create hash
  if (hash)
  {
    _hash.Clear();
    _hash.Resize(_keySizeBytes);
    _hash.Copy(hash, _keySizeBytes);
  }
  else
  {
    GenerateHash();
  }

  return true;    
}

//=======================================================================
void FilebankEncryptionAES::GenerateHash()
{
  Assert(_keySizeBytes != 0);
  _hash.Clear();
  _hash.Resize(_keySizeBytes);
  
  // generation of hash has no internal logic, we need just to generate some numbers
  for (unsigned i=0; i<_keySizeBytes; ++i)
  {
    _hash[i] = i + i*15 + i*i*123 + i*i*i*i*7;
    for (unsigned j=0; j<_keySizeBytes; ++j)
    {
      _hash[i] += 93*i + 81*j*i*i + i*i*j*j;
    }
  }
}

//=======================================================================
bool FilebankEncryptionAES::Decode( char *dst, long lensb, QIStream &in )
{
  if (_keySizeBytes != 16 && _keySizeBytes != 24 && _keySizeBytes != 32)
  {
    Assert(!"Invalid key size!");
    RptF("Invalid key size used for AES algorithm: key size length must be 16B, 24B or 32B. Call Init() before using encryption/decryption");
    return false;
  }
  if (_key.Size() != _keySizeBytes || _hash.Size() != _keySizeBytes || _iv.Size() != IVLengthBytes)
  {
    Assert(!"Invalid data!");
    RptF("Invalid data used for AES algorithm. Call Init() before using encryption/decryption");
    return false;
  }

  aes_context ctx;
  AutoArray<char> buf;
  WriteAutoArrayChar writeBuf(buf);
  in.Process(writeBuf);  // writes everthing into writeBuffer

  // generate real key from our key and hash
  unsigned char key[MaxKeySizeBytes];

  for( unsigned i = 0; i < _keySizeBytes; ++i)
    key[i] = _hash[i] ^ _key[i];


  // set the key and decode the data
  aes_set_key( &ctx, key, _keySizeBytes*8 );
  // IV will be changed in aes_cbc_decrypt, so we have to create local copy
  Temp<unsigned char> locIV(_iv.Data(), _iv.Size());
  aes_cbc_decrypt(&ctx,locIV.Data(),(unsigned char*)buf.Data(),(unsigned char*)buf.Data(),buf.Size());

  // uninitialise the key and aes key from the stack
  ZeroMemory(key,sizeof(key));
  aes_set_key( &ctx, Blank, _keySizeBytes*8 ); 

  memcpy(dst,buf.Data(),lensb);

  return true;
}
//=======================================================================
void FilebankEncryptionAES::Encode( QOStream &out, QIStream &in) 
{
  if (_keySizeBytes != 16 && _keySizeBytes != 24 && _keySizeBytes != 32)
  {
    Assert(!"Invalid key size!");
    RptF("Invalid key size used for AES algorithm: key size length must be 16B, 24B or 32B. Call Init() before using encryption/decryption");
    return;
  }
  if (_key.Size() != _keySizeBytes || _hash.Size() != _keySizeBytes || _iv.Size() != IVLengthBytes)
  {
    Assert(!"Invalid data!");
    RptF("Invalid data used for AES algorithm. Call Init() before using encryption/decryption");
    return;
  }

  aes_context ctx;
  AutoArray<char> buffer;
  WriteAutoArrayChar writeBuffer(buffer);
  in.Process(writeBuffer);

  char padd = '\0';
  while((buffer.Size()%16)!=0)
    buffer.Add(padd);

  // generate real key from our key and hash
  unsigned char key[MaxKeySizeBytes];

  for( unsigned i = 0; i < _keySizeBytes; ++i)
    key[i] = _hash[i] ^ _key[i];

  // set key and encrypt the data
  aes_set_key( &ctx, key, _keySizeBytes*8 );   // setup key
  // IV will be changed in aes_cbc_decrypt, so we have to create local copy
  Temp<unsigned char> locIV(_iv.Data(), _iv.Size());
  aes_cbc_encrypt(&ctx, locIV.Data(),(unsigned char*)buffer.Data(),(unsigned char*)buffer.Data(),buffer.Size());

  // uninitialise the key and aes key from the stack
  ZeroMemory(key,sizeof(key));
  aes_set_key( &ctx, Blank, _keySizeBytes*8 ); 

  out.write(buffer.Data(), buffer.Size());
}

FilebankEncryptionAES *CreateEncryptAES(const void *context)
{
  return new FilebankEncryptionAES();
}

#endif //#if _ENABLE_PBO_PROTECTION
