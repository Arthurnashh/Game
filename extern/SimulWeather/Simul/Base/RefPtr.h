#ifndef SIMUL_REF_PTR_H
#define SIMUL_REF_PTR_H

#include <boost/config.hpp>   // for broken compiler workarounds

#include <boost/checked_delete.hpp>








#include <boost/detail/lightweight_mutex.hpp>

#if defined(BOOST_SP_USE_QUICK_ALLOCATOR)
#include <boost/detail/quick_allocator.hpp>
#endif

#include <memory>           // std::auto_ptr, std::allocator
#include <functional>       // std::less
#include <exception>        // std::exception
#include <new>              // std::bad_alloc
#include <typeinfo>         // std::type_info in get_deleter
#include <cstddef>          // std::size_t

namespace simul
{
class bad_weak_ptr: public std::exception
{
public:

    virtual char const * what() const throw()
    {
        return "simul::bad_weak_ptr";
    }
};

namespace base
{

class sp_counted_base
{
private:

	typedef boost::detail::lightweight_mutex mutex_type;

public:

    sp_counted_base(): use_count_(1), weak_count_(1)
    {
    }

    virtual ~sp_counted_base() // nothrow
    {
    }

    // dispose() is called when use_count_ drops to zero, to release
    // the resources managed by *this.

    virtual void dispose() = 0; // nothrow

    // destruct() is called when weak_count_ drops to zero.

    virtual void destruct() // nothrow
    {
        delete this;
    }

    virtual void * get_deleter(std::type_info const & ti) = 0;

    void add_ref_copy()
    {
#if defined(BOOST_HAS_THREADS)
        mutex_type::scoped_lock lock(mtx_);
#endif
        ++use_count_;
    }

    void add_ref_lock()
    {
#if defined(BOOST_HAS_THREADS)
        mutex_type::scoped_lock lock(mtx_);
#endif
        if(use_count_ == 0) throw(simul::bad_weak_ptr());
        ++use_count_;
    }

    void release() // nothrow
    {
        {
#if defined(BOOST_HAS_THREADS)
            mutex_type::scoped_lock lock(mtx_);
#endif
            long new_use_count = --use_count_;

            if(new_use_count != 0) return;
        }

        dispose();
        weak_release();
    }

    void weak_add_ref() // nothrow
    {
#if defined(BOOST_HAS_THREADS)
        mutex_type::scoped_lock lock(mtx_);
#endif
        ++weak_count_;
    }

    void weak_release() // nothrow
    {
        long new_weak_count;

        {
#if defined(BOOST_HAS_THREADS)
            mutex_type::scoped_lock lock(mtx_);
#endif
            new_weak_count = --weak_count_;
        }

        if(new_weak_count == 0)
        {
            destruct();
        }
    }

    long use_count() const // nothrow
    {
#if defined(BOOST_HAS_THREADS)
        mutex_type::scoped_lock lock(mtx_);
#endif
        return use_count_;
    }

private:

    sp_counted_base(sp_counted_base const &);
    sp_counted_base & operator= (sp_counted_base const &);

