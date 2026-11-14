#ifdef _MSC_VER
#pragma once
#endif

#ifndef _EL_PARAM_FILE_HPP
#define _EL_PARAM_FILE_HPP

/*!
\file
Various declarations for ParamFile.
*/

#include <Es/Strings/rString.hpp>
#include <Es/Containers/array.hpp>
#include <El/QStream/qStream.hpp>

#include <El/Interfaces/iPreproc.hpp>
#include <El/Interfaces/iLocalize.hpp>
#include <El/Interfaces/iSumCalc.hpp>
#include <El/Interfaces/iEval.hpp>
#include <El/FreeOnDemand/memFreeReq.hpp>

#include "paramFileDecl.hpp"

class IParamArrayValue;


//! Access mode for ParamFile entries

/*!
Used for protection against updating config by add-on mechanism.
*/

enum ParamAccessMode
{
  PADefault=-1, //! no set - any modifications enabled
  PAReadAndWrite, //! any modifications enabled
  PAReadAndCreate, //! only adding new class members is allowed
  PAReadOnly, //! no modifications enabled
  PAReadOnlyVerified //! no modifications enabled, CRC test applied
};

class ParamEntry;
class ParamFile;

//! functor interface - used to check if entry can be accessed
class IParamVisibleTest
{
  public:
  //! check if given entry is visible
  /*!
  \param entry check if this entry is visible
  \return true if visible
  */
  virtual bool operator () (const ParamEntry &entry) = 0;
  //! check if given entry is visible
  /*!
  \param parent parent of entry, is guranteed to be visible
  \param entry check if this entry is visible
  \return true if visible
  */
  virtual bool operator () (const ParamEntry &parent, const ParamEntry &entry) = 0;
};

extern IParamVisibleTest &DefaultAccess;

//! most common implementation of context visibility
//! any entry that belong to one of the listed owners can be accessed

#include <Es/Containers/rStringArray.hpp>

class ParamOwnerList: public IParamVisibleTest
{
  FindArrayRStringBCI _addons;

  public:
  void Clear(){_addons.Clear();}
  void Add(RString addon)
  {
    addon.Lower();
    _addons.AddUnique((const char *)addon);
  }
  int GetSize() const {return _addons.Size();}
  const RStringB &Get(int i) const {return _addons[i];}
  //! check if given entry is visible
  /*!
  \param parent parent of entry, is guranteed to be visible
  \param entry check if this entry is visible
  \return true if visible
  */
  bool operator () (const ParamEntry &parent, const ParamEntry &entry);
  bool operator () (const ParamEntry &entry);
};

class IParamClassSource;
class ParamClassPlaceholder;
class ParamClass;

#include <Es/Types/softLinks.hpp>

class ParamClassId;

//! interface that needs to be implemented if SoftLinkTraits should be used
class ISoftLinkParamClassTraits
{
public:
  typedef ParamClassId LinkId;
  typedef ParamClassId LinkIdStorage;

  virtual LinkId GetLinkId() const = 0;
  virtual void ShallowLockLink() = 0;
  virtual void ShallowUnlockLink() = 0;
  virtual void DeepLockLink() = 0;
  virtual void DeepUnlockLink() = 0;
};

//! common softLink traits
/**
Note: there is no real need for this to be template, but without templates
some functions which are not used are compiled and produce error
and we cannot write function inline as well, because they are compiled immediately
*/

template <class Type, class Base=Type>
struct CommonSoftLinkTraits
{
  typedef typename Base::LinkId LinkId;
  typedef typename Base::LinkIdStorage LinkIdStorage;
  typedef SoftLinkIdTraits<LinkId,LinkIdStorage> IdTraits;

  typedef Base BaseType;
  typedef TrackSoftLinks<IdTraits> Tracker;

  static LinkId GetLinkId(const Base *obj) {return obj->GetLinkId();}
  static LinkId GetNullId() {return IdTraits::GetNullId();}

  static bool NeedsRestoring(const Base *obj) {return obj->NeedsRestoring();}
  static bool IsAlwaysLocked() {return false;}
};
//! SoftLink traits - shallow locking (class only)
template <class Type, class Base=Type>
struct ShallowSoftLinkTraits: public CommonSoftLinkTraits<Type,Base>
{
  typedef typename CommonSoftLinkTraits<Type,Base>::LinkId LinkId;
  static Base *RestoreLink(const LinkId &id){return Type::RestoreLinkShallow(id);}
  static bool RequestRestoreLink(const LinkId &id, bool noRequest=false)
  {
    return Type::RequestRestoreLinkShallow(id,noRequest);
  }

  static void LockLink(Base *obj) {obj->ShallowLockLink();}
  static void UnlockLink(Base *obj) {obj->ShallowUnlockLink();}
};
//! SoftLink traits - deep locking (class including subclasses)
template <class Type, class Base=Type>
struct DeepSoftLinkTraits: public CommonSoftLinkTraits<Type,Base>
{
  typedef typename CommonSoftLinkTraits<Type,Base>::LinkId LinkId;
  static Base *RestoreLink(const LinkId &id){return Type::RestoreLinkShallow(id);}
  static bool RequestRestoreLink(const LinkId &id, bool noRequest=false)
  {
    return Type::RequestRestoreLinkShallow(id,noRequest);
  }

  static void LockLink(Base *obj) {obj->DeepLockLink();}
  static void UnlockLink(Base *obj) {obj->DeepUnlockLink();}
};

//! pointer that keeps class locked while existing
typedef SoftLinkLockedPtr<ParamClass, ShallowSoftLinkTraits<ParamClass,ISoftLinkParamClassTraits> > ParamClassLockedPtr;

//! pointer that keeps class including subclasses locked while existing
typedef SoftLinkLockedPtr<ParamClass, DeepSoftLinkTraits<ParamClass,ISoftLinkParamClassTraits> > ParamClassDeepLockedPtr;

class ParamEntryVal;

class ParamClass;
class ParamEntry;

template <class PEntryType>
class PEntryPtr;

template <class PClassType, class StoreType=ParamClassLockedPtr>
class PClassPtr;

typedef PEntryPtr<ParamEntry> ParamEntryPtr;
typedef PEntryPtr<const ParamEntry> ConstParamEntryPtr;

typedef PClassPtr<ParamClass> ParamClassPtr;
typedef PClassPtr<const ParamClass> ConstParamClassPtr;

typedef PClassPtr<const ParamClass,ParamClassDeepLockedPtr> ConstParamClassDeepPtr;

//! representation of config class

struct FilterNone
{
  static bool IsValid(const ParamEntryVal& entry) {return true;};
};

class ParamEntry;
ParamEntry &operator *(const ParamEntryPtr &src);

class ParamEntry
{
  friend class ParamClass;
public:
  protected:
  RStringB _name;

  void SetName( const RStringB &name ) {_name=name;}
  virtual void SetSource(IParamClassSource *source, bool subentries);
  virtual void SetSource(const ParamEntry &entry);

