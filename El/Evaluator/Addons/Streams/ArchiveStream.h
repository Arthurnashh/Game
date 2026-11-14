// ArchiveStream.h: interface for the ArchiveStream class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ARCHIVESTREAM_H__400BBF3C_DD3E_4B56_B94B_92FA91E0D0AB__INCLUDED_)
#define AFX_ARCHIVESTREAM_H__400BBF3C_DD3E_4B56_B94B_92FA91E0D0AB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


/// Base class for envelops an anonymous stream

#define ASTRERR_GENERALERROR -1 //obecna chyba streamu
#define ASTRERR_UNSUPPORTED -2  //pokus o provedeni nepodporovane operace
#define ASTRERR_UNCOMPLETEDATA -3 //data byla prectena/zapsana nekompletni diky tomu, ze stream narazil na konec.
#define ASTRERR_EOF -4 //byl nalezen konec streamu




class ArchiveStream  
{ 
  int _uncomplette;
protected:
  void ReportUncomplette(int uncomplette) 
    {
    _uncomplette=uncomplette;
    SetError(ASTRERR_UNCOMPLETEDATA);
    }
public:
  enum SeekOp {begin,current,end};

  ///Execute general data exchange between stream and memory. 
  /** The direction designates implementation of this function
  @param buffer buffer that reserved for data (or currently data contains)
  @param maxsize size of buffer in bytes<p>
  @return last error code
  if error occcured, function freezes stream, and error can be readed using IsError function
  */
  
  virtual int DataExchange(void *buffer, int maxsize)=0;

  ///Returns last error code. 
  /** If error occured, stream is freezed until error is cleared.
  @return an error code depend on stream implementation
  */
  virtual int IsError()=0;

  virtual void SetError(int err)=0;

  ///Resets last error, unfreezes stream
  virtual void Reset()=0;

  ///Returns position in stream. returns -1, if function is not supported
  virtual __int64 Tell()=0;

  ///Move current pointer.
  /**
  @param lOff Number of bytes to move the pointer.
  @param seekop ArchiveStream::begin  Move the file pointer lOff bytes forward from the beginning of the file.<br>
                ArchiveStream::current Move the file pointer lOff bytes from the current position in the file.<br>
                ArchiveStream::end Note that lOff must be negative to seek into the existing file; positive values will seek past the end of the file.<p>
  if error occcured, function freezes stream, and error can be readed using IsError function
  */
  virtual void Seek(__int64 lOff, SeekOp seekop)=0;

  ///Returns state if archive is storing
  /**return true archive is storing data, otherwise return false
  */
  virtual bool IsStoring()=0;
  ///Returns state if archive is reading
  /**return true archive is storing data, otherwise return false
  */  
  bool IsReading() {return !IsStoring();}

  ///Returns state if archive only counting data
  virtual bool IsCounting() {return false;}

  ///Reads or writes reserved bytes. 
  /**While reading, skips specified bytes. While writting writes specified zeroed bytes 
  @param bytes count of bytes to write / skip
  */

  virtual void Reserved(int bytes);


  ///Flushes output stream
  /** This method flushes output stream. It has no effect on input stream. Function wait until
  all buffered I/O is completted and then returns. If an error occured, Error flag is set, so use
  IsError() for test result. 

  NOTE: Implementation is optional.
  */
  
  virtual void Flush() {}

  virtual ~ArchiveStream()=0 {}

  int ExSimple(bool &p) {return DataExchange(&p,sizeof(p));}
  int ExSimple(int &p) {return DataExchange(&p,sizeof(p));}
  int ExSimple(unsigned int &p) {return DataExchange(&p,sizeof(p));}
  int ExSimple(char &p) {return DataExchange(&p,sizeof(p));}
  int ExSimple(unsigned char &p) {return DataExchange(&p,sizeof(p));}
  int ExSimple(short &p) {return DataExchange(&p,sizeof(p));}
  int ExSimple(unsigned short &p) {return DataExchange(&p,sizeof(p));}
  int ExSimple(long &p) {return DataExchange(&p,sizeof(p));}
  int ExSimple(unsigned long &p) {return DataExchange(&p,sizeof(p));}
  
};

#endif // !defined(AFX_ARCHIVESTREAM_H__400BBF3C_DD3E_4B56_B94B_92FA91E0D0AB__INCLUDED_)
