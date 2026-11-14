#ifdef _MSC_VER
#pragma once
#endif

#ifndef ping_loc_hpp
#define ping_loc_hpp

/// analyze ping information, maintain closest country/region information
#include <Es/Containers/array.hpp>
#include <Es/Strings/rString.hpp>

template <class Type>
inline RString FormatValue(Type value) {return value;}

template <>
inline RString FormatValue(int value) {return Format("%d",value);}

template <typename Id, typename Value>
class ValueLocalize
{
  struct Item
  {
    Id id;

    Value sum;
    Value sum2;
    Value sumInv;
    Value prod;
    int count;

    Item():id(),sum(),sum2(),sumInv(),prod(),count(0){}
    explicit Item(Id id):id(id),sum(),sum2(),sumInv(),prod(),count(0){}

    void Add(Value value)
    {
      Assert(value>0);
      sum += value;
      sum2 += value*value;
      sumInv += 1/value;
      prod *= value;
      count++;
    }

    Value ArithmeticMean() const {return sum/count;}
    Value GeometricMean() const {return pow(prod,1/count);}
    Value HarmonicMean() const {return count/sumInv;}

    ClassIsMovableZeroed(Item);
  };

  struct FindArrayKeyTraitsItem
  {
    typedef const Id &KeyType;
    static bool IsEqual(KeyType a, KeyType b) {return a==b;}
    static KeyType GetKey(const Item &a) {return a.id;}
  };

  FindArrayKey<Item,FindArrayKeyTraitsItem> _stats;

  public:
  bool IsValid() const {return _stats.Size()>=2;}
  void AddSample(Id id, Value value)
  {
    int index = _stats.FindKey(id); 
    if (index<0)
    {
      index = _stats.Add(Item(id));
    }
    _stats[index].Add(value);
  }
  Id Best() const
  {
    bool set = false;
    Value bestValue = Value();
    Id bestId = Id();
    for (int i=0; i<_stats.Size(); i++)
    {
      if (_stats[i].count>0)
      {
        Value vi = _stats[i].HarmonicMean();
        LogF("%s: %f (%d)",cc_cast(FormatValue(_stats[i].id)),vi,_stats[i].count);
        if (!set || bestValue>vi) set = true,bestValue = vi,bestId = _stats[i].id;
      }
    }
    LogF("Best: %s",cc_cast(FormatValue(bestId)));
    return bestId;
  }

};

/// select country/region with best ping
class PingLocalize
{
  ValueLocalize<RString,float> _country;
  ValueLocalize<int,float> _region;
  ValueLocalize<int,float> _subRegion;

  public:

  void Add(RString country, int region, float ping);

  void Best(RString &country, int &region, int &subRegion);

};

/// determine my own longitude / latitude based on ping to servers with known longitute/latitude

class PingGeoLocalize
{
  struct Item
  {
    float lat,lng;
    float ping;
    const char *id;

    Item(){}
    Item(float lat, float lng, float ping, const char *id):lat(lat),lng(lng),ping(ping),id(id){}

    ClassIsMovableZeroed(Item);
  };

  AutoArray<Item> _items;

  public:

  void Add(int latitude, int longitude, float ping, const char *id);

  void Best(int &latitude, int &longitude);
};

#endif

