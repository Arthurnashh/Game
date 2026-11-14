#ifdef _MSC_VER
#pragma once
#endif

#ifndef _PBO_HEADER_PROTECTION_H
#define _PBO_HEADER_PROTECTION_H

#if _ENABLE_PBO_PROTECTION

#include "El/QStream/qStream.hpp"
#include "El/QStream/qbStream.hpp"

class IFilebankEncryption;

namespace PBOHeaderProtection
{
  typedef AutoArray<unsigned char> TByteBuffer;

  /// name of the PBO property that indicates the PBO headers are protected
  static const char* const PBOPropertyName("hprotect");

  /// name of the property that indicates where we can fin serial number for this PBO in windows registry
  static const char* const PRORegSerialProp("registry");

  /// name of the dummy file used to create padding 
  static const char* const DummyPaddingFileName("___dummypadding___");


  /// struct used to pass information from DecryptHeaders
  struct DecryptedHeaderInfo
  {
    DecryptedHeaderInfo():isEncrypted(false), headerSize(0), headerSizeWithPadding(0), headerEncodedSize(0){}

    /// if stream was really encrypted
    bool isEncrypted;
    /// decrypted header size without dummy file padding
    QFileSize headerSize;
    /// decrypted header size with dummy file padding
    QFileSize headerSizeWithPadding;
    /// encrypted header size (without additional data like version and sizes)
    QFileSize headerEncodedSize;
  };

  /// result of transform operation on PBO
  enum TransformResult
  {
    TROk, ///< everything was ok
    TRNotPBO, ///< file is not a PBO
    TRFailedToOpenForReading, ///< failed to open PBO for reading
    TRFailedToOpenForWriting, ///< failed to open PBO for writing
    TRUnexpectedNotEncrypted, ///< we want to decrypt, but headers are not encrypted
    TRUnexpectedEncrypted, ///< PBO is encrypted, but we expected it to be decrypted
    TRNotPreparedForEncryption, /// < PBO is not prepared for encryption
    TRFailedToLoadHeader, ///< failed to load PBO headers
    TRDecryptionFailed, ///< decryption failed
    TREncryptionFailed, ///< encryption failed
    TRInvalidDecryptedHeader ///< decrypted header is invalid
  };

  /// returns error message for given result
  RString TransformResultToString(TransformResult result);

  /// returns int representing version of protection
  int GetProtectionVersion();

  /// Generates key used for encryption of headers of PBOs, that will be distributed with the Setup utility.
  /// This key should never change! If you change this function, PBOs encrypted with the old keys won't be usable by new code!
  /// \param outKey this array will be filled with the generated key
  void GeneratePBOSetupProtectionKey(TByteBuffer& outKey);

  /// Generates IV used for encryption of headers of PBOs, that will be distributed with the Setup utility.
  /// This IV should never change! If you change this function, PBOs encrypted with the old keys won't be usable by new code!
  /// \param outIV this array will be filled with the generated IV
  void GeneratePBOSetupProtectionIV(TByteBuffer& outIV);

  /// Generates key used for encryption of headers of PBOs using 2 serial numbers
  /// \param serial1 first serial number
  /// \param serial2 second serial number
  /// \param outKey this array will be filled with the generated key
  /// \return if key was successfully generated
  bool GeneratePBOSerialProtectionKey(const TByteBuffer& serial1, const TByteBuffer& serial2, TByteBuffer& outKey);

  /// creates header protection that is used to encrypt/decrypt headers for setup
  IFilebankEncryption* CreateSetupHeaderProtection();

  /// creates header protection that is used to encrypt/decrypt headers using two serial keys
  IFilebankEncryption* CreateSerialHeaderProtection(const TByteBuffer& serial1, const TByteBuffer& serial2);

  /// creates serial number for testing
  void CreateTestSerial(TByteBuffer &serial, bool isMain);
  /// creates serial number for testing from testParentSerial and testSerial
  bool CreateTestSerialsFromTestStrings(TByteBuffer &outSerial1, TByteBuffer &outSerial2);

