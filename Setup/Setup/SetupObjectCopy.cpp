/////////////////////////////////////////////////////////////////////////////
// Modifikace hlavního okna


#include "stdafx.h"
#include "SetupObject.h"
#include "Setup.h"
#include <process.h>
#include <stdio.h>
#include <io.h>
#include <sys/stat.h>
#include <El/stringtable/stringtable.hpp>
#include <libpng/include/zlib.h>
#include <El/QStream/qGzStream.hpp>
#if _ENABLE_PBO_PROTECTION
#include <El/QStream/packFiles.hpp>
#endif //#if _ENABLE_PBO_PROTECTION

#define LZMA_API_STATIC
#include <El/QStream/qLzmaStream.hpp>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/// pocitani CRC

class CRCCalculator
{
	unsigned long _table[256];
	unsigned long _crc;

	void InitTable();

	public:
	//! Constructor
	CRCCalculator();
	//! Calculate CRC of memory block
	unsigned long CRC( const void *data, int len );
	//! Calculate CRC of memory block
	/*!
	\name Incremental interface
	\patch_internal 1.11 Date 8/1/2001 by Ondra
	- New: Incremental CRC interface.
	*/
	//@{
	//! initialize
	void Reset();
	//! add memory block
	void Add(const void *data, int len);
	//! add signle character
	void Add(char c);
	//! get result of all Add operations since Reset()
	unsigned long GetResult() const {return ~_crc;}
	//}@

};

CRCCalculator::CRCCalculator()
{
	InitTable();
}

void CRCCalculator::Reset()
{
	_crc = ~0;       // preload shift register, per CRC-32 spec
}

void CRCCalculator::Add(const void *data, int len)
{
	const unsigned char *p = (const unsigned char *)data;
	unsigned long crc = _crc;
	for( ; len>0; p++,len-- )
	{
		crc = (crc<<8) ^ _table[(crc>>24) ^ *p];
	}
	_crc=crc;
}

void CRCCalculator::Add(char c)
{
	_crc = (_crc<<8) ^ _table[(_crc>>24) ^ c];
}

unsigned long CRCCalculator::CRC( const void *data, int len )
{
	const unsigned char *p = (const unsigned char *)data;
	unsigned long crc = ~0;       // preload shift register, per CRC-32 spec
	for( ; len>0; p++,len-- )
	{
		crc = (crc<<8) ^ _table[(crc>>24) ^ *p];
	}
	return ~crc; // transmit complement, per CRC-32 spec
}

void CRCCalculator::InitTable()
{
	const int CRCPoly=0x04c11db7; // AUTODIN II, Ethernet, & FDDI
	for( int i=0; i<256; i++ )
	{
		unsigned long c;
		int j;
		for (c = i << 24, j = 8; j > 0; --j)
		{
			c = c & 0x80000000 ? (c << 1) ^ CRCPoly : (c << 1);
		}
		_table[i] = c;
	}
}


/////////////////////////////////////////////////////////////////////////////
// proces kopírování 

#include <Es/Memory/normalNew.hpp>
/// a buffer style reference to existing data
class QIStreamBufferTmp: public QIStreamBuffer
{
public:
  QIStreamBufferTmp(const void *data, int size);
  ~QIStreamBufferTmp();

  USE_FAST_ALLOCATOR
};
#include <Es/Memory/debugNew.hpp>

DEFINE_FAST_ALLOCATOR(QIStreamBufferTmp)

QIStreamBufferTmp::QIStreamBufferTmp(const void *data, int size)
{
  _buffer = (char *)data;
  _bufferSize = size;
}
QIStreamBufferTmp::~QIStreamBufferTmp()
{
  _buffer = NULL;
  _bufferSize = 0;
}

