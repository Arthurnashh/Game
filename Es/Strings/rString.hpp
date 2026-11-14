#ifdef _MSC_VER
#pragma once
#endif

#ifndef _RSTRING_HPP
#define _RSTRING_HPP

#include <string.h>
#include <ctype.h>


//class RString;
//class RStringB;

#include <Es/Types/pointers.hpp>
#include <Es/Memory/fastAlloc.hpp>

#include <Es/Containers/compactBuf.hpp>
#include <Es/Memory/normalNew.hpp>
#include <Es/Threads/multisync.hpp>

inline void fstrcpy(char *dst, const char *src)
{
  strcpy(dst, src);
}

inline void fstrcpy(wchar_t *dst, const wchar_t *src)
{
  wcscpy(dst, src);
}

inline void fstrncpy(char *dst, const char *src, int n)
{
  strncpy(dst, src, n);
}

inline void fstrncpy(wchar_t *dst, const wchar_t *src, int n)
{
  wcsncpy(dst, src, n);
}

inline size_t fstrlen(const char *str)
{
  return strlen(str);
}

inline size_t fstrlen(const wchar_t *str)
{
  return wcslen(str);
}

inline const char *fstrchr(const char *str, char c)
{
  return strchr(str, c);
}

inline const wchar_t *fstrchr(const wchar_t *str, wchar_t c)
{
  return wcschr(str, c);
}

#ifndef _WIN32

int WideCharToMultiByte(
  unsigned int CodePage,        // code page
  long dwFlags,                 // performance and mapping flags
  const wchar_t *lpWideCharStr, // wide-character string
  int cchWideChar,              // number of chars in string
  char *lpMultiByteStr,         // buffer for new string
  int cbMultiByte,              // size of buffer
  const char *lpDefaultChar,    // default for unmappable chars
  bool *lpUsedDefaultChar       // set when default char used
);
int MultiByteToWideChar(
  unsigned int CodePage,        // code page
  long dwFlags,                 // character-type options
  const char *lpMultiByteStr,   // string to map
  int cbMultiByte,              // number of bytes in string
  wchar_t * lpWideCharStr,      // wide-character buffer
  int cchWideChar               // size of buffer
);
int UTF16ToUTF8 (const char *text, int len, char *buf, int bufLen);

#endif

template <class CharType>
inline CompactBuffer<CharType> *CreateString( const CharType *str, int len )
{
  if (len==0 || *str==0) return NULL; //Fix: len should be tested first, as *str is not initialized in such case (multiple valgrind detections)
  CompactBuffer<CharType> *string = CompactBuffer<CharType>::New(len+1);
  fstrncpy(string->Data(), str, len);
  string->Data()[len]=0;
  return string;
}

template <class CharType>
inline CompactBuffer<CharType> *CreateString(int len)
{
  if (len==0) return NULL;
  CompactBuffer<CharType> *string = CompactBuffer<CharType>::New(len+1);
  string->Data()[0]=0;
  return string;
}

template <class CharType>
inline CompactBuffer<CharType> *CreateString( const CharType *str )
{
  if (*str==0) return NULL;
  size_t len = fstrlen(str)+1;
  return CompactBuffer<CharType>::Copy(str,len);
}

template <class CharType>
inline CompactBuffer<CharType> *CreateString(const CharType *a, const CharType *b)
{
  size_t lena = fstrlen(a);
  size_t len = lena + fstrlen(b);
  if (len==0) return NULL;
  CompactBuffer<CharType> *string = CompactBuffer<CharType>::New(len+1);
  fstrcpy(string->Data(), a);
  fstrcpy(string->Data() + lena, b);
  return string;
}

