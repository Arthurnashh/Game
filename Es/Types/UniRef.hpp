#pragma once



///Traits for UniRef class
/**
 * Use this class to define Threading. This class is best in single threading, because
 * it not provides any locking schema
 */
class UniRefNoThreading
{
public:
  ///Locks the object. 
  /**
   * In this class, function is empty 
   */
  void Lock() const {}
  ///Unlocks the object. 
  /**
  * In this class, function is empty 
  */
  void Unlock() const {}
};

///Traits for UniRef class
/**
 * Use this class to define DeletingTraits. Class provides standard deleting using the
 * operator @b delete
 */
template<class Type>
class UniRefDefDelete
{
public:
  void operator()(Type *ptr) {delete ptr;}
};

///Traits for UniRef class
/**
* Use this class to define DeletingTraits. Class provides standard @b array deleting using the
* operator @b delete []
*/
template<class Type>
class UniRefArrayDelete
{
public:
  void operator()(Type *ptr) {delete [] ptr;}
};



///Traits for UniRef class
/**
 * Use this class to define Compression traits
 * Class provides none compression. 
 */
template<class Type>
class UniRefNoCompression
{
  Type *_ptr;
  UniRefNoCompression<Type> *_next;
  UniRefNoCompression<Type> *_prev;
  void Construct(const UniRefNoCompression<Type> &other)
  {
    _next=const_cast<UniRefNoCompression<Type> *>(&other);
    _prev=other._prev;
    if (_prev==0) _prev=const_cast<UniRefNoCompression<Type> *>(&other);

    _next->_prev=this;
    _prev->_next=this;
  }
  void Destruct()
  {
    if (_next) _next->_prev=_prev;
    if (_prev) _prev->_next=_next;    
  }

public:
  UniRefNoCompression(Type *ptr=0):_ptr(ptr),_next(0),_prev(0) {}
  UniRefNoCompression(const UniRefNoCompression<Type> &other):_ptr(other._ptr)
  {
    Construct(other);
  }
  ~UniRefNoCompression()
  {
    Destruct();
  }
  bool IsSingle() const
  {
    return (_next==0 || _next==this);
  }
  void operator=(Type *ptr)
  {
    Destruct();
    _next=_prev=0;
    _ptr=ptr;
  }
  void operator=(const UniRefNoCompression<Type> &other)
  {
    Destruct();
    _ptr=other._ptr;
    Construct(other);
  }

  Type *GetPtr() const {return _ptr;}

  void SetPtr(Type *value)
  {
    UniRefNoCompression *x=this;
    do 
    {
      x->_ptr=value;
      x=x->_next;
    } 
    while(x!=this);
  }
};


///Traits for UniRef class
/**
* Use this class to define Compression traits
* Class provides simple compression. Best in debug mode
*/
template<class Type>
class UniRefDebugCompressed
{
  Type *_ptr;
  UniRefDebugCompressed<Type> *_next;

  UniRefDebugCompressed<Type> *GetPrev() const
  {
    if (_next==0) return 0;
    const UniRefDebugCompressed<Type> *x=this;
    while (x->_next!=this) x=x->_next;
    return const_cast<UniRefDebugCompressed<Type> *>(x);
  }
  
  void Construct(const UniRefDebugCompressed<Type> &other)
  {
    _next=const_cast<UniRefDebugCompressed<Type> *>(&other);
    UniRefDebugCompressed<Type> *prev=_next->GetPrev();
    if (prev==0) _next->_next=this;
    else prev->_next=this;
  }
  void Destruct()
  {
    UniRefDebugCompressed<Type> *prev=GetPrev();
    if (prev) prev->_next=_next;
  }
public:
  UniRefDebugCompressed(Type *ptr=0):_ptr(ptr),_next(0) {}
  UniRefDebugCompressed(const UniRefDebugCompressed<Type> &other):_ptr(other._ptr)
  {    
    Construct(other);
  }
  ~UniRefDebugCompressed()
  {
    Destruct();
  }

  bool IsSingle() const
  {
    return (_next==0 || _next==this);
  }

