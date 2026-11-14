#ifdef _MSC_VER
#pragma once
#endif

// template class for signle linked list

#ifndef _LIST_HPP
#define _LIST_HPP

// base class for defining elements of SList

// int template will allow multiple inheritance for single object

template <int i=0> class TLink
{
	protected:
	void Reset(){next=NULL;} // reset without any clean up

	public:
	TLink *next;
	
	TLink(){next=NULL;}
	~TLink(){}

	private: // disable copy
	TLink( const TLink &src );
	void operator =( const TLink &src );
};

#define SLink TLink<0>

// circular list - any item can be used as root

template <class Type,class Base=SLink >
class SList: public Base
{
	protected:
	void Reset(){Base::Reset();}

	public:
	SList(){}
	void Insert( Type *element )
	{
		// insert before first element
		element->Base::next=Base::next;
		Base::next=element;		
	}
	void DeleteNext( Base *cur, Type *element )
	{
		Assert( cur->next==element );
		cur->next=element->Base::next;
		element->Base::next=NULL;
	}
	void Delete( Type *element )
	{
		Base *cur=this;
		while( cur && cur->next!=element )
		{
			cur=cur->next;
			if( !cur )
			{
				Fail("Item not in list");
				return;
			}
		}
		DeleteNext(cur,element);
	}
	void InsertAfter( Type *cur, Type *element )
	{
		element->Base::next=cur->Base::next;
		cur->Base::next=element;
	}
	void Clear()
	{
		while( Start() )
		{
			DeleteNext(this,Start());
		}
	}
	Type *Start() const {return static_cast<Type *>(Base::next);}
	bool NotEnd( Type *item ) const {return item!=NULL;}
	Type *Advance( Type *item ) const {return static_cast<Type *>(item->Base::next);}

	#if __GNUC__	
	Type *First() const {return static_cast<Type *>(Base::next);}
	Type *Next( Type *cur ) const {return static_cast<Type *>(cur->Base::next);}
	#else
	Type *First() const {return static_cast<Type *>(Base::next);}
	Type *Next( Type *cur ) const {return static_cast<Type *>(cur->Base::next);}
	#endif

	void Move( SList &src );

	private: // disable copy
	SList( const SList &src );
	void operator = ( const SList &src );
};

template <class Type,class Base>
void SList<Type,Base>::Move( SList &src )
{
	// take whole list src and move it into this
	// result: this contains merged list, src is empty
	// take first and last members of src
	Base *srcFirst=src.First();
	if( !srcFirst ) return; // nothing to merge with
	// insert first.. before first member of this
	// link last of src with first of this
	//find last of src
	Base **last=&Base::next;
	while (*last) last = &last->next;
	*last = srcFirst;
	// remove list from the source
	src.Base::next=NULL;
}

#endif


