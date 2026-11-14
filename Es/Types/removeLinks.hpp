#ifdef _MSC_VER
#pragma once
#endif

#ifndef _REMOVELINKS_HPP
#define _REMOVELINKS_HPP

#include <Es/Containers/list.hpp>
#include <Es/Types/pointers.hpp>

// simple application of double-linked list
// when object of class derived from RemoveLinks is destructed,
// all links (pointers) to that object are set to NULL

#if !_RELEASE
	#define DO_LINK_DIAGS 1
#endif

class BaseLink;
class RemoveLinks;

class BaseLink: public SLink
{
	private:
	RemoveLinks *_ref;

	protected:
	void DoConstruct( RemoveLinks *ref );
	void DoDestruct();

	public:
	RemoveLinks *GetRef() const;

	BaseLink(){_ref=NULL;}
	BaseLink( RemoveLinks *src ){DoConstruct(src);}
	BaseLink( const BaseLink &src ){DoConstruct(src.GetRef());}
	BaseLink &operator = ( const BaseLink &src )
	{
		// note: src may be *this
		RemoveLinks *ref=src.GetRef();
		DoDestruct();
		DoConstruct(ref);
		return *this;
	}

	__forceinline bool NotNull() const {return _ref!=NULL;}
	__forceinline bool IsNull() const {return _ref==NULL;}
	//bool operator ! () const {return _ref==NULL;}
	
	__forceinline void Remove(){DoDestruct();}
	~BaseLink() {DoDestruct();}
};

class RemoveLinks: public RefCount, public SList<BaseLink>
{
	#if DO_LINK_DIAGS
		friend class BaseLink;
		int _nLinks;
	#endif

	public:
	RemoveLinks()
	{
		#if DO_LINK_DIAGS
			_nLinks=0;
		#endif
	}

	// on copy of linked object do not copy links
	RemoveLinks( const RemoveLinks &src )
	{
		#if DO_LINK_DIAGS
			_nLinks=0;
		#endif
	}
	void operator = ( const RemoveLinks &src )
	{
		#if DO_LINK_DIAGS
			_nLinks=0;
		#endif
		SLink::Reset();
	}

  /// set all existing links to NULL
  void DestroyLinkChain() const
  {
	  while(BaseLink *link=First())
	  {
		  link->Remove();
	  }
  }
  /// check no links exist - useful for asserting in caching scenarios
  bool CheckNoLinks() const {return First()==NULL;}
	~RemoveLinks();

	#if DO_LINK_DIAGS
		bool VerifyStructure();
	#endif
};

typedef RemoveLinks RefCountWithLinks;


inline RemoveLinks::~RemoveLinks()
{ // set all existing references to NULL
	#if DO_LINK_DIAGS
		Assert( VerifyStructure() );
	#endif
	DestroyLinkChain();
	#if DO_LINK_DIAGS
		Assert( _nLinks==0 );
	#endif
}	

#if DO_LINK_DIAGS
	inline bool RemoveLinks::VerifyStructure()
	{
		BaseLink *link=First();
		if( !link )
		{
			return( _nLinks==0 );
		}
		for( int i=0; i<_nLinks; i++ )
		{
			//Assert( link->next );
			//Assert( link->prev );
			//Assert( link->next->prev==link );
			link=Next(link);
			if( !link )
			{
				return ( _nLinks==i+1 );
			}
		}
		return false;
	}
#endif

inline void BaseLink::DoConstruct( RemoveLinks *ref )
{
	_ref=ref;
	if( _ref )
	{
		#if DO_LINK_DIAGS
			Assert( _ref->VerifyStructure() );
		#endif
		_ref->RemoveLinks::Insert(this);
		#if DO_LINK_DIAGS
			_ref->_nLinks++;
		#endif
	}
}
inline void BaseLink::DoDestruct()
{
	if( _ref )
	{
		#if DO_LINK_DIAGS
			Assert( _ref->VerifyStructure() );
		#endif
		_ref->Delete(this);
		#if DO_LINK_DIAGS
			_ref->_nLinks--;
		#endif
	}
	_ref=NULL;
}

__forceinline RemoveLinks *BaseLink::GetRef() const {return _ref;}

template <class Type>
class Link: public BaseLink
{
	#if _DEBUG
		Type *_refT; // this enable looking at variable

		public:
		Link():_refT(NULL){}
		Link( Type *src ):BaseLink(src),_refT(src){}
		Link( const Ref<Type> &src ):BaseLink(src.GetRef()),_refT(src.GetRef()){}
		Link( const Link &src ):BaseLink(src),_refT(src._refT){}

	#else
		public:
		Link(){}
		Link( const Ref<Type> &src ):BaseLink(src.GetRef()){}
		Link( Type *src ):BaseLink(src){}
		Link( const Link &src ):BaseLink(src){}
	#endif
	
  //! make data type accessible
  typedef Type ItemType;

	__forceinline Type *GetTypeRef() const {return static_cast<Type *>(GetRef());}
	__forceinline operator Type *() const {return GetTypeRef();}
	__forceinline Type &operator *() const {return *GetTypeRef();}
	__forceinline Type *operator ->() const {return GetTypeRef();}

	ClassIsGeneric(Link);
};

template <class Type>
struct LinkArrayKeyTraits
{
	typedef const Type *KeyType;
	static bool IsEqual(KeyType a, KeyType b)
	{
		return a==b;
	}
	static KeyType GetKey(const Link<Type> &a) {return a.GetTypeRef();}
};

//! array of Link-s

template <class Type,class Allocator=MemAllocD>
class LinkArray: public FindArrayKey< Link<Type>, LinkArrayKeyTraits<Type>, Allocator >
{
	typedef FindArrayKey< Link<Type>, LinkArrayKeyTraits<Type>, Allocator > base;
	typedef LinkArrayKeyTraits<Type> traits;

	public:
	int Count() const;
	void RemoveNulls();

	//! delete at given location
	__forceinline void Delete( int index ) {base::DeleteAt(index);}
	//! delete, given pointer
	__forceinline bool Delete(const Type *src) {return base::DeleteKey(src);}
	//! delete, given Link
	__forceinline bool Delete(const Link<Type> &src) const {return base::DeleteKey(traits::GetKey(src));}

	//! if link is provided, get a key from it
	__forceinline int Find(const Link<Type> &src) const {return base::FindKey(traits::GetKey(src));}
	//! if pointer is provided to search, avoid temporary Link creation
	__forceinline int Find(const Type *src) const {return base::FindKey(src);}

  /// add, reuse any NULL slots if possible
  int AddReusingNull(Type *item)
  {
    int free = base::FindKey(NULL);
    if (free>=0)
    {
      base::Set(free) = item;
      return free;
    }
    else
    {
      return base::Add(item);
    }
  }

	ClassIsMovable(LinkArray)
};

template <class Type,class Allocator>
int LinkArray<Type,Allocator>::Count() const
{
	int i, n = 0;
	for (i=0; i<base::Size(); i++)
  {
		if (base::Get(i)) n++;
  }
	return n;
}

template <class Type,class Allocator>
void LinkArray<Type,Allocator>::RemoveNulls()
{
	int d=0;
	for( int s=0; s<base::Size(); s++ )
	{
		if( base::Get(s) )
		{
			if( s!=d ) base::Set(d)=base::Get(s);
			d++;
		}
	}
	base::Resize(d);
}

#endif