  void operator=(Type *ptr)
  {
    Destruct();
    _next=0;
    _ptr=ptr;
  }
  void operator=(const UniRefDebugCompressed<Type> &other)
  {
    Destruct();
    _ptr=other._ptr;
    Construct(other);
  }

  Type *GetPtr() const {return _ptr;}

  void SetPtr(Type *value)
  {
    UniRefDebugCompressed *x=this;
    do 
    {
      x->_ptr=value;
      x=x->_next;
    } 
    while(x!=this);
  }
};

///Traits for UniRef class
/**
* Use this class to define Compression traits
* Class provides fast compression. Best in release mode
*/
template<class Type>
class UniRefCompressed
{
  UniRefCompressed<Type> *_next;
  unsigned int _prevXorPtr;

  void Construct(const UniRefCompressed<Type> &other)
  {
    Type *obj=other.GetPtr();
    _next=const_cast<UniRefCompressed<Type> *>(&other);
    unsigned int nextPrev=other._prevXorPtr ^ (unsigned int)(other.GetPtr());
    if (nextPrev==0)
    {
      
      _next->_next=this;
      _next->_prevXorPtr ^=(unsigned int)this;
      _prevXorPtr=(unsigned int)_next ^ (unsigned int)obj;
    }
    else
    {    
      _prevXorPtr=other._prevXorPtr;
      UniRefCompressed<Type> *prev=(UniRefCompressed<Type> *)nextPrev;
      prev->_next=this;
      _next->_prevXorPtr^=nextPrev ^ (unsigned int)this;
    }
  }

  void Destruct()
  {
    unsigned int iprev=_prevXorPtr ^ (unsigned int)(GetPtr());
    UniRefCompressed<Type> *prev=(UniRefCompressed<Type> *)iprev;
    if (prev)
    {
      prev->_next=_next;
      _next->_prevXorPtr=_prevXorPtr;
    }
  }

public:
  UniRefCompressed(Type *ptr=0):_prevXorPtr((unsigned int)ptr),_next(0) {}
  UniRefCompressed(const UniRefCompressed<Type> &other)
  {
    Construct(other);
  }
  ~UniRefCompressed()
  {
    Destruct();
  }

  void operator=(Type *ptr)
  {
    Destruct();
    _next=0;
    _prevXorPtr=(unsigned int)ptr;
  }
  void operator=(const UniRefCompressed<Type> &other)
  {
    Destruct();   
    Construct(other);
  }

  Type *GetPtr() const
  {
    if (_next==0) return (Type *)_prevXorPtr;
    else return (Type *)(_next->_prevXorPtr ^ (unsigned int) this);
  }

  bool IsSingle() const
  {
    return (_next==0 || _next==this);
  }

  void SetPtr(Type *value)
  {
    Type *curPtr=GetPtr();
    UniRefCompressed *x=this;
    do 
    {
      x->_prevXorPtr^=(unsigned int)curPtr ^ (unsigned int)value;
      x=x->_next;
    } 
    while(x!=this);
  }

};

///UniRef ptr is shared ptr without need any counter in shared object
/**
 * It is implemented using the bidirection linked list. As pointers are shared
 * pointers' instances are added into the list. As pointers are destroyed, they
 * are removed from the list. So, when last pointer is removed from list, target
 * object is destroyed.
 *
 * Main advantage is that you don't need any support in target object for this. 
 * Disadvantages are two. First, the pointer instance is bigger then single pointer
 * and second, you cannot move pointer's instance (pointer is not movable)
 *
 * As side effect of list, you can command the all pointers that share the same object
 * to change address of target object, or you can set that pointers to NULL (to
 * notify owners, that object has been destroyed.)
 *
 * @param Type Name of class for target object
 * @param Compression Compression traits. Because uncompressed instance has 12 bytes,
 *  you cen use effective compression algorithm to compress pointer's body by 33% ratio (8 bytes)
 *  Use @b UniRefNoCompression to use pointer in full length (12 bytes). <br>
 *  Use @b UniRefCompressed to use compressed version of pointer (8 bytes). Note that
 *   pointer to target object is unreadable in debugger.<br>
 *  Use @b UniRefDebugCompressed to use compressed pointer's instance in Debug. You
 *  will able to browse target object in Watch window. Don't use it in release,
 *   because operations with this pointer can cost some performance.<br>
 * @param DeleteTraits Specify functor responsible on deleting the target object
 * @param Threading Specify class responsible on multithreading safe.
 *
 */
