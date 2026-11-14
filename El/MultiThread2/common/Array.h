#ifndef BREDYLIBS_LIBS_COMMON_ARRAY_H_
#define BREDYLIBS_LIBS_COMMON_ARRAY_H_

#include "Assert.h"

#ifdef _WIN32
#include <new>
#endif

namespace BredyLibs
{


template<class Arr1,class Arr2> class ArrayOperSum;
template<class Arr> class ArrayOperMid;  


template<class ImplementType> 
class GenArray: public ImplementType
{
public:
  GenArray(const ImplementType &other):ImplementType(other) {}
  GenArray(const GenArray<ImplementType> &other):ImplementType(other) {}
  GenArray() {}

  template<class Functor>
  unsigned long EnumItems(Functor &functor) const
  {
    for (int i=0,cnt=this->Size();i<cnt;i++)
    {
      unsigned long res=functor((*this)[i]);
      if (res) return res;
    }
    return 0;
  }
  
  template<class ArrayT>
  bool Load(const ArrayT &anarray, size_t offset=0, size_t count=-1)
  {
    if (count==-1) count=anarray.Size();
    if (offset>=this->Size()) return false;
    if (offset+count>this->Size()) return false;
    for (size_t i=0;i<count;i++)
    {
        (*this)[i+offset]=anarray[i];
    } 
    return true;
  }
  
                
  template<class OtherArr>
  GenArray<ArrayOperSum<ImplementType,OtherArr> > operator+ (const GenArray<OtherArr> &other) const
  {
    return ArrayOperSum<ImplementType,OtherArr>(*this,other);
  }
  
  
  GenArray<ArrayOperMid<ImplementType> > operator()(size_t first, size_t last) const
  {
    return Mid(first,last-first+1);
  }
  
  
  GenArray<ArrayOperMid<ImplementType> > Mid(size_t first, size_t count=-1) const
  {
    if (count==(size_t)-1) count=this->Size()-first;
    return ArrayOperMid<ImplementType>(*this,first,count);
  }
  
    
  template<class OtherArr>
  GenArray<ArrayOperSum<ArrayOperSum<ArrayOperMid<ImplementType>,OtherArr>,ArrayOperMid<ImplementType> > > Insert(size_t pos, const GenArray<OtherArr> &other) const
  {
    return Mid(0,pos)+other+Mid(pos); 
  }
  
  GenArray<ArrayOperSum<ArrayOperMid<ImplementType>,ArrayOperMid<ImplementType> > > Delete(size_t pos=0, size_t count=-1) const
  {
    if (count==(size_t)-1) count=this->Size()-pos;
    return Mid(0,pos)+Mid(pos+count,this->Size()-(pos+count));
  }

  class CompareItems
  {
  public:
    template<class T>
    int operator()(const T &t1, const T &t2) const
    {
      if (t1<t2) return -1;
      if (t1==t2) return 0;
      return 1;
    }
  };

  template<class OtherArr,class CmpFunct>
  int Compare(const OtherArr &other, const CmpFunct &cmp) const
  {
    size_t x=0,s1=this->Size(),s2=other.Size();
    while (x<s1 && x<s2)
    {        
        int r=cmp((*this)[x],other[x]);
        if (r!=0) return r;
        x++;
    }    
    if (x<s1) return 1;
    else if (x<s2) return -1;
    else return 0;    
  }
  
  template<class OtherArr>
  bool operator==(const OtherArr &other) const {return Compare(other,CompareItems())==0;} 
  template<class OtherArr>
  bool operator!=(const OtherArr &other) const {return Compare(other,CompareItems())!=0;} 
  template<class OtherArr>
  bool operator>=(const OtherArr &other) const {return Compare(other,CompareItems())>=0;} 
  template<class OtherArr>
  bool operator<=(const OtherArr &other) const {return Compare(other,CompareItems())<=0;} 
  template<class OtherArr>
  bool operator>(const OtherArr &other) const {return Compare(other,CompareItems())>0;} 
  template<class OtherArr>
  bool operator<(const OtherArr &other) const {return Compare(other,CompareItems())<0;} 
};

template<class Type>
class ArrayBase
{
protected:
  Type *_array;
  size_t _size;
  ArrayBase() {}
public:
  ArrayBase(const ArrayBase& other):_array(other._array),_size(other._size) {}
  ArrayBase &operator=(const ArrayBase& other)
  {
    _array=other._array;
    _size=other._size;
    return *this;
  }
  const Type &operator[](size_t pos) const 
  {
    AssertDesc(pos<_size,"Index out of range");
    return _array[pos];
  }
  
