#ifdef _MSC_VER
#pragma once
#endif

/*!
\file
heap (memory, index, ... ) management
*/

#ifndef _HEAP_HPP
#define _HEAP_HPP

#include <Es/Framework/debugLog.hpp>
#include <Es/Common/fltopts.hpp>
#include <Es/Common/langExt.hpp>
#include <Es/Containers/cachelist.hpp>

// operators required:
// HeapSize must be arithmetic
// Index operator + ( Index, HeapSize )

#include <Es/Memory/normalNew.hpp>

/// empty attributes to Heap template
class HeapNoAttributes
{
};

/// "precious" memory manager with external overhead
/**
External overhead is used. Useful when manager memory is precious (because of alignment of other properties) 
or to manage different resources.
*/

template <class Index,class HeapSize,class Attributes=HeapNoAttributes>
class Heap: private NoCopy
{
	public:
	class HeapItem: public TLinkBidirD, private HeapNoAttributes
	{
		friend class Heap<Index,HeapSize,Attributes>;

		protected:
		Index _pos;
		HeapSize _size;

		public:
		Index Memory() const {return _pos;}
		HeapSize Size() const {return _size;}

    void SetAttributes(const Attributes &attr){Attributes::operator =(attr);}
    const Attributes &GetAttributes(const Attributes &attr) const {return *this;}
    
		USE_FAST_ALLOCATOR;
	};
	
	private:
	TListBidir<HeapItem> _freeRoot;
	TListBidir<HeapItem> _busyRoot;
	HeapSize _align;

	Index _pos;
	HeapSize _size;
	HeapSize _totalBusy; // _size-_totalBusy=_totalFree
	
	Index AlignIndex( Index value ) const
	{
		return (Index)(((HeapSize)value+_align-1)&~(_align-1));
	}
	HeapSize AlignSize( HeapSize value ) const
	{
		return (value+_align-1)&~(_align-1);
	}

	void DoConstruct( Index pool, HeapSize size, HeapSize align );
	void DoDestruct();
	
	public:
	Heap( Index pool, HeapSize size, HeapSize align )
	{
		DoConstruct(pool,size,align);
	}
	Heap()
	{
		//_freeRoot=_busyRoot=NULL;
		_align=1;
		_pos=0;
		_size=0;
	}
	void Init( Index pool, HeapSize size, HeapSize align )
	{
		DoDestruct();
		DoConstruct(pool,size,align);
	}
	
	void Free() {DoDestruct();}
	~Heap() {DoDestruct();}

  /// try allocation, return a difference for best match
  int TryAllocMatch(HeapSize size) const;

  /// allocate given amount
	HeapItem *Alloc( HeapSize size, const Attributes &attr=Attributes() );
  /// allocate given amount at given position
	HeapItem *Alloc( Index at, HeapSize size, const Attributes &attr=Attributes() );
	/// free given block
	void Free( HeapItem *pos );

	Index Memory() const {return _pos;}
	HeapSize Size() const {return _size;}

	void Move( HeapSize offset ); // be careful with this function

	HeapSize MaxFreeLeft() const;
	HeapSize CalcTotalFreeLeft() const;
	HeapSize CalcTotalBusy() const;
	int CountFreeLeft() const;

	HeapSize TotalFreeLeft() const
	{
		AssertDebug( _size-_totalBusy==CalcTotalFreeLeft() );
		return _size-_totalBusy;
	}
	HeapSize TotalBusy() const
	{
		AssertDebug( _totalBusy==CalcTotalBusy() );
		return _totalBusy;
	}

	bool Check() const; // check heap for integrity
};

#include <Es/Memory/debugNew.hpp>

template <class Index, class HeapSize,class Attributes>
void Heap<Index,HeapSize,Attributes>::DoConstruct( Index pool, HeapSize size, HeapSize align )
{
	_totalBusy=0;
	_align=align;
	//if( pool==0 ) pool++,size--; // pool must be >0, 0 is used to indicate error
	// pool must be aligned
	Index alignPool=AlignIndex(pool);
	size-=alignPool-pool;
	pool=alignPool;
	// size must be aligned
	size=size&~(_align-1);
	// setup data members
	//_busyRoot=NULL;
	//_freeRoot
	HeapItem *allFree=new HeapItem;
	allFree->_pos=pool;
	allFree->_size=size;
	_freeRoot.Insert(allFree);
	_pos=pool;
	_size=size;
}

