#ifdef _MSC_VER
#pragma once
#endif

#ifndef _GRID_EVENT_LOGGER_HPP
#define _GRID_EVENT_LOGGER_HPP

#include "El/Rectangle/rectangle.hpp"
#include "Es/Strings/rString.hpp"

struct EventInTime
{
  RString name;
  int frameId;
};

TypeIsMovable(EventInTime)

template <>
struct MapClassTraits<EventInTime>: public DefMapClassTraits<EventInTime>
{
  typedef EventInTime Type;
  
  /// get a key for given a item
  static KeyType GetKey(const Type &item) {return item.name;}
};

struct EventsAtGrid
{
  GridRectangle _rect;
  
  MapStringToClass<EventInTime, AutoArray<EventInTime> > _events;
};

TypeIsMovable(EventsAtGrid)

template <>
struct MapClassTraits<EventsAtGrid>: public DefMapClassTraits<EventsAtGrid>
{
  /// key type
  typedef const GridRectangle &KeyType;
  typedef EventsAtGrid Type;
  /// calculate hash value
  static unsigned int CalculateHashValue(KeyType key)
  {
    return key.xBeg+key.zBeg*31;
  }
  
  /// compare keys, return negative when k1<k2, positive when k1>k2, zero when equal
  static int CmpKey(KeyType k1, KeyType k2)
  {
    return k1.xBeg!=k2.xBeg || k1.zBeg!=k2.zBeg || k1.xEnd!=k2.xEnd || k1.zEnd!=k2.zEnd;
  }
  
  /// get a key for given a item
  static KeyType GetKey(const Type &item) {return item._rect;}
};


/// log last events of given types related to all grid items
struct EventLogger
{
  AutoArray<RString> _events;
  
  MapStringToClass<EventsAtGrid, AutoArray<EventsAtGrid> > _gridEvents;
  
  struct ListTimestamp
  {
    RString &history;

    ListTimestamp(RString &history):history(history){}
    bool operator () (const EventInTime& ev, const MapStringToClass<EventInTime, AutoArray<EventInTime> > *container) const
    {
      history = history + Format(" %s:%d",cc_cast(ev.name),ev.frameId);
      return false;
    }
  };
  
  void Add(GridRectangle key, const char *string)
  {
    EventInTime ev;
    ev.name = string;
    ev.frameId = FreeOnDemandFrameID();
    RString history;
    EventsAtGrid &item = _gridEvents.Set(key);
    if (_gridEvents.NotNull(item))
    {
      // list timestamps of all archived events
      item._events.ForEachF(ListTimestamp(history));
      item._events.Add(ev);
    }
    else
    {
      EventsAtGrid events;
      events._rect = key;
      events._events.Add(ev);
      _gridEvents.Add(events);
    }
    
    
    
    _events.Add(Format("%s %d,%d..%d,%d, history %s",cc_cast(string),key.xBeg,key.zBeg,key.xEnd,key.zEnd,cc_cast(history)));
    if (_events.Size()>10*1024)
      _events.Delete(32); // delete multiple items at once - reduces amortized cost
  }
  
  RString Get(GridRectangle key) const
  {
    const EventsAtGrid &item = _gridEvents.Get(key);
    if (_gridEvents.NotNull(item))
    {
      // list timestamps of all archived events
      RString history;
      item._events.ForEachF(ListTimestamp(history));
      return history;
    }
    return RString();
  }

};

struct DummyEventLogger
{
  RString Get(const GridRectangle &key) const {return RString();}
  void Add(const GridRectangle &key, const char *string){}
};

#endif