  private:
  //! disable copy
  ParamEntry(const ParamEntry &src);
  //! disable copy
  void operator = (const ParamEntry &src);
    
  public:
  explicit ParamEntry( const RStringB &name );
  const RStringB &GetName() const {return _name;}
  RString GetContext( const char *member=NULL ) const; // fully qualified name

  virtual bool IsClass() const {return false;}
  /// detect forward declaration
  virtual bool IsDefined() const {return true;}
  /// special entry deleting the original entry
  virtual bool IsDelete() const {return false;}
  virtual const ParamClass *GetClassInterface() const {return NULL;}
  virtual ParamClass *GetClassInterface() {return NULL;}

  virtual const ParamClassPlaceholder *GetPlaceholderInterface() const {return NULL;}
  virtual ParamClassPlaceholder *GetPlaceholderInterface() {return NULL;}

  virtual void DisableStreaming(bool subentries);

  virtual bool IsArray() const {return false;}
  virtual bool IsTextValue() const {return false;}
  virtual bool IsFloatValue() const {return false;}
  virtual bool IsIntValue() const {return false;}
  virtual bool IsInt64Value() const {return false;}
  virtual bool IsExpression() const {return false;}
  virtual bool IsError() const {return false;}
  virtual ~ParamEntry();
  
  //! calculate how much memory is used directly by this entry
  virtual size_t GetMemorySize() const = 0;
  //! calculate how much memory is used indirectly by this entry
  virtual size_t GetMemoryUsed() const;


  /*!
  \name Access control
  \patch_internal 1.11 Date 08/03/2001 by Ondra
  - New: access protection of ParamClass
  */
  //@{
  virtual void SetAccessMode(ParamAccessMode mode) {}
  virtual void SetAccessModeForAll(ParamAccessMode mode){}
  virtual ParamAccessMode GetAccessMode() const {return PADefault;}
  //@}
  //! owner - used to check if given entry can be accessed
  virtual IParamClassSource *GetSource() const;
  //! owner name - used to display ownership information
  virtual RStringB GetOwnerName() const;
  virtual RString GetOwnerDebugName() const;
  virtual bool CheckVisible(IParamVisibleTest &visible) const;

  virtual ParamEntry *Clone() const;

  // value conversions
  virtual operator RStringB() const;
  virtual operator RString() const;
  virtual operator float() const;
  virtual operator int() const;
  virtual operator bool() const;
  virtual int GetInt() const;
  virtual operator int64() const;
  virtual int64 GetInt64() const;
  virtual RStringB GetValueRaw() const;
  virtual RStringB GetValue() const;

  virtual void SetValue( const RStringB &string );
  virtual void SetValue( float val );
  virtual void SetValue( int val );
  virtual void SetValue( int64 val );
  virtual void SetFile(ParamFile *file) = 0;

  virtual void Add( const RStringB &name, const RStringB &val );
  virtual void Add( const RStringB &name, float val );
  virtual void Add( const RStringB &name, int val );
  virtual void Add(const RStringB &name, int64 val);
  virtual ParamClassPtr AddClass( const RStringB &name, bool guaranteedUnique=false, bool notdefined=false);
  virtual ParamEntryPtr AddArray( const RStringB &name);
  virtual void Clear();
  virtual void Delete(const RStringB &name);

  virtual void AddValue(float val);
  virtual void AddValue(int val);
  virtual void AddValue(int64 val);
  //virtual void AddValue(bool val);
  virtual void AddValue(const RStringB &val);
  //virtual void AddValue(const char *val);
  virtual IParamArrayValue *AddArrayValue();

  virtual int GetEntryCount() const;
  virtual ParamEntryVal GetEntry( int i ) const;
  virtual void OptimizeFind() const;
  
  virtual const IParamArrayValue &operator [] ( int index ) const;
  virtual void SetValue( int index, const RStringB &string );
  virtual void SetValue( int index, float val );
  virtual void SetValue( int index, int val );
  virtual int GetSize() const;
  virtual void DeleteValue(int index);

  //! find entry for modification
  virtual ParamEntryPtr FindEntryNoInheritance(
    const char *name, IParamVisibleTest &visible = DefaultAccess
  ) const;
  //! find entry for modification
  virtual ParamEntryPtr FindEntry(
    const char *name, IParamVisibleTest &visible = DefaultAccess
  ) const;
  
  //! find entry for modification
  virtual bool CheckIfEntryExists(
    const char *name, IParamVisibleTest &visible = DefaultAccess
  ) const;
  
  //! find entry for modification
  virtual ParamEntryPtr FindEntryPath(
    const char *path, IParamVisibleTest &visible = DefaultAccess
  ) const;
  
  virtual ParamEntryVal operator >> ( const char *name ) const;

	float GetFloatDef(const char* name, float defVal = 0.0f) const;
	int GetIntDef(const char* name, int defVal = 0) const;
	bool GetBoolDef(const char* name, bool defVal = false) const;

  /// request background loading of data, returns true once data are ready
  virtual bool Request(const char *name, bool includeBases=true) const;

  virtual ConstParamEntryPtr FindParent() const;
  virtual ConstParamEntryPtr FindBase() const;
  
  /*
  //! force entry existence in the memory
  virtual void AddRefLoaded();
  //! release forced entry
  virtual void ReleaseLoaded();
  */

  virtual void Save( QOStream &f, int indent, const char *eol ) const {}

  virtual void SerializeBin(SerializeBinStream &f) = 0;
  //! compact unused memory - usefull when finished adding entries
  virtual void Compact();
  //! provide an estimation of how many class entries will be added
  virtual void ReserveEntries(int count);
  //! provide an estimation of how many array elements will be added
  virtual void ReserveArrayElements(int count);
  
  //! calculate checksum of entry and all its verified children 
  /*!
  \patch_internal 1.11 Date 08/03/2001 by Ondra
  - New: CRC/checksum verification of ParamClass
  */
  virtual void CalculateCheckValue(SumCalculator &sum) const = 0;
  //! check if entry or some of its children is verified protected
  virtual bool HasChecksum() const;

  template <class Type>
  Type ReadValue
  (
    const char *name, const Type &defVal
  ) const
  {
    const ParamEntryPtr entry = FindEntry(name);
    if (!bool(entry))
    {
      return defVal;
    }
    else
    {
      return *entry;
    }
  }

  //! Read array with elements of a single type
  template <typename Type>
  void ReadArray(AutoArray<Type> &target, const char *name) const
  {
    ConstParamEntryPtr entry = FindEntry(name);

    if (entry && entry->IsArray())
    {
      target.Clear();

      int const n = entry->GetSize();
      target.Realloc(n);
      target.Resize(n);

      for (int i=0; i<n; ++i)
        target[i] = (*entry)[i];
    }
  }
};


class IParamArrayValue: public RefCount //,public ParamRawValue
{
  public:
  // param array value interface