//Stream for concatenating more Windows Resources items into one stream
class QIMultiResStream: public QIStream
{
private:
  RString resourceName;
  int curPart;
public:
  QIMultiResStream(RString resName) : resourceName(resName), curPart(0)
  {
    int resSize;
    void *data = FindAndLoadResource(resSize, NULL, resourceName);
    if (data) _buf = PosQIStreamBuffer(new QIStreamBufferTmp(data,resSize),0,resSize);
    else _buf = PosQIStreamBuffer(NULL,0,0);
    _readFromBuf = 0;
  }
  virtual ~QIMultiResStream() 
  {
    _buf = PosQIStreamBuffer(NULL,0,0);
  }
  virtual PosQIStreamBuffer ReadBuffer( PosQIStreamBuffer &buf, QFileSize pos )
  {
    if (pos-buf._start == buf._len)
    { //we should read from next file part (if there is any)
      int resSize;
      curPart++;
      RString resName; 
      resName = Format("%s__PART_%02d",cc_cast(resourceName),curPart);
      void *data = FindAndLoadResource(resSize, NULL, resName);
      if (!data) return buf;
      _buf = PosQIStreamBuffer(new QIStreamBufferTmp(data,resSize),pos,resSize);
      _readFromBuf = 0;
      return _buf;
    }
    else
    {
      return buf; //there are still data in this buffer
    }
  }
  //other pure virtual functions
  //! reader can advertise it will be reading
  /*!
  \return true when data are as ready as possible
  */
  virtual bool PreloadBuffer(
    RequestableObject *obj, RequestableObject::RequestContext *ctx,
    FileRequestPriority priority, QFileSize pos, QFileSize size
    ) const
  {
    return true; //data are always ready
  }
  //! get total data size
  virtual QFileSize GetDataSize() const
  {
    return 0; //we do not know the proper data size (as it will be concatenated)
  }
};

void BankPosition::Init(RString fileName)
{
  QIFStream in;
  QFileSize fsiz = QIFileFunctions::GetFileSize(fileName);
  in.open(fileName);
  in.seekg(fsiz-sizeof(int));
  int size;
  in.read(&size, sizeof(int));
  QFileSize bankInfoSeek = fsiz-sizeof(int)-size;
  in.seekg(bankInfoSeek);
  BankInfo bankInfo;
  SerializeBinStream stream(&in);
  stream << bankInfo;
  in.close();
  if (strcmp(bankInfo.testCode,"BANK")==0)
  {
    filePos.Resize(bankInfo.bankLen.Size());
    QFileSize bankPos = bankInfoSeek;
    for (int i=bankInfo.bankLen.Size()-1; i>=0; i--)
    {
      int bankSize = bankInfo.bankLen[i];
      bankPos -= bankSize;
      filePos.Set(i)=BankFilePos(bankPos,bankSize);
    }
  }
#if 1
  for (int i=0; i<filePos.Size(); i++)
    LogF("setup%02d.bin start=%u size=%u",i+1,filePos[i].start,filePos[i].size);
#endif
  initialized = true;
}

void BankPosition::Get(int ix)
{
  ix--; //it is supposed, ix in [1..N]
  if (ix<filePos.Size() && ix>=0)
  {
    start = filePos[ix].start;
    size = filePos[ix].size;
  }
  else
  {
    start = size = 0; //bank not found
  }
}

#define COPY_BUFF_SIZE	(256*1024)

void CSetupObject::EnterLock()
{
	EnterCriticalSection(&m_CSection_CopyFiles);
}

void CSetupObject::LeaveLock()
{
	LeaveCriticalSection(&m_CSection_CopyFiles);
}

int GzipMemToFile(LPGZIP pgzip,int len, const char *fname)
{
  CGZIP2FILE toFile(pgzip, len, fname);
  return toFile.Length;
}

int GzipQStreamToFile(QIStream &in, const char *fname)
{
  GZipQStream2File toFile(in, fname);
  return toFile.Length;
}

// Helper structure to work with plain file, gz files and xz files through one interface
struct FileDecompresor
{
  enum {EncNone, EncGZ, EncXZ} encType;
  union {
    voidp         source;   // used only for EncGZ
    struct {
      QIFStream    *in;       
      QILzmaStream *xzStream; // used only for EncXZ
    };
  } decoder;
  FileDecompresor::FileDecompresor() : encType(EncNone) {};
  // the encType is auto detected by file ext
  // returns false when unable to initialize
  bool Init(RString fileName);
  // Reads the given number of uncompressed bytes from the compressed file.
  // returns the number of uncompressed bytes actually read (0 for end of file, -1 for error).
  int Read(void *buf, int size);
  void Close();
};

bool FileDecompresor::Init(RString fileName)
{
  if (stricmp(GetFileExt(fileName), ".xz")==0)
  { // use QILzmaStream
    encType = EncXZ;
    decoder.in = new QIFStream();
    decoder.in->open(fileName);
    decoder.xzStream = new QILzmaStream(decoder.in);
    return true;
  }
  else
  { // use gzread
    encType = EncGZ;
    decoder.source = gzopen(fileName, "rb");
    return decoder.source != NULL;
  }
}

int FileDecompresor::Read(void *buf, int size)
{
  switch (encType)
  {
  case EncGZ:
    if (decoder.source)
      return gzread(decoder.source, buf, size);
    else
      return -1;
  case EncXZ:
    if (decoder.xzStream)
      return decoder.xzStream->read(buf, size);
    else
      return -1;
  default:
    return -1;
  }
}

