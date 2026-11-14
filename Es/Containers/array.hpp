#ifdef _MSC_VER
#pragma once
#endif

#ifndef __ARRAY_HPP
#define __ARRAY_HPP

#include <Es/Framework/debugLog.hpp>
#include <Es/Containers/typeOpts.hpp>
#include <Es/Types/pointers.hpp>

#ifdef _CPPRTTI
  #include "typeinfo.h"
#endif

#include <Es/Memory/memAlloc.hpp>
#include <Es/Memory/normalNew.hpp>

///Base class of all arrays.
/**
It defines basic common operations with arrays.
*/


template<class Type>
class Array
{
protected:
  //! Pointer to array data
  Type *_data;
  //! Number of items in array. 
  int _n;

  enum NoInitEnum {NoInit};
public:  
  //! provide public access to the data type
  typedef Type DataType;

  ///Template class used to remove const from type during conversion from const T to T
  template<class T>
  struct ConstInvert
  {
    typedef const T InvConst;
    typedef T RemovedConst;
  };

  template<class X>
  struct ConstInvert<const X>
  {
    typedef X InvConst;
    typedef X RemovedConst;
  };
  
  typedef typename ConstInvert<Type>::InvConst TypeConstInv;
  ///Useful, when you want to get NotConstType of given type.
  typedef typename ConstInvert<Type>::RemovedConst NotConstType;

  ///Constructs array class at memory block of specified items
  /** 
  @param ptr pointer to statically allocated array, or array allocated in stack
  @param n specifies number of items in array

  @note To construct an empty array, use Array(0)
  */
  Array(Type *ptr, int n=0):_data(ptr),_n(n)   {}
  
  ///Constructs array class at another array.
  /** 
  Copy constructor, but it copies only reference, not data.
  It is useful to pass arrays as parameters to functions.
  @param other reference to source array
  */
  Array(const Array<Type> &other):_data(other._data),_n(other._n) {}

//  Array(const Array<TypeConstInv> &other):_data(const_cast<Type *>(other.Data())),_n(other.Size()) {}
 

  ///Conversion from Array<Type> to Array<const Type>
  operator const Array<const Type> &() const {return *reinterpret_cast<const Array<const Type> *>(this);}
  ///Conversion from Array<const Type> to Array<Type>
  operator const Array<NotConstType> &() const {return *reinterpret_cast<const Array<NotConstType> *>(this);}
  

  ///Constructs array as one-item array
  /**
  You can use this constructor, if you want to pass single variable as array parameter..
  Constructor can convert any variable into array variable
  @param oneItem variable that will be converted to array.
  @note size of this type of array is always 1.
  */
  explicit Array(Type &oneItem): _data(&oneItem),_n(1)  {}

  ///Creates uninitialized array
  /**
  Constructor assumes, that array will be inicialized by derived class
  */
  Array(NoInitEnum noInit) {}
  //! Setting of specified item.
  /*!
    Returned item can be modified.
    \param i Index to desirable item.
    \return Desirable item
  */
  Type &Set( int i )
  {
    AssertDebug( i>=0 && i<_n );
    return _data[i];
  }
  //! Getting of specified item.
  /*!
    Returned item cannot be modified.
    \param i Index to desirable item.
    \return Desirable item.
  */
  const Type &Get( int i ) const
  {
    AssertDebug( i>=0 && i<_n );
    return _data[i];
  }
  //! Defined operator [] for modifying a value.
  Type &operator [] ( int i ) {return Set(i);}
  //! Defined operator [] for reading a value.
  const Type &operator [] ( int i ) const {return Get(i);}
  //! Returns the reference to array data for reading purpose.
  const Type *Data() const {return _data;}
  //! Returns the reference to array data for modifying purpose.
  Type *Data() {return _data;}
  //! Retrieving a number of items used in the array.
  int Size() const {return _n;}

  //@{ access the first element
  const Type &First() const {return Get(0);}
  Type &First() {return Set(0);}
  //@}
  //@{ access the last element
  const Type &Last() const {return Get(_n-1);}
  Type &Last() {return Set(_n-1);}
  //@}

  /// perform given operation on all items - no modification
  /** when function return true, stop executing */
  template <class Functor>
  bool ForEachF(const Functor &f) const
  {
    for (int i=0; i<Size(); i++)
    {
      if (f(Get(i)))
      {
        return true;
      }
    }
    return false;
  }
  
  /// perform given operation on all items - with possible modification
  /** when function return true, stop executing */
  template <class Functor>
  bool ForEachF(const Functor &f)
  {
    for (int i=0; i<Size(); i++)
    {
      if (f(Set(i)))
      {
        return true;
      }
    }
    return false;
  }

  /// assign all elements the same value
  /** useful to initialize arrays of "unitialized" values (empty default constructor) */
  void SetAll(const Type &val) {for (int i=0; i<Size(); i++) Set(i)=val;}
  
  ///Compare two arrays - return integer
  /**
   * @retval -1 left array is smaller then right
   * @retval 0 arrays are the same
   * @retval 1 left array is bigger then right
   */
  template <class ArrayType>
  int CompareWith(const ArrayType &other) const
  {
    if (this==&other) return 0; //array with itself are the same
    if (other.Size()!=Size()) 
    {
      return (Size()>other.Size())-(Size()<other.Size());
    }
    else
    {
      for (int i=0;i<Size();i++)
      {
        if (operator[](i)==other[i]) continue;
        if (operator[](i)>other[i]) return 1;
        if (operator[](i)<other[i]) return -1;
        Fail("Compare wrong");
        return (this>&other)-(this<&other); //when items have bug in compare operator, keep order from addresses
      }
      return 0;
    }
  }

