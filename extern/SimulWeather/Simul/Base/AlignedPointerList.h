#ifndef AlignedPointerListH
#define AlignedPointerListH
//#include <stddef.h>
#ifndef NULL
	#define NULL 0
#endif
#ifdef PLAYSTATION_2
#define __alignof __alignof__
#endif

class OutOfBounds;
class NotInList;
class AlignedDuplicate{};

#include "Simul/Base/SimMemory.h"
#include "Simul/Base/Export.h"

template<class T> class AlignedPointerList
{
private:
T**  Values;   
public:          
int NumValues;
int Check(int i) const
{
#ifdef CHECK_LIST_BOUNDS
	if(i<0||i>=NumValues)
	{
		int errorhere=0;
		errorhere++;
		errorhere;
		throw OutOfBounds();
	}
#endif
	return i;
}
void Resize(int size)
{             
	int i;
	Clear();
	if(size<=0)
		return;
	NumValues=size;
	Values=new T*[NumValues];
	for(i=0;i<NumValues;i++)
		Values[i]=new T;
}
void Destroy(T *t)
{
	t->~T();
	delete t;
}
void Place(int idx,T *t)
{
	Destroy(Values[idx]);
	Values[idx]=t;
}
friend void JoinLists(AlignedPointerList<T> &TargetList,AlignedPointerList<T> &OriginList)
{
	T **NewValues=NULL;
	int i;
	if(TargetList.NumValues+OriginList.NumValues>0)
	{
		NewValues=new T*[TargetList.NumValues+OriginList.NumValues];
		for( i=0;i<TargetList.NumValues;i++)
		{
			NewValues[i]=TargetList.Values[i];
		}
		for( i=0;i<OriginList.NumValues;i++)
		{
			NewValues[TargetList.NumValues+i]=OriginList.Values[i];
		}
	}
	delete[] TargetList.Values;
	delete[] OriginList.Values;
	TargetList.NumValues+=OriginList.NumValues;
	OriginList.NumValues=0;
	TargetList.Values=NewValues;
	OriginList.Values=NULL;
}
//------------------------------------------------------------------------------
friend void ShiftElement(AlignedPointerList<T> &TargetList,
	AlignedPointerList<T> &OriginList,int Index)
{
	OriginList.Check(Index);
	T **NewValues=new T*[TargetList.NumValues+1];
	register int i,j;
	for(i=0;i<TargetList.NumValues;i++)
	{
		NewValues[i]=TargetList.Values[i];
	}
	NewValues[TargetList.NumValues]=OriginList.Values[Index];

	delete[](TargetList.Values);
	TargetList.NumValues++;
	TargetList.Values=NewValues;
	NewValues=NULL;
	if(OriginList.NumValues>1)
	{
		NewValues=new T*[OriginList.NumValues-1];
		for(i=0;i<OriginList.NumValues-1;i++)
		{
			NewValues[i]=OriginList.Values[i+(i>=Index)];
	    }
	}
	delete[](OriginList.Values);
	OriginList.NumValues--;
	OriginList.Values=NewValues;
	for(i=0;i<OriginList.NumValues;i++)
	{
		for(j=0;j<TargetList.NumValues;j++)
		{
			if(OriginList.Values[i]==TargetList.Values[j])
			{
				int errorhere=1;
				errorhere++;
				errorhere;
			}
		}
	}
}
//------------------------------------------------------------------------------
friend void ShiftElementByValue(AlignedPointerList<T> &TargetList,
	AlignedPointerList<T> &OriginList,T *ptr)
{
	int i=OriginList.Index(ptr);
	ShiftElement(TargetList,OriginList,i);
}
//------------------------------------------------------------------------------
const T* operator[](const int i) const
{                  
#ifdef SIMUL_EXCEPTIONS
	if(i<0||i>=NumValues)
	{
		int errorhere=0;
		errorhere++;
		errorhere;
		throw OutOfBounds();
	}
#endif
	return Values[Check(i)];
}
//------------------------------------------------------------------------------
const T* operator()(const int i) const
{
	if(i<0||i>=NumValues)
		return NULL;
	return Values[i];
}
//------------------------------------------------------------------------------
T* operator()(int i)
{                    
	if(i<0||i>=NumValues)
		return NULL;
	return Values[i];
}         
//------------------------------------------------------------------------------
T** AddressOf(int i)
{                    
	if(i<0||i>=NumValues)
		return NULL;
	return &(Values[i]);
}
//------------------------------------------------------------------------------
inline T* operator[](int i)
{
#ifdef CHECK_LIST_BOUNDS
	return Values[Check(i)];
#else
	return Values[i];
#endif
}
//------------------------------------------------------------------------------
inline AlignedPointerList(const AlignedPointerList<T>&Val)
{
	NumValues=Val.NumValues;
	Values=new T*[NumValues];
}
//------------------------------------------------------------------------------
inline AlignedPointerList(void)
{
	Values=NULL;
	NumValues=0;
}
//------------------------------------------------------------------------------
inline ~AlignedPointerList(void)
{
	Clear();
}
//------------------------------------------------------------------------------
inline void Clear(void)
{
	if(NumValues)
    {
		for(int i=0;i<NumValues;i++)
			Destroy(Values[i]);
	}
	delete[] Values;
	Values=NULL;
    NumValues=0;
}
//------------------------------------------------------------------------------
inline void Swap(int Id1,int Id2)
{
	Check(Id1);
	Check(Id2);
	T *temp=Values[Id1];
	Values[Id1]=Values[Id2];   
	Values[Id2]=temp;
}
//------------------------------------------------------------------------------
inline void Shuffle(int OldId,int NewId)
{
	int i;
	Check(OldId);
	Check(NewId);
	T *MovingValue=Values[OldId];
	if(NewId<OldId)
	{
		for(i=OldId;i>NewId;i--)
		{
			Values[i]=Values[i-1];
		}
	}
	else if(NewId>OldId)
	{
		for(i=OldId;i<NewId;i++)
		{
			Values[i]=Values[i+1];
		}
	}
	else return;
	Values[NewId]=MovingValue;
}
//------------------------------------------------------------------------------
int Size() const
{
	return NumValues;
}
//------------------------------------------------------------------------------
inline bool Contains(const T& Val)
{         
	int i;
	for(i=0;i<NumValues;i++)
	{
		if(*(Values[i])==Val)
			return true;
	}
	return false;
}          
//------------------------------------------------------------------------------
inline bool Contains(const T* Val)
{         
	int i;
	for(i=0;i<NumValues;i++)
	{
		if(Values[i]==Val)
			return true;
	}
	return false;
}
//------------------------------------------------------------------------------
AlignedPointerList<T>& operator=(const AlignedPointerList<T>&Lst)
{
	Clear();     
	NumValues=Lst.NumValues;
    Values=new T*[NumValues];
    for(int i=0;i<NumValues;i++)
	{
		Values[i]=new T;
		*(Values[i])=*(Lst.Values[i]);
    }
    return *this;
}
//------------------------------------------------------------------------------
AlignedPointerList<T> SubList(int pos,int len)
{
	AlignedPointerList<T> Ret;
	Ret.Clear();
#ifdef SIMUL_EXCEPTIONS
	if(pos+len>NumValues)
		throw OutOfBounds();
#endif
	for(int i=pos;i<pos+len;i++)
		Ret.Add(Values[i]);
	return Ret;
}
//------------------------------------------------------------------------------
T *Add(void)
{
	T **NewValues=new T*[NumValues+1];
	for(int i=0;i<NumValues;i++)
		NewValues[i]=Values[i];
	T *newptr=new(DefaultMemoryPool,__alignof(T)) T;
	NewValues[NumValues]=newptr;
	NumValues++;
	delete[](Values);
    Values=NewValues;
	return newptr;
}
//------------------------------------------------------------------------------
void Increase(int extra)
{
	int i;
	T **NewValues=new T*[NumValues+extra];
	for(i=0;i<NumValues;i++)
	{
		NewValues[i]=Values[i];
	}
	for(i=NumValues;i<NumValues+extra;i++)
		NewValues[i]=new(DefaultMemoryPool,16/*__alignof(T)*/) T();
	NumValues+=extra;
	delete[](Values);
	Values=NewValues;
}            
//------------------------------------------------------------------------------
void Reduce(int NewSize)
{
	int i;
	if(NewSize>=NumValues)
		return;
	T **NewValues=new T*[NewSize];
	for(i=0;i<NewSize;i++)
		NewValues[i]=Values[i];
	for(i=NewSize;i<NumValues;i++)
		Destroy(Values[i]);
	NumValues=NewSize;
	delete[](Values);
	Values=NewValues;
}
//------------------------------------------------------------------------------
T *Add(const T &t)
{
	Add();
	*(Values[NumValues-1])=t;
	return Values[NumValues-1];
}
//------------------------------------------------------------------------------
T *Add(T *t)
{
	int i;
	T **NewValues=new T*[NumValues+1];
    for(i=0;i<NumValues;i++)
    {
		NewValues[i]=Values[i];
	}
	NewValues[NumValues]=t;
    NumValues++;
	delete[](Values);
    Values=NewValues;
	return t;
}
//------------------------------------------------------------------------------
T *Insert(int index)
{
	int i;
	T *NewPointer=new(DefaultMemoryPool,__alignof(T)) T();
	T **NewValues=new T*[NumValues+1];
	for(i=0;i<NumValues;i++)
    {
		NewValues[i+(i>=index)]=Values[i];
    }
	delete[](Values);
    NewValues[index]=NewPointer;
    NumValues++;
    Values=NewValues;
    return NewPointer;
}        
//------------------------------------------------------------------------------
T *Insert(int index,T *t)
{
	int i;
	T **NewValues=new T*[NumValues+1];
    for(i=0;i<NumValues;i++)
    {
		NewValues[i+(i>=index)]=Values[i];
    }
	delete[](Values);
	NewValues[index]=t;
    NumValues++;
    Values=NewValues;
    return t;
}
//------------------------------------------------------------------------------
void Cut(const T* Val)
{
	int i;
	int index=-1;
    for(i=0;i<NumValues;i++)
    {
		if(Values[i]==Val)
		{	index=i;i=100000000;}
    }
#ifdef SIMUL_EXCEPTIONS
    if(index==-1)
    {
    	throw NotInList();
	}
#endif
	Remove(index);
}     
//------------------------------------------------------------------------------
void Cut(const T& Val)
{
	int i;
	int index=-1;
    for(i=0;i<NumValues;i++)
    {
		if(*(Values[i])==Val)
		{
			index=i;
			i=100000000;
		}
    }
#ifdef SIMUL_EXCEPTIONS
    if(index==-1)
    {
    	throw NotInList();
	}
#endif
	Remove(index);
}
//------------------------------------------------------------------------------
void Remove(int index)
{
	Check(index);
	Destroy(Values[index]);
	T **NewValues=NULL;
	if(NumValues>1)
	{
		NewValues=new T*[NumValues-1];
		for(int i=0;i<NumValues-1;i++)
			NewValues[i]=Values[i+(i>=index)];
	}
	delete[](Values);
	NumValues--;
	Values=NewValues;
}            
//------------------------------------------------------------------------------
void Remove(int index,int length)
{
	Check(index);  
	Check(index+length-1);
	int i;
	for(i=index;i<index+length;i++)
		Destroy(Values[i]);
	T **NewValues=NULL;
	if(NumValues>length)
	{
		NewValues=new T*[NumValues-length];
		for(i=0;i<index;i++)
			NewValues[i]=Values[i];
		for(i=index;i<NumValues-length;i++)
			NewValues[i]=Values[i+length];
	}
	delete[](Values);
	NumValues-=length;
	Values=NewValues;
}
//------------------------------------------------------------------------------
int Index(const T* Val)
{
	int i;
	int result=-1;
	for(i=0;i<NumValues;i++)
	{
		if(Values[i]==Val)
		{
			if(result==-1)
				result=i;
#ifdef SIMUL_EXCEPTIONS
			else
			{
				int errorhere=1;
				errorhere++;
				errorhere;
				throw Duplicate();
			}
#endif
		}
	}
	return result;
}             
//------------------------------------------------------------------------------
int Index(const T& Val)
{
	int i;
	int result=-1;
	for(i=0;i<NumValues;i++)
	{
		if(*Values[i]==Val)
		{
			if(result==-1)
				result=i;
#ifdef SIMUL_EXCEPTIONS
			else
				throw Duplicate();
#endif
		}
	}
	return result;
}
//------------------------------------------------------------------------------
int FindNext(const T* Val,const int index)
{
	int i;
	int result=-1;
	for(i=index;i<NumValues;i++)
	{
		if(Values[i]==Val)
		{
			if(result==-1)
				result=i;
#ifdef SIMUL_EXCEPTIONS
			else
				throw Duplicate();
#endif
		}
	}
	return result;
}
};