template<
   class Type, 
#ifdef _DEBUG
     template<class>class Compression=UniRefDebugCompressed,
#else
     template<class>class Compression=UniRefCompressed,
#endif
   template<class>class DeleteTraits=UniRefDefDelete, 
   class Threading=UniRefNoThreading
>
class UniRef: private Compression<Type>,public Threading
{
  class Locker
  {
    const Threading *thr;
  public:
    Locker(const Threading *thr):thr(thr) {thr->Lock();}
    ~Locker() {thr->Unlock();}    
  };

public:
  ///Constructor. 
  /**
   * Use constructor only when you creating first "shared" pointer. Don't use it
   * when target object is already shared!
   *
   * @param ptr Pointer to newly created object that will be owned by this pointer
   */
  UniRef(Type *ptr=0):Compression<Type>(ptr) {}
  ///Sharing pointer
  /**
   * Creates new pointer that shares target object.
   * @param other source pointer that will be shared
   */
  UniRef(const UniRef<Type> &other):Compression<Type>(other) {}
  ///Changes object in pointer
  /**
   * During assignent, previous sharing is terminated. 
   */
  UniRef &operator=(Type *ptr)
  {
    Locker l(this);
    Compression<Type>::operator=(ptr);
    return *this;
  }
  ///Changes object in pointer
  /**
  * During assignent, previous sharing is terminated. 
  */
  UniRef &operator=(const UniRef<Type> &other)
  {
    Locker l(this);
    Compression<Type>::operator=(other);
    return *this;
  }
  ///Terminates sharing for current pointer
  /**
   * Function destroes target object, when this is the last sharing pointer.
   */
  ~UniRef()
  {
    if (IsSingle() && GetPtr()!=0) 
    {
      DeleteTraits<Type> delFunctor;
      delFunctor(GetPtr());
    }
  }

  ///operator for accessing the members in target object
  Type *operator->() const {Locker l(this);Assert(GetPtr()!=0);return GetPtr();}
  ///operator for accessing the array
  Type &operator[](int a) const {Locker l(this);Assert(GetPtr()!=0);return GetPtr()[a];}
  ///conversion operator
  operator Type *() const {Locker l(this);Assert(GetPtr()!=0);return GetPtr();}
  ///derefernce
  Type &operator*() const {Locker l(this);return *GetPtr();}
  
  ///compare
  bool operator==(const UniRef<Type> &other) const {Locker l(this);return GetPtr()==other.GetPtr();}
  ///compare
  bool operator!=(const UniRef<Type> &other) const {Locker l(this);return GetPtr()!=other.GetPtr();}

  ///NULL test
  operator bool() const {Locker l(this);return GetPtr()!=0;}
  ///Compare with general pointer
  bool operator==(void *ptr) const {Locker l(this);return GetPtr()==ptr;}
  ///Compare with general pointer
  bool operator!=(void *ptr) const {Locker l(this);return GetPtr()!=ptr;}

  ///Changes target object in all instances of shared pointer
  /**
   * @param value address of new target object. This value can be NULL. But
   *  then caller is responsible to destroy target object.
   */
  void SetPtr(Type *value)
  {
    Locker l(this);
    Compression<Type>::SetPtr(value);
  }

  ///Tests pointer to NULL, returns true 
  bool IsNull() const  {Locker l(this);return GetPtr()==0;}
  ///Tests pointer to not NULL, returns true
  bool NotNull() const  {Locker l(this);return GetPtr()!=0;}
  ///Returns sharing status
  /**
   * @retval true Pointer is shared, there is more than one instance of the pointer
   * @retval false Pointer is not shared. There is no more instances that share target object.
   */
  bool IsShared() const {Locker l(this);return !IsSingle();}  

};
  