void FileDecompresor::Close()
{
  switch (encType)
  {
  case EncGZ: 
    if (decoder.source)
    {
      gzclose(decoder.source); 
      decoder.source=NULL; 
    }
    break;
  case EncXZ: 
    if (decoder.xzStream)
    {
      decoder.in->close();
      delete decoder.xzStream;
      decoder.xzStream = NULL;
    }
    break;
  }
}

#if _ENABLE_PBO_PROTECTION
bool CSetupObject::CreateHeaderProtectionSerials(PBOHeaderProtection::TByteBuffer& outSerial1, PBOHeaderProtection::TByteBuffer& outSerial2)
{
  // first check if test serials are given
  // TODO remove this in retail?
  //if (PBOHeaderProtection::CreateTestSerialsFromTestStrings(outSerial1, outSerial2))
    //return true;

  // create serial number from this app serial number
  outSerial2.Clear();
  outSerial2.Copy(m_cdkey, KEY_BYTES);


  if (m_sParentAppRegistryRoot.IsEmpty())
  {
    // TODO error message
    ErrF("Path to parent application registry root is empty - cannot get serial number.");
    return false;      
  }

  // for A2RFT setup, it have only one key
  if (m_sParentAppRegistryRoot=="-")
  {
    outSerial1=outSerial2;
  }
  else
  {
  // get parent application serial number from registry
  if (!PBOHeaderProtection::CreateSerialFromRegistry(CString2RString(m_sParentAppRegistryRoot), outSerial1))
  {
    // TODO error message
    ErrF("Failed to get parent application serial number from registry.");
    return false;            
  }
  }


  // just generate some testing serial numbers
  //PBOHeaderProtection::CreateTestSerial(outSerial1, true);
  //PBOHeaderProtection::CreateTestSerial(outSerial2, false);
  return true;
}

bool CSetupObject::ProtectPBOHeaders(RString fileName, CString& error)
{
  const char *ext = GetFileExt(fileName); 
  if (!ext || stricmp(ext, ".pbo") != 0)
  {
    // not a PBO, return true
    return true;
  }

  /// open PBO
  QFBank bank;
  if (!bank.openFromFile(fileName))
  {        
    ErrF("Failed to open file \"%s\" for reading.", cc_cast(fileName));
    // TODO LOCALIZATION
    error = _T("Failed to open file for reading.");
    return false;
  }

  /// get protection info
  QFBank::BankHeaderProtectionInfo protInfo;
  bank.GetBankHeaderProtectionInfo(protInfo);

  // close bank
  bank.close();

  if (!protInfo.isProtected)
  {
    // not protected, return true
    return true;
  }

  SRef<IFilebankEncryption> decryptClass(PBOHeaderProtection::CreateSetupHeaderProtection());
  if (decryptClass.IsNull())
  {
    ErrF("Failed to create class for PBO header decryption.");
    // TODO LOCALIZATION
    error = _T("Failed to create class for PBO header decryption.");
    return false;
  }

  /// get serial numbers
  PBOHeaderProtection::TByteBuffer serial1;
  PBOHeaderProtection::TByteBuffer serial2;
  if (!CreateHeaderProtectionSerials(serial1, serial2))
  {
    ErrF("Failed to get serial numbers for header encryption.");
    // TODO LOCALIZATION
    error = _T("Failed to get serial numbers for header encryption.");
    return false;      
  }

  /// create protection that we will use to encrypt headers
  SRef<IFilebankEncryption> encryptClass(PBOHeaderProtection::CreateSerialHeaderProtection(serial1, serial2));
  if (encryptClass.IsNull())
  {
    ErrF("Failed to create class for PBO header encryption.");
    // TODO LOCALIZATION
    error = _T("Failed to create class for PBO header encryption.");
    return false;
  }

  /// transform PBO
  PBOHeaderProtection::TransformResult result = PBOHeaderProtection::TransformPBO(fileName, decryptClass, encryptClass);

  switch(result)
  {
  case PBOHeaderProtection::TROk:
    /// everything was ok
    return true;
    break;
  case PBOHeaderProtection::TRNotPBO:
    /// file is not a PBO, that is ok, just return true
    return true;
    break;
  case PBOHeaderProtection::TRUnexpectedNotEncrypted:
    /// PBO is not protected, that is also ok, just return true
    return true;
    break;
  default:
    {
      RString resError = PBOHeaderProtection::TransformResultToString(result);
      ErrF("Failed to protect PBO - \"%s\"", cc_cast(resError));
      CString UTF8ToCString(LPCSTR buff);
      error = UTF8ToCString(cc_cast(resError));
      return false;
    } 
  }
}