//! This class represents a string.
/*!
Using this class you can represent a string with the smart way. Using this class
you need not take care of allocating or releasing the space of the string unlike
the usual way. Moreover in case you are about to represent 2 same strings, it will
allocate just single space for them and split them easily right before you're
about to change one of them.
*/
template <class CharType>
class RStringCT // temporary - general class
{
private:
  /// without this compiler is able to convert false to RString
  RStringCT(bool notAllowed);
  /// without this compiler is able to convert 0 to RString
  RStringCT(int notAllowed);

protected:
  //! Reference to string data.
  Ref< CompactBuffer<CharType> > _ref;

public:
  //! Default constructor.
  RStringCT(){}
  //! Constructor which will set text according to source.
  /*!
  \param text Reference to memory to get text from.
  */
  RStringCT( const CharType *text )
  {
    if( text ) _ref=CreateString<CharType>(text);
  }
  //! Constructor which will set text according to source and given number of chars.
  /*!
  \param text Reference to memory to get text from.
  If text is NULL, space is allocated but not initialized.
  \param len Number of characters to get.
  */
  RStringCT( const CharType *text, int len )
  {
    if( text ) _ref=CreateString<CharType>(text,len);
    else _ref=CreateString<CharType>(len);
  }
  //! Constructor which will set text with concatenation of given strings.
  /*!
  \param a The first part of string.
  \param b The second part of string.
  */
  RStringCT( const CharType *a, const CharType *b )
  {
    _ref=CreateString<CharType>(a,b);
  }
  //! This method will return pointer to data of the string.
  /*!
  Characters are finished with (0x00) char as usually. Note that method will return
  valid pointer to data even in case the data have not been set yet. So it will
  never return NULL pointer.
  \return Pointer to chars.
  */
  const CharType *Data() const
  {
    if( _ref ) return _ref->Data();
    else
    {
      static CharType empty[] = {0};
      return empty; // always return valid string
    }
  }
  //! This method will return pointer to string which will represent the key of this string.
  /*!
  This method serves for hashing class implementation. For simplicity we return
  just the represented string itself.
  \sa MapStringToClass
  \return Key value.
  */
  __forceinline const CharType *GetKey() const {return Data();}

  //! This method makes this string independent on another instances pointing to
  //! the same place and consequently returns pointer to data of independent string.
  /*!
  In case pointer is uninitialized this method returns NULL.
  \return Pointer to chars.
  */
  CharType *MutableData()
  {
    if( !_ref ) return NULL;
    MakeMutable();
    return _ref->Data();
  }
  //! Using this operator you can retrieve directly pointer to characters of the string.
  __forceinline operator const CharType *() const {return Data();}
  //! Returns length of the represented string.
  /*!
  The last char (0x00) is not involved.
  \return The length of string in characters.
  */
  int GetLength() const {return _ref ? fstrlen(_ref->Data()) : 0;}
  //! Returns number of references to this string.
  /*!
  \return Number of references.
  */
  int GetRefCount() const {return _ref ? _ref->RefCounter() : 0;}
  //! This operator is suitable for reading single chars of the string.
  /*!
  The first character has the index 0.
  \return Single character from specified position.
  */
  char operator [] ( int i ) const {return Data()[i];}

  //! This method makes this string independent on another instances pointing to
  //! the same place.
  /*!
  You can change the string right after calling this method. In case there are
  more references pointing to this string it will simply create another copy
  of the string and redirect the pointer to it.
  */
  void MakeMutable()
  {
    if( _ref && _ref->RefCounter()>1 )
    {
      // it is shared - we must split it
      _ref=CreateString<CharType>(_ref->Data());
    }
  }
  //! Converts this string to lowercase.
  /*!
  Before converting it will of course call MakeMutable() method  to not to affect
  another instances.
  */
  void Lower()
  {
    // do not make unique if it is already low case and can not affect anything
    if (IsLowCase()) return;
    MakeMutable();
    if( _ref ) strlwr(_ref->Data());
  }
  //! Converts this string to uppercase.
  /*!
  Before converting it will of course call MakeMutable() method  to not to affect
  another instances.
  */
  void Upper()
  {
    MakeMutable();
    if( _ref ) strupr(_ref->Data());
  }
  
  /// check if string contains no upper case letters
  static bool IsLowCase(const char *string);
  /// check if string contains no upper case letters
  bool IsLowCase() const;
  //! Returns substring of this string.
  /*!
  \param from Index of first character to return.
  \param to Index of the character after last character to return.
  */
  RStringCT Substring( int from, int to ) const
  {
    int len=GetLength();
    if( from>len ) from=len;
    if( to>len ) to=len;
    // special case: return whole string (avoid creating a new copy)
    if (from==0 && to==len) return *this;
    // return substring
    return RStringCT(Data()+from,to-from);
  }