  template <class ArrayType, class IsEqualFunction>
  bool IsEqual(const ArrayType &other, IsEqualFunction isEqual) const
  {
    if (this==&other) return true; //array with itself are the same
    if (other.Size()!=Size()) 
    {
      return false;
    }
    else
    {
      for (int i=0;i<Size();i++)
      {
        if (!isEqual(operator[](i),other[i])) return false;
      }
      return true;
    }
  }

  template <class ArrayType>
  bool IsEqual(const ArrayType &other) const
  {

    return IsEqual(other,[](const Type &a, const Type &b){return a==b;});
  }

  template <class ArrayType>
  bool operator==(const ArrayType &other) const {return IsEqual(other);}
  template <class ArrayType>
  bool operator>=(const ArrayType &other) const {return CompareWith(other)>=0;}
  template <class ArrayType>
  bool operator<=(const ArrayType &other) const {return CompareWith(other)<=0;}
  template <class ArrayType>
  bool operator!=(const ArrayType &other) const {return !IsEqual(other);}
  template <class ArrayType>
  bool operator>(const ArrayType &other) const {return CompareWith(other)>0;}
  template <class ArrayType>
  bool operator<(const ArrayType &other) const {return CompareWith(other)<0;}
  

};


//! Template class suitable for advanced array management.
/*!
  If you need to make an array of some class, you must specify the type
  of the class using predefined macros (See the macros TypeIs*(...)).
  It is a custom to write it right beneath the class declaration.
  
  Allocator which will be used for this class. It is set to MemAllocD by default.
  size of Allocator is often 0, therefore we want to avoid using it as member,
  as it seems member size is always non zero (at least with VC .NET 2003)  
*/
template<class Type, class Allocator=MemAllocD>
class AutoArray: public Array<Type>, protected Allocator //, public CountInstances<AutoArray<Type,Allocator> >
{
  typedef ConstructTraits<Type> CTraits;
  typedef Array<Type> base;
protected:
  //! Allocator which will be used for this class. It is set to MemAllocD by default.
  // Allocator _alloc; // size of _alloc is often 0
  //! Number of allocated items. This value must be greater or equal to _n. \sa _n
  int _maxN;
  //! Function called usually by constructors.
  void DoConstruct();
  //! Function called usually by destructors.
  void DoDestruct();
public:  
  

  static inline int MinGrow() {return Allocator::MinGrow();}
  //! Implicit constructor.
  AutoArray();
  //! Constructor which will allocate space for n items.
  /*!
    \param n Number of items to be allocated.
  */
  explicit AutoArray( int n );


  /// allocate a space and initialize with a default value
  AutoArray( int n, const Type &type);
  //! Constructor which will set a particular storage.
  /*!
    \param alloc Storage to be set.
  */
  explicit AutoArray( const Allocator &alloc )
  :Allocator(alloc),base(base::NoInit)
  {
    DoConstruct();
  }
  //! Constructor which will copy n items from another array.
  /*!
    \param src Source array to be copied from.
    \param n Number of items to copy.
  */
  AutoArray( const Type *src, int n );
  //! Constructor which will copy whole source array.
  /*!
    \param src Source array to be copied.
  */
  AutoArray( const AutoArray &src );
  //! Constructor which will copy whole source array.
  /*!
  \param src Source array to be copied.
  */
  explicit AutoArray( const base &src );
  //! Guess what ;-)
  ~AutoArray();
  #if !defined (_MSC_VER) || _MSC_VER>=1300
  /// Copying of one array to another.
  /** Old MSC cannot handle this properly. */
  template <class AnotherAllocator>
  void operator = ( const AutoArray<Type,AnotherAllocator> &src )
  {
    Copy(src.Data(),src.Size());
  }
  #endif

  /// default operator = for the same type
  void operator = ( const AutoArray<Type,Allocator> &src )
  {
    Copy(src.Data(),src.Size());
  }

  /// assignment for base class arrays
  void operator = ( const base &src )
  {
    Copy(src.Data(),src.Size());
  }

  //! Fast moving of one array into another.
  /*!
    After calling this function source array will be destroyed.
    \param src Source array.
  */
  void Move( AutoArray &src );

  //! Destruction and deallocation
  void Clear(){DoDestruct();}
  //! Copying of n items from the source array.
  /*!
    \param src Source array.
    \param n Number of items.
  */
  void Copy( const Type *src, int n );
  //! Using allocator of this array this function allocates a specified memory.
  /*!
    \param n Number of items to be allocated.
    \return Reference to allocated memory.
  */
  Type *MemAlloc( int &n )
  {
    if (n==0) return NULL;
    size_t size=n*sizeof(Type),size0=size;
    #if ALLOC_DEBUGGER && defined _CPPRTTI
    Type *ret=(Type *)AllocatorType::Alloc(size,__FILE__,__LINE__,typeid(Type).name());
    //return (Type *)new(FileLine(typeid(Type).name()),sizeof(Type)) char[n*sizeof(Type)];
    #else
    Type *ret=(Type *)AllocatorType::Alloc(size);
    #endif
    if( size!=size0 ) n=size/sizeof(Type);
    return ret;
  }
  //! Using allocator of this array this function allocates a specified memory.
  /*!
    \param n Number of items to be allocated.
    \return Reference to allocated memory.
  */
  Type *MemRealloc( Type *old, int oldN, int &n )
  {
    Assert(n!=0);
    Assert(old!=0);
    Assert(oldN!=0);
    size_t oldSize=oldN*sizeof(Type);
    size_t size=n*sizeof(Type),size0=size;
    Type *ret=(Type *)AllocatorType::Realloc(old,oldSize,size);
    if( ret && size!=size0 ) n=size/sizeof(Type);
    return ret;
  }
  //! Set allocator (most typically allocator holds a pre-reserved memory area - storage).
  /*!
    \param alloc Storage to be set.
  */
  void SetStorage( const Allocator &alloc )
  {
    AllocatorType::operator = (alloc);
  }
  
