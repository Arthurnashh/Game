#ifndef StretchyPointerListH
#define StretchyPointerListH         
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
#include "Simul/Base/List.h"

namespace simul
{
	namespace base
	{
		/// \internal \brief Class template for a list of pointers to instances of class T.

		/// \internal \internal The list is stretchy (elastic), meaning that it keeps a buffer equal to or greater than
		/// the size of the list so removing items does not de-allocate memory.
		/// T must have a Clear() method.
		template<class T> class PtrVector
		{
		private:
		T** Values;
		public:
		int NumValues;
		int BufferSize;
		class Duplicate{};
		class OutOfBounds{};
		inline PtrVector(const PtrVector<T>&Val)
		{
			int i;
			NumValues=Val.NumValues;
			BufferSize=Val.BufferSize;
			Values=new T*[BufferSize];
			for(i=0;i<BufferSize;i++)
			{
				Values[i]=new T;
				if(i<NumValues)
					*(Values[i])=*(Val[i]);
			}
		}
		/// \internal Make this list a duplicate of Val, creating new instances equal to those
		/// owned by Val.
		PtrVector<T> &operator=(const PtrVector<T> &Val)
		{
			int i;
			if(BufferSize<Val.NumValues)
			{
				for(i=0;i<BufferSize;i++)
					Destroy(Values[i]);
				delete[] Values;
				BufferSize=Val.BufferSize;  
				Values=new T*[BufferSize];
				for(i=0;i<BufferSize;i++)
					Values[i]=new T;
			}
			NumValues=Val.NumValues;
			for(i=0;i<NumValues;i++)
			{
				*(Values[i])=*(Val[i]);
			}
			return *this;
		}
		inline PtrVector()
		{
			Values=NULL;
			NumValues=0;
			BufferSize=0;
		}
		inline ~PtrVector()
		{
			for(int i=0;i<BufferSize;i++)
				Destroy(Values[i]);
			delete[] Values;
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
		bool DupeCheck(void)
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
			inline void DuplicationCheck(void)
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
				delete[] Values;
				Values=new T*[size];
				for(i=0;i<size;i++)
					Values[i]=new T;
				BufferSize=size;
			}
		#ifdef DEBUG_LIST
			DupeCheck();
		#endif
		}
		/// \internal Resize the list to size; Values are preserved up to the new or old size, whichever is smaller.
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
				T** NewValues=new T*[BufferSize];
				for(i=0;i<oldbuf;i++)
					NewValues[i]=Values[i];
				delete[] Values;
				for(i=oldbuf;i<BufferSize;i++)
					NewValues[i]=new T;
				Values=NewValues;
			}
			NumValues=size;  
			#ifdef DEBUG_LIST
			DupeCheck();
			#endif
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
		#ifdef DEBUG_LIST
			DupeCheck();
		#endif
		}
		/// \internal Get the pointer at position i in the list.
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
		/// \internal Get the pointer at position i in the list.
		const T* operator()(const int i) const
		{
			if(i<0||i>=NumValues)
				return NULL;
		#ifdef DEBUG_LIST
		DupeCheck();
		#endif
			return Values[i];
		}
		/// \internal The last pointer in the list.
		T* Last()
		{
			if(NumValues)
				return Values[NumValues-1];
			return NULL;
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
		inline void clear(void)
		{
			Clear();
		}
		/// \internal Clear the list without deallocating memory. Calls Clear() on each value.
		inline void Clear(void)
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
		/// \internal Returns the size of the list, smaller than or equal to the buffer size.
		inline int Size() const
		{
			return NumValues;
		}
		/// \internal Returns true if there is a value equal to Val, i.e. if *List(i)==Val where i is any index.
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
		/// \internal Returns true if there is a pointer equal to Val, i.e. if List(i)==Val where i is any index.
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
		T *push_back(void)
		{
			return Add();
		}
		T &back(void)
		{
			return *(Values[Check(NumValues-1)]);
		}
		int begin(void)
		{
			return 0;
		}
		/// \internal Create a new T, add its address to the list, return the address.
		T *Add(void)
		{
			int i;
			if(NumValues>=BufferSize)
			{
				int OldSize=BufferSize;
				BufferSize*=2;
				if(!BufferSize)
					BufferSize=1;
				T **NewValues=new T*[BufferSize];
				for(i=0;i<OldSize;i++)
				{
					NewValues[i]=Values[i];
				}            
				for(i=OldSize;i<BufferSize;i++)
				{
					NewValues[i]=new T;
				}
				delete[] Values;
				Values=NewValues;
			}        
			T *NewPointer=Values[NumValues];
			NumValues++;      
			#ifdef DEBUG_LIST
		 DupeCheck();
		 #endif
			return NewPointer;
		}
		/// \internal Swap the pointers at Id1 and Id2.
		inline void Swap(int Id1,int Id2)
		{
			Check(Id1);
			Check(Id2);
			T *temp=Values[Id1];
			Values[Id1]=Values[Id2];   
			Values[Id2]=temp;
		}
		/// \internal Increase the size of the list by extra. Values are preserved.
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
				T **NewValues=new T*[BufferSize];
				for(i=0;i<OldSize;i++)
				{
					NewValues[i]=Values[i];
				}
				for(i=OldSize;i<BufferSize;i++)
				{
					NewValues[i]=new T;
				}
				delete[] Values;
				Values=NewValues;
			}      
			#ifdef DEBUG_LIST
		 DupeCheck();
		  #endif
			NumValues+=extra;
		}
		/// \internal Create and add a new T, let *(new value)=t.
		T *Add(T &t)
		{
			Add();
			*(Values[NumValues-1])=t;
			return Values[NumValues-1];
		}
		/// \internal Add NewPointer to the list. The list takes over the pointer and is responsible for its deletion.
		T *Add(T *NewPointer)
		{
			int i;
			if(NumValues>=BufferSize)
			{
				int OldSize=BufferSize;
				BufferSize*=2;
				if(!BufferSize)
					BufferSize=1;
				T **NewValues=new T*[BufferSize];
				for(i=0;i<OldSize;i++)
					NewValues[i]=Values[i];
				for(i=OldSize+1;i<BufferSize;i++)
					NewValues[i]=new T;
				delete[] Values;
				Values=NewValues;
			}
			else
				Destroy(Values[NumValues]);
			Values[NumValues]=NewPointer;
			NumValues++;
			return NewPointer;
		}
		/// \internal Sort the values using operator< of T.
		void SortContents(void)
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
		T *Insert(int index)
		{
			T *NewPointer;
			if(NumValues>=BufferSize)
			{
				int OldSize=BufferSize;
				BufferSize*=2;
				if(!BufferSize)
					BufferSize=1;
				T **NewValues=new T*[BufferSize];
				for(int i=0;i<OldSize;i++)
					NewValues[i+(i>=index)]=Values[i];
				for(int i=OldSize+1;i<BufferSize;i++)
					NewValues[i]=new T;
				delete[] Values;
				Values=NewValues;
				NewPointer=new T;
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
				T **NewValues=new T*[BufferSize];
				for(int i=0;i<OldSize;i++)
					NewValues[i+(i>=index)]=Values[i];
				for(int i=OldSize+1;i<BufferSize;i++)
					NewValues[i]=NewPointer;
				NumValues++;
				delete[] Values;
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
		/// \internal Remove the instance of T whose address is Val. There must be at least one such instance.
		void Cut(const T* Val)
		{
			for(register int i=0;i<NumValues;i++)
			{
				if(Values[i]==Val)
				{
					Remove(i);
					return;
				}
			}
		#ifdef LIST_EXCEPTIONS
			throw NotInList();
		#endif
		}
		/// \internal Remove the first found instance equal to Val. There must be at least one such instance.
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
		/// \internal Remove the value at index, shift all values above it down by one position.
		void Remove(int index)
		{                       
		#ifdef DEBUG_LIST
			Check(index);  
		#endif
			T *temp=Values[index];
			temp->Clear();
			for(int i=0;i<NumValues-1;i++)
			{
				Values[i]=Values[i+(i>=index)];
			}
			Values[NumValues-1]=temp;
			NumValues--;
		#ifdef DEBUG_LIST
		 DupeCheck();
		#endif
		}                  
		/// \internal Remove the value at index, and swap in the last value. Equivalent to the behaviour
		/// of a simul::base::PtrSet.
		void SwapRemove(int index)
		{
			Check(index);
			Values[index]->Clear();
			std::swap(Values[index],Values[NumValues-1]);
			NumValues--;
		#ifdef DEBUG_LIST
			DupeCheck();
		#endif
		}
		/// \internal Remove a number of values starting at index, where length is the number to remove.
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
		/// \internal Remove the first found instance equal to Val, if there is one, otherwise do nothing.
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
		/// \internal Remove and destroy the T pointed to by Val, if it is in the list, otherwise do nothing.
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
		/// \internal Return a list whose values are duplicates of those in this list and Lst2.
		PtrVector<T> operator+(const PtrVector<T>&Lst2)
		{
			int i;
			PtrVector<T> Ret;
			Ret=*this;
			for(i=0;i<Lst2.NumValues;i++)
			{
				if(!Contains(*Lst2.Values[i]))
					Ret.Add(*Lst2.Values[i]);
			}
			return Ret;
		}
		/// \internal Shift the value at Index in OriginList to the end of TargetList.
		friend void Shift(PtrVector<T> &TargetList,
			PtrVector<T> &OriginList,int Index)
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
				T **NewValues=new T*[TargetList.BufferSize];
				for(i=0;i<OldSize;i++)
				{
					NewValues[i]=TargetList.Values[i];
				}
				for(i=OldSize;i<TargetList.BufferSize;i++)
				{
					NewValues[i]=new T();
				}
				delete[](TargetList.Values);
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

			#ifdef DEBUG_LIST
		 OriginList.DupeCheck();
		TargetList.DupeCheck();
		#endif
		}
		/// \internal Find the pointer ptr, shift it from OriginList to the end of TargetList.
		friend void ShiftElementByValue(PtrVector<T> &TargetList,
			PtrVector<T> &OriginList,T *ptr)
		{
			Shift(TargetList,OriginList,OriginList.Index(ptr));
		}
		/// \internal Return the position in the list of the first instance equal to Val, otherwise -1.
		int Index(const void* Val)
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
						throw Duplicate();
		#endif
				}
			}            
			return result;
		}
		/// \internal Return the position in the list of the first instance equal to Val, otherwise -1.
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

				}
			}
			return result;
		}
		/// \internal Find the next pointer in the list equal to Val, after index.
		/// As pointers should not be duplicated, if one is found already,
		/// FindNext should return -1 after that.
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
		/// \internal Append OriginList to the end of TargetList, clearing OriginList in the process.
		friend void JoinLists(PtrVector<T> &TargetList,PtrVector<T> &OriginList)
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
				NewValues=new T*[TargetList.NumValues+OriginList.NumValues];
				for(i=0;i<TargetList.NumValues;i++)
					NewValues[i]=TargetList.Values[i];
				for(i=0;i<OriginList.NumValues;i++)
					NewValues[TargetList.NumValues+i]=OriginList.Values[i];
				for(i=TargetList.NumValues;i<TargetList.BufferSize;i++)
					TargetList.Destroy(TargetList.Values[i]);
				for(i=OriginList.NumValues;i<OriginList.BufferSize;i++)
					OriginList.Destroy(OriginList.Values[i]);
			}
			delete[](TargetList.Values);
			delete[](OriginList.Values);
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
		/// \internal \brief Check what is touching in a list.

		/// \internal Given a list of things, and some way of finding out which are touching each
		/// other, this function should return the number of partitions, and a list
		/// assigning each "thing" to one of those partitions.
		/// T will usually be a pointer type
		template<class T> bool PartitionCheck(PtrVector<T> &Items,
			std::vector<int> &ItemPartitions,bool(*Touching)(T *t1,T *t2),int *NumPartitions)
		{
			int i,j;
			if(Items.Size()<2)
				return false;
			ItemPartitions.Resize(Items.Size(),-1);
			for(i=0;i<Items.Size();i++)
			{
				ItemPartitions[i]=-1;
			}
			int ItemsLeft;
			ItemPartitions[0]=1;
			*NumPartitions=1;
			ItemsLeft=Items.Size()-1;
			while(ItemsLeft)
			{
				for(i=0;i<Items.Size()&&ItemsLeft;i++)
				{
					for(j=0;j<Items.Size()&&ItemsLeft;j++)
					{
						if(j==i)                                                        
							continue;
						if(!Touching(Items[i],Items[j]))
							continue;
						if(ItemPartitions[i]==-1&&ItemPartitions[j]!=-1)
						{
							ItemPartitions[i]=ItemPartitions[j];                    
							ItemsLeft--;
						}
						else if(ItemPartitions[j]==-1&&ItemPartitions[i]!=-1)
						{
							ItemPartitions[j]=ItemPartitions[i];                    
							ItemsLeft--;
						}                                                               
						else if(ItemPartitions[i]!=ItemPartitions[j])
						{                                                               
							return false;
						}                                                               
					}
				}
				if(ItemsLeft)                                                         
				{                                                                       
					for(i=0;i<Items.Size();i++)                                       
					{                                                                   
						if(ItemPartitions[i]==-1)                                     
						{                                                               
							(*NumPartitions)++;
							ItemPartitions[i]=*NumPartitions;                         
							ItemsLeft--;                                              
							i=Items.Size();                                           
						}                                                               
					}                                                                   
				}                                                                       
			}                                                                           
		#ifdef LIST_EXCEPTIONS
			for(i=0;i<Items.Size();i++)
			{                                                                           
				if(ItemPartitions[i]<0)                                               
					throw SimulError();                                                 
			}                                                                           
		#endif
			if(*NumPartitions==1)
			{
				return false;
			}
			return true;
		}
	}
}
#endif