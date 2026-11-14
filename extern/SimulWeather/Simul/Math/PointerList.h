#ifndef PointerListH
#define PointerListH

#include "Simul/Base/SimMemory.h"
#include <stdlib.h>
class OutBounds{};
template<class T> class ListOfPointers
{
private:  
public:
T*  Values;
unsigned NumValues;
unsigned BufferSize;
unsigned Check(unsigned i) const
{
	if(i<0||i>NumValues)
	{
		throw OutBounds();
	}
	return i;
}
void Resize(unsigned size)
{
	if(size>=BufferSize)
	{          
#ifndef _CRTDBG_MAP_ALLOC    
	operator delete[](Values);
#else
	delete[] Values;
#endif
		BufferSize=size+1;
#ifndef _CRTDBG_MAP_ALLOC    
		Values=new T[BufferSize];
#else
		Values=new T[BufferSize];
#endif
	}
	NumValues=size;
	Values[NumValues]=NULL;
}
void Zero()
{
	for(unsigned i=0;i<BufferSize;i++)
		Values[i]=NULL;
}
void SetBufferSize(unsigned size)
{
	if(size>=BufferSize)
	{
#ifndef _CRTDBG_MAP_ALLOC    
		operator delete[](Values);
		Values=new T[size];
#else
		delete[] Values;
		Values=new T[size];
#endif
		BufferSize=size;
	}
	Values[NumValues]=NULL;
#ifdef DEBUG_LIST
	DupeCheck();
#endif
}
//------------------------------------------------------------------------------
const T& operator[](const unsigned i) const
{
#ifdef SIMUL_EXCEPTIONS
	if(Values[NumValues]!=NULL)
		throw OutBounds();
#endif
#ifdef CHECK_LIST_BOUNDS
	return Values[Check(i)];
#else
	return Values[i];
#endif
}
//------------------------------------------------------------------------------
const T& operator()(const unsigned i) const
{
	if(i<0||i>=NumValues)
		return NULL;
	return Values[i];
}
//------------------------------------------------------------------------------
T& operator()(unsigned i)
{                    
	if(i<0||i>=NumValues)
		return NULL;
	return Values[i];
}
//------------------------------------------------------------------------------
T& operator[](unsigned i)
{
#ifdef CHECK_LIST_BOUNDS
	return Values[Check(i)];
#else
	return Values[i];
#endif
}
//------------------------------------------------------------------------------
ListOfPointers(const ListOfPointers<T>&Val)
{
	NumValues=Val.NumValues;
	BufferSize=Val.BufferSize;
#ifndef _CRTDBG_MAP_ALLOC    
		Values=new T[BufferSize];
#else
		Values=new T[BufferSize];
#endif
	for(unsigned i=0;i<NumValues;i++)
		Values[i]=Val[i];
	Values[NumValues]=NULL;
}
ListOfPointers<T> &operator=(const ListOfPointers<T> &Val)
{
	if(Val.NumValues>=BufferSize)
	{
#ifndef _CRTDBG_MAP_ALLOC    
		operator delete[](Values);
#else
		delete[] Values;
#endif
		BufferSize=Val.BufferSize;
#ifndef _CRTDBG_MAP_ALLOC    
		Values=new T[BufferSize];
#else
		Values=new T[BufferSize];
#endif
	}           
	NumValues=Val.NumValues;
	for(register unsigned i=0;i<NumValues;i++)
		Values[i]=Val[i];
	Values[NumValues]=NULL;
	return *this;
}
inline ListOfPointers(void)
{
	NumValues=0;
	BufferSize=0;
	Values=NULL;
}
inline ~ListOfPointers(void)
{
#ifndef _CRTDBG_MAP_ALLOC    
	operator delete[](Values);
#else
	delete[] Values;
#endif
}               
inline void ReallyClear(void)
{
#ifndef _CRTDBG_MAP_ALLOC    
	operator delete[](Values);
#else
	delete[] Values;
#endif     
	NumValues=0;
	BufferSize=0;
	Values=NULL;
}
inline void Clear(void)
{
	NumValues=0;
}
unsigned Size() const
{
	return NumValues;
}
bool Contains(const T& Val)
{
	for(unsigned i=0;i<NumValues;i++)
	{
		if((*Values[i])==*Val)
			return true;
	}
	return false;
}
T &Add(void)
{
	if(NumValues+1>=BufferSize)
	{
		unsigned OldSize=BufferSize;
		BufferSize*=2;
		if(!BufferSize)
			BufferSize=2;
#ifndef _CRTDBG_MAP_ALLOC    
		T *NewValues=new T[BufferSize];
#else
		T *NewValues=new T[BufferSize];
#endif
		for(unsigned i=0;i<OldSize;i++)
			NewValues[i]=Values[i];
#ifndef _CRTDBG_MAP_ALLOC    
		operator delete[](Values);
#else
		delete[] Values;
#endif
		Values=NewValues;
	}
	NumValues++;
	Values[NumValues]=NULL;
	return Values[NumValues-1];
}
T &Add(T t)
{
	Add();
	(Values[NumValues-1])=t;
	return Values[NumValues-1];
}
void SortContents(void)
{
	for(unsigned i=0;i<NumValues;i++)
	{
		for(unsigned j=0;j<NumValues-1;j++)
		{
			if(*Values[j+1]<*Values[j])
			{
				T temp=Values[j+1];
				Values[j+1]=Values[j];
				Values[j]=temp;
			}
		}
	}
}
void Insert(unsigned index,T val)
{
	if(NumValues>=BufferSize)
	{
		unsigned OldSize=BufferSize;
		BufferSize*=2;
		if(!BufferSize)
			BufferSize=2;
#ifndef _CRTDBG_MAP_ALLOC    
		T *NewValues=new T[BufferSize];
#else
		T *NewValues=new T[BufferSize];
#endif
		for(unsigned i=0;i<OldSize;i++)
			NewValues[i+(i>=index)]=Values[i];
#ifndef _CRTDBG_MAP_ALLOC    
		operator delete[](Values);
#else
		delete[] Values;
#endif
		Values=NewValues;
	}
	else
	{
		for(unsigned i=index;i<NumValues;i++)
			Values[i+1]=Values[i];
	}
	Values[index]=val;
	NumValues++;
}
void Cut(const T& Val)
{
	for(register unsigned i=0;i<NumValues;i++)
	{
		if(*Values[i]==*Val)
		{
			Remove(i);
			return;
		}
    }
#ifdef SIMUL_EXCEPTIONS
	throw NotInList();
#endif
}
bool CutIfPresent(const T& Val)
{
	for(register unsigned i=0;i<NumValues;i++)
	{
		if(*Values[i]==*Val)
		{
			Remove(i);
			return true;
		}
    }
	return false;
}
void Remove(unsigned index)
{
#ifdef CHECK_LIST_BOUNDS
	Check(index);
#endif
	NumValues--;
	Values[index]=Values[NumValues];
	Values[NumValues]=NULL;
}              
void RemoveUnordered(unsigned index)
{
#ifdef CHECK_LIST_BOUNDS
	Check(index);
#endif
	NumValues--;
	Values[index]=Values[NumValues];
	Values[NumValues]=NULL;
}
unsigned Index(const T& Val)
{
	for(unsigned i=0;i<NumValues;i++)
	{
		if(*Values[i]==*Val)
			return i;
	}
	return -1;
}
friend void Shift(ListOfPointers<T> &TargetList,
	ListOfPointers<T> &OriginList,unsigned Index)
{
#ifdef CHECK_LIST_BOUNDS
	OriginList.Check(Index);
#endif
	if(TargetList.NumValues+1==TargetList.BufferSize)
	{
		TargetList.BufferSize*=2;
		if(!TargetList.BufferSize)
			TargetList.BufferSize=1;
#ifndef _CRTDBG_MAP_ALLOC
		T *NewValues=new T[TargetList.BufferSize];
#else
		T *NewValues=new T[TargetList.BufferSize];
#endif
		for(unsigned i=0;i<TargetList.NumValues;i++)
			NewValues[i]=TargetList.Values[i];
#ifndef _CRTDBG_MAP_ALLOC
		operator delete[](TargetList.Values);
#else
		delete[] TargetList.Values;
#endif
		TargetList.Values=NewValues;
	}       
	TargetList.Values[TargetList.NumValues]=OriginList.Values[Index];
	TargetList.NumValues++;
	OriginList.NumValues--;
	for(unsigned i=Index;i<OriginList.NumValues;i++)
		OriginList.Values[i]=OriginList.Values[i+1];
	OriginList.Values[OriginList.NumValues]=NULL;
	TargetList.Values[TargetList.NumValues]=NULL;
}           
friend void ShiftUnordered(ListOfPointers<T> &TargetList,
	ListOfPointers<T> &OriginList,unsigned Index)
{
#ifdef CHECK_LIST_BOUNDS
	OriginList.Check(Index);
#endif
	if(TargetList.NumValues+1==TargetList.BufferSize)
	{
		TargetList.BufferSize*=2;
		if(!TargetList.BufferSize)
			TargetList.BufferSize=1;
#ifndef _CRTDBG_MAP_ALLOC
		T *NewValues=new T[TargetList.BufferSize];
#else
		T *NewValues=new T[TargetList.BufferSize];
#endif
		for(unsigned i=0;i<TargetList.NumValues;i++)
			NewValues[i]=TargetList.Values[i];
#ifndef _CRTDBG_MAP_ALLOC
		operator delete[]( TargetList.Values);
#else
		delete[] TargetList.Values;
#endif
		TargetList.Values=NewValues;
	}
	TargetList.Values[TargetList.NumValues]=OriginList.Values[Index];
	TargetList.NumValues++;
	OriginList.NumValues--;
	OriginList.Values[Index]=OriginList.Values[OriginList.NumValues];
	OriginList.Values[OriginList.NumValues]=NULL;
	TargetList.Values[TargetList.NumValues]=NULL;
}
friend void ShiftElementByValue(ListOfPointers<T> &TargetList,
	ListOfPointers<T> &OriginList,T ptr)
{
	Shift(TargetList,OriginList,OriginList.Index(ptr));
}
friend void JoinLists(ListOfPointers<T> &TargetList,ListOfPointers<T> &OriginList)
{
	if(!OriginList.NumValues)
		return;
	if(TargetList.NumValues+OriginList.NumValues<TargetList.BufferSize)
	{
		for(unsigned i=0;i<OriginList.NumValues;i++)
			TargetList.Values[TargetList.NumValues+i]=OriginList.Values[i];
		TargetList.NumValues+=OriginList.NumValues;
		TargetList.Values[TargetList.NumValues]=NULL;
		OriginList.NumValues=0;
		if(OriginList.Values)
			OriginList.Values[0]=NULL;
		return;
	}
	T *NewValues=NULL;
	if(TargetList.NumValues+OriginList.NumValues+1>0)
	{
		unsigned i;                    
		TargetList.BufferSize=TargetList.NumValues+OriginList.NumValues+1;
#ifndef _CRTDBG_MAP_ALLOC
		NewValues=new T[TargetList.BufferSize];
#else
		NewValues=new T[TargetList.BufferSize];
#endif
		for(i=0;i<TargetList.NumValues;i++)
			NewValues[i]=TargetList.Values[i];
		for(i=0;i<OriginList.NumValues;i++)
			NewValues[TargetList.NumValues+i]=OriginList.Values[i];
	}
#ifndef _CRTDBG_MAP_ALLOC
		operator delete[]( TargetList.Values);
#else
		delete[] TargetList.Values;
#endif
	TargetList.NumValues+=OriginList.NumValues;
	OriginList.NumValues=0;
	TargetList.Values=NewValues;      
	OriginList.Values[0]=NULL;
	TargetList.Values[TargetList.NumValues]=NULL;
}
};

#endif
