#ifndef AlignedStretchyPointerListH
#define AlignedStretchyPointerListH

#ifdef PLAYSTATION_2
	#define __alignof __alignof__
#else
	#ifndef NDEBUG
		#define LIST_EXCEPTIONS
	#endif
#endif

#include <algorithm>

#ifndef _CRTDBG_MAP_ALLOC
	#define USE_ALIGNED_ALLOCATION
#endif

#include "Simul/Base/SimMemory.h"
//
template<class T> class simul::base::PtrVector
{
private:
	T** Values;
public:
	int NumValues;
	int BufferSize;
private:
	static T* CreateNew()
{
#ifdef USE_ALIGNED_ALLOCATION
	return new(DefaultMemoryPool,__alignof(T)) T;
#else
	return new T;
#endif
}
	static void Destroy(T* t)
{
#ifdef USE_ALIGNED_ALLOCATION
	t->~T();
	operator delete(t,DefaultMemoryPool,__alignof(T));
#else
	delete t;
#endif
}
public:
	inline simul::base::PtrVector(const simul::base::PtrVector<T>&Val)
	{
		int i;
		NumValues=Val.NumValues;
		BufferSize=Val.BufferSize;
		Values=new(DefaultMemoryPool) T*[BufferSize];
		for(i=0;i<BufferSize;i++)
		{
			Values[i]=new(DefaultMemoryPool,__alignof(T)) T;
			if(i<NumValues)
				*(Values[i])=*(Val[i]);
		}
	}
	simul::base::PtrVector<T> &operator=(const simul::base::PtrVector<T> &Val)
	{
		int i;
		if(BufferSize<Val.NumValues)
		{
			for(i=0;i<BufferSize;i++)
				Destroy(Values[i]);
			operator delete[] (Values,DefaultMemoryPool);
			BufferSize=Val.BufferSize;  
			Values=new(DefaultMemoryPool) T*[BufferSize];
			for(i=0;i<BufferSize;i++)
				Values[i]=new(DefaultMemoryPool,__alignof(T)) T;
		}
		NumValues=Val.NumValues;
		for(i=0;i<NumValues;i++)
		{
			*(Values[i])=*(Val[i]);
		}
		return *this;
	}
//------------------------------------------------------------------------------
	inline simul::base::PtrVector()
	{
		Values=NULL;
		NumValues=0;
		BufferSize=0;
	}
//------------------------------------------------------------------------------
	inline ~simul::base::PtrVector()
	{
		for(int i=0;i<BufferSize;i++)
			Destroy(Values[i]);
		operator delete[] (Values,DefaultMemoryPool);
	}
	int Check(int i) const
	{
		if(i<0||i>=NumValues)
		{
			int errorhere=0;
			errorhere++;
			errorhere;
	#ifdef LIST_EXCEPTIONS
			throw OutOfBounds();
	#endif
		}
		return i;
	}
bool DupeCheck()
{
	for(int i=0;i<NumValues;i++)
	{
		for(int j=i+1;j<NumValues;j++)
		{
			if(Values[i]==Values[j])  
			{
				int errorhere=0;
				errorhere++;
				errorhere;
#ifdef LIST_EXCEPTIONS
				throw Duplication();
				#else
				return true;
#endif
			}
		}
	}
	return false;
}
	inline void DuplicationCheck()
	{
#ifdef LIST_EXCEPTIONS
		int i,j;
		for(i=0;i<NumValues;i++)
		{
			for(j=0;j<i;j++)
			{
				if(*Values[i]==*Values[j])
					throw Duplication();
			}
		}
#endif
	}
	inline void DuplicationCheck(const T& Val)
	{
#ifdef LIST_EXCEPTIONS
		int i,j;
		for(i=0;i<NumValues;i++)
		{
			if(*Values[i]==Val)
				throw Duplication();
		}
#endif
	}
	void SetBufferSize(int size)
	{
		if(size>BufferSize)
		{               
			register int i;
			for(i=0;i<BufferSize;i++)
				Destroy(Values[i]);
			operator delete[] (Values,DefaultMemoryPool);
			Values=new(DefaultMemoryPool) T*[size];
			for(i=0;i<size;i++)
			{
				Values[i]=new(DefaultMemoryPool,__alignof(T)) T;
			}
			BufferSize=size;
		}
	#ifdef DEBUG_LIST
		DupeCheck();
	#endif
	}
	void Resize(int size)
	{
		if(size>BufferSize)
		{               
			register int i;
			int oldbuf=BufferSize;
			if(!BufferSize)
				BufferSize=1;
			while(size>BufferSize)
				BufferSize*=2;
			T** NewValues=new(DefaultMemoryPool) T*[BufferSize];
			for(i=0;i<oldbuf;i++)
				NewValues[i]=Values[i];
			operator delete[] (Values,DefaultMemoryPool);
			for(i=oldbuf;i<BufferSize;i++)
				NewValues[i]=new(DefaultMemoryPool,__alignof(T)) T;
			Values=NewValues;
		}
		NumValues=size;  
		#ifdef DEBUG_LIST
		DupeCheck();
		#endif
	}
	void Place(int idx,T *t)
	{
		Destroy(Values[idx]);
		Values[idx]=t;   
	#ifdef DEBUG_LIST
		DupeCheck();
	#endif
	}
//------------------------------------------------------------------------------
	const T* operator[](const int i) const
	{            
	#ifdef LIST_EXCEPTIONS    
		if(i<0||i>=NumValues)
		{
			int errorhere=0;
			errorhere++;
			errorhere;
			throw OutOfBounds();
		}           
	#endif
		return Values[i];
	}
//------------------------------------------------------------------------------
	const T* operator()(const int i) const
	{
		if(i<0||i>=NumValues)
			return NULL;
	#ifdef DEBUG_LIST
	DupeCheck();
	#endif
		return Values[i];
	}
//------------------------------------------------------------------------------
	T* operator()(int i)
	{                    
		if(i<0||i>=NumValues)
			return NULL;    
	#ifdef DEBUG_LIST
	DupeCheck();
	#endif
		return Values[i];
	}
//------------------------------------------------------------------------------
	inline T* operator[](int i)
	{                      
		#ifdef DEBUG_LIST
	DupeCheck();           
		return Values[Check(i)];
	#endif

		return Values[i];
	}
//------------------------------------------------------------------------------  
	inline void clear()
	{
		Clear();
	}
	inline void Clear()
	{
		for(int i=0;i<NumValues;i++)
			Values[i]->Clear();
		NumValues=0;
	}
//------------------------------------------------------------------------------     
	int size() const
	{
		return Size();
	}
	inline int Size() const
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
	T *push_back()
	{
		return Add();
	}
	T &back()
	{
		return *(Values[Check(NumValues-1)]);
	}
	int begin()
	{
		return 0;
	}
	T *Add()
	{
		int i;
		if(NumValues>=BufferSize)
		{
			int OldSize=BufferSize;
			BufferSize*=2;
			if(!BufferSize)
				BufferSize=1;
			T **NewValues=new(DefaultMemoryPool) T*[BufferSize];
			for(i=0;i<OldSize;i++)
			{
				NewValues[i]=Values[i];
			}            
			for(i=OldSize;i<BufferSize;i++)
			{
				NewValues[i]=new(DefaultMemoryPool,__alignof(T)) T;
			}
			operator delete[] (Values,DefaultMemoryPool);
			Values=NewValues;
		}        
		T *NewPointer=Values[NumValues];
		NumValues++;      
		#ifdef DEBUG_LIST
	DupeCheck();
	#endif
		return NewPointer;
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
	void Increase(int extra)
	{
		int i;
		if(NumValues+extra>BufferSize)
		{
			int OldSize=BufferSize;
			while(NumValues+extra>BufferSize)
			{
				BufferSize*=2;
				if(!BufferSize)
					BufferSize=1;
			}
			T **NewValues=new(DefaultMemoryPool) T*[BufferSize];
			for(i=0;i<OldSize;i++)
			{
				NewValues[i]=Values[i];
			}
			for(i=OldSize;i<BufferSize;i++)
			{
				NewValues[i]=new(DefaultMemoryPool,__alignof(T)) T;
			}
			operator delete[] (Values,DefaultMemoryPool);
			Values=NewValues;
		}      
		#ifdef DEBUG_LIST
	DupeCheck();
	#endif
		NumValues+=extra;
	}
//------------------------------------------------------------------------------
	T *Add(const T &t)
	{
		Add();
		*(Values[NumValues-1])=t;
		return Values[NumValues-1];
	}
//------------------------------------------------------------------------------
	T *Add(T *NewPointer)
	{
		int i;
		if(NumValues>=BufferSize)
		{
			int OldSize=BufferSize;
			BufferSize*=2;
			if(!BufferSize)
				BufferSize=1;
			T **NewValues=new(DefaultMemoryPool) T*[BufferSize];
			for(i=0;i<OldSize;i++)
				NewValues[i]=Values[i];
			for(i=OldSize+1;i<BufferSize;i++)
				NewValues[i]=new(DefaultMemoryPool,__alignof(T)) T;
			operator delete[] (Values,DefaultMemoryPool);
			Values=NewValues;
		}
		else
			Destroy(Values[NumValues]);
		Values[NumValues]=NewPointer;
		NumValues++;
		return NewPointer;
	}
//------------------------------------------------------------------------------
	void SortContents()
	{
		int i,j;
		for(i=0;i<NumValues;i++)
		{
			for(j=0;j<NumValues-1;j++)
			{
				if(*Values[j+1]<*Values[j])
				{
					T *temp=Values[j+1];
					Values[j+1]=Values[j];
					Values[j]=temp;
				}
			}
		}
	}                
//------------------------------------------------------------------------------
	bool CutIfPresent(const T& Val)
	{
		for(register int i=0;i<NumValues;i++)
		{
			if(*Values[i]==Val)
			{
				Remove(i);
				return true;
			}
		}
		return false;
	}
//------------------------------------------------------------------------------
	bool CutIfPresent(const T* Val)
	{
		for(register int i=0;i<NumValues;i++)
		{
			if(Values[i]==Val)
			{
				Remove(i);
				return true;
			}
		}
		return false;
	}
//------------------------------------------------------------------------------
	simul::base::PtrVector<T> operator+(const simul::base::PtrVector<T>&Lst2)
	{
		int i;
		simul::base::PtrVector<T> Ret;
		Ret=*this;
		for(i=0;i<Lst2.NumValues;i++)
		{
			if(!Contains(*Lst2.Values[i]))
				Ret.Add(*Lst2.Values[i]);
		}
		return Ret;
	}
//------------------------------------------------------------------------------
	friend void Shift(simul::base::PtrVector<T> &TargetList,
		simul::base::PtrVector<T> &OriginList,int Index)
	{
		int i;
		OriginList.Check(Index);
	#ifdef DEBUG_LIST
		OriginList.DupeCheck();
		TargetList.DupeCheck();
	#endif
		if(TargetList.NumValues>=TargetList.BufferSize)
		{
			int OldSize=TargetList.BufferSize;
			TargetList.BufferSize*=2;
			if(!TargetList.BufferSize)
				TargetList.BufferSize=1;
			T **NewValues=new(DefaultMemoryPool) T*[TargetList.BufferSize];
			for(i=0;i<OldSize;i++)
			{
				NewValues[i]=TargetList.Values[i];
			}
			for(i=OldSize;i<TargetList.BufferSize;i++)
			{
			#ifdef USE_ALIGNED_ALLOCATION
				NewValues[i]=new(DefaultMemoryPool,__alignof(T)) T();
			#else
				NewValues[i]=new(DefaultMemoryPool) T;
			#endif
			}
			operator delete[] (TargetList.Values,DefaultMemoryPool);
			TargetList.Values=NewValues;
		}
		T *Temp=TargetList.Values[TargetList.NumValues];
		TargetList.Values[TargetList.NumValues]=OriginList.Values[Index];
		TargetList.NumValues++;
		for(i=Index;i<OriginList.NumValues-1;i++)
		{
			OriginList.Values[i]=OriginList.Values[i+1];
		}                        
		OriginList.Values[i]=Temp;
		OriginList.NumValues--;
	/*	for(int i=0;i<OriginList.BufferSize;i++)
		{
			for(int j=0;j<TargetList.BufferSize;j++)
			{
				if(OriginList.Values[i]==TargetList.Values[j])
				{
					int errorhere=1;
					errorhere++;
					errorhere;
				}
			}
		}         */
		#ifdef DEBUG_LIST
	OriginList.DupeCheck();
	TargetList.DupeCheck();
	#endif
	}
//------------------------------------------------------------------------------
	friend void ShiftElementByValue(simul::base::PtrVector<T> &TargetList,
		simul::base::PtrVector<T> &OriginList,T *ptr)
	{
		Shift(TargetList,OriginList,OriginList.Index(ptr));
	}
//------------------------------------------------------------------------------
	T *Insert(int index)
	{
		T *NewPointer;
		if(NumValues>=BufferSize)
		{
			int OldSize=BufferSize;
			BufferSize*=2;
			if(!BufferSize)
				BufferSize=1;
			T **NewValues=new(DefaultMemoryPool) T*[BufferSize];
			for(int i=0;i<OldSize;i++)
			{
				NewValues[i+(i>=index)]=Values[i];
			}
			for(int i=OldSize+1;i<BufferSize;i++)
			{
				NewValues[i]=new(DefaultMemoryPool,__alignof(T)) T;
			}
			operator delete[] (Values,DefaultMemoryPool);
			Values=NewValues;
			NewPointer=new(DefaultMemoryPool,__alignof(T)) T;
			Values[index]=NewPointer;
		}
		else
		{
			NewPointer=Values[BufferSize-1];
			Values[BufferSize-1]=Values[NumValues];
			for(int i=NumValues-1;i>=index;i--)
				Values[i+1]=Values[i];
			Values[index]=NewPointer;
		}
		NumValues++;
		#ifdef DEBUG_LIST
	DupeCheck();
	#endif
		return NewPointer;
	}
//------------------------------------------------------------------------------
	T *Insert(int index,T *NewPointer)
	{
		if(NumValues>=BufferSize)
		{
			int OldSize=BufferSize;
			BufferSize*=2;
			if(!BufferSize)
				BufferSize=1;
			T **NewValues=new(DefaultMemoryPool) T*[BufferSize];
			for(int i=0;i<OldSize;i++)
				NewValues[i+(i>=index)]=Values[i];
			for(int i=OldSize+1;i<BufferSize;i++)
				NewValues[i]=NewPointer;
			NumValues++;
			operator delete[] (Values,DefaultMemoryPool);
			Values=NewValues;
		}
		else
		{
			Destroy(Values[BufferSize-1]);
			for(int i=0;i<NumValues;i++)
				Values[i+(i>=index)]=Values[i];
		}
		Values[index]=NewPointer;
		NumValues++;
	#ifdef DEBUG_LIST
		DupeCheck();
	#endif
		return NewPointer;
	}
//------------------------------------------------------------------------------
	void Cut(void* Val)
	{
		for(register int i=0;i<NumValues;i++)
		{
			if(Values[i]==Val)
			{
				Remove(i);
				return;
			}
		}
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
	#ifdef LIST_EXCEPTIONS
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
	#ifdef DEBUG_LIST
		Check(index);  
	#endif
		T *temp=Values[index];
		temp->Clear();
		for(int i=0;i<NumValues-1;i++)
			Values[i]=Values[i+(i>=index)];
		Values[NumValues-1]=temp;
		NumValues--;
	#ifdef DEBUG_LIST
	DupeCheck();
	#endif
	}                  
//------------------------------------------------------------------------------
	void SwapRemove(int index)
	{
		Values[index]->Clear();
		std::swap(Values[index],Values[NumValues-1]);
		NumValues--;              
	#ifdef DEBUG_LIST
	DupeCheck();
	#endif
	}
//------------------------------------------------------------------------------
	void Remove(int index,int length)
	{
		Check(index);  
		Check(index+length-1);
		register int i;
		for(i=0;i<length&&index+i+length<NumValues;i++)
		{
			temp->Clear();
		}
		while(index+length<NumValues)
		{
			for(i=0;i<length&&index+i+length<NumValues;i++)
			{
				T *temp=Values[index+i];
				Values[index+i]=Values[index+i+length];
				Values[index+i+length]=temp;
			}
			index+=length;
		}
		NumValues-=length;      
		#ifdef DEBUG_LIST
	DupeCheck();
	#endif
	}
//------------------------------------------------------------------------------
	int Index(const void* Val) const
	{
		int i;
		int result=-1;
		for(i=0;i<NumValues;i++)
		{
			if(Values[i]==Val)
			{
				if(result==-1)
					result=i;
	#ifdef LIST_EXCEPTIONS
				else
					throw AlignedDuplicate();
	#endif
			}
		}            
		return result;
	}
//------------------------------------------------------------------------------
	int Index(const T& Val) const
	{
		int i;
		int result=-1;
		for(i=0;i<NumValues;i++)
		{
			if(*Values[i]==Val)
			{
				if(result==-1)
					result=i;
	#ifdef LIST_EXCEPTIONS
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
	#ifdef LIST_EXCEPTIONS    
				else
					throw Duplicate();
	#endif
			}
		}
		return result;
	}      
	friend void JoinLists(simul::base::PtrVector<T> &TargetList,simul::base::PtrVector<T> &OriginList)
	{                  
		if(TargetList.NumValues+OriginList.NumValues<=TargetList.BufferSize)
		{
			for(int i=0;i<OriginList.NumValues;i++)
				std::swap((TargetList.Values[TargetList.NumValues+i]),(OriginList.Values[i]));
			TargetList.NumValues+=OriginList.NumValues;   
			OriginList.NumValues=0;
			return;
		}
		T **NewValues=NULL;
		register int i;
		if(TargetList.NumValues+OriginList.NumValues>0)
		{
			NewValues=new(DefaultMemoryPool) T*[TargetList.NumValues+OriginList.NumValues];
			for(i=0;i<TargetList.NumValues;i++)
			{
				NewValues[i]=TargetList.Values[i];
			}
			for(i=0;i<OriginList.NumValues;i++)
			{
				NewValues[TargetList.NumValues+i]=OriginList.Values[i];
			}
			for(i=TargetList.NumValues;i<TargetList.BufferSize;i++)
				Destroy(TargetList.Values[i]);
			for(i=OriginList.NumValues;i<OriginList.BufferSize;i++)
				Destroy(OriginList.Values[i]);
		}
		operator delete[] (TargetList.Values,DefaultMemoryPool);
		operator delete[] (OriginList.Values,DefaultMemoryPool);
		TargetList.NumValues+=OriginList.NumValues;
		OriginList.NumValues=0;
		TargetList.Values=NewValues;
		OriginList.Values=NULL;      
		TargetList.BufferSize=TargetList.NumValues;
		OriginList.BufferSize=0;
	#ifdef DEBUG_LIST
		TargetList.DupeCheck();
	#endif
	}
};
#endif