template <class Index, class HeapSize,class Attributes>
void Heap<Index,HeapSize,Attributes>::DoDestruct()
{
	HeapItem *item;
	// caution: pointer aliasing - changes root
	// root will change from item to item->next
	while( (item=_freeRoot.First())!=NULL )
	{
		item->Delete(),delete item;
	}
	while( (item=_busyRoot.First())!=NULL )
	{
		item->Delete(),delete item;
	}
	_pos=0;
	_size=0;
}

template <class Index, class HeapSize,class Attributes>
typename Heap<Index,HeapSize,Attributes>::HeapItem *Heap<Index,HeapSize,Attributes>::Alloc
(
	Index at, HeapSize size, const Attributes &attributes
)
{
	// force allocation at given place
	if( size==0 ) return NULL;
	// align size
	size=AlignSize(size);
	// find best match
	HeapItem *free;
	HeapItem *best=NULL;
	for( free=_freeRoot.First(); free; free=_freeRoot.Next(free) )
	{
		// select free item containing given block
		if( free->_pos<=at && free->_pos+free->_size>=at+size )
		{
			best=free;
			break;
		}
	}
	if( !best ) return NULL;
	if( free->_pos==at && free->_size==size ) // exact match
	{
		// delete best from the free list
		best->Delete();
		// insert best into the the busy list
		_busyRoot.Insert(best);
		_totalBusy+=best->_size;
		Assert( _totalBusy<_size );
		best->SetAttributes(attributes);
		return best;
	}
	else
	{
		// split rest of the item
		Index end=free->_pos+free->_size;
		if( end>at+size )
		{
			HeapItem *rest=new HeapItem;
			rest->_pos=at+size;
			rest->_size=end-rest->_pos;
			_freeRoot.Insert(rest);
		}
		HeapItem *busy=new HeapItem;
		// use last size elements from best
		busy->_pos=at;
		busy->_size=size;
		best->_size=at-best->_pos;
		
		// insert busy into the busy list
		_busyRoot.Insert(busy);
		_totalBusy+=busy->_size;
		Assert( _totalBusy<_size );
		busy->SetAttributes(attributes);
		return busy;
	}
}

/**
@return >=0 -> match found, difference, <0 -> no match found
*/
template <class Index, class HeapSize,class Attributes>
int Heap<Index,HeapSize,Attributes>::TryAllocMatch(HeapSize size) const
{
	if( size==0 ) return 0;
	// align size
	size=AlignSize(size);
	// find best match
	HeapSize bestDiff=INT_MAX;
	HeapItem *best=NULL;
	for( HeapItem *free=_freeRoot.First(); free; free=_freeRoot.Next(free) )
	{
		HeapSize diff=free->_size-size;
		if( diff<0 ) continue;
		if( diff<bestDiff )
		{
			best=free;
			bestDiff=diff;
			if( diff==0 ) break; // no match could be better
		}
	}
	if (!best) return -1;
	return bestDiff;
}

template <class Index, class HeapSize,class Attributes>
typename Heap<Index,HeapSize,Attributes>::HeapItem *Heap<Index,HeapSize,Attributes>::Alloc(
  HeapSize size, const Attributes &attributes )
{
	if( size==0 ) return NULL;
	// align size
	size=AlignSize(size);
	// find best match
	HeapSize bestDiff=INT_MAX;
	HeapItem *best=NULL;
	for( HeapItem *free=_freeRoot.First(); free; free=_freeRoot.Next(free) )
	{
		HeapSize diff=free->_size-size;
		if( diff<0 ) continue;
		if( diff<bestDiff )
		{
			best=free;
			bestDiff=diff;
			if( diff==0 ) break; // no match could be better
		}
	}
	if( !best )
	{
		return NULL;
	}
	if( bestDiff==0 ) // exact match
	{
		// delete best from the free list
		best->Delete();
		
		// insert best into the the busy list
		_busyRoot.Insert(best);
		_totalBusy+=best->_size;
		Assert( _totalBusy<=_size );
		best->SetAttributes(attributes);
		return best;
	}
	else
	{
		// split item into two parts
		HeapItem *busy=new HeapItem;
		// use first size elements from best
		busy->_size=size;
		busy->_pos=best->_pos;
		// eat first size elements from best
		best->_size=bestDiff;
		best->_pos+=size;
		
		// insert busy into the busy list
		_busyRoot.Insert(busy);
		_totalBusy+=busy->_size;
		Assert( _totalBusy<_size );
		busy->SetAttributes(attributes);
		return busy;
	}
}

