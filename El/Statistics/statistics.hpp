#ifdef _MSC_VER
#pragma once
#endif

#ifndef _STATISTICS_HPP
#define _STATISTICS_HPP

#include <Es/Containers/array.hpp>

class QOStream;

#include <Es/Memory/debugAlloc.hpp>

class StatisticsByName
{
	public:
	struct Item
	{
		enum {LenName=128-4};
    enum {LenId = LenName-4-sizeof(int)*2};
		// instead of name ID can be given
		// it is stored as four zero bytes and 4B 
		union
		{
			char name[LenName];
			struct
			{
        int zero; // zero - to guarantee corresponding string is empty
        int idLen;
				char id[LenId]; // name id 
			};
		};
		int count;
		ClassIsSimple(Item);
	};

	private:
	AutoArray<Item,DebugAlloc> _data;
	int _factor; // divide data by factor - enables avg calculation

  //! find by name
	int Find( const char *name );
  //! find by binary ID
	int Find(const void *id, int idSize);

	public:
	StatisticsByName();
	~StatisticsByName();

	void Clear();
	void Count( const char *name, int count=1 );
	void Count( const void *id, int idSize, int count=1 );
	void Report(int minCount=0, bool sortByName=false);
	void ReportTotal();
	void Report( QOStream &f, int minCount=0, bool sortByName=false);
  //! print binary ID report - provide callback
	void Report(QOStream &f, void (*idCallback)(QOStream &f, const void *id, int idSize, int count), int minCount=0, bool sortByName=false);
  //! print binary ID report - provide callback
	void Report(void (*idCallback)(const void *id, int idSize, int count), int minCount=0, bool sortByName=false );
  //! Report using callback
  void Report(void (*idCallback)(const char *name, int count, void *context), int minCount, bool sortByName, void *context);

  void SortData(bool sortByName);

  void Sample(int factor=1);
	int NSamples() const {return _factor;}

  //@{ array like access
  int Size() const {return _data.Size();}
  const Item &Get(int i) const {return _data.Get(i);}
  //@}
  
  /// template report
  template <class Print>
  void Report(const Print &print, bool sortByName=false)
  {
	  if( _data.Size()<=0 ) return;
	  if( _factor<1 ) _factor=1;
    SortData(sortByName);
	  for( int i=0; i<_data.Size(); i++ )
	  {
      int count = _data[i].count/_factor;
      print(_data[i].name,count);
	  }
  }
};


class StatisticsById
{
	public:
	struct Id
	{
	  // id is pair of 32-bit values
    int _id1;
    int _id2;
    Id(){}
    Id(int id1, int id2=0)
    :_id1(id1),_id2(id2)
    {
    }
    int Compare(const Id &with) const
    {
      int r = _id1-with._id1;
      if (r) return r;
      return _id2-with._id2;
    }
    __forceinline bool operator == (const Id &with) const {return Compare(with)==0;}
    __forceinline bool operator != (const Id &with) const {return Compare(with)!=0;}
    __forceinline bool operator < (const Id &with) const {return Compare(with)<0;}
    __forceinline bool operator > (const Id &with) const {return Compare(with)>0;}
    __forceinline bool operator <= (const Id &with) const {return Compare(with)<=0;}
    __forceinline bool operator >= (const Id &with) const {return Compare(with)>=0;}
	};
	struct Item
	{
	  // id is pair of 32-bit values
  	Id id;
		int count;
		ClassIsSimple(Item);
	};

	private:
	AutoArray<Item,DebugAlloc> _data;
	int _factor; // divide data by factor - enables avg calculation

  //! find by binary ID
	int Find(Id id) const;

	public:
	StatisticsById();
	~StatisticsById();

	void Clear();
	void Count(Id id, int count=1);
  void SortData(bool sortByName);
  //! print binary ID report - provide callback
	void Report(QOStream &f, void (*idCallback)(QOStream &f, Id id, int count), int minCount=0, bool sortByName=false);
  //! print binary ID report - provide callback
	void Report(void (*idCallback)(Id id, int count), int minCount=0, bool sortByName=false );
	void ReportTotal();
	void Sample(int factor=1);
	int NSamples() const {return _factor;}

  //@{ array like access
  int Size() const {return _data.Size();}
  const Item &Get(int i) const {return _data.Get(i);}
  //@}
};

class StatisticsAvg
{
	const char *_name;
	int _min,_max,_sum,_n;
	int _after;

	public:
	StatisticsAvg( const char *name, int after=8*1024 ):_name(name),_after(after){}
	void Clear();
	void Count( int value, int n=1 );
	void Report();
};

class StatEventRatio
{
	const char *_name;
	int _event,_total,_maxCount;

	public:
	StatEventRatio(const char *event, int maxCount=1000)
	{
		_name = event,_event=_total=0;_maxCount=maxCount;
	}
	void Count(bool happened)
	{
		_event += happened;
		_total += 1;
		if (_total>_maxCount)
		{
			LogF("%s ratio: %.3f",_name,_event*1.0f/_total);
			_total = 0;
			_event = 0;
		}
	}
};



#endif