    long use_count_;        // #shared
    long weak_count_;       // #weak + (#shared != 0)

#if defined(BOOST_HAS_THREADS) || defined(BOOST_LWM_WIN32)
    mutable mutex_type mtx_;
#endif
};

#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)

template<class T> void cbi_call_constructor_hook(sp_counted_base * pn, T * px, checked_deleter<T> const &, int)
{
    boost::sp_scalar_constructor_hook(px, sizeof(T), pn);
}

template<class T> void cbi_call_constructor_hook(sp_counted_base *, T * px, checked_array_deleter<T> const &, int)
{
    boost::sp_array_constructor_hook(px);
}

template<class P, class D> void cbi_call_constructor_hook(sp_counted_base *, P const &, D const &, long)
{
}

template<class T> void cbi_call_destructor_hook(sp_counted_base * pn, T * px, checked_deleter<T> const &, int)
{
    boost::sp_scalar_destructor_hook(px, sizeof(T), pn);
}

template<class T> void cbi_call_destructor_hook(sp_counted_base *, T * px, checked_array_deleter<T> const &, int)
{
    boost::sp_array_destructor_hook(px);
}

template<class P, class D> void cbi_call_destructor_hook(sp_counted_base *, P const &, D const &, long)
{
}

#endif

//
// Borland's Codeguard trips up over the -Vx- option here:
//
#ifdef __CODEGUARD__
# pragma option push -Vx-
#endif

template<class P, class D> class sp_counted_base_impl: public sp_counted_base
{
private:

    P ptr; // copy constructor must not throw
    D del; // copy constructor must not throw

    sp_counted_base_impl(sp_counted_base_impl const &);
    sp_counted_base_impl & operator= (sp_counted_base_impl const &);

    typedef sp_counted_base_impl<P, D> this_type;

public:

    // pre: initial_use_count <= initial_weak_count, d(p) must not throw

    sp_counted_base_impl(P p, D d): ptr(p), del(d)
    {
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        boost::detail::cbi_call_constructor_hook(this, p, d, 0);
#endif
    }

    virtual void dispose() // nothrow
    {
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        boost::detail::cbi_call_destructor_hook(this, ptr, del, 0);
#endif
        del(ptr);
    }

    virtual void * get_deleter(std::type_info const & ti)
    {
        return ti == typeid(D)? &del: 0;
    }

#if defined(BOOST_SP_USE_STD_ALLOCATOR)

    void * operator new(std::size_t)
    {
        return std::allocator<this_type>().allocate(1, static_cast<this_type *>(0));
    }

    void operator delete(void * p)
    {
        std::allocator<this_type>().deallocate(static_cast<this_type *>(p), 1);
    }

#endif

#if defined(BOOST_SP_USE_QUICK_ALLOCATOR)

    void * operator new(std::size_t)
    {
        return quick_allocator<this_type>::alloc();
    }

    void operator delete(void * p)
    {
        quick_allocator<this_type>::dealloc(p);
    }

#endif
};

#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)

int const shared_count_id = 0x2C35F101;
int const   weak_count_id = 0x298C38A4;

#endif

class weak_count;

class shared_count
{
private:

    sp_counted_base * pi_;

#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
    int id_;
#endif

    friend class weak_count;

public:

    shared_count(): pi_(0) // nothrow
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(shared_count_id)
#endif
    {
    }

    template<class P, class D> shared_count(P p, D d): pi_(0)
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(shared_count_id)
#endif
    {
#ifndef BOOST_NO_EXCEPTIONS

        try
        {
            pi_ = new sp_counted_base_impl<P, D>(p, d);
        }
        catch(...)
        {
            d(p); // delete p
            throw;
        }

#else

        pi_ = new sp_counted_base_impl<P, D>(p, d);

        if(pi_ == 0)
        {
            d(p); // delete p
            boost::throw_exception(std::bad_alloc());
        }

#endif
    }

#ifndef BOOST_NO_AUTO_PTR

    // auto_ptr<Y> is special cased to provide the strong guarantee

    template<class Y>
    explicit shared_count(std::auto_ptr<Y> & r): pi_(new sp_counted_base_impl< Y *, checked_deleter<Y> >(r.get(), checked_deleter<Y>()))
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(shared_count_id)
#endif
    {
        r.release();
    }

#endif 

    ~shared_count() // nothrow
    {
        if(pi_ != 0) pi_->release();
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        id_ = 0;
#endif
    }

    shared_count(shared_count const & r): pi_(r.pi_) // nothrow
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(shared_count_id)
#endif
    {
        if(pi_ != 0) pi_->add_ref_copy();
    }

    explicit shared_count(weak_count const & r); // throws bad_weak_ptr when r.use_count() == 0

