#ifdef _MSC_VER
#pragma once
#endif

#ifndef _EL_SERIALIZE_CLASS_HPP
#define _EL_SERIALIZE_CLASS_HPP

#include <Es/Types/enum_decl.hpp>

// some basic types

class ParamArchive;
/// interface for classes with serialization
class SerializeClass
{
public:
	//! if this function return true, no serialization is necessary
	virtual bool IsDefaultValue(ParamArchive &ar) const {return false;}
	//! serialize all values into current class
	virtual LSError Serialize(ParamArchive &ar) = 0;
	//! if IsDefaultValue returned true during Save, no class is saved
	//! and LoadDefaultValues is used instead of Serialize during Load
	virtual void LoadDefaultValues(ParamArchive &ar) {}
};

/// Used to mark constructor for serialization only
enum ForSerializationOnly {SerializeConstructor};

/// info about the type info, stored in list in the root class, used for serialization of referenced class
template <typename Type>
struct SerializeTypeInfo
{
  typedef Type *CreateFunc();
  typedef Type *LoadRefFunc(ParamArchive &ar);

  /// unique type identification
  RString _typeName;
  /// pointer to function creating an instance
  CreateFunc *_createFunc;
  /// pointer to function searching for an instance (by info given in archive)
  LoadRefFunc *_loadRefFunc;
  /// link to the next info in the list
  SerializeTypeInfo *_next;

  /// create an instance of the correct type
  Type *CreateByName(RString name)
  {
    SerializeTypeInfo *cur = this;
    while (cur)
    {
      if (stricmp(cur->_typeName, name) == 0) return (*cur->_createFunc)();
      cur = cur->_next;
    }
    return NULL;
  };
  /// find the reference to a given instance
  Type *LoadRefByName(RString name, ParamArchive &ar)
  {
    SerializeTypeInfo *cur = this;
    while (cur)
    {
      if (stricmp(cur->_typeName, name) == 0)
      {
        if (!cur->_loadRefFunc) return NULL;
        return (*cur->_loadRefFunc)(ar);
      }
      cur = cur->_next;
    }
    return NULL;
  }
};

/// declare the root of hierarchy of classes with the dynamic creation
#define DECL_SERIALIZE_TYPE_INFO_ROOT(cls) \
  static SerializeTypeInfo<cls> *_typeInfoRoot; \
  static cls *CreateObject(ParamArchive &ar);

/// declare the abstract root of hierarchy of classes with the dynamic creation
#define DECL_SERIALIZE_TYPE_INFO_ROOT_ONLY(cls) \
  DECL_SERIALIZE_TYPE_INFO_ROOT(cls) \
  virtual const SerializeTypeInfo<cls> *Get##cls##TypeInfo() const {return NULL;}

/// define the root of hierarchy of classes with the dynamic creation
#define DEFINE_SERIALIZE_TYPE_INFO_ROOT(cls) \
  SerializeTypeInfo<cls> *cls::_typeInfoRoot = NULL; \
  cls *cls::CreateObject(ParamArchive &ar) \
  { \
    RString typeInfoName; \
    if (ar.Serialize("typeInfoName", typeInfoName, 1, RString()) == LSOK && typeInfoName.GetLength() > 0) \
    { \
      return _typeInfoRoot->CreateByName(typeInfoName); \
    } \
    Fail("Error"); \
    return NULL; \
  }

/// code need to be added to the serialization of the root of hierarchy
#define SERIALIZE_TYPE_INFO_ROOT(cls) \
  if (ar.IsSaving()) \
  { \
    const SerializeTypeInfo<cls> *typeInfo = Get##cls##TypeInfo(); \
    if (typeInfo) \
    { \
      RString typeInfoName = typeInfo->_typeName; \
      CHECK(ar.Serialize("typeInfoName", typeInfoName, 1, RString())) \
    } \
  }