  /// size of the pre-reserved area, 0 if no is available
  int StorageSize() const {return Allocator::StorageSize();}
  /// number of elements in the pre-reserved area, 0 if no is available
  int StorageCount() const {return StorageSize()/sizeof(Type);}
  //! This method will destroy current allocator of this object and replaces it with default memory management.
  /*!
    Data remains valid and unchanged
  */
  void UnlinkStorage()
  {
    AllocatorType::Unlink(base::_data);
  }
  //! Using allocator of this array this function deallocates a specified memory.
  /*!
    \param mem Memory to be deallocated.
    \n Number of items to be deleted.
  */
  void MemFree( Type *mem, int n ) {AllocatorType::Free(mem,n*sizeof(Type));}
  //! Allocation of space for n items.
  /*!
    \param n Number of items the space will be allocated for.
  */
  void Init( int n );
  //! Grants access to the n-th item of the array. Resizes the array if necessary
  /*!
    \param n Item which can be accessed after calling of this method
    \sa Resize()
  */
  void Access( int n );
  //! Reallocation of array only in case actual size is smaller than sizeNeed. This function never shrinks the size of the array. Therefore is it faster than Realloc(int int).
  /*!
    \param sizeNeed When less then this, allocate
    \param sizeWant When allocating, allocate at least this
    \sa Realloc
  */
  void Reserve( int sizeNeed, int sizeWant );
  //! Reallocation of array only in case actual size is smaller than sizeNeed. This function never shrinks the size of the array. Therefore is it faster than Realloc(int int).
  /*!
    \param sizeNeed When less then this, allocate
    \sa Realloc
  */
  void Reserve(int sizeNeed);
  //! Reallocation of array only in case actual size is out of interval <sizeNeed, sizeWant>.
  /*!
    In case there will be a new allocation, new size will be sizeWant.
    \param sizeNeed Lower bound of the interval.
    \param sizeWan Upper bound of the interval.
    \sa Reserve
  */
  void Realloc( int sizeNeed, int sizeWant );
  //! Reallocation of array to new length specified by size.
  /*!
    \param size New number of items to be allocated.
  */
  void Realloc( int size );
  //! Resizing of this array to n items.
  /*!
    In case we will need new allocation there will be allocated at least MinGrow items.
    \param n New number of items to be allocated.
  */
  void Resize( int n );
  //! Resizing of this array to n items, any newly added values will be initialized to a value provided
  /*!
    In case we will need new allocation there will be allocated at least MinGrow items.
    \param n New number of items to be allocated.
  */
  void ResizeWithInit( int n, const Type &src );
  //! Resize the array w/o construction/destruction control. Dangerous! You MUST know what are you doing!
  /*!
    Does no array-resizing!
    \param n New number of controlled items in the array.
  */
    void UnsafeResize( int n );

  //! Shrink the length of the array just to fit actual number of items.
  /*!
    \sa _n, _maxN
  */
  void Compact(){Realloc(base::_n);}
  /// compact, but only if the overhead is too big
  void CompactIfNeeded(int maxOverhead=2, int maxMembers=0)
  {
    if (_maxN>base::_n*maxOverhead && _maxN>maxMembers)
    {
      Realloc(base::_n);
    }
  }
  //! This function will call first destructors on all items and consequently default constructors.
  /*!
    Note that this method is currently used nowhere.
  */
  void InitEmpty(){CTraits::DestructArray(base::_data,base::_n);CTraits::ConstructArray(base::_data,base::_n);}
  //! Adding of an item without resizing the field.
  /*!
    \param src Source item to be added.
    \return New number of items in the array.
  */
  int AddFast(const Type &src)
  {
    Assert( base::_n<_maxN );
    CTraits::CopyConstruct(base::_data[base::_n],src);
    return base::_n++;
  }
  //! Adding of an item without resizing the field.
  /*!
    \return New number of items in the array.
  */
  int AddFast()
  {
    Assert( base::_n<_maxN );
    CTraits::Construct(base::_data[base::_n]);
    return base::_n++;
  }
  //! Adding of an item with resizing of the field.
  /*!
    \param src Source item to be added.
    \return New number of items in the array.
  */
  int Add(const Type &src);

  /// Adding of an item with resizing of the field, with move semantics.
  /**
    \param src Source item to be added.
    \return New number of items in the array.
    
  Intended to provide an optimization possibility for movable types. Item is moved and source item is default constructed.
  */
  int AddMove(Type &src);
  //! Adding of an item with resizing the field.
  /*!
    \return New number of items in the array.
  */
  int Add();
  //! Adding of an item with resizing the field.
  /*!
    \return Reference to new item.
  */
  Type &Append() {return this->Set(this->Add());}
  //! Adding of an item with resizing the field.
  /*!
  \return Reference to new item.
  */
  Type &AppendFast() {return this->Set(this->AddFast());}
  //! Adding of an item without resizing the field.
  /*!
    \return Reference to new item.
  */
  Type &Append(const Type &val) {return this->Set(this->Add(val));}
#ifdef _WIN32
  //! Deleting of specified number of items at the specified position.
  /*!
    \param index Index where the data to be deleted lies.
    \param count Number of items to be deleted.
  */
  void Delete( int index, int count=1 );
#else
  //! Deleting of specified number of items at the specified position.
  /*!
    \param index Index where the data to be deleted lies.
    \param count Number of items to be deleted.
  */
  void Delete( int index, int count );
  //! Deleting one item at the specified position.
  /*!
    \param index Index where the data to be deleted lies.
  */
  inline void Delete( int index )
  {
      Delete(index,1);
  }
#endif
  //! Inserting of one item at specified position.
  /*!
    \param index Index to position where new item should be inserted.
    \param src Item to be inserted.
  */
  void Insert( int index, const Type &src );
  //! Inserting of one position at specified position.
  /*!
    \param index Index to position where new item should be inserted.
  */
  Type &Insert( int index );