  /// encrypts headers
  /// \param normalHeaders stream with normal headers
  /// \param headerProtection encryption class used for headers protection
  /// \return number of bytes written
  QFileSize EncryptHeaders( QOStream &out, QOStrStream &normalHeaders, IFilebankEncryption* headerProtection);

  /// decrypts headers
  /// \param encryptedHeaders stream with encrypted headers
  /// \param headerProtection encryption class used for headers protection
  /// \param decryptInfo if not NULL, then info about decryption is passed here
  /// \return if decryption succeeded
  bool DecryptHeaders( QOStream &out, QIStream &encryptedHeaders, IFilebankEncryption* headerProtection, DecryptedHeaderInfo* decryptInfo = 0);

  /// gets protection info from stream
  bool GetProtectionInfo(QIStream& in, DecryptedHeaderInfo& outInfo);

  /// saves dummy padding to stream
  void SaveDummyPaddingFile(QOStream &out, QFileSize size);

  /// creates cerial from registry, returns if succeeded
  bool CreateSerialFromRegistry(const RString& path, TByteBuffer& outSerial);

  /// creates serial numbers for key generation
  /// \param parentAppRegistryPath root path in registry for parent application
  /// \param pboRegistry value of PBO property that determines path in registry where to find serial number for given PBO
  /// \param outSerial1 first created serial
  /// \param outSerial2 second created serial
  bool CreateProtectionSerials(const RString& parentAppRegPath, const RString& pboRegistry, 
    PBOHeaderProtection::TByteBuffer& outSerial1, PBOHeaderProtection::TByteBuffer& outSerial2);
  /// same as CreateProtectionSerials, but takes pboRegistry directly from given pbo file
  bool CreateProtectionSerialsFromPBO(const RString& parentAppRegPath, const RString& pboFileName,
    PBOHeaderProtection::TByteBuffer& outSerial1, PBOHeaderProtection::TByteBuffer& outSerial2);

  /// transforms given stream
  /// decryptClass == 0, encryptClass == 0 -> NOT ALLOWED, does nothing
  /// decryptClass == 0, encryptClass != 0 -> encrypts in stream to out stream
  /// decryptClass != 0, encryptClass == 0 -> decrypts in stream to out stream
  /// decryptClass != 0, encryptClass != 0 -> decrypts in stream encrypts it to out stream
  /// \param decryptClass class used for decryption (can be 0)
  /// \param encryptClass class used for encryption (can be 0);
  TransformResult TransformStream(QIStream& inStream, QOStream& outStream, IFilebankEncryption* decryptClass, IFilebankEncryption* encryptClass);


  /// transforms given PBO
  /// decryptClass == 0, encryptClass == 0 -> NOT ALLOWED, does nothing
  /// decryptClass == 0, encryptClass != 0 -> NOT ALLOWED
  /// decryptClass != 0, encryptClass == 0 -> decrypts headers of PBO
  /// decryptClass != 0, encryptClass != 0 -> decrypts headers and then encrypts them again
  /// \param decryptClass class used for decryption of headers (can be 0)
  /// \param encryptClass class used for encryption of headers (can be 0);
  /// \param fileNameOut file to write the transformed output. When null, the fileName itself is transformed.
  TransformResult TransformPBO(const RString& fileName, IFilebankEncryption* decryptClass, IFilebankEncryption* encryptClass, const RString fileNameOut=RString());

  /// returns if given file is protected PBO file
  /// \param preciseExtensionCheck if true, then file must have .pbo extension, otherwise .pbo must be found anywhere in the file name (etc. air.pbo.tmp is ok)
  /// \param info if not 0 and bank is protected, then protection info is returned there
  bool IsProtectedPBO(const RString& fileName, bool preciseExtensionCheck = true, QFBank::BankHeaderProtectionInfo* info = 0);

  /// checks if decrypted header is OK
  bool CheckDecryptedHeader(const char* const data, unsigned size);

  /// root registy path for application
  extern RString AppRegistryPath;
  /// testing serial number of parent app for header protection
  extern RString TestParentSerial;
  /// testing serial number of app for header protection
  extern RString TestSerial;

} //namespace PBOHeaderProtection

#endif //#if _ENABLE_PBO_PROTECTION

#endif //_PBO_HEADER_PROTECTION_H