#endif //#if _ENABLE_PBO_PROTECTION
DWORD CSetupObject::CopyFilesFunc()
{
	CString sSrcFile,sDestFile;
  __int64 iFileSize = 0;
	unsigned int nCopyCRC;
	bool bTestCRC;
	// zahájím kopírování
	EnterLock(); 
//	ASSERT(!m_bRunningCopy);
	ASSERT(m_bRunningCopy);
//	m_bRunningCopy = TRUE;
	sSrcFile	= m_sCopySource;
	sDestFile	= m_sCopyDestination;
	nCopyCRC = m_nCopyCRC;
	bTestCRC = m_bTestCRC;
  char whereSaved = m_cWhereSaved; 
  bool errorBinFile = false;

    TCHAR exeFile[1024]; 
    //this part is good to force using other than running setup.exe file (which makes debugging possible)
    if (m_patchingSetup.IsEmpty()) GetModuleFileName(0,exeFile,1024);
    else 
    {
      _tcscpy(exeFile, m_patchingSetup);
    }
    RString exeFileR = LPCTSTR2RString(exeFile);

  RString exeNameNoExt = exeFileR;
  *GetFileExt(exeNameNoExt.MutableData()) = 0; //making exeDir from exeFile 


  if (!m_bankPos.initialized) 
  {
    m_bankPos.Init(exeFileR);
    m_bankPos.Get(whereSaved);
    if (!m_bankPos.filePos.Size() && IsSetupBinUsed())
    {
      extern bool FileExists(CString &path);

      CString binFile = CString(exeNameNoExt) + CString("01.bin"); //exeFile is exeDir now
      if (!FileExists(binFile))
        { //we should report error
          m_sCopyError = SetupLocalizeString(_T("SETUP_STR_BIN_FILE_NOT_FOUND"));
          errorBinFile = true;
        }
      }
    }
  BankPosition bankPos = m_bankPos;
	LeaveLock();
  if (errorBinFile) ; //only to skip copy processing
  else if (whereSaved==-1) // zahájeno kopírování
  {
    BOOL  bEnd = FALSE;
    FileDecompresor Source;
    CFile Dest; 

    // test whether XZ comprimation is used
    int inslen = sSrcFile.GetLength();
    bool isUsingXZ = false;
    if (inslen>3)
    {
      if (sSrcFile.Mid(inslen-3,3).MakeUpper()==CString(".XZ"))
      {
        isUsingXZ = true;
      }
    }

/*
    TCHAR xzbuf[1024]; 
    if (isUsingXZ) 
    {
      swprintf(xzbuf, _T("%s is using xz"), sSrcFile.GetBuffer());
      OutputDebugString(xzbuf);
    }
*/

    TRY
    {
      int maxCRCRetry = 10;

      bool crcOK = false;
      for(int crcRetry=0; crcRetry<maxCRCRetry; crcRetry++)
      {
        CFileException Error;
        CRCCalculator crc;
        if ( !Source.Init(CString2RString(sSrcFile)) )
        {
          EnterLock(); 
          SetupFormatString1(m_sCopyError,SetupLocalizeString (_T("SETUP_STR_ERR_FILE_READ")),sSrcFile);
          m_canIgnoreError = true;
          LeaveLock(); 
          AfxThrowUserException();
        }
        if (_taccess (sDestFile, 00) == 0 && _tchmod (sDestFile, _S_IWRITE) != 0)
        {
          EnterLock(); 
          SetupFormatString1(m_sCopyError,SetupLocalizeString (_T("SETUP_STR_ERR_CREATE_FILE")),sDestFile);
          m_canIgnoreError = true;
          LeaveLock(); 
          AfxThrowUserException();
        }
        if (!Dest.Open(sDestFile,CFile::modeCreate|CFile::modeWrite|CFile::typeBinary))
        {
          EnterLock(); 
          SetupFormatString1(m_sCopyError,SetupLocalizeString (_T("SETUP_STR_ERR_CREATE_FILE")),sDestFile);
          m_canIgnoreError = true;
          LeaveLock(); 
          AfxThrowUserException();
        }
        crc.Reset();
        int   nBytes;
        TCHAR buff[COPY_BUFF_SIZE];
        // kopírování
        DWORD	nLen = 0;
        bool copyFinished = false;
        while (!bEnd && !copyFinished)
        {
          nBytes = Source.Read(buff, COPY_BUFF_SIZE);
          if (nBytes>0)
          {
            crc.Add(buff,nBytes);
            nLen += nBytes;
            TRY
            {
              Dest.Write(buff,nBytes);
            }
            CATCH_ALL(e)
            {
              EnterLock(); 
              SetupFormatString1(m_sCopyError,SetupLocalizeString (_T("SETUP_STR_ERR_FILE_WRITE")),sDestFile);
              m_canIgnoreError = true;
              LeaveLock(); 
              THROW_LAST();
            } 
            END_CATCH_ALL
          }
          else copyFinished = true;
          // zaznamenám zkopírované byty
          EnterLock(); 
          m_nCurrCopySize += nBytes;
          iFileSize += nBytes;
          if (m_bCancelCopy)
            bEnd = TRUE;
          LeaveLock(); 
        };
        Source.Close();
        Dest.Close();	

        if (!bTestCRC || crc.GetResult()==nCopyCRC)
        {
          crcOK = true;
          break;
        }
        else
        {
          EnterLock(); 
          m_nCurrCopySize -= nLen;
          iFileSize += nLen;
          LeaveLock(); 
        }
      }
      if (!crcOK)
      {
        EnterLock(); 
        SetupFormatString1(m_sCopyError,SetupLocalizeString (_T("SETUP_STR_ERR_FILE_CRC")),sDestFile);
        m_canIgnoreError = true;
        LeaveLock(); 
        AfxThrowUserException();
      }
    }
    CATCH_ALL(e)
    {
      Source.Close();
      Dest.Abort();	
      // chyba
    }
    END_CATCH_ALL
  }
  else //file is saved in another location: setup.exe resources XOR setupNN.bin
  {
    bool crcOK = false;
    CRCCalculator crc;
    EnterLock(); 
    CString resName = m_sCopySourceRaw; resName.MakeUpper();
    LeaveLock(); 
    Ref<IFileBuffer> fbuf; //it is here only not to loss data before their usage
    QIStream *in = NULL;
    QFBank bank;
    bool bankUsed = false;
    if (whereSaved==0) //take it from setup.exe resources
    {
#define MULTI_SETUP_RESOURCES 0
#if MULTI_SETUP_RESOURCES
      in = new QIMultiResStream(CString2RString(resName));
#else
      int filesize;
      void *filedata = FindAndLoadResource(filesize, this, CString2RString(resName));
      if (filedata) in = new QIStrStream(filedata,filesize);
#endif
    }
    else //take it from bank setupNN.bin
    {
      CString setupBinName;
      bankPos.Get(whereSaved);
      if (!bankPos.size) //no BANK info at the end of setup.exe file found, so setupNN.bin files should exist
      {
        setupBinName.Format(_T("%02d.bin"), whereSaved);

        EnterLock();
        setupBinName = CString(exeNameNoExt) + setupBinName;
        LeaveLock();
      }
      else
      {
        TCHAR exeFile[1024]; 
        EnterLock();
        if (m_patchingSetup.IsEmpty()) GetModuleFileName(0,exeFile,1024);
        else _tcscpy(exeFile, m_patchingSetup);
        LeaveLock();
        setupBinName = exeFile;
      }
      if (bank.openFromFile(CString2RString(setupBinName),NULL,NULL,bankPos.start,bankPos.size))
      {
        bankUsed = true;
        bank.Load();
        const FileInfoO &fi = bank.FindFileInfo(CString2RString(resName));
        QIFStreamB *inB = new QIFStreamB;
        inB->open(bank,fi.name);
        in = inB;
      }
    }
    DWORD nLen = 0;
    if (in) //data are prepared in stream
    {
      RString resNameR = CString2RString(resName);
      const char *ext = GetFileExt(resNameR); //resNameR must survive longer! One cannot write GetFileExt(CString2RString(resName))
      QIStream *inStr = in;
      FILE *out = _tfopen(sDestFile ,_T("wb"));
      if (!out)
      {
        EnterLock(); 
        SetupFormatString1(m_sCopyError,SetupLocalizeString (_T("SETUP_STR_ERR_CREATE_FILE")),sDestFile);
        m_canIgnoreError = true;
        LeaveLock(); 
      }
      else
      {
        if (ext && stricmp(ext, ".gz")==0)
        { //we should use GZ stream
          inStr = new QIGzStream(in);
        }
        else if (ext && stricmp(ext, ".xz")==0)
        { //we should use GZ stream
          inStr = new QILzmaStream(in);
        }
        //read and write data
        crc.Reset();
        int   nBytes;
        char buff[COPY_BUFF_SIZE];
        bool  copyFinished = false, bEnd = false;
        while (!bEnd && !copyFinished)
        {
          nBytes = inStr->read(buff,COPY_BUFF_SIZE);
          if (nBytes>0)
          {
            crc.Add(buff,nBytes);
            nLen += nBytes;
            if (fwrite(buff, sizeof(char), nBytes, out) != nBytes)
            { //error
              EnterLock(); 
              SetupFormatString1(m_sCopyError,SetupLocalizeString (_T("SETUP_STR_ERR_FILE_WRITE")),sDestFile);
              m_canIgnoreError = true;
              LeaveLock(); 
              copyFinished = true;
            }
          }
          else copyFinished = true;
          // zaznamenám zkopírované byty
          EnterLock(); 
          m_nCurrCopySize += nBytes;
          iFileSize += nBytes;
          if (m_bCancelCopy) bEnd = TRUE;
          LeaveLock(); 
        };
        //read and write data FINISHED
        fclose(out);
      }
      if (bankUsed) bank.close();
      delete inStr;
    }
    else
    {
      EnterLock(); 
      SetupFormatString1(m_sCopyError,SetupLocalizeString (_T("SETUP_STR_ERR_FILE_READ")),sSrcFile);
      LeaveLock();
    }
    if (!bTestCRC || crc.GetResult()==nCopyCRC)
    {
      crcOK = true;
    }
    else
    {
      EnterLock(); 
      m_nCurrCopySize -= nLen;
      iFileSize += nLen;
      LeaveLock(); 
    }
    if (!crcOK)
    {
      EnterLock(); 
      SetupFormatString1(m_sCopyError,SetupLocalizeString (_T("SETUP_STR_ERR_FILE_CRC")),sDestFile);
      m_canIgnoreError = true;
      LeaveLock(); 
    }
    //THE END IS THE SAME AS FOR normal files copy
  }
	// ukonceno kopírování

#if _ENABLE_PBO_PROTECTION
  EnterLock();
  if (!m_bCancelCopy && m_sCopyError.IsEmpty())
  {
    // protect PBO header
    CString protError;
    if (!ProtectPBOHeaders(CString2RString(sDestFile), protError))
    {
      // TODO LOCALIZATION
      m_sCopyError.Format(_T("Protection of headers of file \"%s\" failed - error \"%s\""), sDestFile, protError);
      //SetupFormatString2(m_sCopyError,SetupLocalizeString (_T("SETUP_STR_PBO_PROT_FAILED")),sDestFile, protError);
    }

    AddCheckFileListItem(sDestFile, iFileSize);
  }
LeaveLock();
#endif //#if _ENABLE_PBO_PROTECTION  
	EnterLock(); 
	m_bRunningCopy = FALSE;
	LeaveLock(); 
	return 0;
}

