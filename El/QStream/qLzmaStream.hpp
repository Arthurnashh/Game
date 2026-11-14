#ifndef __Q_LZMA_STREAM__
#define __Q_LZMA_STREAM__

#include <lzma/lzma.h>
#include <El/QStream/qStream.hpp>

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// QILzmaStream should be used as filter which reads from QIStream and decompress it (filter)
// It depends on extern liblzma.lib library (or its DLL equivalent) and lzma headers in extern/lzma/lzma.h
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Typical usage:
//   // When used with static library, #define LZMA_API_STATIC before lzma header include
//   #define LZMA_API_STATIC
//   #include <El/QStream/qLzmaStream.hpp>
//   // and add liblzma.lib into linkage (path such as: extern/liblzma.lib)
//   {
//     QIFStream *in = new QIFStream();  // NOTE: it will be destroyed inside QILzmaStream destructor
//     in->open("bebulek.xz");
//     QILzmaStream inLzmaStr(in);       // QILzmaStream filters input from the given QIStream
//     char buff[1025];
//     int nBytes = 0;
//     int newBytes = 0;
//     do 
//     {
//       newBytes = inLzmaStr.read(buff,1024);
//       buff[newBytes] = 0;
//       nBytes += newBytes;
//       printf("%s", buff);             // process decoded data
//     } while (newBytes>0);
//     printf("LZMA Read %d Bytes\n", nBytes);
//     in->close();
//     // DO NOT CALL delete(in), as it is done inside the destructor of QILzmaStream
//   }
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
class QILzmaStream : public QIStream
{
private:
  QIStream *inStr;
  static const int LzmaBufSize = 4096;
  static const lzma_stream lzma_stream_init;

public:
  QILzmaStream(QIStream *in) : inStr(in) { Init(); }
  virtual ~QILzmaStream() 
  {
    if (inStr) delete inStr; 
    Destroy();
    _buf.Free();
  }
  virtual PosQIStreamBuffer ReadBuffer( PosQIStreamBuffer &buf, QFileSize pos )
  {
    buf.Free();
    char data[65536];
    int size = LzmaRead(data, 65536);
    if (size<0) size = 0;
    QIStreamBuffer *nBuf = new QIStreamBuffer(size);
    nBuf->FillWithData(data,size);
    return PosQIStreamBuffer(nBuf,pos,size);
  }
  //other pure virtual functions
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
    return 0; //we do not know the proper data size (as it will be uncompressed)
  }
  virtual QILzmaStream* Clone()
  {
    Fail("Not tested!");
    QILzmaStream* stream = new QILzmaStream(inStr? inStr->Clone() : 0);

    return stream;
  }

private:
  lzma_stream _lstr;
  int    _lzmaError;  // should be LZMA_OK after Initialization
  bool   _eof;
  uint8_t *_inbuf;

  void Init()
  {
    _lstr = lzma_stream_init;
    _lzmaError = lzma_auto_decoder(&_lstr, -1, 0);
    _eof = false;
    _lstr.next_in = _inbuf = new uint8_t[LzmaBufSize];
  }

  int inread(uint8_t *buf,int size)
  {
    return inStr->read((char *)buf, size);
  }

  int LzmaRead(char* buf,int len)
  {
    _lstr.avail_out = len;
    _lstr.next_out = (uint8_t *)buf;
    while (_lstr.avail_out != 0) 
    {
      if (_lstr.avail_in == 0 && !_eof)
      {
        _lstr.avail_in = inread(_inbuf,LzmaBufSize);
        if (_lstr.avail_in == 0)
        {
          _eof = true;
        }
        _lstr.next_in = _inbuf;
      }

      int ret = lzma_code(&_lstr, LZMA_RUN);
      if(ret < 0) {	/* decompression error */
        return -1;
      }

      if (ret != LZMA_OK || _eof) break;
    }
    return (int)(len - _lstr.avail_out);
  }

  int Destroy()
  {
    lzma_end(&_lstr);
    delete _inbuf;
    return _lzmaError;
  }
};

#endif