  virtual RStringB GetValue() const = 0;
  virtual RStringB GetValueRaw() const = 0;
  virtual float GetFloat() const = 0;
  virtual int GetInt() const = 0;
  virtual int64 GetInt64() const = 0;

  virtual void SetValue(const RStringB &val )= 0;
  virtual void SetValue(float val)= 0;
  virtual void SetValue(int val)= 0;
  virtual void SetFile(ParamFile *file)= 0;
  virtual void SetValue(int64 val)= 0;

  virtual bool IsTextValue() const = 0;
  virtual bool IsFloatValue() const = 0;
  virtual bool IsIntValue() const = 0;
  virtual bool IsArrayValue() const = 0;
  virtual bool IsExpression() const = 0;
  virtual bool IsInt64Value() const = 0;

  virtual size_t GetMemorySize() const = 0;
  //virtual size_t GetMemoryUsed() const = 0;
  
  virtual void Save(QOStream &f, int indent, const char *eol) const = 0;
  virtual void SerializeBin(SerializeBinStream &f) = 0;

  // some handy conversions using virtual functions
  operator float() const {return GetFloat();}
  operator int() const {return GetInt();}
  operator int64() const {return GetInt64();}
  operator RStringB() const {return GetValue();}
  operator RString() const {return GetValue();}
  operator bool() const {return GetInt()!=0;}

  // may be array of values
  virtual const IParamArrayValue *GetItem(int i) const = 0;
  virtual int GetItemCount() const = 0;
  virtual void CalculateCheckValue(SumCalculator &sum) const = 0;

  const IParamArrayValue &operator [] (int i) const {return *GetItem(i);}

  virtual void AddValue(float val) = 0;
  virtual void AddValue(int val) = 0;
  virtual void AddValue(int64 val) = 0;
  virtual void AddValue(const RStringB &val) = 0;
  virtual IParamArrayValue *AddArrayValue() = 0;
};

//! temporary pointer - guarantees ParamEntry locking during its lifetime
template <class PEntryType>
class PEntryPtr
{
  friend class ParamClass;
  friend class ParamEntry;

  //! pointer to the locked value
  ParamClassLockedPtr _ptr;
  //! pointer to the value entry
  PEntryType *_entry;

public:
  PEntryType *GetPointer() const {return _entry;}
  ParamClass *GetClass() const {return _ptr;}

public:


  PEntryPtr()
    :_entry(NULL)
  {
  }

  PEntryPtr(const ParamClass *cls, PEntryType *entry)
    :_ptr(unconst_cast(cls)),_entry(entry)
  {
  }

  PEntryPtr(const ParamClass *cls)
    :_ptr(unconst_cast(cls)),_entry(unconst_cast(cls))
  {
  }

  // enable PEntryPtr creation from any type that can be copied
  template <class SrcPEntryType>
    PEntryPtr(const PEntryPtr<SrcPEntryType> &src)
    :_ptr(const_cast<ParamClass *>(src.GetClass())),_entry(const_cast<PEntryType *>(src.GetPointer()))
  {
  }

  //PEntryType &operator *() const {return *_entry;}
  PEntryType *operator ->() const {return _entry;}
  operator bool () const {return _entry!=NULL;}

  bool IsNull() const {return _entry==NULL;}
  bool NotNull() const {return _entry!=NULL;}

  template <class SrcPEntryType>
    bool operator == (const PEntryPtr<SrcPEntryType> &with) const {return _entry==with._entry;}

    template <class SrcPEntryType>
      bool operator != (const PEntryPtr<SrcPEntryType> &with) const {return _entry!=with._entry;}
};

struct ParamFileContext;

//! interface by which any entry can be loaded when necessary
class IParamClassSource: public RefCount
{
  public:
  //! request data needed for loading
  virtual bool Request(int sourceOffset, int sourceSize) = 0;
  /// perform the loading
  virtual void Load(ParamClass &cls, const char *relPath) = 0;
  virtual bool RequestBaseName(RStringB &baseName, const ParamClassPlaceholder &cls) = 0;
  virtual ~IParamClassSource() {}
  virtual ParamFile *GetFile() const = 0;
  virtual RStringB GetOwnerName() const = 0;
  virtual RString GetOwnerDebugName() const = 0;
  virtual RString GetDebugSuffix() const = 0;
  virtual void SetOwnerName(RString owner) = 0;
  /// check if it is unloadable
  virtual bool CanUnload() const = 0;
  //@{ store/retrieve context values from this ParamFile
  virtual bool ExtractContext(ParamFileContext &ctx) = 0;
  virtual void StoreContext(const ParamFileContext &ctx) = 0;
  //@}
};

class ParamClassOwner: public IParamClassSource
{
  RStringB _owner;
  
  public:
  ParamClassOwner(RString owner);
  virtual RStringB GetOwnerName() const;
  virtual RString GetOwnerDebugName() const;
  virtual void SetOwnerName(RString owner);
};

//! link by the filename
class ParamClassOwnerFilename: public ParamClassOwner
{
  typedef ParamClassOwner base;

  /// file in which given entry exists
  ParamFile *_file;
  /// file handle - buffer is flushed whenever not needed
  QIFStreamB _in;
  /// filename - for debugging purposes only
  RStringB _name;
  
  //@{ Date extracted from ParamFileContext to avoid loading them when streaming  
  int _version; 
  bool _fullLoad;
  int _endOfParamfile;
  //@}

public:
  ParamClassOwnerFilename(RString owner, ParamFile *file, RStringB filename);
  ParamFile *GetFile() const {return _file;}
  virtual RString GetDebugSuffix() const;
  virtual void Load(ParamClass &cls, const char *relPath);
  virtual bool RequestBaseName(RStringB &baseName, const ParamClassPlaceholder &cls);
  virtual bool Request(int sourceOffset, int sourceSize);
  virtual bool CanUnload() const;
  virtual bool ExtractContext(ParamFileContext &ctx);
  virtual void StoreContext(const ParamFileContext &ctx);
};

//! determine owner, but do not allow unloading
class ParamClassOwnerNoUnload: public ParamClassOwner
{
  typedef ParamClassOwner base;
  #ifdef _DEBUG
  //! file in which given entry exists
  RStringB _debugSuffix;
  #endif

  //! file in which given entry exists
  public:
  ParamClassOwnerNoUnload(RString owner, RString filename);
  virtual RString GetDebugSuffix() const;
  //@{ dummy implementation - no streaming supported
  virtual bool Request(int sourceOffset, int sourceSize) {return true;}
  virtual void Load(ParamClass &cls, const char *relPath);
  virtual bool RequestBaseName(RStringB &baseName, const ParamClassPlaceholder &cls);
  virtual bool CanUnload() const {return false;}
  ParamFile *GetFile() const {return NULL;}
  virtual bool ExtractContext(ParamFileContext &ctx) {return false;}
  virtual void StoreContext(const ParamFileContext &ctx) {}
  //@}
  
};