void CSetupObject::SetupFormatString1 (CString &rString, LPCTSTR fString, LPCTSTR lpsz1)
{
  CString format(fString);
	SetupFormatString1 (rString, format, lpsz1);
}

void CSetupObject::SetupFormatString1 (CString &rString, CString &fString, LPCTSTR lpsz1)
{
	rString = "";
	int a = 0;
	int b = fString.Find (_T ("%1"));
	while (b >= 0)
	{
		rString += fString.Mid (a, b - a);
		rString += lpsz1;
		a = b + 2;
		b = fString.Find (_T ("%1"), a);
	}
	rString += fString.Mid (a);
}

void CSetupObject::SetupFormatString2 (CString &rString, LPCTSTR fString, LPCTSTR lpsz1, LPCTSTR lpsz2)
{
  CString format(fString);
	SetupFormatString2 (rString, format, lpsz1, lpsz2);
}

void CSetupObject::SetupFormatString2 (CString &rString, CString &fString, LPCTSTR lpsz1, LPCTSTR lpsz2)
{
	rString = "";
	int a = 0;
	int b = fString.Find (_T ('%'));
	int max = fString.GetLength () - 1;
	while (b >= 0 && b < max)
	{
		switch (fString[b + 1])
		{
		case _T ('1'):
			rString += fString.Mid (a, b - a);
			rString += lpsz1;
			a = b + 2;
			b = fString.Find (_T ('%'), a);
			break;
		
		case _T ('2'):
			rString += fString.Mid (a, b - a);
			rString += lpsz2;
			a = b + 2;
			b = fString.Find (_T ('%'), a);
			break;

		default:
			b = fString.Find (_T ('%'), b + 1);
			break;
		}
	}
	rString += fString.Mid (a);
}

