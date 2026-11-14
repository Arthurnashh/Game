#ifdef _MSC_VER
#pragma once
#endif

#ifndef _EL_PARAM_ARCHIVE_HPP
#define _EL_PARAM_ARCHIVE_HPP

#include <Es/Strings/rString.hpp>
#include <Es/Containers/array.hpp>
#include <Es/Containers/staticArray.hpp>
#include <Es/Types/lLinks.hpp>
#include <Es/Types/softLinks.hpp>
#include <Es/Types/removeLinks.hpp>

#include <El/Interfaces/iClassDb.hpp>
#include <El/Interfaces/iEval.hpp>

// macros
#define ON_ERROR(err, ctx) {OnError(err, ctx); return err;}
#define CANCEL_ERROR {CancelError(); return LSError(0);}

#if _ENABLE_REPORT
void TraceError(const char *command);
#else
#define TraceError(command)
#endif

#define CHECK(command) \
  {LSError err = command; if (err != LSError(0)) {TraceError(#command); return err;}}
#define SERIAL(name) \
  CHECK(ar.Serialize(#name, _##name, 1))
#define SERIAL_DEF(name, value) \
  CHECK(ar.Serialize(#name, _##name, 1, value))
#define SERIAL_ENUM(name, value) \
  CHECK(ar.SerializeEnum(#name, _##name, 1, value))
#define SERIAL_BASE \
  CHECK(base::Serialize(ar))
#define SerializeBitField(type,ar,name,var,version,def) \
  { \
    type t=var; \
    CHECK(ar.Serialize(name, t, version, def)); \
    var=t; \
  }
#define SerializeBitBool(ar,name,var,version,def) \
  SerializeBitField(bool, ar, name, var, version, def)
#define SERIAL_BITFIELD(type, name, value) \
  SerializeBitField(type, ar, #name, _##name, 1, value)
#define SERIAL_BITBOOL(name, value) \
  SerializeBitField(bool, ar, #name, _##name, 1, value)

#define PARS_4_TO_3_FUNC(func)                        \
{                                                     \
  if (_version < minVersion)                          \
  {                                                   \
    if (!_saving) value = defValue;                   \
    return LSError(0);                                      \
  }                                                   \
  if (_saving && value == defValue) return LSError(0);      \
  LSError err = func(name, value, minVersion);        \
  if (err == ErrorNoEntry())                                \
  {value = defValue; CANCEL_ERROR;}                   \
  return err;                                         \
}

//
template <class Type>
LSError SaveRefT(LLink<Type> &value, ParamArchive &arRef)
{
  return Type::SaveLLinkRef(arRef,value);
}

template <class Type>
LSError LoadRefT(LLink<Type> &value, ParamArchive &arRef)
{
  return Type::LoadLLinkRef(arRef,value);
}

template <class Type>
LSError SaveRefT(Ref<Type> &value, ParamArchive &arRef)
{
  return Type::SaveRefRef(arRef,value);
}

template <class Type>
LSError LoadRefT(Ref<Type> &value, ParamArchive &arRef)
{
  return Type::LoadRefRef(arRef,value);
}

template <class Type, class Traits>
LSError SaveRefT(SoftLinkPerm<Type,Traits> &value, ParamArchive &arRef)
{
  return Type::SaveLinkRef(arRef,value);
}

template <class Type, class Traits>
LSError LoadRefT(SoftLinkPerm<Type,Traits> &value, ParamArchive &arRef)
{
  return Type::LoadLinkRef(arRef,value);
}

template <class Type, class Traits>
LSError SaveRefT(SoftLink<Type,Traits> &value, ParamArchive &arRef)
{
  return Type::SaveLinkRef(arRef,value);
}

template <class Type, class Traits>
LSError LoadRefT(SoftLink<Type,Traits> &value, ParamArchive &arRef)
{
  return Type::LoadLinkRef(arRef,value);
}

template <class Type>
LSError SaveRefT(Type *&value, ParamArchive &arRef)
{
  return value->SaveRef(arRef);
}

template <class Type>
LSError LoadRefT(Type *&value, ParamArchive &arRef)
{
  value = Type::LoadRef(arRef);
  return LSOK;
}

template <class Type>
LSError SaveRefT(InitPtr<Type> &value, ParamArchive &arRef)
{
  return Type::SaveInitPtrRef(arRef,value);
}

template <class Type>
LSError LoadRefT(InitPtr<Type> &value, ParamArchive &arRef)
{
  return Type::LoadInitPtrRef(arRef,value);
}

//////////////////////////////////////////////////////////////////////////

class SerializeClass;
struct EnumName;

class ParamArchive
{
  public:
    enum Pass
    {
      PassUndefined,
      PassFirst,
      PassSecond
    };
  protected:
    SRef<ClassEntry> _entry;
    int _version;
    Pass _pass;
    AutoArray<void *> _params;
    bool _saving;
    
    static RString _errorContext;
    
    static ClassDbFunctions *_defaultClassDbFunctions;
    
  public:
    static const AutoArray<void *> noParams;

    ParamArchive();
    void Close();
    virtual ~ParamArchive();
    
    bool IsSaving() const {return _saving;}
    bool IsLoading() const {return !_saving;}
    bool IsValid() const {return _entry != NULL;}
    int GetArVersion() const {return _version;}
    Pass GetPass() const {return _pass;}
    void FirstPass() {_pass = PassFirst;}
    void SecondPass() {_pass = PassSecond;}
    /// set params, return previous value
    void *SetParams(void *params, int slot=0);
    void *GetParams(int slot=0);
    
    RString GetContext(const char *member=NULL) const {return _entry->GetContext(member);}
    
    bool IsSubclass(const RStringB &name);
    void OpenSubclass(SRef<ClassEntry> entry, ParamArchive &ar);
    bool OpenSubclass(const RStringB &name, ParamArchive &ar, bool guaranteedUnique=false);
    void CloseSubclass(ParamArchive &ar);
    
    ClassEntry *OpenArray(const RStringB &name);
    void CloseArray(ClassEntry *arr);
    
    int GetEntryCount() const;
    ClassEntry *GetEntry(int i) const 
    {return _entry->GetEntry(i);}
    
    void OnError(LSError err, RString context); 
    static void CancelError();  
    static RString GetErrorContext() 
    {return _errorContext;}
    static const char *GetErrorName(LSError err);
    
    LSError ErrorNoEntry() 
    {return _defaultClassDbFunctions->ErrorNoEntry();}
    LSError ErrorStructure() 
    {return _defaultClassDbFunctions->ErrorStructure();}

    /// helper function for loading simple values, allowing verification of the value in the second pass
    template <class Type>
    LSError LoadOrVerify(const RStringB &name, Type &value)
    {
      return LoadOrVerifyEx<Type>(name,value);
    }
    
    /// like LoadOrVerify, but different intermediate type can be used for storage
    template <class LoadType, class Type>
    LSError LoadOrVerifyEx(const RStringB &name, Type &value);
    
    /// helper function for serializing simple values
    template <class Type>
    LSError SerializeSimple(const RStringB &name, Type &value, int minVersion);
    
    /// helper function for serializing simple values using intermediate type
    template <class LoadType, class Type>
    LSError SerializeSimpleEx(const RStringB &name, Type &value, int minVersion);
    
    /// serialization with a default value provided
    template <class Type, class DefType>
    LSError Serialize(const RStringB &name, Type &value, int minVersion, DefType defValue);
    
    template <class Type>
    LSError SerializeValue(const RStringB &name, Type &value, int minVersion)
    {
	    if (_version < minVersion) return LSError(0);
	    // call Serialize for both passes
	    ParamArchive arSubcls;
	    if (!OpenSubclass(name, arSubcls)) ON_ERROR(ErrorNoEntry(), name);
	    arSubcls.SetParams(GetParams());
	    LSError err = value.Serialize(arSubcls);
	    CloseSubclass(arSubcls);
	    return err;
    }

    template <class Type, class DefType>
    LSError SerializeValue(const RStringB &name, Type &value, int minVersion, const DefType &defValue)
      PARS_4_TO_3_FUNC(SerializeValue)

    //@{ serialization of native types
    LSError Serialize(const RStringB &name, bool &value, int minVersion);
    LSError Serialize(const RStringB &name, int &value, int minVersion);
    LSError Serialize(const RStringB &name, unsigned int &value, int minVersion);
    LSError Serialize(const RStringB &name, signed char &value, int minVersion);
    LSError Serialize(const RStringB &name, unsigned char &value, int minVersion);
    LSError Serialize(const RStringB &name, float &value, int minVersion);
    LSError Serialize(const RStringB &name, RString &value, int minVersion);
    LSError Serialize(const RStringB &name, SerializeClass &value, int minVersion);
    LSError SerializeDef(const RStringB &name, SerializeClass &value, int minVersion);
//#if _HELISIM_LARGE_OBJECTID
    LSError Serialize(const RStringB &name, int64 &value, int minVersion);
//  LSError Serialize(const RStringB &name, int64 &value, int minVersion, bool defValue);
//#endif  _HELISIM_LARGE_OBJECTID
    //@}
    
    //@{ serialization of enums
    LSError SerializeEnumValue(const RStringB &name, int &value, int minVersion, const EnumName *names);
    
    template <class EnumType>
    LSError SerializeEnum(const RStringB &name, EnumType &value, int minVersion)
    {
      int iVal = value;
      LSError ret = SerializeEnumValue(name,iVal,minVersion,GetEnumNames(value));
      value = EnumType(iVal);
      return ret;
    }
    
    template <class EnumType>
    LSError SerializeEnum(const RStringB &name, EnumType &value, int minVersion, EnumType defValue)
      PARS_4_TO_3_FUNC(SerializeEnum)
        
    #if NO_UNDEF_ENUM_REFERENCE && NO_ENUM_FORWARD_DECLARATION 
    template <class EnumType>
    LSError SerializeEnum(const RStringB &name, EnumType &value, int minVersion, typename EnumType::Helper::EnumType defValue)
      PARS_4_TO_3_FUNC(SerializeEnum)
    #endif
    //@}


        
    template <class Type>
    LSError Serialize(const RStringB &name, SRef<Type> &value, int minVersion)
    {
      if (_version < minVersion) return LSError(0);
      if (_saving || _pass == PassSecond)
      {
        if (!value) return LSError(0);
        ParamArchive arRef;
        if (!OpenSubclass(name, arRef)) ON_ERROR(ErrorNoEntry(), name);
        CHECK(value->Serialize(arRef))
          CloseSubclass(arRef);
      }
      else
      {
        Assert(_pass == PassFirst);
        ParamArchive arRef;
        if (!OpenSubclass(name, arRef))
        {
          value = NULL;
          return LSError(0);
        }
        value = Type::CreateObject(arRef);
        if (value) CHECK(value->Serialize(arRef))
          CloseSubclass(arRef);
      }
      return LSError(0);
    }
    /// serialization of Ref owning the object
    template <class Type>
    LSError Serialize(const RStringB &name, Ref<Type> &value, int minVersion)
    {
      if (_version < minVersion) return LSError(0);
      if (_saving || _pass == PassSecond)
      {
        if (!value) return LSError(0);
        ParamArchive arRef;
        if (!OpenSubclass(name, arRef)) ON_ERROR(ErrorNoEntry(), name);
        CHECK(value->Serialize(arRef));
        CloseSubclass(arRef);
      }
      else
      {
        Assert(_pass == PassFirst);
        ParamArchive arRef;
        if (!OpenSubclass(name, arRef))
        {
          value = NULL;
          return LSError(0);
        }
        value = Type::CreateObject(arRef);
        if (value) CHECK(value->Serialize(arRef));
        CloseSubclass(arRef);
      }
      return LSError(0);
    }
    template <class Type, class StoreAs>
    LSError Serialize(const RStringB &name, RefR<Type, StoreAs> &value, int minVersion)
    {
      if (_version < minVersion) return LSError(0);
      if (_saving || _pass == PassSecond)
      {
        if (!value) return LSError(0);
        ParamArchive arRef;
        if (!OpenSubclass(name, arRef)) ON_ERROR(ErrorNoEntry(), name);
        CHECK(value->Serialize(arRef))
        CloseSubclass(arRef);
      }
      else
      {
        Assert(_pass == PassFirst);
        ParamArchive arRef;
        if (!OpenSubclass(name, arRef))
        {
          value = NULL;
          return LSError(0);
        }
        value = Type::CreateObject(arRef);
        if (value) CHECK(value->Serialize(arRef))
        CloseSubclass(arRef);
      }
      return LSError(0);
    }
    
    template <class RefType>
    LSError SerializeRef(const RStringB &name, RefType &value, int minVersion)
    {
      typedef typename RefType::ItemType Type;
      if (_version < minVersion) return LSError(0);
      if (_saving)
      {
        if (!value) return LSError(0);
        ParamArchive arRef;
        if (!OpenSubclass(name, arRef)) ON_ERROR(ErrorNoEntry(), name);
        CHECK( SaveRefT(value,arRef) );
        //CHECK(value->SaveRef(arRef))
        CloseSubclass(arRef);
      }
      else
      {
        if (_pass != PassSecond) return LSError(0);
        ParamArchive arRef;
        if (!OpenSubclass(name, arRef))
        {
          value = NULL;
          return LSError(0);
        }
        arRef._pass = PassFirst;  // allow LoadRef
        CHECK( LoadRefT(value, arRef) );
        //value = dynamic_cast<Type *>(Type::LoadRef(arRef));
        CloseSubclass(arRef);
      }
      return LSError(0);
    }
    
    template <class Type, class Allocator>
    LSError SerializeArray(const RStringB &name, AutoArray<Type, Allocator> &value, int minVersion)
    {
      return SerializeArray<Type, Allocator, Type>(name, value, minVersion);
    }

    template <class Type, class Allocator, class TypeEntry>
    LSError SerializeArray(const RStringB &name, AutoArray<Type, Allocator> &value, int minVersion)
    {
      if (_version < minVersion) return LSError(0);
      if (_saving)
      {
        if (value.Size()==0) return LSError(0); // default value - empty array
        SRef<ClassEntry> array = _entry->AddArray(name);
        array->ReserveArrayElements(value.Size());
        for (int i=0; i<value.Size(); i++)
        {
          array->AddValue(value[i]);
        }
      }
      else
      {
        if (_pass != PassFirst) return LSError(0);
        SRef<ClassEntry> array = _entry->FindEntry(name);
        if (!array)
        {
          value.Clear();
          return LSError(0);
        }
        int n = array->GetSize();
        value.Realloc(n);
        value.Resize(n);
        for (int i=0; i<n; i++)
        {
          SRef<ClassArrayItem> item = (*array)[i];
          TypeEntry temp = *item;  // Note: GCC protest to call value[i] = TypeEntry(*item); because of ambiguity
          value[i] = temp;
        }
      }
      return LSError(0);
    }
    template <class Type, class TypeEntry>
    LSError SerializeArray(const RStringB &name, StaticArrayAuto<Type> &value, int minVersion)
    {
      if (_version < minVersion) return LSError(0);
      if (_saving)
      {
        if (value.Size()==0) return LSError(0); // default value - empty array
        SRef<ClassEntry> array = _entry->AddArray(name);
        array->ReserveArrayElements(value.Size());
        for (int i=0; i<value.Size(); i++)
        {
          array->AddValue(value[i]);
        }
      }
      else
      {
        if (_pass != PassFirst) return LSError(0);
        SRef<ClassEntry> array = _entry->FindEntry(name);
        if (!array)
        {
          value.Clear();
          return LSError(0);
        }
        int n = array->GetSize();
        value.Resize(n);
        for (int i=0; i<n; i++)
        {
          SRef<ClassArrayItem> item = (*array)[i];
          TypeEntry temp = *item;
          value[i] = Type(temp);
        }
      }
      return LSError(0);
    }
    
    template <class Type>
    LSError SerializeItemDef(const char *name, Type &valueI, int minVersion)
    {
      if (_version < minVersion) return LSError(0);
      ParamArchive arItem;
      if (IsSaving() && valueI.IsDefaultValue(*this))
      {
        return LSError(0);
      }
      if (!OpenSubclass(name, arItem, true))
      {
        if (IsLoading())
        {
          // use default values
          valueI.LoadDefaultValues(*this);
        }
        else
        {
          ON_ERROR(ErrorNoEntry(), _entry->GetContext() + RString(".") + RString(name));
        }
      }
      else
      {
        CHECK(valueI.Serialize(arItem))
          CloseSubclass(arItem);
      }
      return LSError(0);
    }
    template <class Type>
    LSError SerializeItem(const char *name, Type &valueI, int minVersion)
    {
      if (_version < minVersion) return LSError(0);
      ParamArchive arItem;
      if (!OpenSubclass(name, arItem, true))
      {
        ON_ERROR(ErrorNoEntry(), _entry->GetContext() + RString(".") + RString(name));
      }
      else
      {
        CHECK(valueI.Serialize(arItem));
        CloseSubclass(arItem);
      }
      return LSError(0);
    }
    template <class Type>
    LSError SerializeItem(SRef<ClassEntry> entry, Type &valueI, int minVersion)
    {
      if (_version < minVersion) return LSError(0);
      ParamArchive arItem;
      OpenSubclass(entry, arItem);
      CHECK(valueI.Serialize(arItem));
      CloseSubclass(arItem);
      return LSError(0);
    }
    /// serialize array of objects using SerializeClass interface
    template <class Type, class Allocator>
    LSError Serialize(const RStringB &name, AutoArray<Type, Allocator> &value, int minVersion)
    {
      if (_version < minVersion) return LSError(0);
      // call Serialize for both passes
      ParamArchive arCls;
      int n;
      if (_saving)
      {
        n = value.Size();
        if( n==0 ) return LSError(0);

        if (!OpenSubclass(name, arCls)) ON_ERROR(ErrorNoEntry(), name);

        // save item number for compatibility with older readers
        CHECK(arCls.Serialize("items", n, minVersion))

          for (int i=0; i<n; i++)
          {
            char buffer[256];
            sprintf(buffer, "Item%d", i);
            CHECK(arCls.SerializeItem(buffer,value[i],minVersion));
          }
      }
      else
      {
        bool hasItems = false;
        if (_pass == PassSecond)
        {
          n = value.Size();
          if (!OpenSubclass(name, arCls))
          {
            if( n==0 ) return LSError(0);
            ON_ERROR(ErrorNoEntry(), name);
          }
          SRef<ClassEntry> entry = arCls._entry->FindEntry("items");
          if (entry) hasItems = true;
        }
        else
        {
          if (!OpenSubclass(name, arCls))
          {
            value.Clear();
            return LSError(0);
          }

          LSError err = arCls.Serialize("items", n, minVersion);
          if (err==ErrorNoEntry())
          {
            CancelError();
            n = arCls.GetEntryCount();
          }
          else if (err!=LSError(0))
          {
            return err;
          }
          else
          {
            hasItems = true;
          }
          value.Realloc(n);
          value.Resize(n);
        }

        if (!hasItems)
        {
          for (int i=0; i<n; i++)
          {
            SRef<ClassEntry> entry = arCls.GetEntry(i);
            CHECK(arCls.SerializeItem(entry,value[i],minVersion));
          }
        }
        else
        {
          for (int i=0; i<n; i++)
          {
            char buffer[256];
            sprintf(buffer, "Item%d", i);
            CHECK(arCls.SerializeItem(buffer,value[i],minVersion));
          }
        }
      }
      CloseSubclass(arCls);
      return LSError(0);
    }

    template <class Allocator>
    LSError Serialize(const RStringB &name, AutoArray<int, Allocator> &value, int minVersion) 
    { 
      return SerializeArray(name, value, minVersion); 
    }
    template <class Allocator>
    LSError Serialize(const RStringB &name, AutoArray<char, Allocator> &value, int minVersion) 
    { 
      return SerializeArray< char, Allocator, int>(name, value, minVersion); 
    }
    template <class Allocator>
    LSError Serialize(const RStringB &name, AutoArray<float, Allocator> &value, int minVersion) 
    { 
      return SerializeArray(name, value, minVersion); 
    }
    
    template <class Type, class Allocator>
    LSError SerializeDef(const RStringB &name, AutoArray<Type, Allocator> &value, int minVersion)
    {
      if (_version < minVersion) return LSError(0);
      // call Serialize for both passes
      ParamArchive arCls;
      int n;
      if (_saving)
      {
        n = value.Size();
        if( n==0 ) return LSError(0);
        
        if (!OpenSubclass(name, arCls)) ON_ERROR(ErrorNoEntry(), name);
        
        CHECK(arCls.Serialize("items", n, minVersion))
      }
      else if (_pass == PassSecond)
      {
        n = value.Size();
        if (!OpenSubclass(name, arCls))
        {
          if( n==0 ) return LSError(0);
          ON_ERROR(ErrorNoEntry(), name);
        }
      }
      else
      {
        if (!OpenSubclass(name, arCls))
        {
          value.Clear();
          return LSError(0);
        }
        CHECK(arCls.Serialize("items", n, minVersion))
        value.Realloc(n);
        value.Resize(n);
      }
      for (int i=0; i<n; i++)
      {
        char buffer[256];
        sprintf(buffer, "Item%d", i);
        CHECK(arCls.SerializeItemDef(buffer,value[i],minVersion));
      }
      CloseSubclass(arCls);
      return LSError(0);
    }
    /***/
    /*!
      \patch 5164 Date 6/23/2007 by Ondra
  - Fixed: Error when loading a save from a complex mission (containing a lot of variables).
    */
    template<class Type, class Container>
    LSError Serialize(const RStringB &name, MapStringToClass<Type, Container> &value, int minVersion)
    {
      if (_version < minVersion) return LSError(0);
      // call Serialize for both passes
      ParamArchive arCls;
      if (_saving) _entry->Delete(name); // When we serialize some special classes into the User Profile, we need to rewrite the existing values
      if (!OpenSubclass(name, arCls)) ON_ERROR(ErrorNoEntry(), name);
      if (_saving)
      {
        int ii = 0, nn = value.NItems();
        CHECK(arCls.Serialize("items", nn, minVersion))
        if (nn > 0)
        {
          // !!! avoid GetTable when NItems == 0
          int n = value.NTables();
          // save the number of tables so that load can get identical layout
          CHECK(arCls.Serialize("slots", n, minVersion, 0))
          for (int i=0; i<n; i++)
          {
            Container &container = value.GetTable(i);
            int m = container.Size();
            for (int j=0; j<m; j++)
            {
              Type &item = container[j];
              char buffer[256];
              sprintf(buffer, "Item%d", ii++);
              ParamArchive arItem;
              if (!arCls.OpenSubclass(buffer, arItem,true)) ON_ERROR(ErrorNoEntry(), name + RString(".") + RString(buffer));
              CHECK(item.Serialize(arItem))
                arCls.CloseSubclass(arItem);
            }
          }
        }
      }
      else if (_pass != PassSecond)
      {
        value.Clear();
        int nn;
        CHECK(arCls.Serialize("items", nn, minVersion))
        // we must avoid rebuild during serialization
        // as we need to guarantee same order in both passes
        int nTables;
        CHECK(arCls.Serialize("slots", nTables, minVersion, 0))
        if (nTables>0)
        { // when we know the number of tables saved, use the same number for loading
          value.Rebuild(nTables);
        }
        else
        { // with old save we can at least avoid rebuild
          // while this is not 100 % robust, it helps in a majority of cases
          value.Reserve(nn);
        }
        for (int ii=0; ii<nn; ii++)
        {
          char buffer[256];
          sprintf(buffer, "Item%d", ii);
          ParamArchive arItem;
          if (!arCls.OpenSubclass(buffer, arItem, true)) ON_ERROR(ErrorNoEntry(), name + RString(".") + RString(buffer));
          Type item;
          CHECK(item.Serialize(arItem))
          value.Add(item);
          arCls.CloseSubclass(arItem);
        }
      }
      else
      {
        int ii = 0, nn = value.NItems();

        // verify the number of items did not change since the 1st pass
        // if it has, we have a problem
        arCls.FirstPass();
        int fileItems = nn;
        arCls.Serialize("items", fileItems, minVersion);
        arCls.SecondPass();
        if (fileItems!=nn)
        {
          OnError(LSStructure,RString());
          RptF("%s: Item count not matching: %d!=%d",cc_cast(GetErrorContext()),fileItems,nn);
          return LSStructure;
        }

        if (nn > 0)
        {
          // !!! avoid GetTable when NItems == 0
          int n = value.NTables();
          for (int i=0; i<n; i++)
          {
            Container &container = value.GetTable(i);
            int m = container.Size();
            for (int j=0; j<m; j++)
            {
              Type &item = container[j];
              char buffer[256];
              sprintf(buffer, "Item%d", ii++);
              ParamArchive arItem;
              if (!arCls.OpenSubclass(buffer, arItem, true)) ON_ERROR(ErrorNoEntry(), name + RString(".") + RString(buffer));
              CHECK(item.Serialize(arItem))
              arCls.CloseSubclass(arItem);
            }
          }
        }
      }
      CloseSubclass(arCls);
      return LSError(0);
    }
    template <class Type, class Allocator>
    LSError Serialize(const RStringB &name, RefArray<Type,Allocator> &value, int minVersion)
    {
      if (_version < minVersion) return LSError(0);
      // call Serialize for both passes
      ParamArchive arCls;
      int n;
      if (_saving)
      {
        n = value.Size();
        if( n==0 ) return LSError(0);
        
        if (!OpenSubclass(name, arCls)) ON_ERROR(ErrorNoEntry(), name);
        
        CHECK(arCls.Serialize("items", n, minVersion))
      }
      else if (_pass == PassSecond)
      {
        n = value.Size();
        if (!OpenSubclass(name, arCls))
        {
          if( n==0 ) return LSError(0);
          ON_ERROR(ErrorNoEntry(), name);
        }
      }
      else
      {
        if (!OpenSubclass(name, arCls))
        {
          value.Clear();
          return LSError(0);
        }
        CHECK(arCls.Serialize("items", n, minVersion))
        value.Realloc(n);
        value.Resize(n);
      }
      for (int i=0; i<n; i++)
      {
        char buffer[256];
        sprintf(buffer, "Item%d", i);
        CHECK(arCls.Serialize(buffer, value[i], minVersion));
      }
      CloseSubclass(arCls);
      return LSError(0);
    }
    template <class Type>
    LSError Serialize(const RStringB &name, StaticArray< Ref<Type> > &value, int minVersion)
    {
      if (_version < minVersion) return LSError(0);
      // call Serialize for both passes
      ParamArchive arCls;
      int n;
      if (_saving)
      {
        n = value.Size();
        if( n==0 ) return LSError(0);
        
        if (!OpenSubclass(name, arCls)) ON_ERROR(ErrorNoEntry(), name);
        
        CHECK(arCls.Serialize("items", n, minVersion))
      }
      else if (_pass == PassSecond)
      {
        n = value.Size();
        if (!OpenSubclass(name, arCls))
        {
          if( n==0 ) return LSError(0);
          ON_ERROR(ErrorNoEntry(), name);
        }
      }
      else
      {
        if (!OpenSubclass(name, arCls))
        {
          value.Clear();
          return LSError(0);
        }
        CHECK(arCls.Serialize("items", n, minVersion))
        value.Resize(n);
      }
      for (int i=0; i<n; i++)
      {
        char buffer[256];
        sprintf(buffer, "Item%d", i);
        CHECK(arCls.Serialize(buffer, value[i], minVersion))
      }
      CloseSubclass(arCls);
      return LSError(0);
    }
    template <class Type, class StoreAs>
    LSError Serialize(const RStringB &name, AutoArray< RefR<Type, StoreAs> > &value, int minVersion)
    {
      if (_version < minVersion) return LSError(0);
      // call Serialize for both passes
      ParamArchive arCls;
      int n;
      if (_saving)
      {
        n = value.Size();
        if( n==0 ) return LSError(0);

        if (!OpenSubclass(name, arCls)) ON_ERROR(ErrorNoEntry(), name);

        CHECK(arCls.Serialize("items", n, minVersion))
      }
      else if (_pass == PassSecond)
      {
        n = value.Size();
        if (!OpenSubclass(name, arCls))
        {
          if( n==0 ) return LSError(0);
          ON_ERROR(ErrorNoEntry(), name);
        }
      }
      else
      {
        if (!OpenSubclass(name, arCls))
        {
          value.Clear();
          return LSError(0);
        }
        CHECK(arCls.Serialize("items", n, minVersion));
        value.Realloc(n);
        value.Resize(n);
      }
      for (int i=0; i<n; i++)
      {
        char buffer[256];
        sprintf(buffer, "Item%d", i);
        CHECK(arCls.Serialize(buffer, value[i], minVersion));
      }
      CloseSubclass(arCls);
      return LSError(0);
    }
    template <class Type>
    LSError Serialize(const RStringB &name, AutoArray< SRef<Type> > &value, int minVersion)
    {
      if (_version < minVersion) return LSError(0);
      // call Serialize for both passes
      ParamArchive arCls;
      int n;
      if (_saving)
      {
        n = value.Size();
        if( n==0 ) return LSError(0);

        if (!OpenSubclass(name, arCls)) ON_ERROR(ErrorNoEntry(), name);

        CHECK(arCls.Serialize("items", n, minVersion))
      }
      else if (_pass == PassSecond)
      {
        n = value.Size();
        if (!OpenSubclass(name, arCls))
        {
          if( n==0 ) return LSError(0);
          ON_ERROR(ErrorNoEntry(), name);
        }
      }
      else
      {
        if (!OpenSubclass(name, arCls))
        {
          value.Clear();
          return LSError(0);
        }
        CHECK(arCls.Serialize("items", n, minVersion));
        value.Realloc(n);
        value.Resize(n);
      }
      for (int i=0; i<n; i++)
      {
        char buffer[256];
        sprintf(buffer, "Item%d", i);
        CHECK(arCls.Serialize(buffer, value[i], minVersion));
      }
      CloseSubclass(arCls);
      return LSError(0);
    }
    template <class Container>
    LSError SerializeRefs(const RStringB &name, Container &value, int minVersion)
    {
      if (_version < minVersion) return LSError(0);
      if (!_saving && _pass != PassSecond) return LSError(0);
      ParamArchive arCls;
      int n;
      if (_saving)
      {
        n = value.Size();
        if( n==0 ) return LSError(0);
        if (!OpenSubclass(name, arCls)) ON_ERROR(ErrorNoEntry(), name);
        CHECK(arCls.Serialize("items", n, minVersion))
      }
      else
      {
        if (!OpenSubclass(name, arCls))
        {
          value.Clear();
          return LSError(0);
        }
        // cannot use serialize - second pass doesn's serialize int
        SRef<ClassEntry> entry = arCls._entry->FindEntry("items");
        if (!entry) ON_ERROR(ErrorNoEntry(), name + RString(".items"));
        n = *entry;
        value.Realloc(n);
        value.Resize(n);
      }
      for (int i=0; i<n; i++)
      {
        char buffer[256];
        sprintf(buffer, "Item%d", i);
        CHECK(arCls.SerializeRef(buffer, value[i], minVersion))
      }
      CloseSubclass(arCls);
      return LSError(0);
    }
};

/**
Note: DefType is used so that it is possible to pass integer or double default value for a float,
or to use similar compatible types 
*/

template <class Type, class DefType>
LSError ParamArchive::Serialize(const RStringB &name, Type &value, int minVersion, DefType defValue)
{
  // check if the value was already present in the version we are loading, if not, provide the default one
  if (_version < minVersion)
  {
    if (!_saving) value = defValue;
    return LSError(0);
  }
  if (_saving && value == defValue) return LSError(0);
  LSError err = Serialize(name, value, minVersion);
  // if the value is not found, load the default one
  if (err == ErrorNoEntry()) {value = defValue; CANCEL_ERROR;}
  return err;
}

/// external serialization with a default value provided
template <class Type>
LSError Serialize(ParamArchive &ar, const RStringB &name, Type &value, int minVersion, const Type &defValue)
{
  if (ar.GetArVersion() < minVersion)
  {
    if (ar.IsLoading() && ar.GetPass() == ParamArchive::PassFirst) value = defValue;
    return LSError(0);
  }
  if (ar.IsSaving() && value == defValue) return LSError(0);
  LSError err = Serialize(ar, name, value, minVersion);
  if (err == ar.ErrorNoEntry())
  {
    value = defValue;
    ar.CancelError(); 
    return LSError(0);
  }
  return err;
}

#endif