template <class Index, class HeapSize,class Attributes>
void Heap<Index,HeapSize,Attributes>::Free( HeapItem *item )
{
	if( !item ) return;
	HeapItem *free;
	//TryAgain:
	_totalBusy-=item->_size;
	Assert( _totalBusy>=0 );
	for( free=_freeRoot.First(); free; )
	{
		HeapItem *next=_freeRoot.Next(free);
		// try to find free item just before item
		if( free->_pos+free->_size==item->_pos )
		{
			// prepend free to item
			item->_pos=free->_pos;
			item->_size+=free->_size;
			// delete free
			free->Delete(),delete free;
			//goto TryAgain;
		}
		// try to find free item just after item
		else if( free->_pos==item->_pos+item->_size )
		{
			// append free to item
			item->_size+=free->_size;
			// delete free
			free->Delete(),delete free;
			//goto TryAgain;
		}
		free=next;
	}
	// remove item from the busy list
	item->Delete();
	// insert item into the free list
	_freeRoot.Insert(item);
}

template <class Index, class HeapSize,class Attributes>
HeapSize Heap<Index,HeapSize,Attributes>::MaxFreeLeft() const
{
	HeapSize max=0;
	HeapItem *free;
	for( free=_freeRoot.First(); free; free=_freeRoot.Next(free) )
	{
		if( max<free->_size ) max=free->_size;
	}
	return max;
}
template <class Index, class HeapSize,class Attributes>
HeapSize Heap<Index,HeapSize,Attributes>::CalcTotalFreeLeft() const
{
	HeapSize total=0;
	HeapItem *free;
	for( free=_freeRoot.First(); free; free=_freeRoot.Next(free) )
	{
		total+=free->_size;
	}
	return total;
}
template <class Index, class HeapSize,class Attributes>
int Heap<Index,HeapSize,Attributes>::CountFreeLeft() const
{
	int count=0;
	HeapItem *free;
	for( free=_freeRoot.First(); free; free=_freeRoot.Next(free) )
	{
		count++;
	}
	return count;
}

template <class Index, class HeapSize,class Attributes>
HeapSize Heap<Index,HeapSize,Attributes>::CalcTotalBusy() const
{
	int total=0;
	HeapItem *busy;
	for( busy=_busyRoot.First(); busy; busy=_busyRoot.Next(busy) )
	{
		total+=busy->_size;
	}
	return total;
}

template <class Index, class HeapSize,class Attributes>
void Heap<Index,HeapSize,Attributes>::Move( HeapSize offset ) // be carefull with this function
{
	_pos+=offset; // relocate heap base
	// relocate all heap items
	HeapItem *free,*busy;
	for( free=_freeRoot.First(); free; free=_freeRoot.Next(free) )
	{
		free->_pos+=offset;
	}
	for( busy=_busyRoot.First(); busy; busy=_busyRoot.Next(busy) )
	{
		busy->_pos+=offset;
	}
}

template <class Index, class HeapSize,class Attributes>
bool Heap<Index,HeapSize,Attributes>::Check() const
{
	HeapItem *item;
	for( item=_freeRoot.First(); item; item=_freeRoot.Next(item) )
	{
		if( item->_pos<_pos ){Fail("Heap check");return false;}
		if( item->_size<0 ){Fail("Heap check");return false;}
		if( item->_size>_size ){Fail("Heap check");return false;}
		if( item->_pos+item->_size>_pos+_size ){Fail("Heap check");return false;}
	}
	for( item=_busyRoot.First(); item; item=_busyRoot.Next(item) )
	{
		if( item->_pos<_pos ){Fail("Heap check");return false;}
		if( item->_size<0 ){Fail("Heap check");return false;}
		if( item->_size>_size ){Fail("Heap check");return false;}
		if( item->_pos+item->_size>_pos+_size ){Fail("Heap check");return false;}
	}
	return true;
}


#endif