CString CSetupObject::SetupLocalizeString (LPCTSTR str)
{
	RString d;
  RString s = LocalizeString (LPCTSTR2RString(str));
  if (s.IsEmpty()) 
    return CString(str) + CString(_T(" ... not localized!"));

	int a = 0;
	int f = s.Find (_T ('\\'));
	int max = s.GetLength () - 1;
	while (f >= 0 && f < max)
	{
		switch (s[f + 1])
		{
		case ('\\'):
			d = d + s.Mid (a, f - a);
			d = d + "\\";
			a = f + 2;
			f = s.Find ('\\', a);
			break;

		case ('t'):
			d = d + s.Mid (a, f - a);
			d = d + "\t";
			a = f + 2;
			f = s.Find ('\\', a);
			break;

		case ('n'):
			d = d + s.Mid (a, f - a);
			d = d + "\n";
			a = f + 2;
			f = s.Find ('\\', a);
			break;

		case ('r'):
			d = d + s.Mid (a, f - a);
			d = d + "\r";
			a = f + 2;
			f = s.Find ('\\', a);
			break;

		case ('\"'):
			d = d + s.Mid (a, f - a);
			d = d + "\"";
			a = f + 2;
			f = s.Find ('\\', a);
			break;

		case ('\''):
			d = d + s.Mid (a, f - a);
			d = d + "\'";
			a = f + 2;
			f = s.Find ('\\', a);
			break;

		default:
			f = s.Find ('\\', f + 1);
			break;
		}
	}
	d = d + s.Mid (a);
#if UNICODE
  Temp<wchar_t> bufW;
  {
    int wSize = MultiByteToWideChar(CP_UTF8, 0, d, -1, NULL, 0);
    bufW.Realloc(wSize);
    MultiByteToWideChar(CP_UTF8, 0, d, -1, bufW.Data(), wSize);
  }
	return CString(bufW);
#else
  return CString(d);
#endif
}