  // TODO: disable functions by making them private
  //private:
  //! RString comparison is not possible - use RStringI, RStringS or strcmp, strcmpi instead
  bool operator == (const RStringCT &with) const
  {
    return strcmp(*this,with)==0;
  }

  bool operator == (const CharType *with) const
  {
    return strcmp(*this,with)==0;
  }
  //! RString comparison is not possible - use RStringI, RStringS or strcmp, strcmpi instead
  bool operator != (const RStringCT &with) const
  {
    return strcmp(*this,with)!=0;
  }
  //! Creates buffer for writing new string using direct access to string.   
  CharType *CreateBuffer(int len)
  {
    _ref=CreateString<CharType>(len);
    char *x=MutableData();
    x[len]=0;
    return x;
  }

  //! Finds character in string, returns its position, or -1, if not found
  /*! for compatibility with CString */
  int Find(CharType ch, int nStart=0) const
  {
	if (GetLength() == 0) return -1;
    const CharType *pos=strchr(_ref->Data()+nStart,ch);
    if (pos==NULL) return -1;
    return pos-_ref->Data();
  }

  //! Finds (reversed) character in string, returns its position, or -1, if not found  
  /*! for compatibility with CString */
  int ReverseFind(CharType ch) const
  {
    const CharType *pos=strrchr(_ref->Data(),ch);
    if (pos==NULL) return -1;
    return pos-_ref->Data();
  }

  //! Finds substring in string, returns its position, or -1, if not found
  /*! for compatibility with CString */
  int Find(const CharType *sub, int nStart=0) const
  {
    if (_ref==NULL) return -1;
    const CharType *pos=strstr(_ref->Data()+nStart,sub);
    if (pos==NULL) return -1;
    return pos-_ref->Data();
  }
  
  //! Gets substring from string. 
  /*! @param first index of first character
      @param count count of characters, if ommitted, remain of string is returned
      @return substring
      @note for compatibility with CString
      */
  RStringCT Mid(int first, int count=-1) const
  {
    if (_ref==NULL) return RStringCT();
    RStringCT res;
    int len=GetLength();    
    if (len<first) return RStringCT();
    if (count < 0 || count+first>len) count=len-first;
    if (count==0) return RStringCT();
    return RStringCT(Data()+first,count);
  }

  /// Creates string from an array
  /** Template class must define operator [] and Size() member */
  template<class Array>
  void CreateStringFromArray(Array &arr)
  {
    if (arr.Size()<=0) {*this=0;return;}
    CharType *p=CreateBuffer(arr.Size());
    int i,cnt;
    for (i=0,cnt=arr.Size();i<cnt;i++) p[i]=arr[i];
    p[i]=0;
  }

  void LTrim()
  { 
    for (int i=0,cnt=GetLength();i<cnt;i++)
      if (!isspace((unsigned)Data()[i]))
      {
        if (i) (*this)=Data()+i;
        return;
      }
    (*this)=RStringCT();
  }

  void RTrim()
  {
    for (int i=GetLength()-1;i>=0;i--)
      if (!isspace((unsigned)Data()[i]))
      {
        CharType *p=MutableData();
        p[i+1]=0;
        return;
      }
    (*this)=RStringCT();
  }

  ///Removes whitespace on left and right side of the string
  void Trim()
  {
    LTrim();
    RTrim();
  }
  
  ///Checks if string is empty.
  bool IsEmpty() const
  {
    return _ref==0 || _ref->Data()[0]==0;
  }

#if _MSC_VER>1300 || !defined(_MSC_VER)
  /// Use the current string as a format and replace all "%something" by the content returned by the functor
  template <class Func>
  RStringCT ParseFormat(Func &func) const
  {
    // the place where the result is accumulated
    RStringCT<CharType> result;

    const CharType *ptr = Data();
    const CharType *arg = fstrchr(ptr, '%');
    while (arg)
    {
      // copy the string till '%' to the result
      result = result + RStringCT<CharType>(ptr, arg - ptr);

      // check what is after the '%'
      ptr = arg + 1;
      if (*ptr == '%')
      {
        // "%%" means "%" on the output
        CharType temp[2]; temp[0] = '%'; temp[1] = 0;
        result = RStringCT<CharType>(result, temp);
        ptr++;
      }
      else
      {
        // add the result of func
        result = RStringCT<CharType>(result, func(ptr));
        // the func itself will move ptr after the parameter
      }

      arg = fstrchr(ptr, '%');
    }
    // add the rest of the string (after the last "%something")
    return RStringCT<CharType>(result, ptr);
  };
#endif
};

