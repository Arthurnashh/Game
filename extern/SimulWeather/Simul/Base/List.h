#ifndef SIMUL_BASE_LIST_H
#define SIMUL_BASE_LIST_H
#ifndef _MSC_VER
#include <stdlib.h>
#endif
#ifdef CHECK_LIST_BOUNDS
class OutOfBounds{};
class NotInList{};
class Duplicate{};
#endif

namespace simul
{
	namespace base
	{
		/// \internal Class template for a pointer list.

		/// \internal The template handles new and delete. Call Add() then use the pointer it returns to
		/// fill in the properties.
		template<class T> class PointerList
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
		void Place(int idx,T *t)
		{
			Destroy(Values[idx]);
			Values[idx]=t;
		}
		friend void JoinLists(PointerList<T> &TargetList,PointerList<T> &OriginList)
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
			delete [] TargetList.Values;
			delete [] OriginList.Values;
			TargetList.NumValues+=OriginList.NumValues;
			OriginList.NumValues=0;
			TargetList.Values=NewValues;
			OriginList.Values=NULL;
		}
		//------------------------------------------------------------------------------
		friend void ShiftElement(PointerList<T> &TargetList,
			PointerList<T> &OriginList,int Index)
		{
			OriginList.Check(Index);
			T **NewValues=new T*[TargetList.NumValues+1];
			register int i,j;
			for(i=0;i<TargetList.NumValues;i++)
			{
				NewValues[i]=TargetList.Values[i];
			}
			NewValues[TargetList.NumValues]=OriginList.Values[Index];

			delete [] TargetList.Values;
			TargetList.NumValues++;
			TargetList.Values=NewValues;
			NewValues=NULL;
			if(OriginList.NumValues>1)
			{
				NewValues=new T*[OriginList.NumValues-1];
				for(i=0;i<OriginList.NumValues-1;i++)
					NewValues[i]=OriginList.Values[i+(i>=Index)];
			}
			delete [] OriginList.Values;
			OriginList.NumValues--;
			OriginList.Values=NewValues;
		}
		//------------------------------------------------------------------------------
		friend void ShiftElementByValue(PointerList<T> &TargetList,
			PointerList<T> &OriginList,T *ptr)
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
		inline PointerList(const PointerList<T>&Val)
		{
			NumValues=Val.NumValues;
			Values=new T*[NumValues];
		}
		//------------------------------------------------------------------------------
		inline PointerList(void)
		{
			Values=NULL;
			NumValues=0;
		}
		//------------------------------------------------------------------------------
		inline ~PointerList(void)
		{
			Clear();
		}
		static void Destroy(T* t)
		{
			delete t;
		}
		//------------------------------------------------------------------------------
		inline void Clear(void)
		{
			if(NumValues)
			{
				for(int i=0;i<NumValues;i++)
					Destroy(Values[i]);
			}
			delete [] Values;
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
		PointerList<T>& operator=(const PointerList<T>&Lst)
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
		PointerList<T> SubList(int pos,int len)
		{
			PointerList<T> Ret;
			Ret.Clear();
		#ifdef SIMUL_EXCEPTIONS
			if(pos+len>NumValues)
				throw OutOfBounds();
		#endif
			for(int i=pos;i<pos+len;i++)
			{
				Ret.Add(Values[i]);
			}
			return Ret;
		}
		//------------------------------------------------------------------------------
		T *Add(void)
		{
			T **NewValues=new T*[NumValues+1];
			for(int i=0;i<NumValues;i++)
				NewValues[i]=Values[i];
			T *NewPointer=new T;
			NewValues[NumValues]=NewPointer;
			NumValues++;            
			delete [] Values;
			Values=NewValues;
			return NewPointer;
		}
		/// Increase the size of the list by extra. Values are preserved.
		void Increase(int extra)
		{
			int i;
			T **NewValues=new T*[NumValues+extra];
			for(i=0;i<NumValues;i++)
			{
				NewValues[i]=Values[i];
			}
			for(i=NumValues;i<NumValues+extra;i++)
			{
				NewValues[i]=new T();
			}
			NumValues+=extra;
			delete [] Values;
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
			{
				NewValues[i]=Values[i];
			}
			for(i=NewSize;i<NumValues;i++)
			{
				Destroy(Values[i]);
			}
			NumValues=NewSize;
			delete [] Values;
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
			delete [] Values;
			Values=NewValues;
			return t;
		}
		//------------------------------------------------------------------------------
		T *Insert(int index)
		{
			int i;
			T *NewPointer=new T();
			T **NewValues=new T*[NumValues+1];
			for(i=0;i<NumValues;i++)
			{
				NewValues[i+(i>=index)]=Values[i];
			}
			delete [] Values;
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
			delete [] Values;
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
			delete [] Values;
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
				{
					NewValues[i]=Values[i];
				}        
				for(i=index;i<NumValues-length;i++)
				{
					NewValues[i]=Values[i+length];
				}
			}
			delete [] Values;
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
		int Count(const T& Val)
		{
			int i;
			int result=0;
			for(i=0;i<NumValues;i++)
				if(*Values[i]==Val)
					result++;
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
		//------------------------------------------------------------------------------
		// Serialization
		/*
		friend istream &operator>>(istream &is,PointerList<T> &p)
		{     
			int val;
			is.read((char *)&(val),sizeof(val));
			int i;
			for (i=0;i<val;i++)
			{
				p.Add();
				is>>*(p[i]);
			}
			return is;
		}
		//------------------------------------------------------------------------------
		friend ostream &operator<<(ostream &os,PointerList<T> &p)
		{
			int val;
			val=p.Size();
			os.write((char *)&(val),sizeof(val));
			int i;
			for (i=0;i<val;i++)
			{
				os<<*(p[i]);
			}

			return os;
		}
		*/
		};

		/// \internal A list of instances of class T, contiguous in memory.
		template<class T> class ItemList
		{
		private:
			T*  Values;
			inline int Check(int i) const
			{                         
		#ifdef CHECK_LIST_BOUNDS
				if(i<0||i>=NumValues)
					throw OutOfBounds();  
		#endif                                                               
				return i;
			}
		public:            
			int NumValues;
			const T& operator[](int i) const
			{
				return Values[Check(i)];
			}
			inline T& operator[](int i)
			{
			#ifdef CHECK_LIST_BOUNDS
				return Values[Check(i)];
			#else             
				return Values[i];
			#endif
			}           
			const T operator()(const int i) const
			{
			#ifdef CHECK_LIST_BOUNDS
				return Values[Check(i)];
			#else             
				return Values[i];
			#endif
			}
			T& operator()(const int i)
			{
			#ifdef CHECK_LIST_BOUNDS
				return Values[Check(i)];
			#else             
				return Values[i];
			#endif
			}
			bool operator==(const ItemList<T>L)
			{
				if(NumValues!=L.NumValues)
					return false;
				int i;
				for(i=0;i<NumValues;i++)
				{
					if(Values[i]!=L.Values[i])
						return false;
				}
				return true;
			}
		//------------------------------------------------------------------------------
			inline ItemList(const ItemList<T>&Val)
			{
				int i;
				NumValues=Val.NumValues;
				Values=new T[NumValues];
				for(i=0;i<NumValues;i++)
					Values[i]=Val.Values[i];
			}
			inline ItemList(int numvals)
			{
				NumValues=numvals;
				Values=new T[NumValues];
			}               
			inline ItemList(int numvals,const T &val)
			{
				NumValues=numvals;
				Values=new T[NumValues];  
				for(int i=0;i<NumValues;i++)
					Values[i]=val;
			}         
			inline void Swap(int Id1,int Id2)
			{
				Check(Id1);
				Check(Id2);
				T temp=Values[Id1];
				Values[Id1]=Values[Id2];
				Values[Id2]=temp;
			}               
			void RotateDown(void)
			{
				T first=Values[0];
				for(int i=1;i<NumValues;i++)
					Values[i-1]=Values[i];
				Values[NumValues-1]=first;
			}
		//------------------------------------------------------------------------------
		void Increase(int extra)
		{
			int i;
			T *NewValues=new T[NumValues+extra];
			for(i=0;i<NumValues;i++)
			{
				NewValues[i]=Values[i];
			}
			NumValues+=extra;
			delete [] Values;
			Values=NewValues;
		}
		//------------------------------------------------------------------------------
		inline ItemList(void)
		{
			Values=NULL;
			NumValues=0;
		}
		//------------------------------------------------------------------------------
		inline ~ItemList(void)
		{
			Clear();
		}
		//------------------------------------------------------------------------------
		inline void Clear(void)
		{
			delete [] Values;
			Values=NULL;
			NumValues=0;
		}
		//------------------------------------------------------------------------------
		inline int Size(void) const
		{
			return NumValues;
		}
		//------------------------------------------------------------------------------
		ItemList<T> SubList(int pos,int len)
		{
			ItemList<T> Ret;
		#ifdef CHECK_LIST_BONDS
			if(pos+len>NumValues)
				throw SimulError(); 
		#endif
			Ret.NumValues=len;
			Ret.Values=new T[len];
			for(int i=pos;i<pos+len;i++)
			{
				Ret.Values[i-pos]=Values[i];
			}
			return Ret;
		}
		//------------------------------------------------------------------------------
		ItemList<T>& operator=(const ItemList<T>&Lst)
		{
			Clear();
			NumValues=Lst.NumValues;
			Values=new T[NumValues];
			for(int i=0;i<NumValues;i++)
			{
				Values[i]=Lst.Values[i];
			}
			return *this;
		}
		//------------------------------------------------------------------------------
		ItemList<T> operator+(const ItemList<T>&Lst2)
		{
			ItemList<T> Ret;
			Ret.Clear();
			Ret.NumValues=NumValues+Lst2.NumValues;
			Ret.Values=new T[Ret.NumValues];
			for(int i=0;i<NumValues;i++)
				Ret.Values[i]=Values[i];
			for(int i=0;i<Lst2.NumValues;i++)
				Ret.Values[NumValues+i]=Lst2.Values[i];
			return Ret;
		}
		//------------------------------------------------------------------------------
		ItemList<T>& operator+=(const ItemList<T>&Lst2)
		{
			int i;
			T *NewValues=new T[NumValues+Lst2.NumValues];
			for(i=0;i<NumValues;i++)
				NewValues[i]=Values[i];
			for(i=0;i<Lst2.NumValues;i++)
				NewValues[NumValues+i]=Lst2.Values[i];
			NumValues+=Lst2.NumValues;
			delete [] Values;
			Values=NewValues;
			return *this;
		}
		/// Add a T equal to Val to the end of the list.
		void Add(const T& Val)
		{
			T *NewValues=new T[NumValues+1];
			for(int i=0;i<NumValues;i++)
				NewValues[i]=Values[i];
			NewValues[NumValues]=Val;
			NumValues++;        
			delete [] Values;
			// Don't delete  until here because Val might actually be one of those values.
			Values=NewValues;
		}
		/// Resize the list to size, making each value equal to Val.
		void Resize(int size,const T& Val)
		{
			if(size!=NumValues)
			{
				delete [] Values;
				NumValues=size;
				Values=new T[NumValues];
			}
			for(int i=0;i<NumValues;i++)
				Values[i]=Val;
		}         
		/// Resize the list to size; each value will have the default properties. Only call this if you're going to fill in ALL the values immediately.
		void Resize(int size)             // Only call this if you're going to fill in ALL the values immediately.
		{         
			delete [] Values;
			NumValues=size;
			if(size>=0)
				Values=new T[NumValues];
		}
		/// Add a T to the end of the list.
		void Add(void)
		{
			int i;
			T *NewValues=new T[NumValues+1];
			for(i=0;i<NumValues;i++)
				NewValues[i]=Values[i];
			delete [] Values;
			NumValues++;
			Values=NewValues;
		}
		/// Insert a T of value Val at position index.
		void Insert(int index,const T& Val)
		{
			int i;
			T *NewValues=new T[NumValues+1];
			NewValues[index]=Val;
			for(i=0;i<NumValues;i++)
				NewValues[i+(i>=index)]=Values[i];
			delete [] Values;
			NumValues++;
			Values=NewValues;
		} 
		//------------------------------------------------------------------------------
		void InsertToSortedContents(const T& Val)
		{
			int i;
			T *NewValues=new T[NumValues+1];
			int index=NumValues+1;
			if(!NumValues)
				index=0;
			for(i=0;i<NumValues;i++)
			{
				if(index==NumValues+1)
					if(*Values[i]>*Val)
						index=i;
				NewValues[i+(i>=index)]=Values[i];
			}         
			if(index==NumValues+1)
				index=NumValues;
			delete [] Values;
			NewValues[index]=Val;
			NumValues++;
			Values=NewValues;
		}
		//------------------------------------------------------------------------------
		void InsertToSorted(const T& Val)
		{
			int i;
			T *NewValues=new T[NumValues+1];
			int index=NumValues+1;
			if(!NumValues)
				index=0;
			for(i=0;i<NumValues;i++)
			{
				if(index==NumValues+1)
					if(Values[i]>Val)
						index=i;
				NewValues[i+(i>=index)]=Values[i];
			}         
			if(index==NumValues+1)
				index=NumValues;
			delete [] Values;
			NewValues[index]=Val;
			NumValues++;
			Values=NewValues;
		}
		/// Remove the value at index, shift all values above it down by one position.
		void Remove(int index)
		{
			Check(index);
			if(NumValues<=0)
				return;
			T *NewValues;
			if(NumValues>1)
			{                 
				NumValues--;
				NewValues=new T[NumValues];
				for(int i=0;i<NumValues;i++)
				{
        			NewValues[i]=Values[i+(i>=index)];
				}             
				delete [] Values;
				Values=NewValues;
			}
			else
			{        
				delete [] Values;
				NumValues=0;
				Values=NULL;
			}
		}
		/// Remove the first found instance equal to Val. There must be at least one such instance.
		void Cut(const T& Val)
		{
			int i;
			for(i=0;i<NumValues;i++)
			{
				if(Values[i]==Val)
				{
					Remove(i);
					return;
				}
			}
		#ifdef SIMUL_EXCEPTIONS
			throw NotInList();
		#endif
		}   
		/// Remove the first found instance equal to Val, if there is one, otherwise do nothing.
		void CutIfPresent(const T& Val)
		{
			int i;
			for(i=0;i<NumValues;i++)
			{
				if(Values[i]==Val)
				{
					Remove(i);
					return;
				}
			}
		}
		//------------------------------------------------------------------------------
		int Count(const T& Val)
		{         
			int c=0;
			for(int i=0;i<NumValues;i++)
			{
				if(Values[i]==Val)
					c++;
			}
			return c;
		}
		/// Return true if there is any instance equal to Val, otherwise false.
		bool Contains(const T& Val)
		{         
			int i;
			for(i=0;i<NumValues;i++)
			{
				if(Values[i]==Val)
					return true;
			}
			return false;
		}
		/// Return the position ion the list of the first instance equal to Val, otherwise -1.
		int Index(const T& Val)
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
						throw Duplicate();
		#endif
				}
			}
			return result;
		}                 
		/// Return the position ion the list of the first instance equal to *Val, otherwise -1.
		int Index(const T* Val)
		{
			int i;
			int result=-1;
			for(i=0;i<NumValues;i++)
			{
				if(&Values[i]==Val)
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
		#ifdef SIMUL_EXCEPTIONS
					else
						throw Duplicate();
		#endif
				}
			}
			return result;
		}
		/// Sort the list using operator< of T to compare.
		void Sort(void)
		{
			int i,j;
			for(i=0;i<NumValues;i++)
			{
				for(j=0;j<NumValues-1;j++)
				{
					if(Values[j+1]<Values[j])
					{
						T temp=Values[j+1];
						Values[j+1]=Values[j];
						Values[j]=temp;
					}
				}
			}
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
		//------------------------------------------------------------------------------
		// Serialization
		//------------------------------------------------------------------------------
		/*
		friend istream &operator>>(istream &is,ItemList<T> &Lst)
		{
			int val;
			is.read((char *)&(val),sizeof(val));
			int i;
			for (i=0;i<val;i++)
			{
				T temp;
				is>>temp;
				Lst.Add(temp);
			}

			return is;
		}
		friend ostream &operator<<(ostream &os,ItemList<T> &Lst)
		{
			int val;
			val=Lst.Size();
			os.write((char *)&(val),sizeof(val));
			int i;
			for (i=0;i<val;i++)
			{
				os<<Lst[i];
			}

			return os;
		}
		*/
		//------------------------------------------------------------------------------                      
		/// Append OriginList to the end of TargetList, clearing OriginList in the process.
		friend void JoinLists(ItemList<T> &TargetList,ItemList<T> &OriginList)
		{
			int i;
			T *NewValues=0;
			if(TargetList.NumValues+OriginList.NumValues>0)
			{
				NewValues=new T[TargetList.NumValues+OriginList.NumValues];
				for(i=0;i<TargetList.NumValues;i++)
				{
					NewValues[i]=TargetList.Values[i];
				}
				for(i=0;i<OriginList.NumValues;i++)
				{
					NewValues[TargetList.NumValues+i]=OriginList.Values[i];
				}
			}
			delete [] TargetList.Values;
			delete [] OriginList.Values;
			TargetList.NumValues+=OriginList.NumValues;
			OriginList.NumValues=0;
			TargetList.Values=NewValues;
			OriginList.Values=0;
		}
		friend void ShiftElementByValue(ItemList<T> &TargetList,
			ItemList<T> &OriginList,const T &ptr)
		{
			int i=OriginList.Index(ptr);
			ShiftElement(TargetList,OriginList,i);
		}
		friend void ShiftElement(ItemList<T> &TargetList,
			ItemList<T> &OriginList,int Index)
		{
			OriginList.Check(Index);
			T *NewValues=new T[TargetList.NumValues+1];
			for(int i=0;i<TargetList.NumValues;i++)
			{
				NewValues[i]=TargetList.Values[i];
			}
			NewValues[TargetList.NumValues]=OriginList.Values[Index];

			delete [] TargetList.Values;
			TargetList.NumValues++;
			TargetList.Values=NewValues;

			NewValues=0;
			if(OriginList.NumValues>1)
			{
				NewValues=new T[OriginList.NumValues-1];
				for(int i=0;i<OriginList.NumValues-1;i++)
				{
					NewValues[i]=OriginList.Values[i+(i>=Index)];
				}
			}
			delete [] OriginList.Values;
			OriginList.NumValues--;
			OriginList.Values=NewValues;
		}
		};
		/// \internal \brief Check what is touching in a list.

		/// \internal Given a list of things, and some way of finding out which are touching each
		/// other, this function should return the number of partitions, and a list
		/// assigning each "thing" to one of those partitions.
		/// T will usually be a pointer type
		template<class T> bool PartitionCheck(ItemList<T> &Items,
			ItemList<int> &ItemPartitions,bool(*Touching)(T t1,T t2),int *NumPartitions)
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
							j=0;
						}
						else if(ItemPartitions[j]==-1&&ItemPartitions[i]!=-1)       
						{
							ItemPartitions[j]=ItemPartitions[i];                    
							ItemsLeft--;
							i=0;   //Testing this!
							j=0;
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
		#ifdef SIMUL_EXCEPTIONS                                                              
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
		/// \internal \brief Check what is touching in a list.

		/// \internal Given a list of things, and some way of finding out which are touching each
		/// other, this function should return the number of partitions, and a list
		/// assigning each "thing" to one of those partitions.
		/// T will usually be a pointer type
		template<class T> bool PartitionCheck(PointerList<T> &Items,
			ItemList<int> &ItemPartitions,bool(*Touching)(T *t1,T *t2),int *NumPartitions)
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
		#ifdef SIMUL_EXCEPTIONS                                                        
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
// 
#endif