//! determine owner, but do not allow unloading
/*!
owner of this class is indirect (it is inherited from another source)
*/

class ParamClassOwnerIndirectNoUnload: public IParamClassSource
{
  Ref<const IParamClassSource> _indirect;

  //! file in which given entry exists
  public:
  explicit ParamClassOwnerIndirectNoUnload(const IParamClassSource *owner);
  virtual RString GetDebugSuffix() const;
  //@{ dummy implementation - no streaming supported
  virtual void Load(ParamClass &cls, const char *relPath);
  virtual bool RequestBaseName(RStringB &baseName, const ParamClassPlaceholder &cls);
  virtual bool Request(int sourceOffset, int sourceSize) {return true;}
  virtual bool CanUnload() const {return false;}
  
  virtual RStringB GetOwnerName() const;
  virtual RString GetOwnerDebugName() const;
  virtual void SetOwnerName(RString owner);
  
  ParamFile *GetFile() const {return NULL;}
  virtual bool ExtractContext(ParamFileContext &ctx) {return false;}
  virtual void StoreContext(const ParamFileContext &ctx) {}
  //@}
};

//! link by the entry
class ParamClassOwnerEntry: public ParamClassOwner
{
  typedef ParamClassOwner base;
  
  //! reference base - need to exist (its parent needs to be locked)
  ParamEntry *_baseEntry;

  public:
  ParamClassOwnerEntry(RString owner, ParamEntry *baseEntry);
  virtual RString GetDebugSuffix() const;
  virtual void Load(ParamClass &cls, const char *relPath);
  virtual bool CanUnload() const;
};

//! id for class linking (including unload / restore)
class ParamClassId 
{
  friend class ParamClass;
  
  //! parent in which this class exists
  /*!
  parent needs to be loaded while the link exists
  */
  ParamClassLockedPtr _parent;
  //! name within the parent
  RStringB _name;
  //! where should we recover from
  Ref<IParamClassSource> _source;
  
  ParamClassId(ParamClass *parent, const RStringB &name, IParamClassSource *source)
  :_parent(parent),_name(name),_source(source)
  {
  }
  
  public:
  ParamClassId()
  {
  }
  bool operator == (const ParamClassId &with) const
  {
    return _parent== with._parent && !stricmp(_name,with._name) && _source==with._source;
  }
  RString GetDebugName() const;
  
};

class RefCountDummy
{
  public:
  /*
  // AddRef and release doing nothing
  void AddRef(){}
  void Release(){}
  */
};


//! default traits for link IDs
template <>
struct SoftLinkIdTraits<ParamClassId,ParamClassId>
{
  typedef ParamClassId LinkId;
  typedef ParamClassId LinkIdStorage;

  //! type able to AddRef / Release
  typedef RefCountDummy RefCountType;

  static LinkId GetNullId() {return LinkId();}
  static bool IsEqual(const LinkId &id1, const LinkId &id2)
  {
    return id1==id2;
  }
  static bool IsNull(const LinkId &id) {return IsEqual(id,GetNullId());}
  static RString GetDebugName(const LinkId &id)
  {
    return id.GetDebugName();
  }
};

template <>
struct FindArrayKeyTraits< SRef<ParamEntry> > 
{
  typedef SRef<ParamEntry> Type;
  typedef ParamEntry *KeyType;
  static bool IsEqual(KeyType a, KeyType b)
  {
    return a==b;
  }
  static KeyType GetKey(const Type &a) {return a;}
};


#include <Es/Memory/normalNew.hpp>

/// interface to ParamEntry searching
class IParamEntryIndex
{
  public:
  virtual int Find(const char *name) const = 0;
  virtual void Delete(const char *name) = 0;
  virtual void Add(const char *name, int index) = 0;
  virtual ~IParamEntryIndex() {}
};

//! temporary value - guarantees ParamEntry locking during its lifetime
class ParamEntryVal
{
  // reference can be easily constructed using pointer
  ConstParamEntryPtr _ptr;


public:
  const ParamEntry *GetPointer() const {return _ptr.GetPointer();}
  ParamEntry *GetModPointer() const
  {
    return unconst_cast(_ptr.GetPointer());
  }

  ///Gets pointer to class that owning this entry
  /**it is needed when enumerating through base classes */
  const ParamClass *GetClass() const {return _ptr.GetClass();}
  ParamClass *GetModClass() const
  {
    return unconst_cast(_ptr.GetClass());
  }

  ParamEntryVal(const ParamClass &cls, const ParamEntry &entry)
    :_ptr(&cls,&entry)
  {
  }

  ParamEntryVal(const ParamClass &cls)
    :_ptr(&cls)
  {
  }

  //Construct from ConstParamEntryPtr 
  ParamEntryVal(ConstParamEntryPtr ptr):_ptr(ptr) {}

  operator const ParamEntry &() const {return *_ptr.GetPointer();}

  ConstParamEntryPtr operator &() const
  {
    return ConstParamEntryPtr(_ptr.GetClass(),_ptr.GetPointer());
  }

  // 
  bool Request(RStringB name, bool includeBases=true) const
  {
    return _ptr->Request(name,includeBases);
  }

  // following functions enable access to the ParamEntry member functions
  ParamEntryVal operator >> (const char *name) const
  {
    return _ptr->operator >>(name);
  }

  const RStringB &GetName() const {return _ptr->GetName();}
  bool IsClass() const {return _ptr->IsClass();}
  bool IsArray() const {return _ptr->IsArray();}
  bool IsExpression() const {return _ptr->IsExpression();}
  bool IsTextValue() const {return _ptr->IsTextValue();}
  bool IsFloatValue() const {return _ptr->IsFloatValue();}
  bool IsIntValue() const {return _ptr->IsIntValue();}
  bool IsInt64Value() const {return _ptr->IsInt64Value();}

  const ParamClass *GetClassInterface() const {return _ptr->GetClassInterface();}
  IParamClassSource *GetSource() {return _ptr->GetSource();}

  operator RString() const {return _ptr->operator RString();}
  operator RStringB() const {return _ptr->operator RStringB();}
  operator float() const {return _ptr->operator float();}
  operator int() const {return _ptr->operator int();}
  operator bool() const {return _ptr->operator bool();}
  int GetInt() const {return _ptr->GetInt();}
  operator int64() const {return _ptr->operator int64();}
  int64 GetInt64() const {return _ptr->GetInt64();}
  RStringB GetValueRaw() const {return _ptr->GetValueRaw();}
  RStringB GetValue() const {return _ptr->GetValue();}

  void Save(QOStream &f,int indent, const char *eol) const {_ptr->Save(f,indent,eol);}

