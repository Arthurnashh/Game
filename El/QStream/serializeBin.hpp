#ifdef _MSC_VER
#pragma once
#endif

#ifndef _SERIALIZE_BIN_HPP
#define _SERIALIZE_BIN_HPP

#include <Es/Strings/rString.hpp>
#include <Es/Common/global.hpp>
#include "qStream.hpp"

class Vector3P;
typedef Vector3P Vector3;

/// traits for endian related transformations
template <class Type>
struct EndianTraits
{
  /// BlockSize = 1 means no conversion is needed
  static const int BlockSize=1;
};

/// implementation helpers for EndianTraits
template <class Type>
struct EndianTraitsSwap
{
  /// block size is the type size
  static const int BlockSize=sizeof(Type);
};

//@{ basic types ... swap whole item
template <> struct EndianTraits<int>: public EndianTraitsSwap<int> {};
template <> struct EndianTraits<unsigned int>: public EndianTraitsSwap<int> {};
template <> struct EndianTraits<long>: public EndianTraitsSwap<long> {};
template <> struct EndianTraits<unsigned long>: public EndianTraitsSwap<unsigned long> {};
template <> struct EndianTraits<short>: public EndianTraitsSwap<short> {};
template <> struct EndianTraits<unsigned short>: public EndianTraitsSwap<unsigned short> {};
template <> struct EndianTraits<float>: public EndianTraitsSwap<float> {};
//@}

enum SerializeStyle
{
  /// native binary serialization possible
  SerializeStyleNative,
  /// in-place loading with decompression is possible (assumes file size not bigger than memory size)
  SerializeStyleLoadInPlace,
  /// full copy needed for both loading and saving
  SerializeStyleCopy
};

//! stream used to fast "binary" serialization

class SerializeBinStream
{
public:
  enum ErrorCode
  {
    EOK,
    EBadFileType,
    EBadSignature,
    EBadVersion,
    EFileStructure,
    ENoFile,
    EGeneric
  };
  typedef QFileEndian Endian;

private:
  QIStream *_in;
  QOStream *_out;
  ErrorCode _error;
  void *_context;
  int _version;
#ifdef _M_PPC
  /// some file as big endian, some little endian
  /** Files saved from the X360 are big endian. We determine endian state on first Version load */
  Endian _endianState;
#endif
  /// when true, LZO compression is used in LoadCompressed, SaveCompressed etc., otherwise LZN (SSCompress) is used
  bool _lzoCompression;

public:
#ifdef _M_PPC
  // when file is little endian, we need to apply swap
  /** We assert on unknown endian, and we assume little endian to maintain best compatibility with PC files */
  bool EndianSwapNeeded() const {Assert(_endianState!=EndianUnknown);return _endianState!=EndianBig;}
  void SetEndianState(Endian endian){_endianState=endian;}
  Endian GetEndianState()const {return _endianState;}
  /// on big endian platform we load little endian only 
  bool LoadLittleEndian() const {return EndianSwapNeeded();}
#else
  // endian swap never needed on PC
  bool EndianSwapNeeded() const {return false;}
  // used for assertions - on PC we always load little endian
  bool LoadLittleEndian() const {return true;}
#endif
  //! create input stream
  SerializeBinStream( QIStream *in );
  //! create output stream
  SerializeBinStream( QOStream *out );
  bool IsUsingLZOCompression() const {return _lzoCompression;}
  /// switch on using of LZO compression
  void UseLZOCompression() {_lzoCompression = true;}

  bool CheckIntegrity() const
  {
    return _in || _out;
  }
  //! check if stream is output stream
  bool IsSaving() const
  {
    Assert(CheckIntegrity());
    return _out!=NULL;
  }
  //! check if stream is input stream
  bool IsLoading() const
  {
    Assert(CheckIntegrity());
    return _in!=NULL;
  }

  // direct access to streams - needed for signatures
  QIStream *GetLoadStream() {return _in;}
  QOStream *GetSaveStream() {return _out;}

  //! check if (and what) error was encountered
  ErrorCode GetError() const {return _error;}
  //! signal error
  void SetError( ErrorCode code ) {_error=code;}

  //! check context
  void *GetContext() const {return _context;}
  //! set context
  void SetContext( void *context ) {_context=context;}

  //! get current writing position
  int TellP();

  //! get current reading position
  int TellG();
  //! set current reading position
  void SeekG(int offset);

  //! set current writing position
  void SeekP(int offset);