  /// Insert count of items starting on index position 
  void InsertMultiple(int index, int count);

  //! provide public access to the data type
  typedef Type DataType;
  //! provide public access to the allocator type
  typedef Allocator AllocatorType;

  //! Retrieving already allocated size of the array.
  int MaxSize() const {return _maxN;}
  //! QuickSort
  /*!
    Note that QSortBin is very dangerous. Use QSort from <class/QSort.hpp> if possible.
    \param compare Reference to function which compare 2 items v0 and v1
    \param count Number of items to sort. -1 means that whole array should be sorted.
  */
  void QSortBin( int (__cdecl *compare)( const void *v0, const void *v1 ), int count=-1 );
  //typedef int (*COMPARE_FUNC_PTR)( const void *v0, const void *v1 );
  //void QSortBin( COMPARE_FUNC_PTR compare, int count=-1 );
  size_t GetMemoryAllocated() const {return MaxSize()*sizeof(Type);}
  //! calculate memory pointed by this object
  double GetMemoryUsed() const;

  using base::SetAll;
  /// resize and assign all elements the same value
  void SetAll(int n, const Type &val)
  {
    Resize(n);
    for (int i=0; i<base::Size(); i++) base::Set(i)=val;
  }

  ///Concat two arrays, so that it appends specified array to the current array
  /** 
  @param other array that will be appended to current array
  */
  void Append(const base& other)
  {
    int sizeNeed=base::Size()+other.Size();
    // make sure we have a memory to copy to
    Reserve(sizeNeed);
    // change controlled items count
    CTraits::CopyConstruct(base::_data+base::_n,other.Data(),other.Size());
    base::_n += other.Size();
  }

// some template construct are not supported by MSVC 6.0
#if _MSC_VER>=1300 || defined(__GNUC__)
  ///Concat two arrays, so that it appends specified array to the current array
  /** 
  @param other array that will be appended to current array
  */
  template <class ArrayType>
    void Merge(const ArrayType &other)
  {
    int sizeNeed=base::Size()+other.Size();
    // make sure we have a memory to copy to
    Reserve(sizeNeed);
    // change controlled items count
    CTraits::CopyConstruct(base::_data+base::_n,other.Data(),other.Size());
    base::_n += other.Size();
  }

  /// perform given function on all data members, delete any data as required by the functor
  /** functor indicates the member should be deleted by returning true */
  template <class Functor>
  void ForEachWithDelete(const Functor &f)
  {
    int dst = 0;
    for (int i=dst; i<base::Size(); i++)
    {
      const Type &item = base::Get(i);
      bool deleteIt = f(item);
      if (!deleteIt)
      {
        if (dst!=i) base::Set(dst) = item;
        dst++;
      }
    }
    Resize(dst);
  }
#endif


  ClassIsMovable(AutoArray);
};

template<class Type, class Allocator=MemAllocD>
class StackArray: public AutoArray<Type, Allocator>
{
  typedef AutoArray<Type, Allocator> base;
public:
  StackArray() : base() {}

  //! Put the item on the stack
  void Push(const Type &src) { (void) base::Add(src); }
  //! Get the item from the stack (but do not delete it)
  const Type &Top() const {return base::Get(base::_n-1);}
  //! Delete the item from the stack
  void Pop() { base::Delete(base::_n-1); }
  //! Test whether the stack is empty
  bool Empty() const {return base::Size()==0; }

  ClassIsMovable(StackArray);
};
#include <Es/Memory/debugNew.hpp>

template<class Type,class Allocator>
void AutoArray<Type,Allocator>::DoConstruct()
{
  base::_data=NULL,base::_n=_maxN=0;
}

template<class Type,class Allocator>
AutoArray<Type,Allocator>::~AutoArray()
{
  DoDestruct();
}

template<class Type,class Allocator>
AutoArray<Type,Allocator>::AutoArray():base(base::NoInit) {DoConstruct();}

template<class Type,class Allocator>
AutoArray<Type,Allocator>::AutoArray( int n ):base(base::NoInit) {DoConstruct();Realloc(n);}

template<class Type,class Allocator>
AutoArray<Type,Allocator>::AutoArray(int n, const Type &value):base(base::NoInit)
{
  DoConstruct();
  Realloc(n);
  SetAll(n,value);
}

template<class Type,class Allocator>
AutoArray<Type,Allocator>::AutoArray( const Type *src, int n ):base(base::NoInit) {DoConstruct();Copy(src,n);}

template<class Type,class Allocator>
AutoArray<Type,Allocator>::AutoArray( const AutoArray &src ):base(base::NoInit) 
{
  DoConstruct();
  Copy(src.Data(),src.Size());
}

template<class Type,class Allocator>
AutoArray<Type,Allocator>::AutoArray( const base &src ):base(base::NoInit) 
{
  DoConstruct();
  Copy(src.Data(),src.Size());
}

template<class Type,class Allocator>
void AutoArray<Type,Allocator>::Move( AutoArray &src )
{
  Clear(); // free any data in target
  // transfer control from src to target
  base::_data=src._data;
  base::_n=src._n;
  _maxN=src._maxN;
  Allocator::operator = (src);
  // mark src as clear
  src._data=NULL;
  src._n=0;
  src._maxN=0;
}