  Type &operator[](size_t pos) 
  {
    AssertDesc(pos<_size,"Index out of range");
    return _array[pos];
  }

  size_t Size() const {return _size;}
  size_t GetSize() const {return _size;}

  template<class T>
  void Render(T *where, unsigned int offset, size_t count) const
  {
    Assert(offset<_size);
    Assert(offset+count<=_size);
    while (count--)
    {
      *where++=(T)_array[offset++];      
    }
  }

  typedef Type ThisType;
};

///defines standard C array as a class
template<class Type>
class Array: public GenArray<ArrayBase<Type> >
{
public:
  ///array construction
  /**
   * Array is constructed using the pointer and size 
   * @param array pointer to the first item in array
   * @param sz count of elements in array
   */
  Array(Type *array=0, size_t sz=0) {this->_array=array;this->_size=sz;}
  Array &operator=(const Array& other) {ArrayBase<Type>::operator=(other);return *this;}
  
  typedef typename ConstObject<Type>::Remove RemoveConst;

  const Type &operator[](size_t pos) const 
  {
	AssertDesc(pos<this->_size,"Index out of range");
	return this->_array[pos];
  }
  
  Type &operator[](size_t pos) 
  {
	AssertDesc(pos<this->_size,"Index out of range");
	return this->_array[pos];
  }

  size_t Size() const {return this->_size;}
  size_t GetSize() const {return this->_size;}

  const Type *Data() const {return this->_array;}
  Type *Data() {return this->_array;}

  operator const Array<RemoveConst> &() const
  {
    return *reinterpret_cast<const Array<RemoveConst> *>(this);
  }

  operator const Array<const Type> &() const
  {
    return *reinterpret_cast<const Array<const Type> *>(this);
  }
};

template<class Type> struct ArraySumOperProp {typedef const Type &Value;};
template<class T1,class T2> struct ArraySumOperProp<ArrayOperSum<T1,T2> > {typedef ArrayOperSum<T1,T2> Value;};
template<class T> struct ArraySumOperProp<ArrayOperMid<T> > {typedef ArrayOperMid<T> Value;};

template<class ArrT1,class ArrT2> class ArrayOperSum
{
public:
  typedef typename ArraySumOperProp<ArrT1>::Value Arr1;
  typedef typename ArraySumOperProp<ArrT2>::Value Arr2;
private:
    Arr1 _arr1;
    Arr2 _arr2;    
  public:
    ArrayOperSum(const ArrT1 &arr1,const ArrT2 &arr2):_arr1(arr1),_arr2(arr2) {}
    typedef typename ArrT1::ThisType ThisType;
    
    ThisType &operator[](size_t index)
    {
        size_t sz=_arr1.Size();
        if (index<sz) return _arr1[index];
        else return _arr2[index-sz];
    }  
    
    const ThisType &operator[](size_t index) const
    {
        size_t sz=_arr1.Size();
        if (index<sz) return _arr1[index];
        else return _arr2[index-sz];
    }  
    
    size_t Size() const
    {
        return _arr1.Size()+_arr2.Size();
    }
    
    template<class T>
    void Render(T *where, unsigned int offset, size_t count) const
    {
      size_t sz1=_arr1.Size();
//      size_t sz2=_arr2.Size();
      if (offset<sz1)
      {
        size_t rmn=sz1-offset;        
        if (rmn>count) rmn=count;
        _arr1.Render(where,offset,sz1-offset);
        count-=rmn;
        if (count)                  
          _arr2.Render(where+rmn,0,count);
      }
      else
      {
        offset-=sz1;
        if (count) _arr2.Render(where,offset,count);
      }
    }
};
template<class ArrT> class ArrayOperMid
{
public:
  typedef typename ArraySumOperProp<ArrT>::Value Arr;
private:
    Arr _arr;
    size_t _offset;
    size_t _count;
public:
    typedef typename ArrT::ThisType ThisType;
    ArrayOperMid(const ArrT &arr, size_t beg, size_t count):_arr(arr),_offset(beg),_count(count) {}    

    ThisType &operator[](size_t index)
    {
        return _arr[index+_offset];
    }  
    
    const ThisType &operator[](size_t index) const
    {
        return _arr[index+_offset];
    }  
    
    size_t Size() const
    {
        return _count;
    }

    template<class T>
    void Render(T *where, unsigned int offset, size_t count) const
    {
      if (count==0) return;
      offset+=_offset;
      _arr.Render(where,offset,count);
    }
};  


}

#endif /*ARRAY_H_*/