  /// if loading, create background requests for the data needed
  void PreReadSequential(int size);

  //! make space for a future fix-up
  int CreateFixUp();

  //! replace a placeholder (fix-up) value with the actual value
  void FixUp(int offset, int value);

  //! replace a placeholder (fix-up) value with current stream position
  void FixUp(int offset);

  /// override version number - use with caution
  void SetVersion(int version) {_version = version;}
  // load/save helpers
  //! get version number
  int GetVersion() const {return _version;}
  //! load/save version number (range supported, max. version used when saving)
  //! GetVersion() can be used to check version after this call
  bool Version(int verMin, int verMax);
  //! load/save version number (only one version supported)
  //! GetVersion() can be used to check version after this call
  bool Version(int ver) {return Version(ver,ver);}
  //! load binary data region
  void Load(void *data, int size) {_in->read(data,size);}
//#if _HELISIM_LARGE_OBJECTID
  //! load 64-bit integer
  int64 LoadInt64()
  {
    if(EndianSwapNeeded()) return _in->getill();
    else return _in->readBinaryVal<int64>();
  }
//#endif // _HELISIM_LARGE_OBJECTID

  //! load 32-bit integer
  int LoadInt()
  {
    if (EndianSwapNeeded()) return _in->getil();
    else return _in->readBinaryVal<int>();
  }
  //! load 16-bit integer
  short LoadShort()
  {
    if (EndianSwapNeeded()) return _in->getiw();
    else return _in->readBinaryVal<short>();
  }
  //! load 8-bit integer
  __forceinline char LoadChar() {return _in->get();}
  //! load float
  float LoadFloat()
  {
    if (EndianSwapNeeded()) return _in->getf();
    else return _in->readBinaryVal<float>();
  }
  //! load double
  double LoadDouble()
  {
    if (EndianSwapNeeded()) return _in->getd();
    else return _in->readBinaryVal<double>();
  }
  //! load string
  RString LoadString()
  {
	  static char str[512];
	  int len = LoadShort();
	  _in->read(str,len);
	  str[len] = 0;
	  return RString(str,len);
  }
  //! load vector
  void LoadVector3( float* fX, float* fY, float* fZ )
  {
	*fX = LoadFloat();
	*fY = LoadFloat();
	*fZ = LoadFloat();
//	return Vector3P(x,y,z);
  }

  void LoadVector3( Vector3 & v );

  //! check how much data is left in the file during loading
  int GetRest() {return _in ? _in->rest() : 1;}

  //! save binary data region
  void Save(const void *data, int size) {_out->write(data,size);}
//#if _HELISIM_LARGE_OBJECTID
  //! save 64-bit integer
  void SaveInt64( int64 t) {_out->write(&t,sizeof(t));}
//#endif // _HELISIM_LARGE_OBJECTID
  //! save 32-bit integer
  void SaveInt( int t ) {_out->write(&t,sizeof(t));}
  //! save 16-bit integer
  void SaveShort( short t ) {_out->write(&t,sizeof(t));}
  //! save 8-bit integer
  void SaveChar( char t ) {_out->put(t);}
  //! save float
  void SaveFloat( float f ) {_out->write(&f,sizeof(float));}
  //! save double
  void SaveDouble( float d ) {_out->write(&d,sizeof(double));}
  //! save string
  void SaveString( const char* str )
  {
	  int len = strlen(str);
	  SaveShort(len);
	  _out->write(str,len);
  }
  //! save vector
  void SaveVector3( float fX, float fY, float fZ )
  {
	  SaveFloat(fX);
	  SaveFloat(fY);
	  SaveFloat(fZ);
  }
  //! save vector, for real
  void SaveVector3( const Vector3 & v );

  // generic helpers
  //! transfer (save or load) binary data region
  void TransferBinary(void *data, int size)
  {
    if (_in) _in->read(data,size);
    else _out->write(data,size);
  }

//#if _HELISIM_LARGE_OBJECTID
  void TransferLittleEndian64b(void *data)
  {
    // perform Little Endian conversion on load
    if (_in)
    {
      Assert(LoadLittleEndian());
      *(int64 *)data=_in->getill();
    }
    else
    {
      _out->write(data,sizeof(int64));
      Assert(!EndianSwapNeeded());
    }
  }
//#endif // _HELISIM_LARGE_OBJECTID

