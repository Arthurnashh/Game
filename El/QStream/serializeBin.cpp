#include <El/elementpch.hpp>

#include "serializeBin.hpp"
#include <Es/Containers/staticArray.hpp>
#include <El/Math/math3dP.hpp>

// optimized "native" file loading / saving

SerializeBinStream::SerializeBinStream( QIStream *in )
{
	_in=in,_out=NULL;
	_error = EOK;
	_version = -1;
	_context = NULL;
#ifdef _M_PPC
  // Version is needed to recognize endian
  _endianState = EndianUnknown;
#endif
  _lzoCompression = false;
}
SerializeBinStream::SerializeBinStream( QOStream *out )
{
	_out=out,_in=NULL;
	_error = EOK;
	_version = -1;
	_context = NULL;
	#ifdef _M_PPC
	// we are never saving as little endian
	_endianState = EndianBig;
	#endif
  _lzoCompression = false;
}

int SerializeBinStream::TellG()
{
	if (_in) return _in->tellg();
	Fail("File is not readable");
	return 0;
}

void SerializeBinStream::PreReadSequential(int size)
{
  if (_in) _in->PreReadSequential(size);
}

int SerializeBinStream::TellP()
{
	if (_out) return _out->tellp();
	Fail("File is not writeable");
	return 0;
}

/*!
	Only value retrieved by TellG should be passed to SeekG.
*/
void SerializeBinStream::SeekG(int offset)
{
	if (_in) _in->seekg(offset,QIOS::beg);
	else
	{
		Fail("Cannot SeekG on output stream");
	}
}

//! set current reading position

void SerializeBinStream::SeekP(int offset)
{
	if (_out) _out->seekp(offset,QIOS::beg);
	else
	{
		Fail("Cannot SeekP on input stream");
	}
}

int SerializeBinStream::CreateFixUp()
{
  Assert(_out);
	int offset = TellP();
	SaveInt(-1);
	return offset;
}

void SerializeBinStream::FixUp(int offset, int value)
{
	int actPos = TellP();
	SeekP(offset);
	SaveInt(value);
	SeekP(actPos);
}

void SerializeBinStream::FixUp(int offset)
{
	int actPos = TellP();
	SeekP(offset);
	SaveInt(actPos);
	SeekP(actPos);
}




bool SerializeBinStream::Version(int verMin, int verMax)
{
	if (_in)
	{
	  int version;
    TransferBinary(version);
#ifdef _M_PPC
    // opportunity to check endian
    // first version should be a magic
    // we should avoid a magic which is symmetrical
    if (_endianState==EndianUnknown)
    {
      // magic should be one value
      Assert(verMin==verMax);
      int versionLE = version;
      // TODO: optimize using direct calculation
      SwapEndian32b(&versionLE,sizeof(versionLE));
      if (versionLE>=verMin && versionLE<=verMax)
      {
        _endianState = EndianLittle;
        version = versionLE;
      }
      else if (version>=verMin && version<=verMax)
      {
        _endianState = EndianBig;
      }
      else
      {
        LogF("Unrecognized endian: file %x(%x), expected %x..%x",version,versionLE,verMin,verMax);
        // if unsure we assume little endian
        version = versionLE;
      }
      
      //
      //TransferBinary();
    }
    else if (_endianState==EndianLittle)
    {
      SwapEndian32b(&version,sizeof(version));
    }
#endif
		_version = version;
		return _version>=verMin && _version<=verMax;
	}
	else
	{
    TransferBinary(verMax);
    _version = verMax;
    return true;
	}
}

#define MIN_COMPRESS_SIZE 1024
#define LZO_MIN_COMPRESS_SIZE 1024

#include <El/LZO/lzo/lzo1x.h>
#include <El/LZO/lzo/lzopro/lzo1x.h>
#include <El/LZO/lzo/lzoconf.h>



void SerializeBinStream::SaveCompressed(const void *data, int size)
{
  if (_lzoCompression)
  {
    if (size >= LZO_MIN_COMPRESS_SIZE)
    {
      // initialize the library
      int ok = lzo_init();
      if (ok != LZO_E_OK)
      {
        _error = EFileStructure;
        return;
      }

      // prepare the buffer for compressed content
      lzo_uint compressedSize = size + size / 16 + 64 + 3;
      Buffer<char> compressedBuffer;
      compressedBuffer.Init(compressedSize);
      Buffer<char> workingBuffer;

      workingBuffer.Init(LZOPRO_LZO1X_1_06_MEM_COMPRESS);

      // compression
      ok = lzopro_lzo1x_1_06_compress((lzo_bytep)data, size, // source
        (lzo_bytep)compressedBuffer.Data(), &compressedSize,  // destination
        workingBuffer.Data());

      if (ok != LZO_E_OK)
      {
        _error = EFileStructure;
        return;
      }

      // save the compressed data
      Save(compressedBuffer.Data(), compressedSize);
      return;
    }
  }
  else
  {
	  if (size>=MIN_COMPRESS_SIZE)
	  {
		  SSCompress compress;
		  compress.Encode(*_out,(const char *)data,size);
		  return;
	  }
  }
  // fall-back
	Save(data,size);
}

void SerializeBinStream::Append(const QOStrStream &out, bool compress)
{
	if (compress)
	{
    SaveCompressed(out.str(), out.pcount());
	}
  else
  {
	  Save(out.str(),out.pcount());
  }
}