template<class Type,class Allocator>
void AutoArray<Type,Allocator>::Copy( const Type *src, int n )
{
  Reserve(n);
  // copy construct old items
  int keepControlled = base::_n;
  if (keepControlled>n) keepControlled = n;
  CTraits::CopyData(base::_data,src,keepControlled);
  if (n>base::_n)
  {
    // copy construct newly controlled items
    CTraits::CopyConstruct(base::_data+base::_n,src+base::_n,n-base::_n);
    base::_n = n;
  }
  else
  {
    // destruct items now uncontrolled
    Resize(n);
  }
}

template<class Type,class Allocator>
void AutoArray<Type,Allocator>::Realloc( int size )
{
  if( _maxN!=size )
  {
    // if shortening, be careful when shortening below size
    if( base::_n>size )
    {
      //Fail("Realloc intro controlled area");
      Resize(size);
    }
    // first we will ask the allocator to realloc the current block
    // if that fails, we have to move on our own
    if (base::_data && size>0 && MemRealloc(base::_data,_maxN,size))
    {
      // if sucessfull, we are done
      _maxN = size;
      return;
    }
    Type *newData = MemAlloc(size);
    if( base::_data )
    {
      // move data
      CTraits::MoveData(newData,base::_data,base::_n);
      MemFree(base::_data,_maxN);
    }
    base::_data=newData;
    _maxN=size;
  }
}

template<class Type,class Allocator>
void AutoArray<Type,Allocator>::Realloc( int sizeNeed, int sizeWant )
{
  if( _maxN<sizeNeed || _maxN>sizeWant )
  {
    Realloc(sizeWant);
  }
}

template<class Type,class Allocator>
inline void AutoArray<Type,Allocator>::Reserve( int sizeNeed, int sizeWant )
{
  if( _maxN<sizeNeed ) Realloc(sizeWant);
}

template<class Type,class Allocator>
inline void AutoArray<Type,Allocator>::Reserve(int sizeNeed)
{
  if( _maxN<sizeNeed ) Realloc(sizeNeed);
}

template<class Type,class Allocator>
inline void AutoArray<Type,Allocator>::Access( int i )
{
  if( base::_n<i+1 ) Resize(i+1);
}

template<class Type,class Allocator>
void AutoArray<Type,Allocator>::Init( int size )
{
  if( _maxN!=size )
  {
    Clear();
    Realloc(size);
  }
  else
  {
    Resize(0);
  }
}

template<class Type,class Allocator>
inline void AutoArray<Type,Allocator>::Resize( int n )
{
  if ( n < 0 )
  {
    Fail("Error: can't resize AutoArray to negative size!");
  }
  if( n>_maxN )
  {
    /*
    int grow=_maxN;
    if( grow<MinGrow() ) grow=MinGrow();
    // grow 2x on each reallocation
    Realloc(n,n-1+grow);
    */
    // growing twice is not very smart, as it can leave a lot of unusable holes
    // we prefer growing 1.5x
    int grow=_maxN>>1;
    if( grow<MinGrow() ) grow=MinGrow();
    Realloc(n,n-1+grow);
    Assert(_maxN>=n);
  }
  // change length of controlled items
  if( n>base::_n ) CTraits::ConstructArray(base::_data+base::_n,n-base::_n);
  else if( base::_n>n ) CTraits::DestructArray(base::_data+n,base::_n-n);
  base::_n=n;
}

template<class Type,class Allocator>
inline void AutoArray<Type,Allocator>::ResizeWithInit( int n, const Type &src )
{
  int oldSize = _n;
  Resize(n);
  if (n>oldSize)
  {
    for (int i=oldSize; i<n; i++) Set(i) = src;
  }
}

template<class Type,class Allocator>
void AutoArray<Type,Allocator>::UnsafeResize( int n )
{
  if( n>_maxN )
  {
    int grow=_maxN>>1;
    if( grow<MinGrow() ) grow=MinGrow();
    Realloc(n,n-1+grow);
  }
  if ( (base::_n = n) < 0 ) base::_n = 0;
}

template<class Type,class Allocator>
int AutoArray<Type,Allocator>::Add(const Type &src)
{
  // make sure we have a space to add to
  if( base::_n>=_maxN )
  {
    int grow = _maxN>>1;
    if( grow<MinGrow() ) grow = MinGrow();
    Realloc(base::_n+1,base::_n+grow);
  }
  return AddFast(src);
}

template<class Type,class Allocator>
int AutoArray<Type,Allocator>::AddMove(Type &src)
{
  // make sure we have a space to add to
  if( base::_n>=_maxN )
  {
    int grow = _maxN>>1;
    if( grow<MinGrow() ) grow = MinGrow();
    Realloc(base::_n+1,base::_n+grow);
  }
  Assert( base::_n<_maxN );
  // move the item - destroy the source one
  CTraits::MoveData(base::_data+base::_n,&src,1);
  // as we cannot leave the source item uncontrolled, use default contstuctor to init it again
  CTraits::Construct(base::_data[base::_n]);
  
  return base::_n++;
}

template<class Type,class Allocator>
int AutoArray<Type,Allocator>::Add()
{
  if( base::_n<_maxN )
  {
    return this->AddFast();
  }
  else
  {
    int s=base::_n;
    this->Resize(s+1);
    return s;
  }
}

template<class Type,class Allocator>
void AutoArray<Type,Allocator>::Delete(int index, int count)
{
  if( index<0 || index+count>base::_n )
  {
    Fail("Array index out of range");
    return;
  }
  CTraits::DestructArray(base::_data+index,count);
  CTraits::DeleteData(base::_data+index,base::_n-index,count);
  base::_n-=count;
}

template<class Type,class Allocator>
void AutoArray<Type,Allocator>::Insert( int index, const Type &src )
{
  Resize(base::_n+1); // note: _n is changed by Resize
  CTraits::InsertData(base::_data+index,base::_n-index);
  CTraits::CopyConstruct(base::_data[index],src);
}