  void TransferLittleEndian32b(void *data)
  {
    // perform Little Endian conversion on load
    if (_in)
    {
      Assert(LoadLittleEndian());
      *(int *)data=_in->getil();
    }
    else
    {
      _out->write(data,sizeof(int));
      Assert(!EndianSwapNeeded());
    }
  }
  void TransferLittleEndian16b(void *data)
  {
    // perform Little Endian conversion on load
    if (_in)
    {
      Assert(LoadLittleEndian());
      *(short *)data=_in->getiw();
    }
    else
    {
      _out->write(data,sizeof(short));
      Assert(!EndianSwapNeeded());
    }
  }

  /** size assumed to be constant */
  __forceinline void TransferLittleEndian(void *data, size_t size)
  {
    if (size==1) return TransferBinary(data,1); // 1B has no endian difference
    if (EndianSwapNeeded())
    {
      if (size==2) return TransferLittleEndian16b(data);
      if (size==4) return TransferLittleEndian32b(data);
//#if _HELISIM_LARGE_OBJECTID
      if (size==8) return TransferLittleEndian64b(data);
//#endif

      Fail("Unsupported size for little endian transfer");
    }
    TransferBinary(data,size);
  }

  template <class Type>
  __forceinline void TransferLittleEndian(Type &data)
  {
    TransferLittleEndian(&data,sizeof(data));
  }

  template <class Type>
  void TransferBinary(Type &data)
  {
    TransferBinary(&data,sizeof(data));
  }
  //! transfer (save or load) binary data region, compress if necessary
  void TransferBinaryCompressed(void *data, int size, bool enableCompression=true);
  //! skip (load only) binary data region, compress if necessary
  void SkipBinaryCompressed(int size, bool enableCompression=true);
  //! skip (load only) binary data region
  void SkipBinary(int size);
  //! skip (load only) data item
  template <class Type>
  void Skip(Type &dummy)
  {
    // most portable way to skip any item is load it using Transfer, which calls corresponding operator <<
    Transfer(dummy);
  }

  //! save binary data region, compress if necessary
  void SaveCompressed(const void *data, int size);
  //! load binary data region, decompress if necessary
  void LoadCompressed(void *data, int size);

  //! append another stream at the end of this one
  void Append(const QOStrStream &out, bool compress=false);

  // transfer basic types
  void operator << ( int &data )
  {
    if (EndianSwapNeeded())
      TransferLittleEndian32b(&data);
    else
      TransferBinary(&data, sizeof(data));
  }
  void operator << ( float &data )
  {
    if (EndianSwapNeeded())
      TransferLittleEndian32b(&data);
    else
      TransferBinary(&data, sizeof(data));
  }
//#if _HELISIM_LARGE_OBJECTID
  void operator << ( int64 &data )
  {
    if (EndianSwapNeeded())
      TransferLittleEndian64b(&data);
    else
      TransferBinary(&data, sizeof(data));
  }
//#endif // _HELISIM_LARGE_OBJECTID

  void operator << ( bool &data ) {TransferBinary(&data,sizeof(data));}
  void operator << ( char &data ) {TransferBinary(&data,sizeof(data));}
  void operator << ( signed char &data ) {TransferBinary(&data,sizeof(data));}
  void operator << ( unsigned char &data ) {TransferBinary(&data,sizeof(data));}

  void operator << ( RString &data );
  void operator << ( RStringB &data );

  template <class Type>
  SerializeBinStream &operator << (AutoArray<Type> &data)
  {
    if (IsLoading())
    {
      int size = LoadInt();
      data.Realloc(size);
      data.Resize(size);
    }
    else
    {
      SaveInt(data.Size());
    }
    // transfer array data
    for (int i=0; i<data.Size(); i++)
    {
      Transfer(data[i]);
    }
    return *this;
  }
  //void operator << ( RStringIB &data );

  /*
  void Transfer( Vector3 &data );
  void Transfer( Matrix4 &data );
  void Transfer( Matrix3 &data );
  */

  template <class Type>
  void Transfer(Type &val)
  {
    (*this)<< val;
  }

  template <class ArrayType>
  void TransferBinaryCondensedArray(ArrayType &data, bool enableCompression);

  template <class ArrayType>
  void TransferBinaryArray( ArrayType &data, bool enableCompression=true );

  template <class ArrayType>
  void SkipBinaryCondensedArray(ArrayType &data, bool enableCompression);

  template <class ArrayType>
  void SkipBinaryArray( ArrayType &data, bool enableCompression=true );