void SerializeBinStream::LoadCompressed(void *data, int size)
{
  if (_lzoCompression)
  {
    if (size >= LZO_MIN_COMPRESS_SIZE)
    {
      // initialize the library
      int ok = lzo_init();
      if (ok != LZO_E_OK)
      {
        _error = EFileStructure;
        return;
      }

      // decompression
      ok = lzopro_lzo1x_decompress_from_stream(*_in,
        (lzo_bytep)data, (lzo_uint *)&size, NULL);

      if (ok != LZO_E_OK)
      {
        _error = EFileStructure;
        return;
      }

      return;
    }
  }
  else
  {
	  if (size>=MIN_COMPRESS_SIZE)
	  {
		  SSCompress compress;
		  if (!compress.Decode((char *)data,size,*_in))
		  {
			  RptF("Error in SerializeBinStream decoding");
			  _error = EFileStructure;
		  }
		  return;
	  }
  }
	Load(data,size);
}

void SerializeBinStream::SkipBinaryCompressed(int size, bool enableCompression)
{
  Assert(_in);

	if (enableCompression)
  {
    if (_lzoCompression)
    {
      if (size >= LZO_MIN_COMPRESS_SIZE)
      {
        // TODO: if needed, replace by a more efficient implementation
        Buffer<char> dummy;
        dummy.Init(size);
        LoadCompressed(dummy.Data(), size);
        return;
      }
    }
    else
    {
      if (size>=MIN_COMPRESS_SIZE)
	    {
		    SSCompress compress;
		    if (!compress.Skip(size,*_in))
		    {
			    RptF("Error in SerializeBinStream decoding");
			    _error = EFileStructure;
		    }
		    return;
	    }
    }
  }
  _in->seekg(size,QIOS::cur);
}

void SerializeBinStream::SkipBinary(int size)
{
  Assert(_in);
  _in->seekg(size,QIOS::cur);
}

void SerializeBinStream::TransferBinaryCompressed(void *data, int size, bool enableCompression)
{
	// apply LZW compression - data will be repeated
	// no additional fields required - size is known
	if (_in)
	{
	  if (enableCompression)
	  {
		  LoadCompressed(data,size);
	  }
	  else
	  {
	    Load(data,size);
	  }
	}
	else
	{
	  if (enableCompression)
	  {
		  SaveCompressed(data,size);
	  }
	  else
	  {
	    Save(data,size);
	  }
	}
}

#ifdef _M_PPC

// SwapEndian process huge amount of memory - we want them to be fast
// they are already quite stable and do not need to be debugged any more
#pragma optimize("tg",on)

void SerializeBinStream::SwapEndian32b(void *data, size_t size)
{
  Assert((size&3)==0);
  unsigned *data32b = (unsigned *)data;
  int count = size>>2;
  while (--count>=0)
  {
    *data32b = __loadwordbytereverse(0,data32b);
    //unsigned d = *data32b;a
    //*data32b = ((d&0xff)<<24)|((d&0xff00)<<8)|((d&0xff0000)>>8)|(d>>24);
    data32b++;
  }
}
void SerializeBinStream::SwapEndian16b(void *data, size_t size)
{
  Assert((size&1)==0);
  unsigned short *data16b = (unsigned short*)data;
  int count = size>>1;
  while (--count>=0)
  {
    *data16b = __loadshortbytereverse(0,data16b);
    //unsigned short d = *data16b;
    //*data16b = ((d&0xff)<<8)|((d&0xff00)>>8);
    data16b++;
  }
}
/**
@param wordSize size of the word in bytes
*/
void SerializeBinStream::SwapEndian(void *data, size_t size, size_t wordSize)
{
  Assert((size%wordSize)==0);
  unsigned char *data8b = (unsigned char*)data;
  
  AutoArray<unsigned char,MemAllocLocal<unsigned char,16> > temp;
  temp.Resize(wordSize);
  
  while (size>0)
  {
    memcpy(temp.Data(),data8b,wordSize);
    // copy reversed
    for (unsigned i=0; i<wordSize; i++)
    {
      data8b[i] = temp[wordSize-1-i];
    }
    data8b += wordSize;
    size -= wordSize;
  }

}

#pragma optimize("",on)

#endif

void SerializeBinStream::operator << ( RString &data )
{
	if (IsLoading())
	{
    CharArray< MemAllocLocal<char,4096> > buf;
		for(;;)
		{
			char c = _in->get();
			if (!c || c==EOF) break;
      buf.Add(c);
		}
    buf.Add(0);
    data = buf;
	}
	else
	{
		// transfer zero terminated string
		int len = strlen(data);
		_out->write(data,len+1);
	}
}

void SerializeBinStream::operator << ( RStringB &data )
{
	if (IsLoading())
	{
    CharArray< MemAllocLocal<char,4096> > buf;
		for(;;)
		{
			int c = _in->get();
			if (!c) break;
			if (c==EOF)
			{
				Fail("EOF in string");
				SetError(EFileStructure);
				break;
			}
      buf.Add(c);
		}
		buf.Add(0);
		data = buf;
	}
	else
	{
		// transfer zero terminated string
		const char *strData = data;
		int len = strlen(strData);
		_out->write(strData,len+1);
	}
}

void SerializeBinStream::LoadVector3( Vector3 & v )
{
  for (size_t i = 0; i < 3; ++i)
    v[i] = LoadFloat();
}

void SerializeBinStream::SaveVector3( const Vector3 & v )
{
  for (size_t i = 0; i < 3; ++i)
    SaveFloat(v[i]);
}
/*
void SerializeBinStream::operator << ( RStringIB &data )
{
	if (IsLoading())
	{
		char buf[4096];
		int n=0;
		for(;;)
		{
			char c = _in->get();
			if (!c) break;
			if (n<sizeof(buf)-1)
			{
				buf[n++]=c;
			}
		}
		buf[n]=0;
		data = buf;
	}
	else
	{
		// transfer zero terminated string
		int len = strlen(data);
		_out->write(data,len+1);
	}
}
*/

