/*
Copyright (C)2006 bo Ondrej Novak
http://bredy.jinak.cz
novak.on@centrum.cz


This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef _BREDY_LIBS_TYPES_SYNCTYPES_
#define _BREDY_LIBS_TYPES_SYNCTYPES_

#include "../common/Assert.h"

#include "Interlocked.h"


namespace MultiThread
{




  ///Implements basic synchronized type - signed int (long)
  /**Synchronized type ensures, that all operation are carried out atomically
  This is promised on all "get modify and store" functions, such as operators ++,--, +=, -= etc.
  There are some special functions, that provides atomic exchange, or test and store operations
  @see SyncInt::Exchange, SyncInt::setValueWhen
  @note SyncInt type must be aligned to 32-bit address
  */
  class SyncInt
  {
    long _value; ///<Variable that stores value
  public:
    SyncInt() {} ///<Default constructor (constructs uninitialized value)
    SyncInt(long value):_value(value) {} ///<Constructor (constructs initialized value)
    SyncInt(const SyncInt &other):_value(other._value) {} ///<Copy constructor

    ///Provides atomic assignment
    long operator=(long value) {_value=value;return value;}
    ///Provides atomic assignment
    SyncInt &operator=(const SyncInt &other) {_value=other._value;return *this;}

    ///Provides atomic increment. 
    /**No other processors or threads can change this value during this operation*/
    long operator++() {return MTIncrement(&_value);}

    ///Provides atomic decrement. 
    /**No other processors or threads can change this value during this operation*/
    long operator--() {return MTDecrement(&_value);}

    ///Provides atomic left increment. 
    /**No other processors or threads can change this value during this operation*/
    long operator++(int z) {return MTExchangeAdd(&_value,1);}

    ///Provides atomic left decrement. 
    /**No other processors or threads can change this value during this operation*/
    long operator--(int z) {return MTExchangeAdd(&_value,-1);}

    ///Returns value. 
    /**
    @note Result shouldn't reflects current value, because, it can be changed before result is returned
    */
    operator long () {return _value;}


    ///Provides atomic add. 
    /**No other processors or threads can change this value during this operation*/
    long operator+=(long add) {return MTExchangeAdd(&_value,add)+add;}

    ///Provides atomic subtract 
    /**No other processors or threads can change this value during this operation*/
    long operator-=(long add) {return MTExchangeAdd(&_value,-add)-add;}

    ///Provides atomic multiply
    /**  
    @note Implementation can process operation multiple times, until it ensures, that no other
    threads did not change value during operation
    */
    long operator*=(long mm)
    {
      long mval;
      long mres;
      do {mval=_value;mres=mval*mm;} while (MTCompareExchange(&_value,mres,mval)!=mval);
      return mres;
    }

    ///Provides atomic divide
    /**  
    @note Implementation can process operation multiple times, until it ensures, that no other
    threads did not change value during operation
    */
    long operator/=(long mm)
    {
      long mval;
      long mres;
      do {mval=_value;mres=mval/mm;} while (MTCompareExchange(&_value,mres,mval)!=mval);
      return mres;
    }

    ///Provides atomic shift right
    /**  
    @note Implementation can process operation multiple times, until it ensures, that no other
    threads did not change value during operation
    */
    long operator>>(long mm)
    {
      long mval;
      long mres;
      do {mval=_value;mres=mval>>mm;} while (MTCompareExchange(&_value,mres,mval)!=mval);
      return mres;
    }

    ///Provides atomic shift left
    /**  
    @note Implementation can process operation multiple times, until it ensures, that no other
    threads did not change value during operation
    */
    long operator<<(long mm)
    {
      long mval;
      long mres;
      do {mval=_value;mres=mval<<mm;} while (MTCompareExchange(&_value,mres,mval)!=mval);
      return mres;
    }

    ///Provides atomic exchange
    /**  
    gets value and stores another. Operation is interlocked
    */
    long Exchange(long with)
    {
      return MTExchange(&_value,with);
    }

    ///Provides atomic test with set value
    /**
    Function tests value for oldvalue. When value is equal to parameter, sets value to newvalue and returns
    true. Otherwise, value remains unchanged and returns false
    @param oldvalue value to compare
    @param newvalue new value, that will be set to object, when test passed
    @return true, if value has been changed (test passed), false if test failed.
    @note don't use "if (syncInt==0) then {syncIntr=1;...}". Instead use "if (syncInt.SetValueWhen(0,1)) then {...}"
    */
    bool SetValueWhen(long oldvalue, long newvalue)
    {
      return MTCompareExchange(&_value,newvalue,oldvalue)==oldvalue;
    }


  };
  ///Implements basic synchronized type - typed pointer -
  /** This is promised on all "get modify and store" functions, such as operators ++,--, +=, -= etc.
  There are some special functions, that provides atomic exchange, or test and store operations
  @see SyncInt::Exchange, SyncInt::setValueWhen
  @note SyncInt type must be aligned to 32-bit address
  */
  template<class Type>
  class SyncPtr
  { 
  protected:
    Type *_ptr;
  public:
    SyncPtr():_ptr(0) {}
    SyncPtr(Type *ptr):_ptr(ptr) {}
    SyncPtr(const SyncPtr &other):_ptr(other._ptr) {}

    SyncPtr &operator=(const SyncPtr &other) {_ptr=other._ptr;return *this;}
    Type *operator=(Type *other) {_ptr=other;return _ptr;}

    operator Type *() {return _ptr;}
    operator Type *() const {return _ptr;}

    Type *operator->() {return _ptr;}
    Type *operator->() const {return _ptr;}

    Type *operator++() {return (Type *)MTExchangeAdd((long *)&_ptr,sizeof(Type));}
    Type *operator--() {return (Type *)MTExchangeAdd((long *)&_ptr,-sizeof(Type));}
    Type *operator++(int) {return (Type *)MTExchangeAdd((long *)&_ptr,sizeof(Type))-1;}
    Type *operator--(int) {return (Type *)MTExchangeAdd((long *)&_ptr,-sizeof(Type))+1;}

    Type *Exchange(Type *with) {return (Type *)MTExchange((long *)&_ptr,(long)with);}
    bool SetValueWhen(Type *oldvalue, Type *newvalue)
    {
      return MTCompareExchange((long *)&_ptr,(long)newvalue,(long)oldvalue)==(long)oldvalue;
    }

    bool operator! () const {return _ptr==0;}
    bool IsNull() const {return _ptr==0;}
    bool NotNull() const {return _ptr!=0;}

    Type *Ptr() {return _ptr;}
    const Type *Ptr() const {return _ptr;}

  };

}
#endif
