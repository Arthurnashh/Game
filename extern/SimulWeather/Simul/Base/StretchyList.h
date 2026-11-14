#ifndef StretchyListH
#define StretchyListH         
//------------------------------------------------------------------------------


#ifdef PLAYSTATION_2
#define NULL 0
#else
#ifndef NDEBUG
#define LIST_EXCEPTIONS
#endif
#endif

#ifdef PLAYSTATION_2
#include <algorithm.h>
#else
#include <algorithm>
#endif

#include "Simul/Base/SimMemory.h"
#include "Simul/Base/Export.h"
class StretchyItemListOutOfBounds{};
class StretchyItemListDuplication{};
class StretchyItemListNotInList{};

/// \internal Class template for a list of instances of class T.

/// The list is stretchy (elastic), meaning that it keeps a buffer equal to or greater than
/// the size of the list so removing items does not de-allocate memory.
template<class T> class std::vector
{
public:
T*  Values;
unsigned NumValues;
unsigned BufferSize;
unsigned Check(unsigned i) const
{
#ifdef LIST_EXCEPTIONS
		if(i<0||i>=NumValues)
			throw StretchyItemListOutOfBounds();
#endif
		return i;
	}
	bool DupeCheck()
	{
		for(unsigned i=0;i<NumValues;i++)
		{
			for(unsigned j=i+1;j<NumValues;j++)
			{
				if(Values[i]==Values[j])  
				{
	#ifdef LIST_EXCEPTIONS
					throw StretchyItemListDuplication();
					#else
					return true;
	#endif
				}
			}
		}
		return false;
	}
/// Resize the list to size; each value will have the default properties. Only call this if you're going to fill in ALL the values immediately.
	void Resize(unsigned size)
	{
		if(size>BufferSize)
		{          
			for(unsigned i=0;i<BufferSize;i++)
				Values[i].~T();
			DestroyValues();
			Values=CreateValues(size);
			BufferSize=size;
		}
		NumValues=size;
	}            
/// Resize the list to size, making each value equal to Val.
	void Resize(unsigned size,T t)
	{
		if(size>BufferSize)
		{
			for(unsigned i=0;i<BufferSize;i++)
				Values[i].~T();
			DestroyValues();
			Values=CreateValues(size);
			BufferSize=size;
		}              
		for(unsigned i=0;i<size;i++)
		{
			Values[i]=t;
		}
		NumValues=size;
	}       
	void RotateDown()
	{
		T first=Values[0];
		for(unsigned i=1;i<NumValues;i++)
			Values[i-1]=Values[i];
		Values[NumValues-1]=first;
	}
/// Swap the values at Id1 and Id2.
	void Swap(unsigned Id1,unsigned Id2)
	{
		Check(Id1);
		Check(Id2);
		T temp=Values[Id1];
		Values[Id1]=Values[Id2];
		Values[Id2]=temp;
	}  
/// Set the buffer size if size>List.Size(), otherwise do nothing.
void SetBufferSize(unsigned size)
{
	if(size>BufferSize)
	{
		for(unsigned i=0;i<BufferSize;i++)
			Values[i].~T();
		DestroyValues();
		Values=CreateValues(size);
		BufferSize=size;
	}
#ifdef DEBUG_LIST
	DupeCheck();
#endif
}
void Zero()
{
	memset(Values,0,sizeof(T)*NumValues);
}
void Place(unsigned idx,T t)
{
	Destroy(Values[idx]);
	Values[idx]=t;
}
//------------------------------------------------------------------------------
const T& operator[](const unsigned i) const
{
#ifdef LIST_EXCEPTIONS
	if(i<0||i>=NumValues)
	{
		unsigned errorhere=0;
		errorhere++;
		errorhere;
		throw StretchyItemListOutOfBounds();
	}
#endif
	return Values[Check(i)];
}
//------------------------------------------------------------------------------
T operator()(const unsigned i) const
{
	if(i<0||i>=NumValues)
		return NULL;
	return Values[i];
}
//------------------------------------------------------------------------------
T operator()(unsigned i)
{                    
	if(i<0||i>=NumValues)
		return NULL;
	return Values[i];
}
//------------------------------------------------------------------------------
T& operator[](unsigned i)
{
#ifdef LIST_EXCEPTIONS
	return Values[Check(i)];
#else
	return Values[i];
#endif
}
//------------------------------------------------------------------------------
inline std::vector(const std::vector<T>&Val)
{         
	NumValues=Val.NumValues;
	BufferSize=Val.BufferSize;
	Values=CreateValues(BufferSize);
	for(unsigned i=0;i<BufferSize;i++)
	{
		if(i<NumValues)
			(Values[i])=(Val.Values[i]);
	}
}           
/// Return a list whose values are duplicates of those in this list and Lst2.
std::vector<T> operator+(const std::vector<T>&Lst2)
{
	unsigned i;
	std::vector<T> Ret;
	Ret=*this;
	for(i=0;i<Lst2.NumValues;i++)
	{
		if(!Contains(Lst2.Values[i]))
			Ret.Add(Lst2.Values[i]);
	}
	return Ret;
}
/// Set this list equal to Val.
std::vector<T> &operator=(const std::vector<T> &Val)
{
	if(Val.NumValues>BufferSize)
	{
		for(unsigned i=0;i<BufferSize;i++)
			Values[i].~T();
		DestroyValues();
		BufferSize=Val.BufferSize;
		Values=CreateValues(BufferSize);
	}           
	NumValues=Val.NumValues;
	for(unsigned i=0;i<NumValues;i++)
		Values[i]=Val.Values[i];
	return *this;
}
//------------------------------------------------------------------------------
inline std::vector()
{
	Values=0;//NULL;
	NumValues=0;
	BufferSize=0;
}
//------------------------------------------------------------------------------
inline ~std::vector()
{
	for(unsigned i=0;i<BufferSize;i++)
		Values[i].~T();
	DestroyValues();
}
/// Clear the list without deallocating memory. Calls Clear() on each value.
inline void Clear()
{
	NumValues=0;
}      
/// Clear the list and deallocate memory.
inline void ClearMemory()
{           
		for(unsigned i=0;i<BufferSize;i++)
			Values[i].~T();
	DestroyValues();
	Values=NULL;
	NumValues=0;      
	BufferSize=0;
}
/// Returns the size of the list, smaller than or equal to the buffer size.
unsigned Size() const
{
	return NumValues;
}
/// Returns true if there is a value equal to Val, i.e. if *List(i)==Val where i is any index.
inline bool Contains(const T& Val) const
{
	for(unsigned i=0;i<NumValues;i++)
	{
		if((Values[i])==Val)
			return true;
	}
	return false;
}
//------------------------------------------------------------------------------
private:
void DestroyValues()
{
	if(!Values)
		return;
	//Values=(T*)((unsigned)Values-(unsigned)Offset<(sizeof(T)>4)>::Size);
	operator delete[] (Values,DefaultMemoryPool);
}
T* CreateValues(unsigned num)
{
	T *V=(T*)(operator new[](num*sizeof(T),DefaultMemoryPool));
	for(unsigned i=0;i<num;i++)
		new(&V[i]) T();//placement new -does no allocation
	return V;
}
public:
/// Add a T to the end of the list.
T &Add()
{
	if(NumValues>=BufferSize)
	{
		unsigned OldSize=BufferSize;
		BufferSize*=2;
		if(!BufferSize)
			BufferSize=1;
		T *NewValues=CreateValues(BufferSize);
		for(unsigned i=0;i<OldSize;i++)
		{
			NewValues[i]=Values[i];
			Values[i].~T();
		}
		DestroyValues();
		Values=NewValues;
	}
	NumValues++;
	return Values[NumValues-1];
}
/// Increase the size of the list by extra. Values are preserved.
void Increase(unsigned extra)
{
	unsigned i;
	if(NumValues+extra>BufferSize)
	{
		unsigned OldSize=BufferSize;
		while(NumValues+extra>BufferSize)
		{
			BufferSize*=2;
			if(!BufferSize)
				BufferSize=1;
		}
		T *NewValues=new(DefaultMemoryPool) T[BufferSize];
		for(i=0;i<OldSize;i++)
			NewValues[i]=Values[i];
		for(unsigned i=0;i<OldSize;i++)
			Values[i].~T();
		DestroyValues();
		Values=NewValues;
	}
	NumValues+=extra;
}
/// Add a T and set it equal to t.
T &Add(T t)
{
	Add();
	(Values[NumValues-1])=t;
	return Values[NumValues-1];
}
/// Add a T to the end of the list if an equal value is not there already.
T &AddIfNotPresent(T t)
{
	unsigned idx=Index(t);
	if(idx>=0)
		return Values[idx];
	else
		return Add(t);
}
/// Equivalent to Sort() but used only if this is a StretchyList of pointers.
/// The sort is done on the instances pointed to.
void SortContents()
{
	unsigned i,j;
	for(i=0;i<NumValues;i++)
	{
		for(j=0;j<NumValues-1;j++)
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
/// Shift the value at Index in OriginList to the end of TargetList.
friend void ShiftElement(std::vector<T> &TargetList,
	std::vector<T> &OriginList,unsigned Index)
{
#ifdef LIST_EXCEPTIONS
	OriginList.Check(Index);
#endif
	if(TargetList.NumValues>=TargetList.BufferSize)
	{
		TargetList.BufferSize*=2;
		if(!TargetList.BufferSize)
			TargetList.BufferSize=1;
		T *NewValues=new(DefaultMemoryPool) T[TargetList.BufferSize];
		for(unsigned i=0;i<TargetList.NumValues;i++)
		{
			NewValues[i]=TargetList.Values[i];
			TargetList.Values[i].~T();
		}
		TargetList.DestroyValues();
		TargetList.Values=NewValues;
	}       
	TargetList.Values[TargetList.NumValues]=OriginList.Values[Index];
	TargetList.NumValues++;
	OriginList.NumValues--;
	for(unsigned i=Index;i<OriginList.NumValues;i++)
	{
		OriginList.Values[i]=OriginList.Values[i+1];
	}
}                                 
/// Shift the first value equal to t in OriginList to the end of TargetList.
friend void ShiftElementByValue(std::vector<T> &TargetList,
	std::vector<T> &OriginList,T t)
{
	ShiftElement(TargetList,OriginList,OriginList.Index(t));
}
/// Insert a T of value Val at position index.
void Insert(unsigned index,T val)
{
	if(NumValues>=BufferSize)
	{
		unsigned OldSize=BufferSize;
		BufferSize*=2;
		if(!BufferSize)
			BufferSize=1;
		T *NewValues=new(DefaultMemoryPool) T[BufferSize];
		for(unsigned i=0;i<OldSize;i++)
		{
			NewValues[i+(i>=index)]=Values[i];
			Values[i].~T();
		}
		DestroyValues();
		Values=NewValues;
	}
	else
	{
		for(unsigned i=NumValues-1;i>=index;i--)
			Values[i+1]=Values[i];
	}
	Values[index]=val;
	NumValues++;
}
/// Insert a T at position index.
T &Insert(unsigned index)
{
	T *NewValues=Values;
	if(NumValues>=BufferSize)
	{
		unsigned OldSize=BufferSize;
		BufferSize*=2;
		if(!BufferSize)
			BufferSize=1;
		NewValues=new(DefaultMemoryPool) T[BufferSize];
		for(size_t i=0;i<OldSize;i++)
		{
			NewValues[i+(i>=index)]=Values[i];
			Values[i].~T();
		}
		NumValues++;
		DestroyValues();
		Values=NewValues;
	}
	else
	{
		for(size_t i=0;i<NumValues;i++)
		{
			Values[i+(i>=index)]=Values[i];
		}
	}
	NumValues++;
	Values=NewValues;
	return NewValues[index];
}        
/// Remove the value at index, shift all values above it down by one position.
void Remove(unsigned index)
{
#ifdef LIST_EXCEPTIONS
	Check(index);
#endif
	NumValues--;
	for(unsigned i=index;i<NumValues;i++)
	{
		Values[i]=Values[i+1];
	}
}
/// Remove the first found instance equal to Val. There must be at least one such instance.
void Cut(const T& Val)
{
	for(unsigned i=0;i<NumValues;i++)
	{
		if(Values[i]==Val)
		{
			Remove(i);
			return;
		}
    }
#ifdef LIST_EXCEPTIONS
	throw StretchyItemListNotInList();
#endif
}
/// Remove the first found instance equal to Val, if there is one, otherwise do nothing.
bool CutIfPresent(const T& Val)
{
	for(unsigned i=0;i<NumValues;i++)
	{
		if(Values[i]==Val)
		{
			Remove(i);
			return true;
		}
    }
	return false;
}
/// Remove a number of values starting at index, where length is the number to remove.
void Remove(unsigned index,unsigned length)
{
	Check(index);  
	Check(index+length-1);
	unsigned i;
	for(i=0;i<NumValues-1;i++)
	{
		Values[i]=Values[i+length*(i>=index)];
	}
	NumValues-=length;
	//Values=NewValues;
}
bool operator==(const std::vector<T> &L)
{
	if(NumValues!=L.NumValues)
		return false;
	for(unsigned i=0;i<NumValues-1;i++)
		if(Values[i]!=L.Values[i])
			return false;
	return true;
}
/// Return the position in the list of the first instance equal to Val, otherwise -1.
unsigned Index(const T& Val) const
{
	for(unsigned i=0;i<NumValues;i++)
	{
		if(Values[i]==Val)
			return i;
	}
	return -1;
}
/// Find the next value in the list equal to Val, after index.
unsigned FindNext(const T& Val,const unsigned index)
{
	unsigned i;
	unsigned result=-1;
	for(i=index;i<NumValues;i++)
	{
		if(Values[i]==Val)
		{
			if(result==-1)
				result=i;
#ifdef LIST_EXCEPTIONS    
			else
				throw StretchyItemListDuplication();
#endif
		}
	}
	return result;
}                 
/// Append OriginList to the end of TargetList, clearing OriginList in the process.
friend void JoinLists(std::vector<T> &TargetList,std::vector<T> &OriginList)
{
	if(TargetList.NumValues+OriginList.NumValues<=TargetList.BufferSize)
	{
		for(unsigned i=0;i<OriginList.NumValues;i++)
			TargetList.Values[TargetList.NumValues+i]=OriginList.Values[i];
		TargetList.NumValues+=OriginList.NumValues;   
		OriginList.NumValues=0;
		return;
	}
	T *NewValues=NULL;
	if(TargetList.NumValues+OriginList.NumValues>0)
	{          
		unsigned i;         
		for(i=0;i<TargetList.BufferSize;i++)
			TargetList.Values[i].~T();           
		TargetList.BufferSize=TargetList.NumValues+OriginList.NumValues;
		NewValues=new(DefaultMemoryPool) T[TargetList.BufferSize];
		for(i=0;i<TargetList.NumValues;i++)
			NewValues[i]=TargetList.Values[i];
		for(i=0;i<OriginList.NumValues;i++)
			NewValues[TargetList.NumValues+i]=OriginList.Values[i];
	}
	TargetList.DestroyValues();
	TargetList.NumValues+=OriginList.NumValues;
	OriginList.NumValues=0;
	TargetList.Values=NewValues;
}
};
#endif