unsigned int __stdcall ThreadCopyFilesFunc(LPVOID pParam)
{
	CSetupObject* pSetupObj = (CSetupObject*)pParam;
	return pSetupObj->CopyFilesFunc();
};



/////////////////////////////////////////////////////////////////////////////
// komunikace s procesem

// probíhá kopírování
BOOL CSetupObject::IsRunningCopy()
{
	BOOL bResult = FALSE;
	EnterLock(); 
	bResult = m_bRunningCopy;
	LeaveLock(); 
	return bResult;
};
 
// chyba pri kopírování ?
BOOL CSetupObject::IsLastCopyErr(CString& sText)
{
	if (IsRunningCopy())
		return FALSE;
	// neprobíhá kopírování
	sText = m_sCopyError;
	return (sText.GetLength()>0);
};

// prerušení kopírování
void CSetupObject::CancelCopy()						
{
	if ( IsRunningCopy() )
	{

		EnterLock(); 
		m_bCancelCopy = TRUE;
		LeaveLock(); 

		::WaitForSingleObject(_copyThread,INFINITE);
	}
	if( _copyThread )
	{
		::CloseHandle(_copyThread);
		_copyThread=NULL;
	}
};

void CSetupObject::CancelRunApp()						
{
  EnterCriticalSection(&m_CSection_RUNAPP); 
	bool bResult = m_bRunningRUNAPP;
	LeaveCriticalSection(&m_CSection_RUNAPP); 

	if ( bResult )
	{
    EnterCriticalSection(&m_CSection_RUNAPP);
    m_bRunningRUNAPP = TRUE;
    LeaveCriticalSection(&m_CSection_RUNAPP);
		::WaitForSingleObject(_RunAppThread,INFINITE);
	}
	if( _RunAppThread )
	{
		::CloseHandle(_RunAppThread);
		_RunAppThread=NULL;
	}
};