  bool CheckIfEntryExists(
    const char *name, IParamVisibleTest &visible = DefaultAccess
    ) const
  {
    return _ptr->CheckIfEntryExists(name,visible);
  }
  /// find an entry, make sure it is loaded and return a pointer
  ConstParamEntryPtr FindEntry(
    const char *name, IParamVisibleTest &visible = DefaultAccess
    ) const
  {
    return _ptr->FindEntry(name,visible);
  }
  const IParamArrayValue &operator [] ( int index ) const
  {
    return _ptr->operator [](index);
  }
  int GetSize() const {return _ptr->GetSize();}
  int GetEntryCount() const {return _ptr->GetEntryCount();}
  ParamEntryVal GetEntry(int index) const {return _ptr->GetEntry(index);}

  RString GetContext( const char *member=NULL ) const
  {
    return _ptr->GetContext(member);
  }
  bool CheckVisible(IParamVisibleTest &visible) const {return _ptr->CheckVisible(visible);}
  RStringB GetOwnerName() const {return _ptr->GetOwnerName();}
  RString GetOwnerDebugName() const {return _ptr->GetOwnerDebugName();}

  template <class Type>
    Type ReadValue
    (
    const char *name, const Type &defVal
    ) const
  {
    return _ptr->ReadValue(name,defVal);
  }
};

