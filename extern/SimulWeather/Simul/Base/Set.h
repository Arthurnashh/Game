#ifndef SIMUL_BASE_SET_H
#define SIMUL_BASE_SET_H
//------------------------------------------------------------------------------
#ifndef PLAYSTATION_2
	#ifndef NDEBUG
		#define BOUNDS_CHECKING
	#endif
#endif

#include <vector>
#include "Simul/Base/SimMemory.h"

namespace simul
{
	namespace base
	{
		/// \internal An unordered elastic list of instances of class T, contiguous in memory.

		/// \internal Removing a member from within the list causes the member at the end to be swapped into its place.
		/// This makes for faster list management as the rest of the list need not be moved.
		template<class T> class Set
		{
		private:
			T*  Values;
			int BufferSize;
			static T* CreateValues(int num)
			{
				T *V=(T*)(operator new[](num*sizeof(T)));
				for(int i=0;i<num;i++)
					new(&V[i]) T();//placement new -does no allocation
				return V;
			}
			void DestroyValues(void)
			{
				for(int i=0;i<BufferSize;i++)
					BufferSize[i].~T();
				operator delete [] (Values,16);
			}
		public:
			class OutOfBounds{};
			int NumValues;
			int Check(int i) const
			{
			#ifdef BOUNDS_CHECKING    
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
		/// \internal Resize the list to size.
			void Resize(int size)
			{
				if(size>BufferSize)
				{
					BufferSize=size*2;
					operator delete[](Values);
					Values=CreateValues(BufferSize);
				}
				NumValues=size;
			}
		/// \internal Resize the list to size, making each value equal to Val.
			void Resize(int size,T t)
			{
				if(size>BufferSize)
				{
					BufferSize=size*2;
					operator delete[](Values);
					Values=CreateValues(BufferSize);
				}
				for(int i=0;i<size;i++)
				{
					Values[i]=t;
				}
				NumValues=size;
			}
			void SetBufferSize(int size)
			{
				if(size>BufferSize)
				{
					operator delete[](Values);
					Values=CreateValues(size);
					BufferSize=size;
				}
			}
			void Place(int idx,T t)
			{
				delete Values[idx];
				Values[idx]=t;
			}
		/// \internal Return a const reference to the value at i.
			const T& operator[](const int i) const
			{
			#ifdef BOUNDS_CHECKING    
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
		/// \internal Return a const reference to the value at i.
			const T& operator()(const int i) const
			{
				if(i<0||i>=NumValues)
					return NULL;
				return Values[i];
			}
		/// \internal Return a reference to the value at i.
			T& operator()(int i)
			{                    
				if(i<0||i>=NumValues)
					return NULL;
				return Values[i];
			}
		/// \internal Return a reference to the value at i.
			T& operator[](int i)
			{
			#ifdef CHECK_LIST_BOUNDS
				return Values[Check(i)];
			#else
				return Values[i];
			#endif
			}
		/// \internal Copy constructor.
			inline Set(const Set<T>&Val)
			{         
				int i;
				NumValues=Val.NumValues;
				BufferSize=Val.BufferSize;
				Values=CreateValues(BufferSize);
				for(i=0;i<BufferSize;i++)
				{
					if(i<NumValues)
						(Values[i])=(Val[i]);
				}
			}           
			Set<T> operator+(const Set<T>&Lst2)
			{
				int i;
				simul::base::Set<T> Ret;
				Ret=*this;
				for(i=0;i<Lst2.NumValues;i++)
				{
					if(!Contains(Lst2.Values[i]))
						Ret.Add(Lst2.Values[i]);
				}
				return Ret;
			}
			Set<T> &operator=(const Set<T> &Val)
			{
				if(Val.NumValues>BufferSize)
				{
					operator delete[](Values);
					BufferSize=Val.BufferSize;
					Values=CreateValues(BufferSize);
				}           
				NumValues=Val.NumValues;
				for(register int i=0;i<NumValues;i++)
					Values[i]=Val[i];
				return *this;
			}
			bool operator==(const Set<T> &Val)
			{
				if(NumValues!=Val.NumValues)
					return false;
				for(register int i=0;i<NumValues;i++)
					if(Values[i]!=Val.Values[i])
						return false;
				return true;
			}
			bool operator!=(const Set<T> &Val)
			{
				if(NumValues!=Val.NumValues)
					return true;
				for(register int i=0;i<NumValues;i++)
					if(Values[i]!=Val.Values[i])
						return true;
				return false;
			}
		/// Make this list a duplicate of Val, creating new instances equal to those owned by Val.
			Set<T> &operator=(const std::vector<T> &Val)
			{
				if(Val.NumValues>BufferSize)
				{
					operator delete[](Values);
					BufferSize=Val.BufferSize;
					Values=CreateValues(BufferSize);
				}           
				NumValues=Val.NumValues;
				for(register int i=0;i<NumValues;i++)
					Values[i]=Val[i];
				return *this;
			}
		/// Default Constructor.
			inline Set(void)
			{
				NumValues=0;
				BufferSize=1;
				Values=CreateValues(1);
			}
		/// Default destructor.
			inline ~Set(void)
			{
				operator delete[](Values);
			}
		/// Clear the list without deallocating memory. Calls Clear() on each value.
			inline void Clear(void)
			{
				NumValues=0;
			}
		/// Returns the size of the list, smaller than or equal to the buffer size.
			int Size() const
			{
				return NumValues;
			}
		/// Returns true if there is a value equal to Val, i.e. if List(i)==Val where i is any index.
			inline bool Contains(const T& Val)
			{
				int i;
				for(i=0;i<NumValues;i++)
				{
					if((Values[i])==Val)
						return true;
				}
				return false;
			}
		/// Add a T to the end of the list.
			T &Add(void)
			{
				if(NumValues==BufferSize)
				{
					int OldSize=BufferSize;
					BufferSize*=2;
					if(!BufferSize)
						BufferSize=1;
					T *NewValues=CreateValues(BufferSize);
					for(int i=0;i<OldSize;i++)
					{
						NewValues[i]=Values[i];
					}
					operator delete[](Values);
					Values=NewValues;
				}
				NumValues++;
				return Values[NumValues];
			}
		/// Increase the size of the list by extra. Values are preserved.
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
					T *NewValues=CreateValues(BufferSize);
					for(i=0;i<OldSize;i++)
					{
						NewValues[i]=Values[i];
					}
					operator delete[](Values);
					Values=NewValues;
				}
				NumValues+=extra;
			}
		/// Add a T equal to t to the end of the list.
			T &Add(T t)
			{
				Add();
				(Values[NumValues-1])=t;
				return Values[NumValues-1];
			}
		//------------------------------------------------------------------------------
			void SortContents(void)
			{
				int i,j;
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
		/// Insert a T of value Val at position index.
			void Insert(int index,T val)
			{
				if(NumValues==BufferSize)
				{
					int OldSize=BufferSize;
					BufferSize*=2;
					if(!BufferSize)
						BufferSize=1;
					T *NewValues=CreateValues(BufferSize);
					for(int i=0;i<OldSize;i++)
					{
						NewValues[i+(i>=index)]=Values[i];
					}
					operator delete[](Values);
					Values=NewValues;
				}
				else
				{
					for(int i=index;i<NumValues;i++)
					{
						Values[i+1]=Values[i];
					}
				}
				Values[index]=val;
				NumValues++;
			}
		/// Insert a T of at position index, return a reference to it.
			T &Insert(int index)
			{
				if(NumValues==BufferSize)
				{
					int OldSize=BufferSize;
					BufferSize*=2;
					if(!BufferSize)
						BufferSize=1;
					T *NewValues=CreateValues(BufferSize);
					for(i=0;i<OldSize;i++)
					{
						NewValues[i+(i>=index)]=Values[i];
					}
					NumValues++;
					operator delete[](Values);
					Values=NewValues;
				}
				else
				{
					for(i=0;i<NumValues;i++)
					{
						Values[i+(i>=index)]=Values[i];
					}
				}
				NumValues++;
				Values=NewValues;
				return NewValues[index];
			}        
		/// Remove the first found instance equal to Val. There must be at least one such instance.
			void Cut(const T& Val)
			{
				for(register int i=0;i<NumValues;i++)
				{
					if(Values[i]==Val)
					{
						Remove(i);
						return;
					}
				}
			#ifdef BOUNDS_CHECKING
				throw NotInList();
			#endif
			}
		/// Remove the first found instance equal to Val, if there is one, otherwise do nothing.
			bool CutIfPresent(const T& Val)
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
		/// Remove the value at index, swapping in the value at the end of the list.
			void Remove(int index)
			{
			#ifdef CHECK_LIST_BOUNDS
				Check(index);
			#endif
				NumValues--;
				Values[index]=Values[NumValues];
			}
		//------------------------------------------------------------------------------
			void Remove(int index,int length)
			{
				Check(index);  
				Check(index+length-1);
				int i;
				for(i=0;i<NumValues-1;i++)
				{
					Values[i]=Values[i+length*(i>=index)];
				}
				NumValues-=length;
				Values=NewValues;
			}
		/// Return the position ion the list of the first instance equal to Val, otherwise -1.
			int Index(const T& Val)
			{
				for(int i=0;i<NumValues;i++)
				{
					if(Values[i]==Val)
						return i;
				}
				return -1;
			}
		/// Find the position of the first instance equal to Val after position index.
			int FindNext(const T& Val,const int index)
			{
				int i;
				int result=-1;
				for(i=index;i<NumValues;i++)
				{
					if(Values[i]==Val)
					{
						if(result==-1)
							result=i;
			#ifdef BOUNDS_CHECKING    
						else
							throw Duplicate();
			#endif
					}
				}
				return result;
			}               
#include "simul/base/Set.h"
		/// Append OriginList to the end of TargetList, clearing OriginList in the process.                
			friend void JoinLists(Set<T> &TargetList,Set<T> &OriginList)
			{
				if(TargetList.NumValues+OriginList.NumValues<=TargetList.BufferSize)
				{
					for(int i=0;i<OriginList.NumValues;i++)
						TargetList.Values[TargetList.NumValues+i]=OriginList.Values[i];
					TargetList.NumValues+=OriginList.NumValues;   
					OriginList.NumValues=0;
					return;
				}
				T *NewValues=NULL;
				if(TargetList.NumValues+OriginList.NumValues>0)
				{          
					int i;                    
					TargetList.BufferSize=2*(TargetList.NumValues+OriginList.NumValues);
					NewValues=TargetList.CreateValues(TargetList.BufferSize);
					for(i=0;i<TargetList.NumValues;i++)
						NewValues[i]=TargetList.Values[i];
					for(i=0;i<OriginList.NumValues;i++)
						NewValues[TargetList.NumValues+i]=OriginList.Values[i];
				}
				operator delete[](TargetList.Values);
				TargetList.NumValues+=OriginList.NumValues;
				OriginList.NumValues=0;
				TargetList.Values=NewValues;
			}
		/// Shift the value at Index from OriginList to the end of TargetList.
			friend void ShiftElement(Set<T> &TargetList,
				Set<T> &OriginList,int Index)
			{
			#ifdef CHECK_LIST_BOUNDS
				OriginList.Check(Index);
			#endif
				if(TargetList.NumValues==TargetList.BufferSize)
				{
					TargetList.BufferSize*=2;
					if(!TargetList.BufferSize)
						TargetList.BufferSize=1;
					T *NewValues=TargetList.CreateValues(TargetList.BufferSize);
					for(int i=0;i<TargetList.NumValues;i++)
						NewValues[i]=TargetList.Values[i];
					operator delete[](TargetList.Values);  
					TargetList.Values=NewValues;
				}       
				TargetList.Values[TargetList.NumValues]=OriginList.Values[Index];
				TargetList.NumValues++;
				OriginList.NumValues--;
				for(int i=Index;i<OriginList.NumValues;i++)
					OriginList.Values[i]=OriginList.Values[i+1];
			}
		/// Find the value ptr, shift it from OriginList to the end of TargetList.
			friend void ShiftElementByValue(Set<T> &TargetList,
				Set<T> &OriginList,T ptr)
			{
				ShiftElement(TargetList,OriginList,OriginList.Index(ptr));
			}
		};
	}
}
#endif