  template <class ArrayType>
  void TransferBasicArray( ArrayType &data )
  {
    if (_in)
    {
      int size = LoadInt();
      data.Realloc(size);
      data.Resize(size);
      for (int i=0; i<data.Size(); i++)
      {
        Transfer(data.Set(i));
      }
    }
    else
    {
      SaveInt(data.Size());
      // transfer array data
      for (int i=0; i<data.Size(); i++)
      {
        typename ArrayType::DataType val = data.Get(i);
        Transfer(val);
      }
    }
  }

  template <class ArrayType>
  void SkipBasicArray( ArrayType &data )
  {
    if (_in)
    {
      int size = LoadInt();
      SkipBinary(size*sizeof(ArrayType::DataType));
      Assert(data.Size()==size);
      // we cannot skip here by seeking, because with basic array we do not know the size when stored
      for (int i=0; i<data.Size(); i++)
      {
        typename ArrayType::DataType val;
        Transfer(val);
      }
    }
    else
    {
      Fail("Skip canout be used when saving");
    }
  }

  template <class ArrayType>
  void TransferArray( ArrayType &data )
  {
    if (_in)
    {
      int size = LoadInt();
      data.Realloc(size);
      data.Resize(size);
    }
    else
    {
      SaveInt(data.Size());
    }
    // transfer array data
    for (int i=0; i<data.Size(); i++)
    {
      data[i].SerializeBin(*this);
    }
  }

  /// allow TransferRefArrayT
  template <class Type>
  struct TransferRefArrayTraits
  {
    static Type *CreateObject(SerializeBinStream &f) {return new Type;}
  };
  template <class Type,class ArrayType>
  void TransferRefArrayT(ArrayType &data, Type *type)
  {
    if (_in)
    {
      int size = LoadInt();
      data.Realloc(size);
      data.Resize(size);
      for (int i=0; i<data.Size(); i++)
      {
        data[i] = Type::CreateObject(*this);
        data[i]->SerializeBin(*this);
      }
    }
    else
    {
      SaveInt(data.Size());
      // transfer array data
      for (int i=0; i<data.Size(); i++)
      {
        data[i]->SerializeBin(*this);
      }
    }
  }
  template <class Type>
  void TransferRefArray( RefArray<Type> &data )
  {
    TransferRefArrayT(data,(Type *)NULL);
  }

  //@{ change memory endian-ness (used for Big Endian platform loading Little Endian data)
  static void SwapEndian32b(void *data, size_t size);
  static void SwapEndian16b(void *data, size_t size);
  static void SwapEndian(void *data, size_t size, size_t wordSize);

#ifdef _M_PPC
  static void ProcessLittleEndian32b(void *data, size_t size){SwapEndian32b(data,size);}
  static void ProcessLittleEndian16b(void *data, size_t size){SwapEndian16b(data,size);}
  static void ProcessLittleEndian(void *data, size_t size, size_t wordSize){SwapEndian(data,size,wordSize);}
  __forceinline static void ProcessLittleEndianData(void *data, size_t size, size_t itemSize)
  {
    if (itemSize==1) return;
    else if (itemSize==2) ProcessLittleEndian16b(data,size);
    else if (itemSize==4) ProcessLittleEndian32b(data,size);
    else ProcessLittleEndian(data,size,itemSize);
  }
  __forceinline void LoadLittleEndian(void *data, size_t count, size_t itemSize)
  {
    size_t size = count*itemSize;
    Load(data,size);
    if (EndianSwapNeeded())
    {
      ProcessLittleEndianData(data,size,itemSize);
    }
  }
#else
  __forceinline static void ProcessLittleEndian32b(void *data, size_t size){}
  __forceinline static void ProcessLittleEndian16b(void *data, size_t size){}
  __forceinline static void ProcessLittleEndian(void *data, size_t size, size_t wordSize){}
  __forceinline static void ProcessLittleEndianData(void *data, size_t size, size_t itemSize){}
  __forceinline void LoadLittleEndian(void *data, size_t count, size_t itemSize){Load(data,count*itemSize);}

#endif
  //@}
};

/// some types need to be serialized using a different type, e.g. Vector3K using Vector3P
template <class Type>
struct SerializeAs
{
  /* by default each type serializes as itself */
  typedef Type As;
  /// if Type == As, we may use simplified processing
  static SerializeStyle GetStyle() {return SerializeStyleNative;}