/// code need to be added to the SaveRef of the root of hierarchy
#define SAVE_REF_TYPE_INFO_ROOT(cls) \
  const SerializeTypeInfo<cls> *typeInfo = Get##cls##TypeInfo(); \
  if (typeInfo) \
  { \
    RString typeInfoName = typeInfo->_typeName; \
    CHECK(ar.Serialize("typeInfoName", typeInfoName, 1, RString())) \
  }

/// declare the member of hierarchy of classes with the dynamic creation
#define DECL_SERIALIZE_TYPE_INFO(cls, root) \
  struct root##TypeInfo##cls : public SerializeTypeInfo<root> \
  { /* the constructor will add the node to the list in the root */ \
    root##TypeInfo##cls(); \
  }; \
  static root##TypeInfo##cls _type##root##Info; \
  virtual const SerializeTypeInfo<root> *Get##root##TypeInfo() const {return &_type##root##Info;}

/// define the member of hierarchy of classes with the dynamic creation
#define DEFINE_SERIALIZE_TYPE_INFO(cls, root) \
  static root *cls##Create##root() \
  { \
    return new cls(SerializeConstructor); \
  } \
  cls::root##TypeInfo##cls::root##TypeInfo##cls() \
  { \
    _typeName = #cls; \
    _createFunc = &cls##Create##root; \
    _loadRefFunc = NULL; \
    /* connect to the list of infos in the root */ \
    _next = root::_typeInfoRoot; \
    root::_typeInfoRoot = this; \
  } \
  cls::root##TypeInfo##cls cls::_type##root##Info;

//////////////////////////////////////////////////////////////////////////
//
// Following macros are adding support for reference serialization (SaveRef / LoadRef) of classes
// where reference serialization differs by type.
//

/// declare the root of hierarchy of classes with the dynamic creation and reference serialization
#define DECL_SERIALIZE_TYPE_REF_INFO_ROOT(cls) \
  DECL_SERIALIZE_TYPE_INFO_ROOT(cls) \
  static cls *LoadRef(ParamArchive &ar);

/// declare the abstract root of hierarchy of classes with the dynamic creation and reference serialization
#define DECL_SERIALIZE_TYPE_REF_INFO_ROOT_ONLY(cls) \
  DECL_SERIALIZE_TYPE_INFO_ROOT_ONLY(cls) \
  static cls *LoadRef(ParamArchive &ar);

/// define the root of hierarchy of classes with the dynamic creation and reference serialization
#define DEFINE_SERIALIZE_TYPE_REF_INFO_ROOT(cls) \
  DEFINE_SERIALIZE_TYPE_INFO_ROOT(cls) \
  cls *cls::LoadRef(ParamArchive &ar) \
  { \
    RString typeInfoName; \
    if (ar.Serialize("typeInfoName", typeInfoName, 1, RString()) == LSOK && typeInfoName.GetLength() > 0) \
    { \
      return _typeInfoRoot->LoadRefByName(typeInfoName, ar); \
    } \
    Fail("Error"); \
    return NULL; \
  }

/// define the member of hierarchy of classes with the dynamic creation and reference serialization
#define DEFINE_SERIALIZE_TYPE_REF_INFO(cls, root) \
  static root *cls##Create##root() \
  { \
    return new cls(SerializeConstructor); \
  } \
  static root *cls##Load##root##Ref(ParamArchive &ar) \
  { \
    return cls::LoadRef(ar); /* retype the result to the root class */ \
  } \
  cls::root##TypeInfo##cls::root##TypeInfo##cls() \
  { \
    _typeName = #cls; \
    _createFunc = &cls##Create##root; \
    _loadRefFunc = &cls##Load##root##Ref; \
    /* connect to the list of infos in the root */ \
    _next = root::_typeInfoRoot; \
    root::_typeInfoRoot = this; \
  } \
  cls::root##TypeInfo##cls cls::_type##root##Info;

#endif