    shared_count & operator= (shared_count const & r) // nothrow
    {
        sp_counted_base * tmp = r.pi_;

        if(tmp != pi_)
        {
            if(tmp != 0) tmp->add_ref_copy();
            if(pi_ != 0) pi_->release();
            pi_ = tmp;
        }

        return *this;
    }

    void swap(shared_count & r) // nothrow
    {
        sp_counted_base * tmp = r.pi_;
        r.pi_ = pi_;
        pi_ = tmp;
    }

    long use_count() const // nothrow
    {
        return pi_ != 0? pi_->use_count(): 0;
    }

    bool unique() const // nothrow
    {
        return use_count() == 1;
    }

    friend inline bool operator==(shared_count const & a, shared_count const & b)
    {
        return a.pi_ == b.pi_;
    }

    friend inline bool operator<(shared_count const & a, shared_count const & b)
    {
        return std::less<sp_counted_base *>()(a.pi_, b.pi_);
    }

    void * get_deleter(std::type_info const & ti) const
    {
        return pi_? pi_->get_deleter(ti): 0;
    }
};

#ifdef __CODEGUARD__
# pragma option pop
#endif


class weak_count
{
private:

    sp_counted_base * pi_;

#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
    int id_;
#endif

    friend class shared_count;

public:

    weak_count(): pi_(0) // nothrow
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(weak_count_id)
#endif
    {
    }

    weak_count(shared_count const & r): pi_(r.pi_) // nothrow
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(shared_count_id)
#endif
    {
        if(pi_ != 0) pi_->weak_add_ref();
    }

    weak_count(weak_count const & r): pi_(r.pi_) // nothrow
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(shared_count_id)
#endif
    {
        if(pi_ != 0) pi_->weak_add_ref();
    }

    ~weak_count() // nothrow
    {
        if(pi_ != 0) pi_->weak_release();
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        id_ = 0;
#endif
    }

    weak_count & operator= (shared_count const & r) // nothrow
    {
        sp_counted_base * tmp = r.pi_;
        if(tmp != 0) tmp->weak_add_ref();
        if(pi_ != 0) pi_->weak_release();
        pi_ = tmp;

        return *this;
    }

    weak_count & operator= (weak_count const & r) // nothrow
    {
        sp_counted_base * tmp = r.pi_;
        if(tmp != 0) tmp->weak_add_ref();
        if(pi_ != 0) pi_->weak_release();
        pi_ = tmp;

        return *this;
    }

    void swap(weak_count & r) // nothrow
    {
        sp_counted_base * tmp = r.pi_;
        r.pi_ = pi_;
        pi_ = tmp;
    }

    long use_count() const // nothrow
    {
        return pi_ != 0? pi_->use_count(): 0;
    }

    friend inline bool operator==(weak_count const & a, weak_count const & b)
    {
        return a.pi_ == b.pi_;
    }

    friend inline bool operator<(weak_count const & a, weak_count const & b)
    {
        return std::less<sp_counted_base *>()(a.pi_, b.pi_);
    }
};

inline shared_count::shared_count(weak_count const & r): pi_(r.pi_)
#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS)
        , id_(shared_count_id)
#endif
{
    if(pi_ != 0)
    {
        pi_->add_ref_lock();
    }
    else
    {
        throw(simul::bad_weak_ptr());
    }
}

} // namespace detail

} // namespace boost

 // #ifndef BOOST_DETAIL_SHARED_COUNT_HPP_INCLUDED











#include <memory>               // for std::auto_ptr
#include <algorithm>            // for std::swap
#include <functional>           // for std::less
#include <assert.h>   

# pragma warning(push)
# pragma warning(disable:4284) // odd return type for operator->

namespace simul
{
	namespace base
	{
		template<class T> class weak_ptr;
		template<class T> class enable_shared_from_this;

		struct static_cast_tag {};
		struct const_cast_tag {};
		struct dynamic_cast_tag {};
		struct polymorphic_cast_tag {};