typedef RStringCT<char> RString;
typedef const RString &RStringVal; // value and parameter passing

typedef RStringCT<wchar_t> RWString;
typedef const RWString &RWStringVal; // value and parameter passing

TypeIsMovableZeroed(RString);

//! RString with added comparison
template <class Compare>
class RStringT: public RString // temporary - general class
{
public:
  //! Default constructor.
  RStringT(){}
  //! Constructor which will set text according to source.
  /*!
  \param text Reference to memory to get text from.
  */
  RStringT(const RString &text)
    :RString(text)
  {
  }
  RStringT( const char *text )
    :RString(text)
  {
  }
  //! Constructor which will set text according to source and given number of chars.
  /*!
  \param text Reference to memory to get text from.
  \param len Number of characters to get.
  */
  RStringT( const char *text, int len )
    :RString(text,len)
  {
  }
  //! Constructor which will set text with concatenation of given strings.
  /*!
  \param a The first part of string.
  \param b The second part of string.
  */
  RStringT( const char *a, const char *b )
    :RString(a,b)
  {
  }

  //! string comparison
  bool operator == (const RStringT &with) const
  {
    Compare cmp;
    return cmp(*this,with)==0;
  }
  //! string comparison
  bool operator != (const RStringT &with) const
  {
    Compare cmp;
    return cmp(*this,with)!=0;
  }
  //! string comparison
  bool operator >= (const RStringT &with) const
  {
    Compare cmp;
    return cmp(*this,with)>=0;
  }
  //! string comparison
  bool operator <= (const RStringT &with) const
  {
    Compare cmp;
    return cmp(*this,with)<=0;
  }
  bool operator > (const RStringT &with) const
  {
    Compare cmp;
    return cmp(*this,with)>0;
  }
  //! string comparison
  bool operator < (const RStringT &with) const
  {
    Compare cmp;
    return cmp(*this,with)<0;
  }
  //! string comparison
  /**
   * Useful, when you need comparison result as sign.
   * It is slightly faster then (a>b)-(a<b)
   */
  int Cmp(const RStringT &with) const
  {
    Compare cmp;
    return cmp(*this,with);
  }

private:
  void Lower();
  void Upper();

public:
  ClassIsMovableZeroed(RStringT)
};

//! functor - compare string, case sensitive
/*!
Suitable as template argument for RStringT
*/

class CmpStrCS
{
public:
  int operator () (const char *a, const char *b)
  {
    return strcmp(a,b);
  }
};

//! functor - compare string, case insensitive
/*!
Suitable as template argument for RStringT
*/
class CmpStrCI
{
public:
  int operator () (const char *a, const char *b)
  {
    return stricmp(a,b);
  }
};

//! case sensitive string
typedef RStringT<CmpStrCS> RStringS;
//! case insensitive string
typedef RStringT<CmpStrCI> RStringI;

#include <Es/Memory/debugNew.hpp>

#include <Es/Containers/hashMap.hpp>

template <>
struct MapClassTraits<RStringI>: public MapClassTraitsNoCase<RStringI>
{
};

// string pool implementation
template <class RStringType>
class RStringBankT
{
  // TODO: keep strings sorted
  // maintain index table (at least for all first letters (256 entries)
  typedef MapStringToClass<RStringType, AutoArray<RStringType> > RStringBankType;

  RStringBankType _bank;

  CriticalSection	_cs;

  // note: items are reference countet
  // if any item with reference count 1 is detected, if must be removed

public:
  RStringBankT();
  ~RStringBankT();
  void Clear();

  static bool IsNull(const RStringType &value) {return RStringBankType::IsNull(value);}
  static bool NotNull(const RStringType &value) {return RStringBankType::NotNull(value);}
  static RStringType &Null() {return RStringBankType::Null();}