//! representation of config class
class ParamClass:
  public ParamEntry,
  public RemoveSoftLinks< SoftLinkIdTraits<ParamClassId,ParamClassId> >,
  public ISoftLinkParamClassTraits,
  public TLinkBidirD,
  public CountInstances<ParamClass>
{
  public:
  typedef ISoftLinkParamClassTraits::LinkId LinkId;
  

  private:
  friend class ParamEntry;
  friend class ParamFile;
  friend class ParamClassPlaceholder;
  friend class ParamClassOwnerFilename;
  typedef ParamEntry base;

  SRef<IParamEntryIndex> _index;
  FindArrayKey< SRef<ParamEntry> > _entries;
  //! base class
  /*! when any class exists, base must exist as well*/
  ParamClassLockedPtr _base;

  //! parent class
  /*! when any class exists, base must exist as well*/
  ParamClassLockedPtr _parent;
  
  //! access right for modification by merge
  ParamAccessMode _access;
  //! count how many times this class is locked (forced into the memory)
  int _loadCount;
  
  //! id identifying source (including owner) and relative path within it
  Ref<IParamClassSource> _source;
  //! offset where given class should be loaded from in case of unloading
  int _sourceOffset;
  //! size of the data needed for class restoration (0 if restoration not supported)
  int _sourceSize;
#if _VBS3 // saveConfig can save base class
  //! string representation of base class (used with file saving)
  RString _baseClass;
  //! don't write {} for an empty class?
  bool _noParenthesisIfEmpty;
#endif
  //! remove the class from the cache
  void RemoveFromTheCache();
  /// make this entry MRU (top of the cache)
  void RefreshInTheCache();
  
  //! load the class
  void DoLoad();
  /// unload the class
  void DoUnload();
  
  virtual void SetSource(IParamClassSource *source, bool subentries);
  virtual void SetSource(const ParamEntry &entry);

  //! load entry in the background, report true when loaded
  bool RequestEntry(int index) const;

  //! make sure an entry with given index is loaded
  void LoadEntry(int index);
  //! make sure an entry with given index is unloaded (replaced by a placeholder)
  void UnloadEntry(int index);

  /// request and load base name for given entry
  bool RequestEntryBaseName(int index, RStringB &baseName) const;

  void CreateEntryIndex();
  
public:
  bool IsClass() const {return true;}
  const ParamClass *GetClassInterface() const {return this;}
  ParamClass *GetClassInterface() {return this;}

  //! implementation of RemoveSoftLinks
  virtual LinkId GetLinkId() const
  {
    return ParamClassId(_parent,_name,_source);
  }
#if _VBS3 // saveConfig can save base class
  void SetBaseClass(RString base) {_baseClass = base;}
  void SetNoParenthesisIfEmpty(bool set) {_noParenthesisIfEmpty = set;}
#endif
  ParamClassPtr GetClass(const char *name) const;

  RStringB GetValue(const char *name ) const;
  RStringB GetValue(const char *name, int i ) const;

  const ParamClass *GetParent() const {return _parent;}
  const ParamClass *GetRoot() const
  {
    const ParamClass *root = this;
    while (root->_parent) root = root->_parent;
    return root;
  }
  int GetRootDepth() const
  {
    int depth = 0;
    const ParamClass *root = this;
    while (root->_parent) root = root->_parent, depth++;
    return depth;
  }
  const ParamClass *GetDeepParent(int depth) const
  {
    const ParamClass *cls = this;
    while (--depth>=0)
    {
      cls = cls->_parent;
    }
    return cls;
  }

  LSError Parse( QIStream &f, ParamFile *file); // read until close bracket
  void Save( QOStream &f, int indent, const char *eol="\r\n" ) const; // read until close bracket
  /// load/save whole content of the class
  void SerializeBin(SerializeBinStream &f);
  
  /// load base class name only - needed for preloading
  static bool RequestBaseName(SerializeBinStream &f, RStringB &baseName);
  
  void Compact();
  /// change base class (class this class is inherited from)
  void SetBase(RStringB base);
  /// change base class (class this class is inherited from)
  /**
  Function can move current class and its derived classes to end of paramfile
  that allows set new base class. 
  @param base new name of base class. Can be any class from parent class
  @param nomoveClass disables moving feature. Setting this parameter to false causes, that
    function will work the same way as SetBase
  @return function return true, if base class has been set.
    this function will not print any diagnostics or error messsages
    */
  bool SetBaseEx(RStringB base, bool nomoveClass=false);
  /// 
  void MoveEntryToEnd(int entryIndex);

  void CheckInheritedAccess();
  void ReserveEntries(int count);
  virtual void DisableStreaming(bool subentries);

  //! increase load count
  void AddRefLoad()
  {
    Assert(_loadCount>=0);
    if (_loadCount++==0)
    {
      DoLoad();
    }
  }
  //! decrease load count
  void ReleaseLoad()
  {
    if (--_loadCount==0)
    {
      DoUnload();
    }
    Assert(_loadCount>=0);
  }

  //! unload all entries that can be unloaded - save as much memory as possible
  void ReleaseAll();
  // optional protection
  bool Update(const ParamClass &source, bool protect=true, bool canUpdateBase = false);
  bool Update(const ParamClass &source, bool &containsDeleteEntry, bool protect=true, bool canUpdateBase = false);

  ParamEntryPtr FindEntry(const char *name, IParamVisibleTest &visible = DefaultAccess) const;
  bool CheckIfEntryExists(const char *name, IParamVisibleTest &visible = DefaultAccess) const;
  ParamEntryPtr FindEntryNoInheritance(const char *name,IParamVisibleTest &visible = DefaultAccess) const;
  ParamEntryPtr FindEntryPath(const char *path, IParamVisibleTest &visible = DefaultAccess) const;

  ParamEntryVal operator >> ( const char *name ) const;
	float GetFloatDef(const char* name, float defVal = 0.0f) const;
	int GetIntDef(const char* name, int defVal = 0) const;
	bool GetBoolDef(const char* name, bool defVal = false) const;

  bool Request(const char *name, bool includeSubclasses=true) const;

  bool RequestAllEntries(bool includeSubclasses=true) const;
  
  virtual ConstParamEntryPtr FindParent() const;
  virtual ConstParamEntryPtr FindBase() const;

  //! check if entry can be unloaded / restored
  bool NeedsRestoring() const;
  void ShallowLockLink() {AddRefLoad();}
  void ShallowUnlockLink() {ReleaseLoad();}

  void DeepLockLink();
  void DeepUnlockLink();

  ParamClassPtr Restore(IParamClassSource *src, const char *name);
  bool RequestEntry(IParamClassSource *src, const char *name) const;
  
  static ParamClass *RestoreLinkDeep(const LinkId &id);
  static bool RequestRestoreLinkDeep(const LinkId &id, bool noRequest=false);
  static ParamClass *RestoreLinkShallow(const LinkId &id);
  static bool RequestRestoreLinkShallow(const LinkId &id, bool noRequest=false);

  void Add( const RStringB &name, const RStringB &val );
  void Add( const RStringB &name, float val );
  void Add( const RStringB &name, int val );
  void Add( const RStringB &name, int64 val);

  void Delete(const RStringB &name);

  ParamClassPtr AddClass( const RStringB &name, bool guaranteedUnique=false, bool notdefined=false);
  ParamEntryPtr AddArray( const RStringB &name);

  void AccessDenied(const char *name);

  virtual void SetAccessModeForAll(ParamAccessMode mode);
  virtual void SetAccessMode(ParamAccessMode mode) {_access=mode;}
  virtual ParamAccessMode GetAccessMode() const {return _access;}
  //! owner - used to check if given entry can be accessed
  virtual IParamClassSource *GetSource() const;
  virtual RStringB GetOwnerName() const;
  virtual RString GetOwnerDebugName() const;
  virtual bool CheckVisible(IParamVisibleTest &visible) const;

  const ParamClass *GetFile() const;
  void SetFile(ParamFile *file);
  virtual ParamFile *GetFileInterface() {return NULL;}

  void OptimizeFind() const;
  int GetEntryCount() const {return _entries.Size();}
  bool IsFindOptimized() const {return _index.NotNull() || _entries.Size()==0;}
  
  ParamEntryVal GetEntry( int i ) const;
  RStringB GetEntryName( int i ) const;
  const char *GetBaseName() const {return _base.NotNull() ? _base->GetName() : NULL;}

  bool IsDerivedFrom( ParamEntryPar parent ) const;

  virtual void CalculateCheckValue(SumCalculator &sum) const;
  virtual bool HasChecksum() const;

  //! select class for checking
  const ParamClass *SelectClassForChecking(int index) const;
  int GetNumberOfClassesForChecking() const;

  //! check if the class can be unloaded
  bool CanBeUnloaded() const;
  ParamClass *GetClassForUnloading();
  
  //! calculate how much memory is used directly by this entry
  size_t GetMemorySize() const;
  //! calculate how much memory is used indirectly by this entry
  size_t GetMemoryUsed() const;
  
  protected:
  ParamClass();
  ParamClass( const RStringB &name );
  ~ParamClass();
  void UnlinkClasses();
  
  bool CheckIntegrity() const;
  int CountLinksToClass(const ParamClass &cls) const;
  

  void NewEntry(ParamEntry *entry, bool guaranteedUnique=false);

  ParamEntryPtr Find(
    const char *name, bool parent, bool base,
    IParamVisibleTest &visible
  ) const;
  int FindIndex(const char *name) const;
  

  void Diagnostics( int indent );

  USE_FAST_ALLOCATOR

private:
  /// helper for ForEachEntry
  template <class Functor>
  inline bool ForEachEntryOrigin(const Functor &func, const ParamClass &origin) const;

public:
  /// walk through entries, inherited entries first
  template <class Functor>
  bool ForEachEntry(const Functor &func) const
  {
    if (_base)
    {
      bool done = _base->ForEachEntryOrigin(func,*this);
      if (done) return done;
    }
    // on this level we know entry is visible
    for (int i=0; i<GetEntryCount(); i++)
    {
      ParamEntryVal entry = GetEntry(i);
      bool done = func(entry);
      if (done) return done;
    }
    return false;
  }
  
  // Iterator... 
  template<bool withBases = true, class FilterTraits = FilterNone>
  class Iterator : private FilterTraits
  {
  protected:    
    int _item;
    ConstParamEntryPtr _current; 
    ConstParamEntryPtr _origin;

    virtual void FindNextValid()
    { 
      _item++;
      if (!_current->IsClass())
        return;

      if (!withBases)
      {      
        while (_item < _current->GetEntryCount() && !FilterTraits::IsValid(_current->GetEntry(_item)))
        {        
          _item++;
        }
      }
      else
      {
        while (_item >= _current->GetEntryCount() && _current->FindBase().NotNull())
        {
          _current = _current->FindBase();
          _item = 0;
        }            

        while (_item < _current->GetEntryCount() && 
          ((*_origin >> _current->GetEntry(_item).GetName()).GetPointer() != _current->GetEntry(_item).GetPointer() || // check that class is not overwritten in parent
          !FilterTraits::IsValid(_current->GetEntry(_item))))  
        {        
          _item++;
          while (_item == _current->GetEntryCount() && _current->FindBase().NotNull())
          {
            _current = _current->FindBase();
            _item = 0;
          }
        }
      }
    }

  public:
    Iterator(ConstParamEntryPtr base) 
    {
      _item = -1; 
      _current = base;
      if (withBases)
        _origin = base;

      FindNextValid();
    }

    Iterator(ConstParamEntryPtr base, FilterTraits filter): FilterTraits(filter)
    {
      _item = -1; 
      _current = base;
      if (withBases)
        _origin = base;      

      FindNextValid();
    }
    operator bool () const {return _current->IsClass() ? (_item < _current->GetEntryCount()) : _item == 0;};
    void operator ++ () {FindNextValid();};
    ParamEntryVal operator * ();      
  };

  int GetLockCount() const  {return _loadCount;}
};

//! class of callback functions
class CRCFunctions
{
public:
  CRCFunctions() {}
  virtual ~CRCFunctions() {}

  //! callback function to add buffer to CRC sum
  virtual void Add(SumCalculator &sum, const void *buffer, int size) {}
};

//! element and memory counting
template <class Type>
class MemoryCounter
{
  int _count;
  size_t _size;

  public:
  MemoryCounter()
  {
    _count = 0;
    _size = 0;
  }
  #if _DEBUG
  ~MemoryCounter()
  {
    // counter destructor should be called after the list is destructed
    Assert(_count==0);
    Assert(_size==0);
  }
  #endif

  void AddedItem(const Type *item)
  {
    _count++;
    _size += item->GetMemoryUsed();
  }
  
