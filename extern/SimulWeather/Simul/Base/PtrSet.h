#ifndef UnorderedStretchyListH
#define UnorderedStretchyListH
//------------------------------------------------------------------------------
#ifndef PLAYSTATION_2
#ifndef NDEBUG
#define BOUNDS_CHECKING
#endif
#endif
#include <vector>
class uDuplication{};
namespace simul
{
	namespace base
	{
		/// \internal An unordered elastic list of pointers to instances of class T, which are managed by the list.

		/// \internal Removing a member from within the list causes the member at the end to be swapped into its place.
		/// This makes for faster list management as the rest of the list need not be moved.
		template<class T> class PtrSet
		{
		private:
		T** Values;
		public:
		int NumValues;
		int BufferSize;
		inline PtrSet(const PtrSet<T>&Val)
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
		PtrSet<T> &operator=(const PtrSet<T> &Val)
		{
			int i;
			if(BufferSize<Val.NumValues)
			{
				for(i=0;i<BufferSize;i++)
					delete(Values[i]);
				delete[] (Values);
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
		//------------------------------------------------------------------------------
		PtrSet<T>()
		{
			Values=NULL;
			NumValues=0;
			BufferSize=0;
		}
		//------------------------------------------------------------------------------
		~PtrSet()
		{
			int i;
			for(i=0;i<BufferSize;i++)
			{
				delete(Values[i]);
			}
			delete[] (Values);
		}
		int Check(int i) const
		{
			if(i<0||i>=NumValues)
			{
				int errorhere=0;
				errorhere++;
				errorhere;
		#ifdef BOUNDS_CHECKING
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
						return true;
					}
				}
			}
			return false;
		}
			inline void uDuplicationCheck(void)
			{
		#ifdef BOUNDS_CHECKING
				int i,j;
				for(i=0;i<NumValues;i++)
				{
					for(j=0;j<i;j++)
					{
						if(*Values[i]==*Values[j])
							throw uDuplication();
					}
				}
		#endif
			}
			inline void uDuplicationCheck(const T& Val)
			{
		#ifdef BOUNDS_CHECKING
				int i,j;
				for(i=0;i<NumValues;i++)
				{
					if(*Values[i]==Val)
						throw uDuplication();
				}
		#endif
			}
		void SetBufferSize(int size)
		{
			if(size>BufferSize)
			{               
				register int i;
				for(i=0;i<BufferSize;i++)
				{
					delete(Values[i]);
				}
				delete[] (Values);
				Values=new T*[size];
				for(i=0;i<size;i++)
				{
					Values[i]=new T;
				}
				BufferSize=size;
			}
		#ifdef BOUNDS_CHECKING
			DupeCheck();
		#endif
		}
		void Resize(int size)
		{
			if(size>BufferSize)
			{               
				register int i;
				if(!BufferSize)
					BufferSize=1;
				while(size>BufferSize)
					BufferSize*=2;
				T** NewValues=new T*[BufferSize];
				for(i=0;i<NumValues;i++)
				{
					NewValues[i]=Values[i];
				}
				delete[] (Values);
				for(i=NumValues;i<BufferSize;i++)
				{
					NewValues[i]=new T;
				}
				Values=NewValues;
			}
			NumValues=size;  
			#ifdef BOUNDS_CHECKING
			DupeCheck();
			#endif
		}
		void Place(int idx,T *t)
		{
			delete(Values[idx]);
			Values[idx]=t;   
		#ifdef BOUNDS_CHECKING
			DupeCheck();
		#endif
		}
		//------------------------------------------------------------------------------
		const T* operator[](const int i) const
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
			return Values[i];
		}
		//------------------------------------------------------------------------------
		const T* operator()(const int i) const
		{
			if(i<0||i>=NumValues)
				return NULL;
		#ifdef BOUNDS_CHECKING
		DupeCheck();
		#endif
			return Values[i];
		}
		//------------------------------------------------------------------------------
		T* operator()(int i)
		{                    
			if(i<0||i>=NumValues)
				return NULL;    
		#ifdef BOUNDS_CHECKING
		 DupeCheck();
		#endif
			return Values[i];
		}
		//------------------------------------------------------------------------------
		inline T* operator[](int i)
		{                      
		#ifdef BOUNDS_CHECKING
			DupeCheck();           
			return Values[Check(i)];
		#else
			return Values[i];
		#endif
		}
		//------------------------------------------------------------------------------  
		inline void clear(void)
		{
			Clear();
		}
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
		T *Add(void)
		{
			if(NumValues>=BufferSize)
			{
				int OldSize=BufferSize;
				BufferSize*=2;
				if(!BufferSize)
					BufferSize=1;
				T **NewValues=new T*[BufferSize];
				for(int i=0;i<OldSize;i++)
					NewValues[i]=Values[i];
				for(int i=OldSize;i<BufferSize;i++)
					NewValues[i]=new T;
				delete[] (Values);
				Values=NewValues;
			}        
			T *NewPointer=Values[NumValues];
			NumValues++;      
		#ifdef BOUNDS_CHECKING
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
				T **NewValues=new T*[BufferSize];
				for(i=0;i<OldSize;i++)
				{
					NewValues[i]=Values[i];
				}
				for(i=OldSize;i<BufferSize;i++)
				{
					NewValues[i]=new T;
				}
				delete[] (Values);
				Values=NewValues;
			}      
			#ifdef BOUNDS_CHECKING
		 DupeCheck();
		  #endif
			NumValues+=extra;
		}
		T *Add(T &t)
		{
			Add();
			*(Values[NumValues-1])=t;
			return Values[NumValues-1];
		}
		//------------------------------------------------------------------------------
		T *Add(T *NewPointer)
		{
			int i;
			if(NumValues==BufferSize)
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
				for(i=OldSize+1;i<BufferSize;i++)
				{
					NewValues[i]=new T;
				}
				delete[] (Values);
				Values=NewValues;
			}
			else
			{
				delete(Values[NumValues]);
			}
			Values[NumValues]=NewPointer;
			NumValues++;
			return NewPointer;
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
		PtrSet<T> operator+(const PtrSet<T>&Lst2)
		{
			int i;
			simul::base::PtrSet<T> Ret;
			Ret=*this;
			for(i=0;i<Lst2.NumValues;i++)
			{
				if(!Contains(*Lst2.Values[i]))
					Ret.Add(*Lst2.Values[i]);
			}
			return Ret;
		}
		//------------------------------------------------------------------------------
		friend void Shift(PtrSet<T> &TargetList,
			PtrSet<T> &OriginList,int Index)
		{
			int i;
			OriginList.Check(Index);
		#ifdef BOUNDS_CHECKING
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
				delete[] (TargetList.Values);
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
			#ifdef BOUNDS_CHECKING
		 OriginList.DupeCheck();
		TargetList.DupeCheck();
		#endif
		}
		//------------------------------------------------------------------------------
		friend void ShiftElementByValue(PtrSet<T> &TargetList,
			PtrSet<T> &OriginList,T *ptr)
		{
			Shift(TargetList,OriginList,OriginList.Index(ptr));
		}
		//------------------------------------------------------------------------------
		T *Insert(int index)
		{
			T *NewPointer;
			if(NumValues==BufferSize)
			{
				int OldSize=BufferSize;
				BufferSize*=2;
				if(!BufferSize)
					BufferSize=1;
				T **NewValues=new T*[BufferSize];
				for(int i=0;i<OldSize;i++)
				{
					NewValues[i+(i>=index)]=Values[i];
				}
				for(int i=OldSize+1;i<BufferSize;i++)
				{
					NewValues[i]=new T;
				}
				delete[] (Values);
				Values=NewValues;
				NewPointer=new T;
				Values[index]=NewPointer;
			}
			else
			{
				NewPointer=Values[BufferSize-1];
				for(int i=0;i<NumValues;i++)
				{
					Values[i+(i>=index)]=Values[i];
				}
				Values[index]=NewPointer;
			}
			NumValues++;
			#ifdef BOUNDS_CHECKING
		 DupeCheck();
		 #endif
			return NewPointer;
		}
		//------------------------------------------------------------------------------
		T *Insert(int index,T *NewPointer)
		{
			if(NumValues==BufferSize)
			{
				int OldSize=BufferSize;
				BufferSize*=2;
				if(!BufferSize)
					BufferSize=1;
				T **NewValues=new T*[BufferSize];
				for(i=0;i<OldSize;i++)
				{
					NewValues[i+(i>=index)]=Values[i];
				}
				for(i=OldSize+1;i<BufferSize;i++)
				{
					NewValues[i]=NewPointer;
				}
				NumValues++;
				delete[] (Values);
				Values=NewValues;
			}
			else
			{
				delete(Values[BufferSize-1]);
				for(i=0;i<NumValues;i++)
				{
					Values[i+(i>=index)]=Values[i];
				}
			}
			NewValues[index]=NewPointer;
			NumValues++;
			Values=NewValues;
		#ifdef BOUNDS_CHECKING
			DupeCheck();
		#endif
			return NewPointer;
		}
		//------------------------------------------------------------------------------
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
		#ifdef BOUNDS_CHECKING
			throw NotInList();
		#endif
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
		#ifdef CHECK_LIST_BOUNDS
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
		#ifdef BOUNDS_CHECKING
			Check(index);  
		#endif
			//T *temp=Values[index];
			//temp->Clear();
			//for(register int i=0;i<NumValues-1;i++)
			//	Values[i]=Values[i+(i>=index)];
			//Values[NumValues-1]=temp;
			NumValues--;
			std::swap(Values[index],Values[NumValues]);
			Values[NumValues]->Clear();
		#ifdef BOUNDS_CHECKING
		 DupeCheck();
		#endif
		}                  
		//------------------------------------------------------------------------------
		void SwapRemove(int index)
		{
		//	Check(index);
			Values[index]->Clear();
			std::swap(Values[index],Values[NumValues-1]);
			NumValues--;              
		#ifdef BOUNDS_CHECKING
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
			#ifdef BOUNDS_CHECKING
		 DupeCheck();
		 #endif
		}
		//------------------------------------------------------------------------------
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
		#ifdef BOUNDS_CHECKING
					else
						throw Duplicate();
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
		#ifdef BOUNDS_CHECKING
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
		#ifdef BOUNDS_CHECKING    
					else
						throw Duplicate();
		#endif
				}
			}
			return result;
		}      
		friend void JoinLists(PtrSet<T> &TargetList,PtrSet<T> &OriginList)
		{
			T **NewValues=NULL;
			register int i;
			if(TargetList.NumValues+OriginList.NumValues>0)
			{
				NewValues=new T*[TargetList.NumValues+OriginList.NumValues];
				for(i=0;i<TargetList.NumValues;i++)
				{
					NewValues[i]=TargetList.Values[i];
				}
				for(i=0;i<OriginList.NumValues;i++)
				{
					NewValues[TargetList.NumValues+i]=OriginList.Values[i];
				}
				for(i=TargetList.NumValues;i<TargetList.BufferSize;i++)
				{
					delete(TargetList.Values[i]);
				}          
				for(i=OriginList.NumValues;i<OriginList.BufferSize;i++)
				{
					delete(OriginList.Values[i]);
				}
			}
			delete[] (TargetList.Values);
			delete[] (OriginList.Values);
			TargetList.NumValues+=OriginList.NumValues;
			OriginList.NumValues=0;
			TargetList.Values=NewValues;
			OriginList.Values=NULL;      
			TargetList.BufferSize=TargetList.NumValues;
			OriginList.BufferSize=0;
		#ifdef BOUNDS_CHECKING
			TargetList.DupeCheck();
		#endif
		}
		};

		#include "Simul/Base/PtrSet.h"
		#include "Simul/Base/List.h"
		/// \internal \brief Check what is touching in a list.

		/// \internal Given a list of things, and some way of finding out which are touching each
		/// other, this function should return the number of partitions, and a list
		/// assigning each "thing" to one of those partitions.
		/// T will usually be a pointer type
		template<class T> bool PartitionCheck(PtrSet<T> &Items,
			std::vector<int> &ItemPartitions,bool(*Touching)(T *t1,T *t2),int *NumPartitions)
		{
			int i,j;
			if(Items.Size()<2)
				return false;
			ItemPartitions.resize(Items.Size(),-1);
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
		#ifdef BOUNDS_CHECKING
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
		};
	}
}
#endif