  RStringType Add(const char *t);

  void Unregister(const char *str);

  void Compact();
  //! make hash table big enough to hold given number of entries
  void Reserve(int count)
  {
    _bank.Reserve(count);
  }
};


// Banked string
template <class RStringType, RStringBankT<RStringType> *bank>
class RStringBT: public RStringType // banked string
{
  typedef RStringBankT<RStringType> RStringBankType;
  typedef RStringType base;

  void Free();
  void Copy( const RStringBT &src ) {base::_ref=src._ref;}

public:
  RStringBT() {}
  RStringBT( RString str );
  RStringBT( RStringType str );
  RStringBT( const char *str );

  RStringBT( const RStringBT &src ){Copy(src);}
  const RStringBT &operator =( const RStringBT &src )
  {
    RStringBT temp=src;
    Free();
    Copy(temp);
    return *this;
  }
  ~RStringBT(){Free();}

  bool operator == ( const RStringBT &with ) const {return base::_ref==with._ref;}
  bool operator != ( const RStringBT &with ) const {return base::_ref!=with._ref;}

  //! make hash table big enough to hold given number of entries
  static void ReserveTotalCount(int count)
  {
    bank->Reserve(count);
  }

  class LowCase: public RStringBT
  {
    public:
    LowCase( const RStringBT &src)
    {
      RString temp = src;
      temp.Lower();
      RStringBT::operator = (temp);
    }
    
  };
  ClassIsMovableZeroed(RStringBT);
};


#if 1
typedef RStringBankT<RStringS> RStringBank;
extern RStringBank StringBank; // global string bank
typedef RStringBT<RStringS,&StringBank> RStringB;
extern RStringB RStringBEmpty;

#endif

#if 1
typedef RStringBankT<RStringI> RStringBankI;
extern RStringBankI StringBankI; // global string bank
typedef RStringBT<RStringI,&StringBankI> RStringIB;
extern RStringIB RStringIBEmpty;
#endif


template <class RStringType, RStringBankT<RStringType> *bank>
RStringBT<RStringType,bank>::RStringBT( const char *str )
{
  // search for string
  if( !str ) return;
  RStringType::operator =(bank->Add(str));
}

template <class RStringType, RStringBankT<RStringType> *bank>
RStringBT<RStringType,bank>::RStringBT( RString str )
{
  // search for string
  if( !str ) return;
  RStringType::operator =(bank->Add(str));
}

template <class RStringType, RStringBankT<RStringType> *bank>
RStringBT<RStringType,bank>::RStringBT( RStringType str )
{
  // search for string
  if( !str ) return;
  RStringType::operator =(bank->Add(str));
}


template <class RStringType, RStringBankT<RStringType> *bank>
void RStringBT<RStringType,bank>::Free()
{
  if( base::_ref && base::_ref->RefCounter()==2 )
  {
    bank->Unregister(*this);
  }
  base::_ref.Free();
}

template <class RStringType, class RStringBankType>
void CheckRefsWeak(RStringType &str, RStringBankType *bank, void *context )
{
  if (str.GetRefCount() > 1)
  {
    LogF
      (
      "Error: String %s late release (%d)",
      (const char *)str, str.GetRefCount()
      );
    // do not delete, bank will be cleared
  }
}

template <class RStringType, class RStringBankType>
void CheckRefsStrong(RStringType &str, RStringBankType *bank, void *context )
{
  if (str.GetRefCount() > 0)
  {
    LogF
      (
      "Warning: String %s late release (%d)",
      (const char *)str, str.GetRefCount()
      );
    // do not delete, bank will be cleared
  }
}

template <class RStringType>
RStringBankT<RStringType>::RStringBankT()
{
}

template <class RStringType>
RStringBankT<RStringType>::~RStringBankT()
{
  LogF("RStringBank destruct");
  _bank.ForEach(CheckRefsWeak);
  _bank.Clear();
}

template <class RStringType>
void RStringBankT<RStringType>::Clear()
{
  LogF("RStringBank clear");
  _bank.ForEach(CheckRefsWeak);
  _bank.Clear();
}