  void RemovedItem(const Type *item)
  {
    _size -= item->GetMemoryUsed();
    _count--;
    // verify counter integrity - zero items should take zero memory
    Assert(_count>0 || _size==0);
  }
  static bool IsCounted() {return true;}
  int GetCount() const {return _count;}
  size_t GetSize() const {return _size;}

  void operator += (const MemoryCounter &src)
  {
    _count += src._count;
    _size += src._size;
  }
};

struct ParamFileContext;

//! file-level config object
class ParamFile: public ParamClass, public MemoryFreeOnDemandHelper
{
  typedef ParamClass base;
protected:
  SRef<Evaluator> _evaluator;
  static EvaluatorFunctions *_defaultEvalFunctions;
  static PreprocessorFunctions *_defaultPreprocFunctions;
  static LocalizeStringFunctions *_defaultLocalizeStringFunctions;
  static SumCalculatorFunctions *_defaultSumCalculatorFunctions;

  /// list of unlocked classes, sorted by time used (LRU is first)
  /** a list of candidates to be released anytime when memory is needed */
  TListBidir<ParamClass,TLinkBidirD,SimpleCounter<ParamClass> > _cache;

private:
  //! no copy
  ParamFile(const ParamFile &src);
  //! no copy
  void operator = (const ParamFile &src);

  protected:
  void SetSource(IParamClassSource *source);
  
public:
  ParamFile();
  ~ParamFile();

  // public for ParamFile
  void SetName( const RStringB &name ) {ParamClass::SetName(name);}

  LSError Parse(const char *name, IParamClassSource *source = NULL, IEvaluatorVariables *parentVariables = NULL, IEvaluatorNamespace *globalVariables = NULL);
  LSError Parse(QFBank &bank, const char *name, IEvaluatorVariables *parentVariables = NULL, IEvaluatorNamespace *globalVariables = NULL);
  LSError Save( const char *name ) const;
  LSError Save() const {return Save(GetName());}

  LSError Parse(QIStream &f, IParamClassSource *source = NULL, IEvaluatorVariables *parentVariables = NULL, IEvaluatorNamespace *globalVariables = NULL);
  void Save( QOStream &f, int ident, const char *eol="\r\n" ) const;

  ParamFile *GetFileInterface() {return this;}
  
  LSError ParsePlainText(const char *name, IEvaluatorVariables *parentVariables = NULL, IEvaluatorNamespace *globalVariables = NULL);
  void ParsePlainText(QIStream &f, IEvaluatorVariables *parentVariables = NULL, IEvaluatorNamespace *globalVariables = NULL);

  typedef SerializeBinStream::ErrorCode ErrorCode;
  
  bool ParseBin(const char *name, IParamClassSource *source = NULL, IEvaluatorVariables *parentVariables = NULL, IEvaluatorNamespace *globalVariables = NULL);
  bool ParseSigned(const char *name, IParamClassSource *source=NULL, IEvaluatorVariables *parentVariables = NULL, IEvaluatorNamespace *globalVariables = NULL);
  bool ParseBin(QFBank &bank, const char *name, IEvaluatorVariables *parentVariables = NULL, IEvaluatorNamespace *globalVariables = NULL);
  bool ParseBin(QIStream &f, IParamClassSource *source = NULL, IEvaluatorVariables *parentVariables = NULL, IEvaluatorNamespace *globalVariables = NULL);
  bool ParseSigned(QIStream &f, IParamClassSource *source = NULL, IEvaluatorVariables *parentVariables = NULL, IEvaluatorNamespace *globalVariables = NULL);
  bool SaveBin( const char *name );
  bool SaveBin( QOStream &out );
  bool SaveBin() {return SaveBin(GetName());}
  bool SaveSigned( const char *name );
  bool SaveSigned() {return SaveSigned(GetName());}

  bool ParseBinOrTxt(const char *name, IParamClassSource *source = NULL, IEvaluatorVariables *parentVariables = NULL, IEvaluatorNamespace *globalVariables = NULL);
  bool ParseBinOrTxt(QFBank &bank, const char *name, IEvaluatorVariables *parentVariables = NULL, IEvaluatorNamespace *globalVariables = NULL);

  void Clear();

  void Delete(const RStringB &name)
  {
    base::Delete(name);
  }

  void CreateEvaluator(IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables, bool enableSerialization)
  {
    _evaluator = _defaultEvalFunctions->CreateEvaluator(parentVariables, globalVariables, enableSerialization);
  }

  int BeginContext() {return _evaluator->BeginContext();}
  void EndContext(int handle) {_evaluator->EndContext(handle);}

  float EvaluateFloat(const char *expr) {return _evaluator ? _evaluator->EvaluateFloat(expr) : 0;}
  float EvaluateFloatInternal(const char *expr) {return _evaluator ? _evaluator->EvaluateFloatInternal(expr) : 0;}
  RString EvaluateStringInternal(const char *expr) {return _evaluator ? _evaluator->EvaluateStringInternal(expr) : RString();}
  void ExecuteInternal(const char *expr) {if (_evaluator) _evaluator->ExecuteInternal(expr);}
  void VarSetFloatInternal(const char *name, float value, bool readOnly = false, bool forceLocal = false)
  {if (_evaluator) _evaluator->VarSetFloatInternal(name, value, readOnly, forceLocal);}

  RString LocalizeString(const char *str) {return _defaultLocalizeStringFunctions->LocalizeString(str);}
  
  static SumCalculator *CreateSumCalculator() {return _defaultSumCalculatorFunctions->CreateCalculator();}

  // functions that serve to maintain _cache
  void CacheRemove(ParamClass *cls);
  void CacheInsert(ParamClass *cls);
  void CacheRefresh(ParamClass *cls);

  void DeleteVariables()
  {
    if (_evaluator)
    {
      _evaluator->DeleteVariables();
    }
  }

  // implementation
protected: 
  ErrorCode ParseBinEx(const char *name, IParamClassSource *source, ParamFileContext &context, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables);
  ErrorCode ParseBinEx(QIStream &in, IParamClassSource *source, ParamFileContext &context, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables);
  ErrorCode ParseBinEx(const char *name, IParamClassSource *source, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables);
  ErrorCode ParseBinEx(QFBank &bank, const char *name, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables);

  void SerializeBin(SerializeBinStream &f, IParamClassSource *source, ParamFileContext &context);

  LSError Reload(IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables);

  void LoadVariables(SerializeBinStream &f, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables)
  {
    // TODO: evaluator should load / save global variables, not local ones
    if (!_evaluator) CreateEvaluator(parentVariables, globalVariables, true); // global space not needed to load local variables
    _evaluator->LoadVariables(f);
  }
  void SaveVariables(SerializeBinStream &f, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables)
  {
    // TODO: evaluator should load / save global variables, not local ones
    if (!_evaluator) CreateEvaluator(parentVariables, globalVariables, true); // global space not needed to save local variables
    _evaluator->SaveVariables(f);
  }

