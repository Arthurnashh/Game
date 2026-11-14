#ifndef encodeString_h__
#define encodeString_h__

template <int key>
struct EncoderXOR
{
  typedef char EncodedType;

  template <int pos>
  static __forceinline EncodedType Encode(char x)
  {
    return x^key;
  }

  static __forceinline char Decode(int pos, EncodedType x)
  {
    return x^key;
  }
};


template <int key>
struct EncoderShiftPos
{
  typedef char EncodedType;

  static __forceinline unsigned char ROL(unsigned char x) {return (x>>7) | (x << 1);}
  static __forceinline unsigned char ROR(unsigned char x) {return (x<<7) | (x >> 1);}
   
  template <int pos>
  static __forceinline EncodedType Encode(char x)
  {
    return ((ROL(x)-pos))^key;
  }

  static __forceinline char Decode(int pos, EncodedType x)
  {
    return ROR((x^key)+pos);
  }
};

template<size_t N, size_t I, class Encoder>
struct EncodeImpl
{
  static __forceinline void Do(typename Encoder::EncodedType (&dst)[N], const char(&data)[N])
  {
    dst[I] = Encoder::Encode<I>(data[I]);
    EncodeImpl<N,I-1,Encoder>::Do(dst, data);
  }
};

template<size_t N, class Encoder>
struct EncodeImpl<N, 0, Encoder>
{
  static __forceinline void Do(typename Encoder::EncodedType (&dst)[N], const char(&data)[N])
  {
    dst[0] = Encoder::Encode<0>(data[0]);
  }
};

template<size_t N, class Encoder>
struct EncodedStringImpl
{
  typename Encoder::EncodedType str[N];

  __forceinline EncodedStringImpl(const char(&data)[N])
  {
    EncodeImpl<N,N-2,Encoder>::Do(str,data);
    str[N-1] = 0;
  }

};

template<size_t N, class Encoder>
struct DecodedStringImpl
{
  char str[N];

  __forceinline DecodedStringImpl(const EncodedStringImpl<N,Encoder> &encoded)
  {
    for (int i=0; i<N-1; i++)
    {
      str[i] = Encoder::Decode(i,encoded.str[i]);
    }
    str[N-1] = 0;
  }

  operator const char *() const {return str;}
};


template<class Encoder, size_t N>
__forceinline EncodedStringImpl<N,Encoder> EncodedString(const char(&data)[N])
{
  return EncodedStringImpl<N,Encoder>(data);
}

template<size_t N, class Encoder>
DecodedStringImpl<N,Encoder> DecodeString(const EncodedStringImpl<N,Encoder> &encoded)
{
  return DecodedStringImpl<N,Encoder>(encoded);
}

#endif // encodeString_h__