template<class Type,class Allocator>
Type &AutoArray<Type,Allocator>::Insert( int index )
{
  Resize(base::_n+1); // note: _n is changed by Resize
  CTraits::InsertData(base::_data+index,base::_n-index);
  CTraits::Construct(base::_data[index]);
  return base::_data[index];
}

template<class Type,class Allocator>
void AutoArray<Type,Allocator>::InsertMultiple(int index, int count)
{
  Resize(base::_n + count); // note: _n is changed by Resize
  CTraits::InsertData(base::_data + index, base::_n - index, count);
  CTraits::ConstructArray(base::_data + index, count);
}

template<class Type,class Allocator>
void AutoArray<Type,Allocator>::QSortBin
(
  int (__cdecl *compare)( const void *v0, const void *v1 ), int count
)
{
  int size=base::Size();
  if( count<0 ) count=size;
  else if( count>size) count=size;
  qsort(base::_data,count,sizeof(base::_data[0]),compare);
}


template<class Type,class Allocator>
void AutoArray<Type,Allocator>::DoDestruct()
{
  Resize(0);
  if( base::_data )
  {
    MemFree(base::_data,_maxN);
    base::_data=NULL,_maxN=0;
  }
}

template<class Type,class Allocator>
double AutoArray<Type,Allocator>::GetMemoryUsed() const
{
  double res = _maxN*sizeof(Type);
  // we cannot assume items have GetMemoryUsed member
  for (int i=0; i<base::Size(); i++)
  {
    res += GetMemoryUsedTraits<Type>::GetMemoryUsed(base::Get(i));
  }
  return res;
}

/// FindArrayKey is nicer implementation using traits
template <class Type, class Allocator=MemAllocD>
class FindArray: public AutoArray<Type,Allocator>
{
  typedef AutoArray<Type,Allocator> base;
  public:
  int AddUnique( const Type &src );
  int Find( const Type &src ) const;
  /// if it already exists, return existing entry. If not, create a new one
  int FindOrAdd( const Type &src );
  bool Delete( const Type &src );
  void Delete( int index ) {AutoArray<Type,Allocator>::Delete(index,1);}
  void DeleteAt( int index ) {AutoArray<Type,Allocator>::Delete(index,1);}
  void DeleteAt( int index, int count ) {AutoArray<Type,Allocator>::Delete(index,count);}
  
  #if (defined (_MSC_VER) && _MSC_VER>=1300) || defined (__GNUC__)
  /// AddUnique all elements from another array
  template <class ArrayType>
  void MergeUnique(const ArrayType &src)
  {
    for (int i=0; i<src.Size(); i++)
    {
      this->AddUnique(src[i]);
    }
  }
  #endif
  
  ClassIsMovable(FindArray);
};

template<class Type,class Allocator>
int FindArray<Type,Allocator>::Find( const Type &src ) const
{
  for( int i=0; i<base::Size(); i++ ) if( (*this)[i]==src ) return i;
  return -1;
}

template<class Type,class Allocator>
int FindArray<Type,Allocator>::AddUnique( const Type &src )
{
  if( Find(src)>=0 ) return -1;
  return this->Add(src);
}

template<class Type,class Allocator>
int FindArray<Type,Allocator>::FindOrAdd( const Type &src )
{
  int index = Find(src);
  if (index>=0 ) return index;
  return this->Add(src);
}

template<class Type,class Allocator>
bool FindArray<Type,Allocator>::Delete( const Type &src )
{
  int index=Find(src);
  if( index<0 ) return false;
  this->Delete(index);
  return true;
}

/// traits for key comparion and searching
template <class Type>
struct FindArrayKeyTraits
{
  typedef const Type &KeyType;
  /// check if two keys are equal
  static bool IsEqual(KeyType a, KeyType b) {return a==b;}
  /// get a key from an item
  static KeyType GetKey(const Type &a) {return a;}
};

//! array with searching based on key
template <class Type, class Traits=FindArrayKeyTraits<Type>, class Allocator=MemAllocD>
class FindArrayKey: public AutoArray<Type,Allocator>
{
  typedef AutoArray<Type,Allocator> base;

  public:
  typedef typename Traits::KeyType KeyType;
  
  /// if it already exists, return existing entry. If not, create a new one
  int FindOrAdd( const Type &src );
  //! add unless there already is an item with the same key
  int AddUnique( const Type &src );
  //! find first item with the same key
  int Find( const Type &src ) const {return FindKey(Traits::GetKey(src));}
  //! find item by key
  int FindKey(KeyType key) const;
  //! delete first item with the same key
  bool Delete( const Type &src ){return DeleteKey(Traits::GetKey(src));}
  //! delete first item with given key
  bool DeleteKey(KeyType src);
  //! delete all items with given key
  void DeleteAllKey(KeyType src);
  void DeleteAt( int index ) {AutoArray<Type,Allocator>::Delete(index,1);}
  void DeleteAt( int index, int count ) {AutoArray<Type,Allocator>::Delete(index,count);}

  #if (defined (_MSC_VER) && _MSC_VER>=1300) || defined (__GNUC__)
  /// AddUnique all elements from another array
  template <class ArrayType>
  void MergeUnique(const ArrayType &src)
  {
    for (int i=0; i<src.Size(); i++)
    {
      AddUnique(src[i]);
    }
  }
  #endif
  
  ClassIsMovable(FindArrayKey);
};

template<class Type,class Traits,class Allocator>
int FindArrayKey<Type,Traits,Allocator>::FindKey(KeyType key) const
{
  //for( int i=0; i<base::Size(); i++ ) if( (*this)[i]==src ) return i;
  for( int i=0; i<base::Size(); i++ ) if( Traits::IsEqual(Traits::GetKey(base::Get(i)),key) ) return i;
  return -1;
}