  bool Preprocess(QOStream &out, const char *name) {return _defaultPreprocFunctions->Preprocess(out, name, true);}
  bool Preprocess(QOStream &out, QFBank &bank, const char *name) {return _defaultPreprocFunctions->Preprocess(out, bank, name, true);}

  // implementation of MemoryFreeOnDemandHelper abstract interface
  virtual size_t FreeOneItem();
  virtual float Priority() const;
  virtual size_t MemoryControlled() const;
  virtual RString GetDebugName() const;
  
  //! perform registration based on given source
  void RegisterUnloading(IParamClassSource *source);

  USE_FAST_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>

//! temporary pointer - guarantees ParamClass locking during its lifetime
template <class PClassType, class StoreType>
class PClassPtr
{
  friend class ParamClass;
  //! pointer to the locked value
  StoreType _ptr;
  
  public:

  PClassType *GetPointer() const {return unconst_cast(_ptr.GetPointer());}
    
  PClassPtr(){}

  PClassPtr(const ParamClass *cls)
  :_ptr(unconst_cast(cls))
  {
  }

  PClassPtr &operator =(const ParamClass *cls)
  {
    _ptr = unconst_cast(cls);
    return *this;
  }

  // enable PClassPtr creation from any type that can be copied
  template <class SrcPClassType, class SrcStoreType>
  PClassPtr(const PClassPtr<SrcPClassType,SrcStoreType> &src)
  :_ptr(src.GetPointer())
  {
  }
  
  PClassType &operator *() const {return *_ptr;}
  PClassType *operator ->() const {return _ptr;}
  operator bool () const {return _ptr!=NULL;}
  
  bool operator == (const ParamClass *with) const {return _ptr.GetPointer()==with;}
  bool operator != (const ParamClass *with) const {return _ptr.GetPointer()!=with;}
  
  template <class SrcPClassType, class SrcStoreType>
  bool operator == (const PClassPtr<SrcPClassType,SrcStoreType> &with) const
  {
    return _ptr.GetPointer()==with._ptr.GetPointer();
  }

  template <class SrcPClassType, class SrcStoreType>
  bool operator != (const PClassPtr<SrcPClassType,SrcStoreType> &with) const
  {
    return _ptr.GetPointer()!=with._ptr.GetPointer();
  }

  __forceinline bool IsNull() const {return _ptr.IsNull();}

  __forceinline void Free() {_ptr.Free();}
};

//@{
//\name PClassPtr against PEntryPtr comparison
template <class SrcPClassType, class SrcStoreType, class SrcPEntryType>
bool operator == (const PClassPtr<SrcPClassType,SrcStoreType> &a, const PEntryPtr<SrcPEntryType> &b)
{
  return a.GetPointer()==b.GetPointer();
}
template <class SrcPClassType, class SrcStoreType, class SrcPEntryType>
bool operator != (const PClassPtr<SrcPClassType,SrcStoreType> &a, const PEntryPtr<SrcPEntryType> &b)
{
  return a.GetPointer()!=b.GetPointer();
}
template <class SrcPEntryType, class SrcPClassType, class SrcStoreType>
bool operator == (const PEntryPtr<SrcPEntryType> &a, const PClassPtr<SrcPClassType,SrcStoreType> &b)
{
  return a.GetPointer()==b.GetPointer();
}

template <class SrcPEntryType, class SrcPClassType, class SrcStoreType>
bool operator != (const PEntryPtr<SrcPEntryType> &a, const PClassPtr<SrcPClassType,SrcStoreType> &b)
{
  return a.GetPointer()!=b.GetPointer();
}
//}@

//@{
//\name PClassPtr against ParamEntry *comparison
template <class SrcPType, class SrcStoreType>
bool operator == (const PClassPtr<SrcPType,SrcStoreType> &a, const ParamEntry *b)
{
  return a.GetPointer()==b;
}
template <class SrcPType, class SrcStoreType>
bool operator != (const PClassPtr<SrcPType,SrcStoreType> &a, const ParamEntry *b)
{
  return a.GetPointer()!=b;
}
template <class SrcPType, class SrcStoreType>
bool operator == (const ParamEntry *b, const PClassPtr<SrcPType,SrcStoreType> &a)
{
  return a.GetPointer()==b;
}
template <class SrcPType, class SrcStoreType>
bool operator != (const ParamEntry *b, const PClassPtr<SrcPType,SrcStoreType> &a)
{
  return a.GetPointer()!=b;
}
//}@

//@{
//\name PEntryPtr against ParamEntry *comparison
template <class SrcPType>
bool operator == (const PEntryPtr<SrcPType> &a, const ParamEntry *b)
{
  return a.GetPointer()==b;
}
template <class SrcPType>
bool operator != (const PEntryPtr<SrcPType> &a, const ParamEntry *b)
{
  return a.GetPointer()!=b;
}
template <class SrcPType>
bool operator == (const ParamEntry *b, const PEntryPtr<SrcPType> &a)
{
  return a.GetPointer()==b;
}
template <class SrcPType>
bool operator != (const ParamEntry *b, const PEntryPtr<SrcPType> &a)
{
  return a.GetPointer()!=b;
}
//}@

class ParamEntryLists
{
  const ParamEntryVal &_list1, &_list2;
public:
  ParamEntryLists(const ParamEntryVal &list1, const ParamEntryVal &list2) : _list1(list1), _list2(list2) {}
  int GetSize() {return _list1.GetSize()+ (_list2.GetPointer()!=NULL ? _list2.GetSize() : 0);}
  const IParamArrayValue &operator [] ( int index ) const
  {
    int size1 = _list1.GetSize();
    if (index<size1) return _list1[index];
    return _list2[index-size1];
  }
};

template <class Functor>
bool ParamClass::ForEachEntryOrigin(const Functor &func, const ParamClass &origin) const
{
  if (_base)
  {
    bool done = _base->ForEachEntryOrigin(func,origin);
    if (done) return done;
  }
  for (int i=0; i<GetEntryCount(); i++)
  {
    ParamEntryVal entry = GetEntry(i);
    // check if this entry is visible to the caller (origin class)
    if (&(origin>>entry.GetName())!=&entry) continue;
    bool done = func(entry);
    if (done) return done;
  }
  return false;
}

inline ParamEntryVal operator *(const ConstParamEntryPtr &src)
{
  return ParamEntryVal(*src.GetClass(),*src.GetPointer());
}
inline ParamEntry &operator *(const ParamEntryPtr &src)
{
  return *src.GetPointer();
}
inline ParamEntryVal GetParamEntryVal(const ParamEntryPtr &src)
{
  return ParamEntryVal(*src.GetClass(),*src.GetPointer());
}

template<bool withBases, class FilterTraits>
ParamEntryVal ParamClass::Iterator<withBases,FilterTraits>::operator * ()
{
  return _current->IsClass() ? _current->GetEntry(_item) : *_current;
}

#endif
