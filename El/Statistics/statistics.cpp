#include <El/elementpch.hpp>

#include "statistics.hpp"
#include <Es/Algorithms/qsort.hpp>
#include <El/QStream/qStream.hpp>

// StatisticsAvg allows tracking basic statistics of single value

void StatisticsAvg::Clear()
{
	_min=INT_MAX,_max=INT_MIN,_sum=0,_n=0;
}
void StatisticsAvg::Count( int value, int n )
{
	if( _min>value ) _min=value;
	if( _max<value ) _max=value;
	_sum+=value;
	_n+=n;
	if( _n>=_after ) Report(),Clear();
}

void StatisticsAvg::Report()
{
	if( _n>0 ) LogF("%s %d..%d, avg %.1f",_name,_min,_max,(float)_sum/_n);
}


// StatisticsByName allows tracking histogram (by name)

static int CompareIdId( const StatisticsById::Item *t1, const StatisticsById::Item *t2 )
{
	return t1->id.Compare(t2->id);
}
static int CompareIdCount( const StatisticsById::Item *t1, const StatisticsById::Item *t2 )
{
	return t2->count-t1->count;
}


int StatisticsByName::Find( const char *name )
{
	for( int i=0; i<_data.Size(); i++ )
	{
		if( !strcmp(name,_data[i].name) ) return i;
	}
	return -1;
}

  //! find by binary ID
int StatisticsByName::Find(const void *id, int idSize)
{
  if (idSize>Item::LenId) idSize = Item::LenId;
	for( int i=0; i<_data.Size(); i++ )
	{
    const Item &item = _data[i];
    if (item.idLen!=idSize) continue;
    if (item.zero!=0) continue;
    if (memcmp(item.id,id,idSize)) continue;
    return i;
	}
	return -1;
  
}

void StatisticsByName::Sample( int factor )
{
	_factor+=factor;
}

void StatisticsByName::Count( const char *name, int count )
{
	char shortName[Item::LenId];
	strncpy(shortName,name,sizeof(shortName));
	shortName[sizeof(shortName)-1]=0;
	int index=Find(shortName);
	if( index<0 )
	{
		index=_data.Add();
		if( index<0 )
		{
			Fail("StatisticsByName buffer too small");
			return;
		}
		Item &item=_data[index];
		strcpy(item.name,shortName);
		item.count=0;		
	}
	_data[index].count+=count;
}

void StatisticsByName::Count( const void *id, int idSize, int count )
{
	int index=Find(id,idSize);
	if( index<0 )
	{
		index=_data.Add();
		if( index<0 )
		{
      LogF("StatisticsByName items: %d",_data.Size());
			LogF("StatisticsByName buffer too small");
			return;
		}
		Item &item=_data[index];
    item.zero = 0;
    if (idSize>Item::LenId) idSize = Item::LenId;
    item.idLen = idSize;
    memcpy(item.id,id,idSize);
		item.count=0;		
	}
	_data[index].count+=count;
}

StatisticsByName::StatisticsByName()
{
	_factor=0;
}
StatisticsByName::~StatisticsByName()
{
}

void StatisticsByName::Clear()
{
	_data.Clear();
	_factor = 0;
}

inline int CompareItem( const StatisticsByName::Item *t1, const StatisticsByName::Item *t2 )
{
	int d = t2->count-t1->count;
	if (d) return d;
	d = strcmp(t1->name,t2->name);
	return d;
}

inline int CompareName( const StatisticsByName::Item *t1, const StatisticsByName::Item *t2 )
{
	int d = strcmp(t1->name,t2->name);
	return d;
}


void StatisticsByName::SortData(bool sortByName)
{
  QSort(_data,sortByName ? CompareName : CompareItem);
}


void StatisticsByName::Report(QOStream &f, int minCount, bool sortByName)
{
	if( _data.Size()<=0 ) return;
	if( _factor<1 ) _factor=1;
  QSort(_data,sortByName?CompareName:CompareItem);
	for( int i=0; i<_data.Size(); i++ )
	{
		char buf[1024];
    int count = _data[i].count/_factor;
		if (count<minCount) continue;
		sprintf(buf,"%32s: %6d\r\n",_data[i].name,count);
		f.write(buf,strlen(buf));
	}
}

void StatisticsByName::Report(QOStream &f, void (*idCallback)(QOStream &f, const void *id, int idSize, int count), int minCount, bool sortByName)
{
	if( _data.Size()<=0 ) return;
	if( _factor<1 ) _factor=1;
    QSort(_data,sortByName?CompareName:CompareItem);
	for( int i=0; i<_data.Size(); i++ )
	{
		//char buf[1024];
    int count = _data[i].count/_factor;
		if (count<minCount) continue;
    idCallback(f,_data[i].id,_data[i].idLen,count);
	}
}

//! print binary ID report - provide callback
void StatisticsByName::Report(void (*idCallback)(const void *id, int idSize, int count), int minCount, bool sortByName)
{
	if( _data.Size()<=0 ) return;
	if( _factor<1 ) _factor=1;
	QSort(_data,sortByName?CompareName:CompareItem);
	for( int i=0; i<_data.Size(); i++ )
	{
    int count = _data[i].count/_factor;
		if (count<minCount) continue;
    idCallback(_data[i].id,_data[i].idLen,count);
	}
}

//! report using callback
void StatisticsByName::Report(void (*idCallback)(const char *name, int count, void *context), int minCount, bool sortByName, void *context)
{
	if( _data.Size()<=0 ) return;
	if( _factor<1 ) _factor=1;
	QSort(_data,sortByName?CompareName:CompareItem);
	for( int i=0; i<_data.Size(); i++ )
	{
    int count = _data[i].count/_factor;
		if (count<minCount) continue;
    idCallback(_data[i].name, count, context);
	}
}