template <class RStringType>
RStringType RStringBankT<RStringType>::Add( const char *t )
{
  ScopeLockSection lock(_cs);
  RStringType newstr(t);
  // we are sure string is not in bank
  const RStringType &str = _bank[newstr];
  if (NotNull(str)) return str;
  _bank.Add(newstr);
  return _bank[newstr];
}

template <class RStringType, class RStringBankType>
static void DeleteUnused(RStringType &str, RStringBankType *bank, void *context )
{
  if (str.GetRefCount() == 1)
  {
    LogF("String %s not released",(const char *)str);
    bank->Remove(str);
    // ForEach will restart (this array)
  }
}

template <class RStringType>
void RStringBankT<RStringType>::Compact()
{
  _bank.ForEach(DeleteUnused);
}


#define RStringBVal const RStringB &
#define RStringIBVal const RStringIB &


// define static RStringB
#define DEF_RSB(x) static RStringB str_##x(#x);
#define DEF_RSB_EXT(x) RStringB str_##x(#x);
#define DECL_RSB_EXT(x) extern RStringB str_##x;
#define RSB(x) str_##x

#define DEF_RSBI(x) static RStringIB stri_##x(#x);
#define RSBI(x) stri_##x

// basic operators
inline RString operator + ( const RString &a, const RString &b ) {return RString(a,b);}
inline RString operator + ( const RString &a, const char *b ) {return RString(a,b);}

inline RWString operator + ( const RWString &a, const RWString &b ) {return RWString(a,b);}
inline RWString operator + ( const RWString &a, const wchar_t *b ) {return RWString(a,b);}

template <class RStringType>
inline void RStringBankT<RStringType>::Unregister( const char *str)
{
  ScopeLockSection lock(_cs);
  _bank.Remove(str);
}


//! convert integer to string
RString FormatNumber(int number);

//! printf variant with RString output
RString Format(const char *format, ...);

//! format size to be human-readable
RString FormatByteSize(size_t size);

RString FormatByteSize(unsigned long long size);

RString FormatByteSize(long long size);

inline RString FormatByteSize(int size){return FormatByteSize(size_t(size));}

/// array of chars based on AutoArray
template<class Allocator=MemAllocD, class CharType=char>
class CharArray: public AutoArray<CharType, Allocator>
{
  typedef AutoArray<CharType, Allocator> base;
public:
  /// check if the string is zero terminated
  bool CheckClosed() const
  {
    return base::Size()>0 && base::Get(base::Size()-1)==0;
  }
  operator const CharType *() const {Assert(CheckClosed());return base::Data();}
  operator RStringCT<CharType> () const {Assert(CheckClosed());return RStringCT<CharType>(base::Data());}
  operator RStringB () const {Assert(CheckClosed());return RStringB(base::Data());}
  
  const CharArray &operator = (const char *src)
  {
    base::Resize(strlen(src)+1);
    strcpy(base::Data(),src);
    return *this;
  }
  
  const CharArray &operator += (const char *src)
  {
    Assert(CheckClosed());
    int origSize = base::Size();
    base::Resize(origSize+strlen(src));
    strcpy(base::Data()+origSize-1,src);
    return *this;  
  }
  
  void Reset()
  {
    base::Resize(1);
    *base::Data() = 0;
  }
  
};

template <class Allocator, class CharType>
void strcpy(CharArray<Allocator,CharType> &dst, const CharType *src)
{
  dst = src;
}
template <class Allocator,class CharType>
void strcat(CharArray<Allocator,CharType> &dst, const CharType *src)
{
  dst += src;
}

template <class Allocator,class CharType>
void wcscpy(CharArray<Allocator,CharType> &dst, const CharType *src)
{
  dst = src;
}
template <class Allocator,class CharType>
void wcscat(CharArray<Allocator,CharType> &dst, const CharType *src)
{
  dst += src;
}

/// temporary string - useful for constructing RString part by part
template <int size=256, class CharType = char>
class TempString: public CharArray< MemAllocLocal<CharType,size>, CharType >
{
};

/// temporary wide string - useful for constructing RString part by part
template <int size=256>
class TempWString: public TempString<size,wchar_t>
{
};

#endif