template<class T> class AlignedPointerGrid
{
public:
	T***  Values;
	void Check(const int i,const int j) const
	{
#ifdef SIMUL_EXCEPTIONS
		if(i<0||i>=Rows)
		{
			int errorhere=0;
			errorhere++;
			errorhere;
			throw OutOfBounds();
		}
		if(j<0||j>=Columns)
		{                
			int errorhere=0;
			errorhere++;
			errorhere;
			throw OutOfBounds();
		}
#endif
	}
	int Rows;
	int Columns;
	AlignedPointerGrid(void)
	{
		Rows=0;
		Columns=0;
		Values=NULL;
	}
	~AlignedPointerGrid(void)
	{
		Clear();
	}         
	void Destroy(T *t)
	{
		t->~T();
		delete(t,__alignof(T));
	}
	void Clear(void)
	{
		int i,j;
		for(i=0;i<Rows;i++)
		{
			for(j=0;j<Columns;j++)
			{
				Destroy(Values[i][j]);
			}
			//delete [] Values[i];
			delete[](Values[i]);
		}
		delete(Values);
		Values=NULL;
		Rows=0;
		Columns=0;
	}        
	void Place(int i,int j,T *t)
	{
		Destroy(Values[i][j]);
		Values[i][j]=t;
	}
	void Resize(int h,int w)
	{
		int i,j;
		Clear();
		Rows=h;
		Columns=w;
		Values=new T**[Rows];
		for(i=0;i<Rows;i++)
		{
			Values[i]=new T*[Columns];
			for(j=0;j<Columns;j++)
			{
				Values[i][j]=new(DefaultMemoryPool,__alignof(T)) T;
			}
		}
	}
	inline T* operator()(const int i,const int j)
	{
		return Values[i][j];
	}
	inline const T* operator()(const int i,const int j) const
	{
		return Values[i][j];
	}
};
#endif