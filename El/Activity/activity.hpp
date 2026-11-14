#ifdef _MSC_VER
#pragma once
#endif

#ifndef _EL_ACTIVITY_HPP
#define _EL_ACTIVITY_HPP

/**
@file
generic activity concept - any activity may be notified of any event.
*/

#include <Es/Types/lLinks.hpp>
#include <El/ParamArchive/paramArchive.hpp>

class ActivityEvent;
class ActivityClassTypeInfo;

/// activity is an object controlling any process
/**
Activity has a simulation, which is called regularly by activity owner, and event callback,
which is called when event created by Activity is triggered.
*/

class Activity: public RemoveLLinks
{
  public:
  
  /// called when a registered event is triggered
  virtual void OnEvent(ActivityEvent *event) = 0;
  
  //@{ type info manager interface
  virtual const ActivityClassTypeInfo &GetTypeInfo() const = 0;
  static void RegisterType(const ActivityClassTypeInfo *info);
  //@}
  
  //@{ serialization related functions
  virtual LSError Serialize(ParamArchive &ar) = 0;
  
  static Activity *CreateObject(ParamArchive &ar);
  static LSError LoadLLinkRef(ParamArchive &ar, LLink<Activity> &ref);
  static LSError SaveLLinkRef(ParamArchive &ar, LLink<Activity> &ref);
  //@}
};

/// event will notify activity, once something happened
class ActivityEvent
{
  LLink<Activity> _activity;
    
  public:
  ActivityEvent() {}
  ActivityEvent(Activity *activity):_activity(activity){}
  
  virtual LSError Serialize(ParamArchive &ar);
  
  static Activity *CreateObject(ParamArchive &ar);
  
  /// notify an activity the event is triggered
  void Do()
  {
    if (_activity) _activity->OnEvent(this);
  }
};

/// functions used for ParamArchive manipulation of given activity type
class ActivityClassTypeInfo
{
  RStringB _name;
  
  public:
  const RStringB &GetName() const {return _name;}
  
  ActivityClassTypeInfo(const RStringB &name):_name(name){}
  
  virtual Activity *CreateObject(ParamArchive &ar) const = 0;
  virtual Activity *LoadRef(ParamArchive &ar) const = 0;
  // virtual destructor not needed - objects of this type are always static
};

template <class ClassTypeInfo>
struct FindArrayKeyClassTypeInfoTraits
{
  typedef RStringB KeyType;
  static bool IsEqual(KeyType a, KeyType b){return a==b;}
  static KeyType GetKey(const ClassTypeInfo *a) {return a->GetName();}
};

//TODO: separate module

/// maintain functions or state connected with a given class type
template <class ClassTypeInfo>
class ClassTypeManager
{

  FindArrayKey<
    const ClassTypeInfo *, FindArrayKeyClassTypeInfoTraits<ClassTypeInfo>
  > _types;
  
  public:
  /// find given typeinfo
  const ClassTypeInfo *Find(RStringB id) const
  {
    int index = _types.FindKey(id);
    if (index<0)
    {
      ErrF("Unknown type %s",cc_cast(id));
      return NULL;
    }
    return _types[index];
  }
  
  /// register a particular typeinfo
  void Register(const ClassTypeInfo *info)
  {
    Assert( _types.FindKey(info->GetName())<0 );
    _types.Add(info);
  }
};


//extern ClassTypeManager<Activity> GActivityTypeManager;

#endif