void StatisticsByName::Report(int minCount, bool sortByName)
{
	if( _data.Size()<=0 ) return;
	if( _factor<1 ) _factor=1;
	QSort(_data,sortByName?CompareName:CompareItem);
	for( int i=0; i<_data.Size(); i++ )
	{
    int count = _data[i].count/_factor;
		if (count<minCount) continue;
		LogF("%32s: %6d",_data[i].name,count);
	}
}

void StatisticsByName::ReportTotal()
{
	int total = 0;
	for( int i=0; i<_data.Size(); i++ )
	{
    int count = _data[i].count/_factor;
		total += count;
	}
	LogF("%32s: %6d","==Total==",total);
}


//// {{ StatisticsById

int StatisticsById::Find(Id id) const
{
  // assume array is sorted by id
  // find by binary search or by direct interpolation

  if (_data.Size()<1) return 0;
  // find first item so that item.id>=id
  // return the item that is equal to
  // if non is found, return first item that is bigger
  // check first
  if (_data[0].id>=id) return 0;
  // check last
  int last = _data.Size()-1;
	if( _data[last].id<id ) return _data.Size();
	if( _data[last].id==id ) return last;
  // it is somewhere in between
  #if 0 //_DEBUG
  int slowIndex = -1;
	for( int i=0; i<_data.Size(); i++ )
	{
		//if( _map[i].*id>=address )
		if (_data[i].id>=id)
		{
      slowIndex = i;
      break;

		}
	}
  #endif
  // we know id is monotone
  // we may fast iterate
  // check if 1. section is enough
  int hi = last;
  int lo = 0;
  // always: _data[lo].id<=id, _map[hi].id>fAddress
  Assert( _data[lo].id<=id );
  Assert( _data[hi].id>id );
  while (hi>lo)
  {
    Assert( _data[lo].id<=id );
    Assert( _data[hi].id>id );
    if (_data[lo+1].id>id) {hi=lo;break;}
    #if 0
      int hiAddr = _data[hi].id;
      int loAddr = _data[lo].id;
      // estimate where could the result be
      float approxIndex = (id-loAddr)/float(hiAddr-loAddr)*(hi-lo)+lo;
      int approxI = toInt(approxIndex);
      if (approxI>=hi) approxI = hi-1;
      if (approxI<=lo) approxI = lo+1;
    #else
      // binary search
      int approxI = (hi+1+lo)>>1;
      Assert (approxI>lo);
    #endif
    const Item &info = _data[approxI];
    if (info.id>id) hi = approxI;
    else lo = approxI;
    Assert( _data[lo].id<=id );
    Assert( _data[hi].id>id );
  }
  Assert( _data[lo].id<=id );
  Assert( _data[lo+1].id>id || lo+1>=_data.Size());

  Assert(hi == lo);
  int fastIndex = lo;
  if (_data[fastIndex].id!=id)
  {
    fastIndex++;
    Assert(fastIndex<_data.Size());
    Assert(_data[fastIndex].id>id);
    Assert(fastIndex==0 || _data[fastIndex-1].id<id);
  }
  //Assert(fastIndex == slowIndex);
  return fastIndex;
}

void StatisticsById::Sample( int factor )
{
	_factor+=factor;
}

void StatisticsById::Count(Id id, int count )
{
	int index=Find(id);
  // some index is always found
	if( index>=_data.Size())
	{
    Assert(index<=0 || _data[index-1].id<=id);
		index=_data.Add();
		if( index<0 )
		{
      LogF("StatisticsById items: %d",_data.Size());
			LogF("StatisticsById buffer too small");
			return;
		}
		Item &item=_data[index];
    item.id = id;
		item.count=0;		
	}
  else if (_data[index].id!=id)
  {
    Assert(_data[index].id>id);
    Assert(index<=0 || _data[index-1].id<=id);
    // we need to insert the item inside
		_data.Insert(index);
		Item &item=_data[index];
    item.id = id;
		item.count=0;		
  }
	_data[index].count+=count;
}

StatisticsById::StatisticsById()
{
	_factor=0;
}
StatisticsById::~StatisticsById()
{
}

void StatisticsById::Clear()
{
	_data.Clear();
	_factor = 0;
}

void StatisticsById::Report(QOStream &f, void (*idCallback)(QOStream &f, Id id, int count), int minCount, bool sortByName)
{
	if( _data.Size()<=0 ) return;
	if( _factor<1 ) _factor=1;
	SortData(false);
	for( int i=0; i<_data.Size(); i++ )
	{
		if (_data[i].count<minCount) continue;
		//char buf[1024];
    int count = _data[i].count/_factor;
    idCallback(f,_data[i].id,count);
	}
}

void StatisticsById::SortData(bool sortByName)
{
	QSort(_data.Data(),_data.Size(),sortByName?CompareIdId:CompareIdCount);
}

//! print binary ID report - provide callback
void StatisticsById::Report(void (*idCallback)(Id id, int count), int minCount, bool sortByName)
{
	if( _data.Size()<=0 ) return;
	if( _factor<1 ) _factor=1;
	SortData(false);
	for( int i=0; i<_data.Size(); i++ )
	{
		if (_data[i].count<minCount) continue;
    int count = _data[i].count/_factor;
    idCallback(_data[i].id,count);
	}
}

void StatisticsById::ReportTotal()
{
	int total = 0;
	for( int i=0; i<_data.Size(); i++ )
	{
    int count = _data[i].count/_factor;
		total += count;
	}
	LogF("%32s: %6d","==Total==",total);
}