BOOL CSetupObject::IsCdRomPresent (CCdRom *cd, CString &drive)
{
	// change CD-ROM
	CString drives;
	LPTSTR buf = drives.GetBuffer (1024);
	DWORD res = ::GetLogicalDriveStrings (1023, buf);
	drives.ReleaseBuffer ();
	if (res > 1023)
	{
		buf = drives.GetBuffer (res + 1);
		res = ::GetLogicalDriveStrings (res, buf);
		drives.ReleaseBuffer ();
	}
	if (res == 0)
	{
		// error
		return FALSE;
	}
	LPCTSTR ptr = (LPCTSTR) drives;
	for (int i = 0;;)
	{
		CString volumeName;
		DWORD serialNumber, maxFileNameLength, fileSystemOptions;
		::GetVolumeInformation (ptr + i, volumeName.GetBuffer (256), 255, 
			&serialNumber, &maxFileNameLength, &fileSystemOptions, NULL, 0);
		volumeName.ReleaseBuffer ();
		if (volumeName.CompareNoCase (cd->_label) == 0 || !m_testVolumeLabel)
		{
			CString tempDrive = ptr + i;
			
			// check the file!
			if (_taccess (tempDrive + cd->_file, 00) == 0)
			{
				drive = tempDrive;
				break;
			}
		}
		
		i += _tcslen (ptr + i) + 1;
		if (ptr[i] == _T ('\0'))
		{
			// error
			return FALSE;
		}
	}

	return TRUE;
}

void CSetupObject::DoRunCopyThread()
{
  MakePath(_T(""), m_sCopyDestination,TRUE);
  // run the process
  m_bRunningCopy	   = TRUE;
  unsigned ThreadID;

  //CreateThread(NULL,0,ThreadCopyFilesFunc,this,0,&ThreadID);
  _copyThread=(HANDLE)_beginthreadex(NULL,0,ThreadCopyFilesFunc,this,0,&ThreadID);
  if(!_copyThread)
  {
    SetupMessageBox(_T("Cannot create Thread."),MB_OK|MB_ICONSTOP);
  }
}

// kopírování dvou souborù				
BOOL CSetupObject::DoRunCopy(LPCTSTR pDest,LPCTSTR pSrc, CSetupFileItem *pItem)
{
	if (IsRunningCopy())
	{
		ASSERT(FALSE);
		return FALSE;	// probíhá kopírování !!!
	};
	// inicializuji proces
	m_sCopyDestination = pDest;
	if (m_currentCdRom == NULL)
	{
		m_sCopySource	   = m_sSetupSourceDir + pSrc;
	}
	else
	{
		m_sCopySource	   = m_currentDrive + m_currentCdRom->_dir + pSrc;
	}

  m_bTestCRC = FindCRC(pSrc,m_nCopyCRC);
	m_bCancelCopy	   = FALSE;
	m_bRunningCopy	   = FALSE;
	m_sCopyError.Empty();
  m_canIgnoreError = false;
  m_cWhereSaved = pItem->whereSaved;
  m_nCopySavedSize = pItem->savedFileSize;
  m_sCopySourceRaw = pSrc;
	// check the source file
  if (pItem->whereSaved==-1)
  { //the last chance is temporary directory
    CString tempFilePath = GetTempDirectory(m_sApplicationName) + CString(_T("\\")) + CString(pSrc);
    if (!VerifyFileRead(tempFilePath,FALSE))
    {
      //test whether it is saved in setup directory
      if (!VerifyFileRead(m_sCopySource,TRUE))
        return FALSE;
    }
    else m_sCopySource = tempFilePath;
  }
	// create the path
  if (pItem->m_arLogFileName.GetSize()>0)
  {
	  for (int i = 0; i < pItem->m_arLogFileName.GetSize (); ++i)
	  {
		  if (!MakePath(pItem->m_arLogFileName[i], pDest,TRUE))
		  {
			  CString sText;
			  SetupFormatString1(sText,SetupLocalizeString (_T("SETUP_STR_UNABLE_CREATE_PATH")),pDest);
			  SetupMessageBox(sText,MB_OK|MB_ICONSTOP);
			  return FALSE;
		  }
	  }
  }
  else
  {
    if (!MakePath(_T(""), pDest,TRUE)) //path is created, but no logFile involved
    {
      CString sText;
      SetupFormatString1(sText,SetupLocalizeString (_T("SETUP_STR_UNABLE_CREATE_PATH")),pDest);
      SetupMessageBox(sText,MB_OK|MB_ICONSTOP);
      return FALSE;
    }
  }
	
  // run the process
  DoRunCopyThread();


	// retìzec pro SETUP.LOG
	for (int i = 0; i < pItem->m_arLogFileName.GetSize (); ++i)
	{
    if (pItem->m_sCondition.IsEmpty())
		AddSetupLogLine(pItem->m_arLogFileName[i], 'F', m_sCopyDestination);
    else
      AddSetupLogLine(pItem->m_arLogFileName[i], 'E', pItem->m_sCondition+":"+m_sCopyDestination);
	}
	return TRUE;
};