template<class Type,class Traits,class Allocator>
int FindArrayKey<Type,Traits,Allocator>::AddUnique( const Type &src )
{
  if( Find(src)>=0 ) return -1;
  return this->Add(src);
}

template<class Type,class Traits,class Allocator>
int FindArrayKey<Type,Traits,Allocator>::FindOrAdd( const Type &src )
{
  int index = Find(src);
  if (index>=0 ) return index;
  return this->Add(src);
}

template<class Type,class Traits,class Allocator>
bool FindArrayKey<Type,Traits,Allocator>::DeleteKey(KeyType key)
{
  int index=FindKey(key);
  if( index<0 ) return false;
  DeleteAt(index);
  return true;
}

template<class Type,class Traits,class Allocator>
void FindArrayKey<Type,Traits,Allocator>::DeleteAllKey(KeyType key)
{
  int d=0;
  for( int s=0; s<base::Size(); s++ )
  {
    if( !Traits::IsEqual(Traits::GetKey(base::Get(s)),key) )
    {
      if( s!=d ) base::Set(d)=base::Get(s);
      d++;
    }
  }
  base::Resize(d);
}

template <class Type>
struct RefArrayKeyTraits
{
  typedef const Type *KeyType;
  static bool IsEqual(KeyType a, KeyType b)
  {
    return a==b;
  }
  static KeyType GetKey(const Ref<Type> &a) {return a.GetRef();}
};

#if !defined (_MSC_VER) || _MSC_VER>=1300
/// partial specialization of GetMemoryUsedTraits for Ref<>
template <class Type>
struct GetMemoryUsedTraits< class Ref<Type> >
{
  static double GetMemoryUsed(const Ref<Type> &src)
  {
    return src.GetMemoryUsed();
  }
};
#endif

template <class Type,class Allocator=MemAllocD>
class RefArray: public FindArrayKey< Ref<Type>, RefArrayKeyTraits<Type>, Allocator >
{
  typedef Ref<Type> RefType;
  typedef FindArrayKey< RefType, RefArrayKeyTraits<Type>, Allocator > base;
  typedef RefArrayKeyTraits<Type> traits;

  public:
  //! delete at given location
  __forceinline void Delete( int index, int count ) {base::DeleteAt(index,count);}
  //! delete at given location
  __forceinline void Delete( int index ) {base::DeleteAt(index);}
  //! delete, given pointer
  __forceinline bool Delete(const Type *src) {return this->DeleteKey(src);}
  //! delete, given Ref
  __forceinline bool Delete(const RefType &src) const {return this->DeleteKey(traits::GetKey(src));}

  //! if Ref is provided, get a key from it
  __forceinline int Find(const RefType &src) const {return base::FindKey(traits::GetKey(src));}
  //! if pointer is provided to search, avoid temporary Ref creation
  __forceinline int Find(const Type *src) const {return base::FindKey(src);}

  /// add, reuse any NULL slots if possible
  int AddReusingNull(Type *item)
  {
    int free = base::FindKey(NULL);
    if (free>=0)
    {
      base::Set(free) = item;
      return free;
    }
    else
    {
      return this->Add(item);
    }
  }
  void RemoveNulls();

  ClassIsMovable(RefArray);
};


template<class Type,class Allocator>
void RefArray<Type,Allocator>::RemoveNulls()
{
  base::DeleteAllKey(NULL);
}

/// by default we assume heap stores pointers to value, operator < is defined on values
template <class Type>
struct HeapTraits
{
  // default traits: Type is pointer to actual value
  static bool IsLess(const Type &a, const Type &b)
  {
    return *a < *b;
  }
};

/// integer heap traits: Type is actual value
template <>
struct HeapTraits<int>
{
  static bool IsLess(int a, int b) {return a<b;}
};

/// heap order maintanined in array
/**
Type can be pointer (*, Ptr, Ref etc.) or value
*/

template <class Type,class Allocator=MemAllocD,class Traits=HeapTraits<Type> >
class HeapArray: public FindArrayKey<Type, FindArrayKeyTraits<Type>, Allocator>, public Traits
{
  /// use traits as if it is a functor
  //Traits _traits; 
 typedef FindArrayKey<Type, FindArrayKeyTraits<Type>, Allocator> base;  
 typedef typename base::KeyType KeyType;
public:
  HeapArray(){}
  HeapArray(const Traits &traits):Traits(traits){}
  
  bool HeapGetFirst(Type &item)
  {
    if (base::Size()<= 0) return false;
    item = base::Get(0);
    return true;
  }
  bool HeapRemoveFirst(Type &item);
  bool HeapRemoveFirst();
  void HeapInsert(const Type &item);
  void HeapUpdateUpAt(int index)
  {
    HeapUp(index);
  }
  void HeapUpdateUp(KeyType item);
  void HeapUpdateDown(KeyType item);
  void HeapDelete(KeyType item);
  void HeapDeleteAt(int index);

  // verify heap order
  bool CheckIntegrity() const;
protected:
  void Swap(int i, int j);
  int GetParent(int i) const;
  int GetFirstSon(int i) const;
  int GetBrother(int i) const;
  void HeapDown(int i);
  void HeapUp(int i);
  
  ClassIsMovable(HeapArray);
};

template <class Type,class Allocator,class Traits>
inline void HeapArray<Type,Allocator,Traits>::Swap(int i, int j)
{
  Type temp = base::Get(i);
  base::Set(i) = base::Get(j);
  base::Set(j) = temp;
}

template <class Type,class Allocator,class Traits>
inline int HeapArray<Type,Allocator,Traits>::GetParent(int i) const
{
  if (i <= 0)
    return -1;
  else
    return (i - 1) / 2;
}

