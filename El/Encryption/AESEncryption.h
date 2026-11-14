#ifdef _MSC_VER
#pragma once
#endif

#ifndef _AES_ENCRYPTION_HPP
#define _AES_ENCRYPTION_HPP

#if _ENABLE_PBO_PROTECTION

#include <El/QStream/qbStream.hpp>
#include <Es/Common/win.h>
#include "../Encryption/aes.h"

#define AESDebug 0 && _ENABLE_CHEATS

#if AESDebug 
#define AESLogF LogF
#else
#define AESLogF		    
#endif

/// AES filebank encryption
class FilebankEncryptionAES : public IFilebankEncryption, private NoCopy
{
public:
  /// length of IV (initial vector) in Bytes
  static const unsigned int IVLengthBytes = 16;
  /// max key size in bytes
  static const int MaxKeySizeBytes = 32;
  /// blank array (used to clean key from the stack)
  static unsigned char Blank[MaxKeySizeBytes];


  FilebankEncryptionAES();
  virtual ~FilebankEncryptionAES();

  /// Initializes encryption/decryption
  /// \param key key used to encrypt/decrypt (must have length keySizeBytes)
  /// \param keySizeBytes key size in bytes (must be 16, 24 or 32)
  /// \param iv initial vector used to encrypt first block - must be 16B (128b) long
  /// \param hash hash used for modifying key before encrypting/decrypting (must be the same size as key). Can be NULL - then integrated hash is used.
  /// \return if initialization succeeded
  bool Init(unsigned char* key, unsigned int keySizeBytes, unsigned char *iv, unsigned char *hash=NULL);

  //@{ Implementation of IFilebankEncryption

  //! decode block of data

  /*!
  \param dst pointer to data destination
  \param lensb destination length
  \param in encoded data stored in stream

  requirement: inBuf data to start at 16 byte boundaries. Cannot start at
  5 for example, only at offsets%16==0.
  */
  virtual bool Decode( char *dst, long lensb, QIStream &in );
  //! encode block of data
  /*!
  \param out stream into which encoding is done
  \param in input stream
  */
  virtual void Encode( QOStream &out, QIStream &in);

  //@}

  
private:
  /// generates the hash for modifying the key
  void GenerateHash();

  /// size of the key in bytes (16, 24 or 32)
  unsigned int _keySizeBytes;
  /// key used for encryption/decryption
  AutoArray<unsigned char> _key;
  /// IV (initial vector) used for encryption/decryption
  AutoArray<unsigned char> _iv;
  /// hash used to modify the key for encryption
  AutoArray<unsigned char> _hash;

};

FilebankEncryptionAES *CreateEncryptAES(const void *context);


#endif //#if _ENABLE_PBO_PROTECTION


#endif //_AES_ENCRYPTION_HPP