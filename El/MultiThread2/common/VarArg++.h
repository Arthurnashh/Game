#ifndef BREDYLIBS_LIBS_COMMON___VARARGCPP_H_
#define BREDYLIBS_LIBS_COMMON___VARARGCPP_H_
#pragma once

#include "Array.h"

namespace BredyLibs
{

template<class T, int count> class ArrayArgType;
///namespace used to hide all internal helper classes and structs 
namespace VarArgHidden
{
  
  template<class T, int count, int remain>
  class ArrayArgIter
  {
    friend class ArrayArgType<T,count>;
    friend class ArrayArgIter<T,count,remain+1>;
    template <int> friend class ArrayArg;
    const ArrayArgType<T,count> &owner;
    
  public:
    ArrayArgIter(const ArrayArgType<T,count> &owner):owner(owner) {}


    ArrayArgIter<T,count,remain-1> operator,(const T &val) const
    {
      owner.buffer[count-remain]=val;
      return ArrayArgIter<T,count,remain-1>(owner);
    }
  };

  template<class T, int extra>
  class Too_Many_Parameters
  {
  public:
    Too_Many_Parameters() {}
    Too_Many_Parameters<T,extra+1> operator,(const T &val) const
    {      
      return Too_Many_Parameters<T,extra+1>();
    }
  };

  template<class T, int count>
  class ArrayArgIter<T,count,0>:public Array<T>
  {         
    friend class ArrayArgType<T,count>;
    friend class ArrayArgIter<T,count,1>;
    ArrayArgIter& operator=(const ArrayArgIter<T,count,0> &other);
    template <int> friend class ArrayArg;
  public:
    ArrayArgIter(const ArrayArgType<T,count> &owner):Array<T>(owner.buffer,count) {}    
    Too_Many_Parameters<T,1> operator,(const T &val) const
    {      
      return Too_Many_Parameters<T,1>();
    }
  };
}

///Class provides variable argument count for given type
/**
 * Best usage of this class is to convert variable count of arguments to a single array, when it passed
 * to the funtion
 * 
 * @b Usage: foo((ArrayArgType<int,5>(),2,5,4,3,1));
 *
 * Target function must use type 'const Array<T> &' for retrieve arguments
 * 
 * @param T common type of arguments
 * @param count count of arguments
 */
template<class T, int count>
class ArrayArgType
{
protected:
  mutable T buffer[count];
  template<class, int,int> friend class VarArgHidden::ArrayArgIter;  

public:
  ArrayArgType() {}

    
  VarArgHidden::ArrayArgIter<T,count,count-1> operator,(const T &val) const
  {
    buffer[0]=val;
    return VarArgHidden::ArrayArgIter<T,count,count-1>(*this);
  }
};

namespace VarArgHidden
{
  template<class T, int count>
  class Helper: public ArrayArgType<T,count>
  {
  	typedef ArrayArgType<T,count> B;
  public:
   Helper(const T &value) {B::buffer[0]=value;}
   ArrayArgIter<T,count,count-2> operator,(const T &val) const
   {
     B::buffer[1]=val;
     return ArrayArgIter<T,count,count-2>(*this);
   }
  };

  template<class T>
  class Helper<T,1>: public ArrayArgType<T,1>, public Array<T>
  {
  	typedef ArrayArgType<T,1> B;
  public:
   Helper(const T &value):Array<T>(B::buffer,1) {B::buffer[0]=value;}
  };

};

///Class provides variable argument count for given type
/**
 * Best usage of this class is to convert variable count of arguments to a single array, when it passed
 * to the funtion
 * 
 * @b Usage: foo((ArrayArg<5>(),2,5,4,3,1));
 *
 * Target function must use type 'const Array<T> &' for retrieve arguments
 * 
 * @param count count of arguments
 * @note common type of the arguments is given from first argument. If you need to convert parameters
 * int one common argument, use ArrayArgType class
 */

template<int count>
class ArrayArg
{

public:
  template<class T>
  VarArgHidden::Helper<T,count> operator,(const T &val)
  {
    return VarArgHidden::Helper<T,count>(val);
  }
};

#define ArrArg(count) ArrayArg<count>()
#define ArrArgType(type,count) ArrayArgType<type,count>()

namespace VarArgHidden
{

    class EmptyTail
    {
    public:
      static const int ListSize=0;

      template<class Functor>
      bool ForEach(const Functor &functor) const
      {
        return false;
      }

      template<class Functor>
      bool ForOne(int pos,const Functor &functor) const
      {
        return false;
      }

      static int Size() {return ListSize;}
    };

    template<class T, int index> struct GetType;
    template<class T, int index> struct GetValueAt;

    template<class T, int index>
    struct GetValueAt
    {
      static typename GetType<T,index>::Type Get(const T &owner) 
      {
        return GetValueAt<T,index-1>::Get(owner.tail);
      }
    };

    template<class T,int index>
    struct GetType{ typedef typename GetType<typename T::Tail, index-1>::Type Type;};

    template<class H,class T>
    class List
    {
      const H &head;
      const T &tail;
      typedef List<H,T> ThisType;
      template<class X, int index> friend struct GetValueAt;
    public:        
      typedef H Head;
      typedef T Tail;

      const T &GetHead() const {return head;}
      const T &GetTail() const {return tail;}

      static const int ListSize=T::ListSize+1;

      List(const H &head, const T &tail):head(head),tail(tail) {}
      
      template<class Type>
      List<Type,ThisType> operator,(const Type &other) const
      {
        return List<Type,ThisType>(other,*this);
      }
      
      template<class Functor>
      bool ForEach(const Functor &functor) const
      {
        if (tail.ForEach(functor)) return true;
        return functor(head);
      }

      template<class Functor>
      bool ForOne(int pos,const Functor &functor) const
      {
        if (pos+1==ListSize) return functor(head);
        else return tail.ForOne(pos,functor);
      }

      static int Size() {return ListSize;}

      template<class RetType>
      RetType Get(int pos) const
      {
        if (pos+1!=ListSize) return tail.Get<RetType>(pos);
        else return head;
      }

      

      template<int index>
      typename GetType<ThisType,index>::Type Get() const
      {
        return GetValueAt<ThisType,index>::Get(*this);
      }
    };


      template<class T>
      struct GetType<T, 0> {typedef typename T::Head Type;};


      template<class T>
      struct GetValueAt<T,0>
      {
        static typename GetType<T,0>::Type Get(const T &owner) 
        {
          return owner.head;
        }
      };
}

///Provides variable count of arguments of any type
/**
 * Best usage is foo((VarArgs(),arg1,arg2,arg3,....))
 * 
 * target function must be declared as template<class T>void foo(const T &args).
 * 
 * Functions to read arguments from object see VarArgHidden::List 
 */
 
class VarArgs
{
public:
    
  template<class T>
  VarArgHidden::List<T,VarArgHidden::EmptyTail> operator,(const T &other)
  {
    return VarArgHidden::List<T,VarArgHidden::EmptyTail>(other,VarArgHidden::EmptyTail());
  }
};

}
  #endif