template <class Type,class Allocator,class Traits>
inline int HeapArray<Type,Allocator,Traits>::GetFirstSon(int i) const
{
  int n = 2 * i + 1;
  if (n >= base::Size())
    return -1;
  else
    return n;
}

template <class Type,class Allocator,class Traits>
inline int HeapArray<Type,Allocator,Traits>::GetBrother(int i) const
{
  int n = i + 1;
  if (n >= base::Size())
    return -1;
  else
    return n;
}

#ifdef USE_SWAP

template <class Type,class Allocator,class Traits>
void HeapArray<Type,Allocator,Traits>::HeapDown(int i)
{
  while (true)
  {
    int n = GetFirstSon(i);
    if (n < 0) return;
    int m = GetBrother(n);
    if (m >= 0)
    {
      if (Traits::IsLess(base::Get(m),base::Get(n))) n = m;
    }
    if (!Traits::IsLess(base::Get(n),base::Get(i))) return;
    Swap(i, n);
    i = n;
  }
}

template <class Type,class Allocator,class Traits>
void HeapArray<Type,Allocator,Traits>::HeapUp(int i)
{
  while (true)
  {
    int n = GetParent(i);
    if (n < 0) return;
    if (!Traits::IsLess(base::Get(i),base::Get(n))) return;
    Swap(n, i);
    i = n;
  }
}

#else

template <class Type,class Allocator,class Traits>
void HeapArray<Type,Allocator,Traits>::HeapDown(int i)
{
  int down = GetFirstSon(i);
  if ( down < 0 ) return;
  int down2 = GetBrother(down);
  if ( down2 >= 0 && Traits::IsLess(base::Get(down2),base::Get(down)) )
    down = down2;
    // [down] is smallest of both sons
  if ( Traits::IsLess(base::Get(down),base::Get(i)) )
  {   // will permute some items: tmp == base::Get(i) will go down
    Type tmp = base::Get(i);
    do  // move one item up ([down] -> [i])
    {
      base::Set(i) = base::Get(down);
      i = down;
      down = GetFirstSon(i);
      if ( down < 0 ) break;
      down2 = GetBrother(down);
      if ( down2 >= 0 && Traits::IsLess(base::Get(down2),base::Get(down)) )
        down = down2;
        // [down] is smallest of both sons
    } while ( Traits::IsLess(base::Get(down),tmp) );
      // .. and finally set tmp to [i]
    base::Set(i) = tmp;
  }
}

template <class Type,class Allocator,class Traits>
void HeapArray<Type,Allocator,Traits>::HeapUp(int i)
{
  int up = GetParent(i);
  if ( up >= 0 && Traits::IsLess(base::Get(i),base::Get(up)) )
  {   // will permute some items: tmp == base::Get(i) will go up
    Type tmp = base::Get(i);
    do  // move one item down ([up] -> [i])
    {
      base::Set(i) = base::Get(up);
      i = up;
      up = GetParent(i);
        // test if tmp goes on:
    } while ( up >= 0 && Traits::IsLess(tmp,base::Get(up)) );
      // .. and finally set tmp to [i]
    base::Set(i) = tmp;
  }
}

#endif

template <class Type,class Allocator,class Traits>
bool HeapArray<Type,Allocator,Traits>::CheckIntegrity() const
{
  for (int i=1; i<base::Size(); i++)
  {
    int up = GetParent(i);
    Assert(up>=0); // parent always exists
    if (Traits::IsLess(base::Get(i),base::Get(up)))
    {
      LogF(" heap order broken: %d,%d",i,up);
      return false;
    }
  }
  return true;
}

template <class Type,class Allocator,class Traits>
bool HeapArray<Type,Allocator,Traits>::HeapRemoveFirst(Type &item)
{
  int n = base::Size() - 1;
  if (n < 0) return false;
  item = base::Get(0);
  Swap(0, n);
  base::DeleteAt(n);
  HeapDown(0);
  return true;
}

template <class Type,class Allocator,class Traits>
bool HeapArray<Type,Allocator,Traits>::HeapRemoveFirst()
{
  int n =base:: Size() - 1;
  if (n < 0) return false;
  Swap(0, n);
  base::DeleteAt(n);
  HeapDown(0);
  return true;
}

template <class Type,class Allocator,class Traits>
void HeapArray<Type,Allocator,Traits>::HeapInsert(const Type &item)
{
  int n = this->Add(item);
  HeapUp(n);
}

template <class Type,class Allocator,class Traits>
void HeapArray<Type,Allocator,Traits>::HeapUpdateUp(KeyType item)
{
  int n = this->Find(item);
  if (n >= 0)
    HeapUp(n);
}

template <class Type,class Allocator,class Traits>
void HeapArray<Type,Allocator,Traits>::HeapUpdateDown(KeyType item)
{
  int n = this->Find(item);
  if (n >= 0)
    HeapDown(n);
}

template <class Type,class Allocator,class Traits>
void HeapArray<Type,Allocator,Traits>::HeapDelete(KeyType item)
{
  int n = this->Find(item);
  if (n >= 0)
    HeapDeleteAt(n);
}

template <class Type,class Allocator,class Traits>
void HeapArray<Type,Allocator,Traits>::HeapDeleteAt(int index)
{
  int n = base::Size() - 1;
  if (index > n) return;
  Swap(index, n);
  base::DeleteAt(n);

  // if we are deleting the last item, ordering is never changed
  if (index<n)
  {
    int parent = GetParent(index);
    if (parent < 0 || Traits::IsLess(base::Get(parent),base::Get(index)))
      HeapDown(index);
    else
      HeapUp(index);
  }
}



#endif