  /// in-place conversion, src == dst
  static void ConvertOnLoad(Type *dst, const As *src, int nElems, bool endianSwap)
  {
    if (endianSwap)
    {
      Assert(dst==src);
      SerializeBinStream::ProcessLittleEndianData(dst,nElems*sizeof(*src),EndianTraits<Type>::BlockSize);
    }
  }
};

template <class ArrayType>
void SerializeBinStream::TransferBinaryCondensedArray(ArrayType &data, bool enableCompression)
{
  typedef SerializeAs<typename ArrayType::DataType> DataSerializeAs;
  if (_in)
  {
    int size = LoadInt();
    char condensed = LoadChar();
    Assert(condensed>=0 && condensed<=1);
    if (!condensed)
    {
      data.Realloc(size);
      data.Resize(size);
      if (DataSerializeAs::GetStyle()<=SerializeStyleLoadInPlace)
      {
        TransferBinaryCompressed(data.Data(),data.Size()*sizeof(typename DataSerializeAs::As),enableCompression);
        // we may need to convert data now
        // this may be endian conversion, or anything else
        DataSerializeAs::ConvertOnLoad(data.Data(),(typename DataSerializeAs::As *)data.Data(),data.Size(),EndianSwapNeeded());
      }
      else
      {
        Fail("Full copy on load not implemented yet");
      }
    }
    else
    {
      Assert(size>0);
      typename ArrayType::DataType val;
      Transfer(val);
      data.SetCondensed(val,size);
    }
  }
  else
  {
    SaveInt(data.Size());
    if (data.IsCondensed())
    {
      SaveChar(true);
      typename ArrayType::DataType val = data.Get(0);
      Transfer(val);
    }
    else
    {
      SaveChar(false);
      // type may require some special serialization, as Vector3P instead of Vector3K
      if (DataSerializeAs::GetStyle()==SerializeStyleNative)
      {
        TransferBinaryCompressed(data.Data(),data.Size()*sizeof(typename DataSerializeAs::As),enableCompression);
      }
      else
      {
        Fail("Copy on save not supported yet");
      }
    }
  }
}

template <class ArrayType>
void SerializeBinStream::TransferBinaryArray( ArrayType &data, bool enableCompression )
{
  typedef SerializeAs<typename ArrayType::DataType> DataSerializeAs;
  if (_in)
  {
    int size = LoadInt();
    data.Realloc(size);
    data.Resize(size);

    if (DataSerializeAs::GetStyle()<=SerializeStyleLoadInPlace)
    {
      TransferBinaryCompressed(data.Data(),data.Size()*sizeof(typename DataSerializeAs::As),enableCompression);
      // we may need to convert data now
      // this may be endian conversion, or anything else
      DataSerializeAs::ConvertOnLoad(data.Data(),(typename DataSerializeAs::As *)data.Data(),data.Size(),EndianSwapNeeded());
    }
    else
    {
      Fail("Full copy on load not implemented yet");
    }
  }
  else
  {
    SaveInt(data.Size());
    if (DataSerializeAs::GetStyle()==SerializeStyleNative)
    {
      TransferBinaryCompressed(data.Data(),data.Size()*sizeof(*data.Data()),enableCompression);
    }
    else
    {
      Fail("Copy on save not supported yet");
    }
  }
}

template <class ArrayType>
void SerializeBinStream::SkipBinaryCondensedArray(ArrayType &data, bool enableCompression)
{
  typedef SerializeAs<typename ArrayType::DataType> DataSerializeAs;
  if (_in)
  {
    int size = LoadInt();
    char condensed = LoadChar();
    Assert(condensed>=0 && condensed<=1);
    if (!condensed)
    {
      SkipBinaryCompressed(size*sizeof(typename DataSerializeAs::As),enableCompression);
    }
    else
    {
      Assert(size>0);
      typename ArrayType::DataType dummy;
      Skip(dummy);
    }
  }
  else
  {
    Fail("Skip canout be used when saving");
  }
}

template <class ArrayType>
void SerializeBinStream::SkipBinaryArray( ArrayType &data, bool enableCompression )
{
  typedef SerializeAs<typename ArrayType::DataType> DataSerializeAs;
  if (_in)
  {
    int size = LoadInt();
    SkipBinaryCompressed(size*sizeof(typename DataSerializeAs::As),enableCompression);
  }
  else
  {
    Fail("Skip canout be used when saving");
  }
}

#endif