		template<class T> struct shared_ptr_traits
		{
			typedef T & reference;
		};

		template<> struct shared_ptr_traits<void>
		{
			typedef void reference;
		};
		template<> struct shared_ptr_traits<void const>
		{
			typedef void reference;
		};

		template<> struct shared_ptr_traits<void volatile>
		{
			typedef void reference;
		};

		template<> struct shared_ptr_traits<void const volatile>
		{
			typedef void reference;
		};

// enable_shared_from_this support

		template<class T, class Y> void sp_enable_shared_from_this( simul::base::shared_count const & pn, enable_shared_from_this<T> const * pe, Y const * px )
		{
			if(pe != 0) pe->_internal_weak_this._internal_assign(const_cast<Y*>(px), pn);
		}

		inline void sp_enable_shared_from_this( simul::base::shared_count const & /*pn*/, ... )
		{
		}


//  RefPtr
//
//  A smart pointer with reference counted copy semantics.
//  The object pointed to is deleted when the last RefPtr pointing to it
//  is destroyed or reset.
//

template<class T> class RefPtr
{
private:

    typedef RefPtr<T> this_type;

public:

    typedef T element_type;
    typedef T value_type;
    typedef T * pointer;
	typedef typename shared_ptr_traits<T>::reference reference;

    RefPtr(): px(0), pn() // never throws in 1.30+
    {
    }

    template<class Y>
	explicit RefPtr(Y * p): px(p), pn(p,boost::checked_deleter<Y>()) // Y must be complete
    {
        sp_enable_shared_from_this( pn, p, p );
    }

    //
    // Requirements: D's copy constructor must not throw
    //
    // RefPtr will release p by calling d(p)
    //

    template<class Y, class D> RefPtr(Y * p, D d): px(p), pn(p, d)
    {
        sp_enable_shared_from_this( pn, p, p );
    }

//  generated copy constructor, assignment, destructor are fine.

    template<class Y>
    explicit RefPtr(weak_ptr<Y> const & r): pn(r.pn) // may throw
    {
        // it is now safe to copy r.px, as pn(r.pn) did not throw
        px = r.px;
    }

    template<class Y>
    RefPtr(RefPtr<Y> const & r): px(r.px), pn(r.pn) // never throws
    {
    }

    template<class Y>
    RefPtr(RefPtr<Y> const & r, static_cast_tag): px(static_cast<element_type *>(r.px)), pn(r.pn)
    {
    }

    template<class Y>
    RefPtr(RefPtr<Y> const & r, const_cast_tag): px(const_cast<element_type *>(r.px)), pn(r.pn)
    {
    }

    template<class Y>
    RefPtr(RefPtr<Y> const & r, dynamic_cast_tag): px(dynamic_cast<element_type *>(r.px)), pn(r.pn)
    {
        if(px == 0) // need to allocate new counter -- the cast failed
        {
            pn = simul::base::shared_count();
        }
    }

    template<class Y>
    RefPtr(RefPtr<Y> const & r, polymorphic_cast_tag): px(dynamic_cast<element_type *>(r.px)), pn(r.pn)
    {
        if(px == 0)
        {
            boost::throw_exception(std::bad_cast());
        }
    }
    template<class Y>
    RefPtr & operator=(RefPtr<Y> const & r) // never throws
    {
        px = r.px;
        pn = r.pn; // simul::base::shared_count::op= doesn't throw
        return *this;
    }

    void reset() // never throws in 1.30+
    {
        this_type().swap(*this);
    }

    template<class Y> void reset(Y * p) // Y must be complete
    {
        assert(p == 0 || p != px); // catch self-reset errors
        this_type(p).swap(*this);
    }

    template<class Y, class D> void reset(Y * p, D d)
    {
        this_type(p, d).swap(*this);
    }

    reference operator* () const // never throws
    {
        assert(px != 0);
        return *px;
    }

    T * operator-> () const // never throws
    {
        assert(px != 0);
        return px;
    }
    
    T * get() const // never throws
    {
        return px;
    }
    // implicit conversion to "bool"

    typedef T * this_type::*unspecified_bool_type;

    operator unspecified_bool_type() const // never throws
    {
        return px == 0? 0: &this_type::px;
    }

    // operator! is redundant, but some compilers need it

    bool operator! () const // never throws
    {
        return px == 0;
    }

    bool unique() const // never throws
    {
        return pn.unique();
    }

    long use_count() const // never throws
    {
        return pn.use_count();
    }

    void swap(RefPtr<T> & other) // never throws
    {
        std::swap(px, other.px);
        pn.swap(other.pn);
    }

    template<class Y> bool _internal_less(RefPtr<Y> const & rhs) const
    {
        return pn < rhs.pn;
    }

    void * _internal_get_deleter(std::type_info const & ti) const
    {
        return pn.get_deleter(ti);
    }

// Tasteless as this may seem, making all members public allows member templates
// to work in the absence of member template friends. (Matthew Langston)

private:

    template<class Y> friend class RefPtr;
    template<class Y> friend class weak_ptr;

    T * px;                     // contained pointer
    simul::base::shared_count pn;    // reference counter

};  // RefPtr

template<class T, class U> inline bool operator==(RefPtr<T> const & a, RefPtr<U> const & b)
{
    return a.get() == b.get();
}

template<class T, class U> inline bool operator!=(RefPtr<T> const & a, RefPtr<U> const & b)
{
    return a.get() != b.get();
}

template<class T, class U> inline bool operator<(RefPtr<T> const & a, RefPtr<U> const & b)
{
    return a._internal_less(b);
}

template<class T> inline void swap(RefPtr<T> & a, RefPtr<T> & b)
{
    a.swap(b);
}

template<class T, class U> RefPtr<T> static_pointer_cast(RefPtr<U> const & r)
{
    return RefPtr<T>(r, detail::static_cast_tag());
}

template<class T, class U> RefPtr<T> const_pointer_cast(RefPtr<U> const & r)
{
    return RefPtr<T>(r, detail::const_cast_tag());
}

template<class T, class U> RefPtr<T> dynamic_pointer_cast(RefPtr<U> const & r)
{
    return RefPtr<T>(r, detail::dynamic_cast_tag());
}

// shared_*_cast names are deprecated. Use *_pointer_cast instead.

template<class T, class U> RefPtr<T> shared_static_cast(RefPtr<U> const & r)
{
    return RefPtr<T>(r, detail::static_cast_tag());
}

template<class T, class U> RefPtr<T> shared_dynamic_cast(RefPtr<U> const & r)
{
    return RefPtr<T>(r, detail::dynamic_cast_tag());
}

template<class T, class U> RefPtr<T> shared_polymorphic_cast(RefPtr<U> const & r)
{
    return RefPtr<T>(r, detail::polymorphic_cast_tag());
}

template<class T, class U> RefPtr<T> shared_polymorphic_downcast(RefPtr<U> const & r)
{
    assert(dynamic_cast<T *>(r.get()) == r.get());
    return shared_static_cast<T>(r);
}

// get_pointer() enables boost::mem_fn to recognize RefPtr

template<class T> inline T * get_pointer(RefPtr<T> const & p)
{
    return p.get();
}

// operator<<


template<class E, class T, class Y> std::basic_ostream<E, T> & operator<< (std::basic_ostream<E, T> & os, RefPtr<Y> const & p)
{
    os << p.get();
    return os;
}

// get_deleter (experimental)


template<class D, class T> D * get_deleter(RefPtr<T> const & p)
{
    return static_cast<D *>(p._internal_get_deleter(typeid(D)));
}

} // namespace boost
}

# pragma warning(pop)
#endif  // #ifndef SIMUL_REF_PTR_H
