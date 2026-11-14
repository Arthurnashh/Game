#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <El/elementpch.hpp>
#include "pboheaderprotection.h"
#include "../CDKey/serial.hpp"
#include "AESEncryption.h"
#include <Es/Containers/staticArray.hpp>
#include <El/DataSignatures/dataSignatures.hpp>

#if _ENABLE_PBO_PROTECTION

namespace PBOHeaderProtection
{

/// size in bytes of setup key
static const unsigned SetupProtectionKeySizeBytes = 16;

/// size in bytes of serial key
/// at this moment serial numbers have 15 bytes, so we need at least 30 bytes to store 2 serial numbers in key
static const unsigned SerialProtectionKeySizeBytes = 32;

/// number of key bits per one serial number byte
static const unsigned KeyBitsPerSerialByte = 4;

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!   W A R N I N G   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Never change this function !
// If you change it, PBOs protected with old key won't be usable by new code
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!   W A R N I N G   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
void GeneratePBOSetupProtectionKey(TByteBuffer& outKey)
{
  outKey.Clear();
  outKey.Resize(SetupProtectionKeySizeBytes);

  // there is no internal logic in this, we just need to generate some numbers...
  for (unsigned i=0; i<SetupProtectionKeySizeBytes; ++i)
  {
    outKey[i] = i*1835+1221;
    for (unsigned j=0; j<SetupProtectionKeySizeBytes; ++j)
    {
      outKey[i] += i+j+j*10+i*j*123+j*85;
    }
  }
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!   W A R N I N G   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Never change this function !
// If you change it, PBOs protected with old key won't be usable by new code
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!   W A R N I N G   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
void GeneratePBOSetupProtectionIV(TByteBuffer& outIV)
{
  outIV.Clear();
  outIV.Resize(FilebankEncryptionAES::IVLengthBytes);

  // there is no internal logic in this, we just need to generate some numbers...
  for (unsigned i=0; i<FilebankEncryptionAES::IVLengthBytes; ++i)
  {
    outIV[i] = i*721 + 882;
    for (unsigned j=0; j<FilebankEncryptionAES::IVLengthBytes; ++j)
    {
      outIV[i] += 123 + i*447 + i*i*j*7 + j*j*j*j*j*71;
    }
  }
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!   W A R N I N G   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Never change this function !
// If you change it, PBOs protected with old key won't be usable by new code
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!   W A R N I N G   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
void GeneratePBOSerialProtectionIV(TByteBuffer& outIV)
{
  outIV.Clear();
  outIV.Resize(FilebankEncryptionAES::IVLengthBytes);

  // there is no internal logic in this, we just need to generate some numbers...
  for (unsigned i=0; i<FilebankEncryptionAES::IVLengthBytes; ++i)
  {
    outIV[i] = i*311 + 555;
    for (unsigned j=0; j<FilebankEncryptionAES::IVLengthBytes; ++j)
    {
      outIV[i] += 11 + i*j*17 + i*i*j*1112 + j*j*j*j*j*13;
    }
  }
}


//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!   W A R N I N G   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Never change this function !
// If you change it, PBOs protected with old key won't be usable by new code
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!   W A R N I N G   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
bool GeneratePBOSerialProtectionKey(const TByteBuffer& serial1, const TByteBuffer& serial2, TByteBuffer& outKey)
{
  // lets try to do is as general as possible - do not depend on current serial number representation
  
  /// total size of two serial numbers
  unsigned serialTotalSize = serial1.Size() + serial2.Size();
  if (0 == serialTotalSize)
  {
    Assert(!"Invalid serial numbers size!");
    RptF("Cannot generate AES key from empty serial numbers.");
    return false;
  }

  outKey.Clear();
  outKey.Resize(SerialProtectionKeySizeBytes);

  // if sum of serial numbers size > size of key, then we fill the key with serial number bytes, then return to the start
  // of the key and xor existing values with remaining bytes from serial numbers

  // if size of key > sum of serial numbers size, then we fill the key with serial number bytes, then return to the start of serial
  // numbers and fill the remaining bytes of key

  unsigned totalCount = std::max(serialTotalSize, SerialProtectionKeySizeBytes);

  /// offset in the key
  unsigned keyOffset = 0;
  /// offset in the serials
  unsigned serialOffset = 0;

  for (unsigned i=0; i<totalCount; ++i)
  {
    unsigned realKeyOffset = keyOffset % SerialProtectionKeySizeBytes;
    unsigned realSerialOffset = serialOffset % serialTotalSize;    

    // get value from serial number
    unsigned serialValue = 0;
    if (realSerialOffset < (unsigned)serial1.Size())
    {
      // value is from first serial number
      serialValue = serial1[realSerialOffset];
    }
    else if ((realSerialOffset - serial1.Size()) < (unsigned)serial2.Size())
    {
      // value is from second serial number
      serialValue = serial2[realSerialOffset - serial1.Size()];
    }
    else
    {
      Assert(!"Invalid realSerialOffset!");
    }
    
    // set key value
    if (keyOffset >= SerialProtectionKeySizeBytes)
    {
      // we are outside the key size (sum of serial number sizes is larger then key size)
      // just xor the new value with previous one
      outKey[realKeyOffset] ^= serialValue;
    }
    else
    {
      outKey[realKeyOffset] = serialValue;
    }

    keyOffset++;
    serialOffset++;
  }
  

  return true;
}

IFilebankEncryption* CreateSetupHeaderProtection()
{
  FilebankEncryptionAES* prot = CreateEncryptAES(0);
  TByteBuffer key;
  GeneratePBOSetupProtectionKey(key);
  TByteBuffer IV;
  GeneratePBOSetupProtectionIV(IV);
  if (!prot->Init(key.Data(), key.Size(), IV.Data()))
  {
    Fail("Creation of AES key for PBO header encryption failed.");
    delete prot;
    return 0;
  }
  return prot;
}

IFilebankEncryption* CreateSerialHeaderProtection(const TByteBuffer& serial1, const TByteBuffer& serial2)
{
  FilebankEncryptionAES* prot = CreateEncryptAES(0);
  TByteBuffer key;
  GeneratePBOSerialProtectionKey(serial1, serial2, key);
  TByteBuffer IV;
  GeneratePBOSerialProtectionIV(IV);
  if (!prot->Init(key.Data(), key.Size(), IV.Data()))
  {
    Fail("Creation of AES key for PBO header encryption failed.");
    delete prot;
    return 0;
  }
  return prot;
}

/// creates serial number for testing
void CreateTestSerial(TByteBuffer &serial, bool isMain)
{
  static const int serialSize = 20;
  serial.Clear();
  serial.Resize(serialSize);
  for (int i=0; i<serialSize; ++i)
  {
    serial[i] = isMain ? (71+i*113): (1313 + i*i*1717);
  }
}

int GetProtectionVersion()
{
#ifdef _MSC_VER
  int version = 'HP00';
#else
  int version = StrToInt("00PH");
#endif
  return version;
}

void SaveDummyPaddingFile(QOStream &out, QFileSize size)
{
  for (QFileSize i=0; i<size; ++i)
  {
    out.put(0);
  }
}

QFileSize EncryptHeaders( QOStream &out, QOStrStream &normalHeaders, IFilebankEncryption* headerProtection)
{
  Assert(headerProtection);
  if (!headerProtection)
    return 0;

  // encrypt headers
  QOStrStream headersEncoded;
  headerProtection->Encode(headersEncoded, normalHeaders.str(), normalHeaders.pcount());

  int startPos = out.tellp();

  // write version
  int version = GetProtectionVersion();
  out.write(&version,sizeof(version));
  // save headers decrypted size
  int headersSize = normalHeaders.pcount();
  out.write(&headersSize,sizeof(headersSize));
  // save headers encrypted size
  int headersEncodedSize = headersEncoded.pcount();
  out.write(&headersEncodedSize,sizeof(headersEncodedSize));
  // write headers
  out.write(headersEncoded.str(), headersEncoded.pcount());

  // return number of written bytes
  return out.tellp() - startPos;
}

bool GetProtectionInfo(QIStream& in, DecryptedHeaderInfo& outInfo)
{
  outInfo.isEncrypted = false;

  QFileSize startPos = in.tellg();

  // read version
  int version = 0;
  int rd = in.read(&version, sizeof(version));
  if (rd != sizeof(version) || in.fail() || in.eof())
  {
    return false;
  }

  if (version != GetProtectionVersion())
  {
    return false;
  }

  // read headers decrypted size
  QFileSize headersSize = 0;
  rd = in.read(&headersSize,sizeof(headersSize));
  if (rd != sizeof(headersSize) || in.fail() || in.eof())
  {
    return false;
  }

  // read headers encrypted size
  QFileSize headersEncodedSize = 0;
  rd = in.read(&headersEncodedSize,sizeof(headersEncodedSize));
  if (rd != sizeof(headersEncodedSize) ||in.fail() || in.eof())
  {
    return false;
  }

  // check if sizes are OK
  if (headersEncodedSize < headersSize)
  {
    RptF("Invalid size of encrypted header");
    headersSize = headersEncodedSize;
  }

  // get stream size
  QFileSize tmp = in.tellg();
  in.seekg(0, QIOS::end);
  QFileSize streamSize = in.tellg();
  in.seekg(tmp);
  if (headersEncodedSize > streamSize)
  {
    RptF("Invalid size of encrypted header");
    headersEncodedSize = streamSize;
  }
  if (headersSize > streamSize)
  {
    RptF("Invalid size of encrypted header");
    headersSize = streamSize;
  }

  /// offset = size of additional data (version, header size,...)
  QFileSize additionalDataSize = in.tellg() - startPos;

  outInfo.isEncrypted = true;
  outInfo.headerSize = headersSize;
  outInfo.headerSizeWithPadding = headersEncodedSize + additionalDataSize;
  outInfo.headerEncodedSize = headersEncodedSize;

  return true;
}

bool DecryptHeaders( QOStream &out, QIStream &encryptedHeaders, IFilebankEncryption* headerProtection, 
  DecryptedHeaderInfo* decryptInfo)
{
  DecryptedHeaderInfo info;
  if (decryptInfo)
    decryptInfo->isEncrypted = false;

  if (!GetProtectionInfo(encryptedHeaders, info))
  {
    RptF("Failed to decrypt header - cannot get header protection info.");
    return false;
  }

  // check sizes
  static const QFileSize MaxAllowedSize = 128*1024*1024; //128MB should be enough for a header 
  if (info.headerSize > MaxAllowedSize || info.headerEncodedSize > MaxAllowedSize)
  {
    RptF("Failed to decrypt header - header sizes are too large.");
    return false;
  }
  
  // read encr. headers
  Temp<unsigned char> encrypted(info.headerEncodedSize);
  QFileSize rd = encryptedHeaders.read(encrypted.Data(), encrypted.Size());
  if (rd != encrypted.Size() || encryptedHeaders.fail() || encryptedHeaders.eof())
  {
    RptF("Error reading stream while decrypting headers.");
    return false;
  }
 
  // create stream with encr headers
  QIStrStream encHeaders;
  encHeaders.init(encrypted.Data(), encrypted.Size());

  // decrypt headers
  Temp<char> decryptedHeaders(info.headerSize);
  if (!headerProtection->Decode(decryptedHeaders.Data(), decryptedHeaders.Size(), encHeaders))
  {
    RptF("Error while decrypting headers.");
    return false;
  }

  // write headers to output
  out.write(decryptedHeaders.Data(), decryptedHeaders.Size());

  // write dummy file to output
  SaveDummyPaddingFile(out, info.headerSizeWithPadding - info.headerSize);

  if (decryptInfo)
  {
    *decryptInfo = info;
  }

  return true;  
}

RString TestParentSerial;
RString TestSerial;
RString AppRegistryPath;

bool CreateTestSerialsFromTestStrings(TByteBuffer &outSerial1, TByteBuffer &outSerial2)
{
  // TODO disable this for retail

  if (!TestParentSerial.IsEmpty() && TestParentSerial.GetLength() != KEY_BYTES)
  {
    RptF("protTestParentSerial is not used - it has invalid length");
  }
  if (!TestSerial.IsEmpty() && TestSerial.GetLength() != KEY_BYTES)
  {
    RptF("protTestSerial is not used - it has invalid length");
  }

  if (TestParentSerial.GetLength() == KEY_BYTES && TestSerial.GetLength() == KEY_BYTES)
  {
    outSerial1.Clear();
    outSerial1.Resize(TestParentSerial.GetLength());
    outSerial1.Copy((unsigned char*)TestParentSerial.Data(), TestParentSerial.GetLength());

    outSerial2.Clear();
    outSerial2.Resize(TestSerial.GetLength());
    outSerial2.Copy((unsigned char*)TestSerial.Data(), TestSerial.GetLength());

    return true;
  }

  return false;
}

RWString UTFToUnicode(RString text)
{
  // convert from UTF-8 to Unicode
  int len = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
  AUTO_STATIC_ARRAY(wchar_t, wBuffer, 1024);
  wBuffer.Resize(len);
  MultiByteToWideChar(CP_UTF8, 0, text, -1, wBuffer.Data(), len);
  wBuffer[len - 1] = 0; // make sure result is always null terminated
  return wBuffer.Data();
}

bool CreateSerialFromRegistry(const RString& path, TByteBuffer& outSerial)
{
  outSerial.Clear();
  outSerial.Resize(KEY_BYTES);
  for (int i=0; i<outSerial.Size(); ++i)
  {
    outSerial[i] = 0;
  }

  HKEY root = HKEY_LOCAL_MACHINE;
  HKEY key;

#if UNICODE
  RWString tmpPath = UTFToUnicode(path);
#else
  RString tmpPath = path;
#endif

  if (::RegOpenKeyEx(root, tmpPath.Data(), 0, KEY_READ, &key ) != ERROR_SUCCESS) 
  {
    RptF("Failed to get key \"%s\" from registry.", cc_cast(path));
    return false;
  }

  DWORD size = KEY_BYTES;
  DWORD type = REG_BINARY;
#if UNICODE
  HRESULT result = ::RegQueryValueEx(key, L"KEY", 0, &type, outSerial.Data(), &size);
#else
  HRESULT result = ::RegQueryValueEx(key, "KEY", 0, &type, outSerial.Data(), &size);  
#endif
  ::RegCloseKey(key);
  return result == ERROR_SUCCESS && size == KEY_BYTES;
}

bool CreateProtectionSerials(const RString& parentAppRegPath, const RString& pboRegistry, 
  PBOHeaderProtection::TByteBuffer& outSerial1, PBOHeaderProtection::TByteBuffer& outSerial2)
{
  if (!CreateSerialFromRegistry(parentAppRegPath, outSerial1))
  {
    RptF("Cannot get serial number from registry path \"%s\"", cc_cast(parentAppRegPath));
    return false;
  }

  RString pboRegistryPath;

  // check if PBO registry contains full path
  if (pboRegistry.Find('\\') == -1)
  {
    // not a full path - we have to construct full path based on parent application registry path

    // create path from app path by removing app name and adding registry name from PBO
    // TODO maybe there is a better way to do this?
    int pos = parentAppRegPath.ReverseFind('\\');
    if (-1 == pos)
    {
      RptF("Failed to create PBO registry path.");
      return false;
    }

    pboRegistryPath = parentAppRegPath.Substring(0, pos) + "\\" + pboRegistry;
  }
  else
  {
    pboRegistryPath = pboRegistry;
  }


  if (!PBOHeaderProtection::CreateSerialFromRegistry(pboRegistryPath, outSerial2))
  {
    RptF("Cannot get serial number from registry path \"%s\"", cc_cast(pboRegistryPath));
    return false;
  }

  return true;
}

bool CreateProtectionSerialsFromPBO(const RString& parentAppRegPath, const RString& pboFileName, 
  PBOHeaderProtection::TByteBuffer& outSerial1, PBOHeaderProtection::TByteBuffer& outSerial2)
{
  // open bank
  QFBank bank;
  if (!bank.openFromFile(pboFileName))
  {
    RptF("Failed to open file \"%s\".", cc_cast(pboFileName));
    return false;
  }

  // get property
  RString pboRegistry = bank.GetProperty(PRORegSerialProp);
  if (pboRegistry.IsEmpty())
  {
    RptF("File \"%s\" has empty property \"%s\" - this property is needed to get serial number from registry.", cc_cast(pboFileName), PRORegSerialProp);
    return false;
  }
  
  return CreateProtectionSerials(parentAppRegPath, pboRegistry, outSerial1, outSerial2);
}

TransformResult TransformStream(QIStream& inStream, QOStream& outStream, IFilebankEncryption* decryptClass, IFilebankEncryption* encryptClass)
{
  Assert(decryptClass);
  if (!decryptClass)
    return TRDecryptionFailed;

  /// decrypted header (without padding)
  QOStrStream normalHeader;
  /// decrypted header (with padding)
  QOStrStream normalHeaderWithPadding;

  // decrypt header
  PBOHeaderProtection::DecryptedHeaderInfo decryptInfo;
  if (!PBOHeaderProtection::DecryptHeaders(normalHeaderWithPadding, inStream, decryptClass, &decryptInfo))
  {
    RptF("Stream cannot be decrypted - decryption failed.");   
    return TRDecryptionFailed;
  }

  // check decrypted size
  if (normalHeaderWithPadding.pcount() != decryptInfo.headerSizeWithPadding || decryptInfo.headerSize > decryptInfo.headerSizeWithPadding)
  {
    RptF("Stream cannot be decrypted - size of decrypted header is wrong.");   
    return TRDecryptionFailed;
  }

  // create stream with decrypted header, but without padding
  normalHeader.write(normalHeaderWithPadding.str(), decryptInfo.headerSize);

  // check decrypted header
  if (!CheckDecryptedHeader(normalHeader.str(), normalHeader.pcount()))
  {
    RptF("Decrypted header of stream is invalid - probably wrong decryption key.");   
    return TRInvalidDecryptedHeader;
  }

  if (encryptClass)
  {
    // encrypt headers
    // create encrypted header
    QFileSize encrSize = PBOHeaderProtection::EncryptHeaders(outStream, normalHeader, encryptClass);

    // check size
    if (encrSize != normalHeaderWithPadding.pcount())
    {
      RptF("Failed to encrypt header of stream - encrypted header size is wrong.");   
      return TREncryptionFailed;
    }
  }
  else
  {
    // just write decrypted headers
    outStream.write(normalHeader.str(), normalHeader.pcount());
  }

  return TROk;
}


TransformResult TransformPBO(const RString& fileName, IFilebankEncryption* decryptClass, IFilebankEncryption* encryptClass, const RString fileNameOut)
{
  Assert(decryptClass || encryptClass);

  /// open PBO
  QFBank bank;
  if (!bank.openFromFile(fileName))
  {        
    RptF("Failed to open file \"%s\" for reading.", cc_cast(fileName));
    return TRFailedToOpenForReading;
  }

  /// get protection info
  QFBank::BankHeaderProtectionInfo protInfo;
  bank.GetBankHeaderProtectionInfo(protInfo);

  /// decrypted header (without padding)
  QOStrStream normalHeader;
  /// decrypted header (with padding)
  QOStrStream normalHeaderWithPadding;

  /// starting offset of header in the PBO
  QFileSize headerStartOffset = bank.GetHeaderStartOffset();

  if (decryptClass)
  {
    /// we need to decrypt headers first
    if (!protInfo.isProtected)
    {
      RptF("Bank \"%s\" is not encrypted.", cc_cast(fileName));
      // bank is not protected
      return TRUnexpectedNotEncrypted;
    }

    Assert(headerStartOffset == protInfo.protHeaderStart);

    //close bank
    bank.close();

    // open bank again for reading
    QIFStream ifs;
    ifs.open(cc_cast(fileName));
    if (ifs.fail())
    {
      RptF("Failed to open file \"%s\" for reading.", cc_cast(fileName));
      return TRFailedToOpenForReading;
    }

    // seek to the header start
    ifs.seekg(headerStartOffset);

    // decrypt header
    PBOHeaderProtection::DecryptedHeaderInfo decryptInfo;
    if (!PBOHeaderProtection::DecryptHeaders(normalHeaderWithPadding, ifs, decryptClass, &decryptInfo))
    {
      RptF("Headers of bank \"%s\" cannot be decrypted - decryption failed.", cc_cast(fileName));   
      return TRDecryptionFailed;
    }

    //close file
    ifs.close();

    // check decrypted size
    if (normalHeaderWithPadding.pcount() != decryptInfo.headerSizeWithPadding || decryptInfo.headerSize > decryptInfo.headerSizeWithPadding)
    {
      RptF("Headers of bank \"%s\" cannot be decrypted - size of decrypted header is wrong.", cc_cast(fileName));   
      return TRDecryptionFailed;
    }

    // create stream with decrypted header, but without padding
    normalHeader.write(normalHeaderWithPadding.str(), decryptInfo.headerSize);
  }
  else
  {
    if (protInfo.isProtected)
    {
      // PBO is protected, but no decryption class given
      if (!protInfo.isProtected)
      {
        RptF("Bank \"%s\" is encrypted, but no decryption class was given.", cc_cast(fileName));
        return TRUnexpectedEncrypted;
      }
    }

    // header must be ready for encryption
    // first check property
    RString prop = bank.GetProperty(PBOHeaderProtection::PBOPropertyName);
    if (prop.IsEmpty())
    {
      RptF("Bank \"%s\" is not prepared for header encryption!", cc_cast(fileName));
      return TRNotPreparedForEncryption;
    }

    // load file infos
    if (!bank.Load())
    {
      RptF("Failed to load headers of bank \"%s\"", cc_cast(fileName));
      return TRFailedToLoadHeader;
    }

    // get dummy file info
    const FileInfoO &info = bank.FindFileInfo(PBOHeaderProtection::DummyPaddingFileName);
    if (!bank.NotNull(info))
    {
      // info not found
      RptF("Bank \"%s\" is not prepared for header encryption - cannot find dummy file info.", cc_cast(fileName));
      return TRNotPreparedForEncryption;
    }

    // get size of the header 
    int headerSize = info.startOffset - headerStartOffset;

    if (headerSize <= 0)
    {
      RptF("Bank \"%s\" is not prepared for header encryption - invalid header size.", cc_cast(fileName));
      return TRNotPreparedForEncryption;
    }

    // size of the header including padding
    int headerSizeIncPadding = headerSize + info.length;

    // close bank
    bank.close(); 

    // open bank again for reading
    QIFStream ifs;
    ifs.open(cc_cast(fileName));

    // seek to the header start
    ifs.seekg(headerStartOffset);

    // read header
    Temp<unsigned char> headerData(headerSizeIncPadding);
    int rd = ifs.read(headerData.Data(), headerData.Size());
    if (rd != headerData.Size() || ifs.fail() || headerSize > headerSizeIncPadding)
    {
      RptF("Headers of bank \"%s\" cannot be encrypted - error reading headers from stream.", cc_cast(fileName));   
      return TRFailedToLoadHeader;
    }

    // write header to new stream
    normalHeaderWithPadding.write(headerData.Data(), headerData.Size());
    normalHeader.write(headerData.Data(), headerSize);

    // close file
    ifs.close();
  }

  // check decrypted header
  if (!CheckDecryptedHeader(normalHeader.str(), normalHeader.pcount()))
  {
    RptF("Decrypted header of \"%s\" is invalid - probably wrong decryption key.", cc_cast(fileName));   
    return TRInvalidDecryptedHeader;
  }

  if (encryptClass)
  {
    // encrypt headers
    // create encrypted header
    QOStrStream encryptedHeader;
    QFileSize encrSize = PBOHeaderProtection::EncryptHeaders(encryptedHeader, normalHeader, encryptClass);

    // check size
    if (encrSize != normalHeaderWithPadding.pcount())
    {
      RptF("Failed to encrypt header of \"%s\" - encrypted header size is wrong.", cc_cast(fileName));   
      return TREncryptionFailed;
    }

    // open bank again for writing
    QOFStream ofs;

    if (!fileNameOut.IsEmpty() && stricmp(fileNameOut, fileName)!=0)
    {
      // copy the fileName file into the fileNameOut
      QIFileFunctions::Copy(fileName, fileNameOut);
      ofs.openForAppend(fileNameOut);
    }
    else ofs.openForAppend(fileName);

    if (ofs.fail())
    {
      RptF("Failed to open file \"%s\" for writing.", cc_cast(fileName));   
      return TRFailedToOpenForWriting;
    }

    // seek to the start of header
    ofs.seekp(headerStartOffset, QIOS::beg);

    // write encrypted header
    ofs.write(encryptedHeader.str(), encryptedHeader.pcount());

    ofs.close();
  }
  else
  {
    // just write decrypted headers

    // open bank again for writing
    QOFStream ofs;

    if (!fileNameOut.IsEmpty() && stricmp(fileNameOut, fileName)!=0)
    {
      // copy the fileName file into the fileNameOut
      QIFileFunctions::Copy(fileName, fileNameOut);
      ofs.openForAppend(fileNameOut);
    }
    else ofs.openForAppend(fileName);

    if (ofs.fail())
    {
      RptF("Failed to open file \"%s\" for writing.", cc_cast(fileName));   
      return TRFailedToOpenForWriting;
    }

    // seek to the start of header
    ofs.seekp(headerStartOffset, QIOS::beg);

    // write encrypted header
    ofs.write(normalHeader.str(), normalHeader.pcount());

    ofs.close();
  }

  return TROk;      
}

bool IsProtectedPBO(const RString& fileName, bool preciseExtensionCheck, QFBank::BankHeaderProtectionInfo* info)
{
  if (preciseExtensionCheck)
  {
    const char *ext = GetFileExt(fileName); 
    if (!ext || stricmp(ext, ".pbo") != 0)
    {
      // not a PBO
      return false;
    }
  }
  else
  {
    RString temp = fileName;
    temp.Lower();
    if (temp.Find(".pbo") == -1)
      return false; //not a pbo
  }

  /// open PBO
  QFBank bank;
  if (!bank.openFromFile(fileName))
  {        
    RptF("Failed to open file \"%s\" for reading.", cc_cast(fileName));
    return false;
  }

  /// get protection info
  QFBank::BankHeaderProtectionInfo protInfo;
  bank.GetBankHeaderProtectionInfo(protInfo);

  // close bank
  bank.close();

  if (info)
    *info = protInfo;

  return protInfo.isProtected;
}

bool CheckDecryptedHeader(const char* const data, unsigned size)
{
  Assert(data);
  if (!data)
    return false;

  /// check if the start of the header contains dummy file name
  unsigned dummySize = strlen(DummyPaddingFileName);

  if (size < dummySize)
    return false;
  
  return memcmp(data, DummyPaddingFileName, dummySize) == 0;
}


RString TransformResultToString(TransformResult result)
{
  switch (result)
  {
  case TROk:
    return "OK";
  case TRFailedToOpenForReading:
    return "Failed to open file for reading.";
  case TRFailedToOpenForWriting:
    return "Failed to open file for writing.";
  case TRUnexpectedNotEncrypted:
    return "File is not encrypted.";
  case TRUnexpectedEncrypted:
    return "File is encrypted.";
  case TRNotPreparedForEncryption:
    return "File is not prepared for encryption.";
  case TRFailedToLoadHeader:
    return "Failed to load header.";
  case TRDecryptionFailed:
    return "Decryption failed.";
  case TREncryptionFailed:
    return "Encryption failed.";
  case TRInvalidDecryptedHeader:
    return "Decrypted header is invalid.";
  default:
    return "Unknown error.";
  }
}

}//namespace PBOHeaderProtection

#endif //#if _ENABLE_PBO_PROTECTION
