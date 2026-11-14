#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
/*!
\file 
Container traversal templates.
*/

#ifndef _ES_CONTAINERS_FOR_EACH_HPP
#define _ES_CONTAINERS_FOR_EACH_HPP

///////////////////////////////////////////////////////////////////////////////
//
// Predicates, adapters, functors
//
///////////////////////////////////////////////////////////////////////////////

//! Predicate True
template <class Type>
struct PredTrue
{
	typedef Type Argument;

	PredTrue() {}
	bool operator()(const Argument &arg) const {return true;}
};

//! Predicate False
template <class Type>
struct PredFalse
{
	typedef Type Argument;

	PredFalse() {}
	bool operator()(const Argument &arg) const {return false;}
};

//! Predicate Not
template <class Pred>
struct PredNot
{
	typedef typename Pred::Argument Argument;
	Pred pred;

	PredNot(Pred p) : pred(p) {}
	bool operator()(const Argument &arg) const {return !pred(arg);}
};

//! Creating function for predicate Not
template <class Pred>
PredNot<Pred> PNot(Pred p) {return PredNot<Pred>(p);}

//! Predicate And
template <class Pred1, class Pred2>
struct PredAnd
{
	typedef typename Pred1::Argument Argument;
	Pred1 pred1;
	Pred2 pred2;

	PredAnd(Pred1 p1, Pred2 p2) : pred1(p1), pred2(p2) {}
	bool operator()(const Argument &arg) const {return pred1(arg) && pred2(arg);}
};

//! Creating function for predicate And
template <class Pred1, class Pred2>
PredAnd<Pred1, Pred2> PAnd(Pred1 p1, Pred2 p2) {return PredAnd<Pred1, Pred2>(p1, p2);}

//! Predicate Or
template <class Pred1, class Pred2>
struct PredOr
{
	typedef typename Pred1::Argument Argument;
	Pred1 pred1;
	Pred2 pred2;

	PredOr(Pred1 p1, Pred2 p2) : pred1(p1), pred2(p2) {}
	bool operator()(const Argument &arg) const {return pred1(arg) || pred2(arg);}
};

//! Creating function for predicate Or
template <class Pred1, class Pred2>
PredOr<Pred1, Pred2> POr(Pred1 p1, Pred2 p2) {return PredOr<Pred1, Pred2>(p1, p2);}

//! Adapter for transform function (with single argument) into functor
template <class TypeArg, class TypeRes>
struct AdapterFunc1
{
	typedef TypeArg Argument;
	typedef TypeRes Result;
	typedef Result Function(Argument);

	Function *func;

	AdapterFunc1(Function *f) : func(f) {}
	Result operator()(Argument arg) const {return func(arg);}
};

//! Creating function for adapter
template <class TypeArg, class TypeRes>
AdapterFunc1<TypeArg, TypeRes> AFunc1(TypeRes (*func)(TypeArg)) {return AdapterFunc1<TypeArg, TypeRes>(func);}

//! Adapter for transform binary function into functor, fixing second argument
template <class TypeArg1, class TypeArg2, class TypeRes>
struct AdapterFix2Func2
{
	typedef TypeArg1 Argument;
	typedef TypeRes Result;
// VC.NET: typedef Result Function(const Argument &, TypeArg2);
	typedef Result Function(Argument, TypeArg2);

	Function *func;
	TypeArg2 arg2;

	AdapterFix2Func2(Function *f, TypeArg2 a2) : func(f), arg2(a2) {}
// VC.NET: Result operator()(const Argument &arg) const {return func(arg, arg2);}
	 Result operator()(Argument arg) const {return func(arg, arg2);}
};

//! Creating function for adapter
template <class TypeArg1, class TypeArg2, class TypeRes>
// VC.NET: AdapterFix2Func2<TypeArg1, TypeArg2, TypeRes> AFix2Func2(TypeRes (*func)(const TypeArg1 &, TypeArg2), TypeArg2 arg2)
AdapterFix2Func2<TypeArg1, TypeArg2, TypeRes> AFix2Func2(TypeRes (*func)(TypeArg1, TypeArg2), TypeArg2 arg2)
{return AdapterFix2Func2<TypeArg1, TypeArg2, TypeRes>(func, arg2);}

///////////////////////////////////////////////////////////////////////////////
//
// Some predefined traits for ForEach
//
///////////////////////////////////////////////////////////////////////////////

//! traits that enable traversing lists or other containters with Start,Advance,NotEnd on items
template <class Container>
struct ContainerTraits
{
	typedef typename Container::ItemType *Iterator;
	typedef typename Container::ItemType ItemType;

	static Iterator Start(const Container &c) {return c.Start();}
	static void Advance(const Container &c, Iterator &it) {it = c.Advance(it);}
	static bool NotEnd(const Container &c, Iterator it) {return c.NotEnd(it);}

	static ItemType &Select(Container &c, Iterator it)
	{
		return *it;
	}
	static const ItemType &Select(const Container &c, Iterator it)
	{
		return *it;
	}
};

//! traits that enable traversing arrays or other containters with Size(), Get(int i), Set(int i)
template <class Container>
struct ArrayTraits
{
	typedef int Iterator;
	typedef typename Container::DataType ItemType;

	static Iterator Start(const Container &c) {return 0;}
	static void Advance(const Container &c, Iterator &it) {it++;}
	static bool NotEnd(const Container &c, Iterator it) {return it<c.Size();}

	static ItemType &Select(Container &c, Iterator it)
	{
		return c.Set(it);
	}
	static const ItemType &Select(const Container &c, Iterator it)
	{
		return c.Get(it);
	}
};

///////////////////////////////////////////////////////////////////////////////
//
// ForEach itself
//
///////////////////////////////////////////////////////////////////////////////

//! traverse any container using provided traits
/*!
when Function returns true, finish traversing
*/
template
<
	class Traits, class Container, class Function, class Context
>
bool ForEach(Container &container, Function function, Context &context)
{
	for
	(
		typename Traits::Iterator it = Traits::Start(container);
		Traits::NotEnd(container,it);
		Traits::Advance(container,it)
	)
	{
		bool ret = function(Traits::Select(container,it),context);
		if (ret) return ret;
	}
	return false;
}

//! traverse any container using provided traits, using predicate for filtering
/*!
when Function returns true, finish traversing
*/
template
<
	class Traits, class Container, class Pred, class Function, class Context
>
bool ForSome(Container &container, Pred pred, Function function, Context &context)
{
	for
	(
		typename Traits::Iterator it = Traits::Start(container);
		Traits::NotEnd(container,it);
		Traits::Advance(container,it)
	)
	{
		if (pred(Traits::Select(container,it)))
		{
			bool ret = function(Traits::Select(container,it),context);
			if (ret) return ret;
		}
	}
	return false;
}

//! traverse array-type container
/*!
when Function returns true, finish traversing
*/
template
<
	class Container, class Function, class Context
>
bool ForEachA(Container &con, Function fc, Context &ctx)
{
	return ForEach<ArrayTraits<Container> >(con,fc,ctx);
}

//! traverse linked list-type container
/*!
when Function returns true, finish traversing
*/
template
<
	class Container, class Function, class Context
>
bool ForEachC(Container &con, Function fc, Context &ctx)
{
	return ForEach<ContainerTraits<Container> >(con,fc,ctx);
}

//! adapter for passinf context in functor value

template <class Type,class Function,class Context>
class FuncWithContext
{
	Context &ctx;
	Function fc;
	public:
	FuncWithContext(Context &c, Function f):ctx(c), fc(f) {}
	Type operator () ()
	{
		return fc(ctx);
	}
};

/// adapter for calling a class member function
template <class Type, class Arg>
class CallMemberAdapt
{
  Type *_obj;
  
  typedef bool (Type::*MemberFunction)(Arg arg) const;
  
  MemberFunction _member;
  
  public:
  CallMemberAdapt(Type *obj, MemberFunction member):_obj(obj),_member(member)
  {
  }
  __forceinline bool operator () (Arg arg) const
  {
    return (_obj->*_member)(arg);
  }
};

/// creative function for CallMemberAdapt
template <class Type, class Arg>
CallMemberAdapt<Type,Arg> CallMember(Type *obj, bool (Type::*func)(Arg arg) const)
{
  return CallMemberAdapt<Type,Arg>(obj,func);
}

/// adapter for calling a class member function with no return value
template <class Type, class Arg>
class CallVoidMemberAdapt
{
  Type *_obj;
  
  typedef void (Type::*MemberFunction)(Arg arg);
  
  MemberFunction _member;
  
  public:
  CallVoidMemberAdapt(Type *obj, MemberFunction member):_obj(obj),_member(member)
  {
  }
  __forceinline bool operator () (Arg arg) const
  {
    (_obj->*_member)(arg);
    return false;
  }
};

/// creative function for CallVoidMemberAdapt
template <class Type, class Arg>
CallVoidMemberAdapt<Type,Arg> CallVoidMember(Type type, void (Type::*func)(Arg arg) const)
{
  return CallVoidMemberAdapt<Type,Arg>(type,func);
}

/// adapter for calling a member function of the iterated object
template <class Type>
class CallObjMemberAdapt
{
  typedef bool (Type::*MemberFunction)() const;
  
  MemberFunction _member;
  
  public:
  CallObjMemberAdapt(MemberFunction member):_member(member)
  {
  }
  __forceinline bool operator () (Type *obj) const
  {
    return (obj->*_member)();
  }
};

/// creative function for CallObjMemberAdapt
template <class Type>
CallObjMemberAdapt<Type> CallObjMember(bool (Type::*func)() const)
{
  return CallObjMemberAdapt<Type>(func);
}

/// adapter for calling a member function of the iterated object with no return value
template <class Type>
class CallVoidObjMemberAdapt
{
  typedef void (Type::*MemberFunction)() const;
  
  MemberFunction _member;
  
  public:
  CallVoidObjMemberAdapt(MemberFunction member):_member(member)
  {
  }
  __forceinline bool operator () (Type *obj) const
  {
    (obj->*_member)();
    return false;
  }
};

/// adapter for calling a member function of the iterated object with no return value
template <class Type>
class CallVoidObjMemberAdaptNC
{
  typedef void (Type::*MemberFunction)();
  
  MemberFunction _member;
  
  public:
  CallVoidObjMemberAdaptNC(MemberFunction member):_member(member)
  {
  }
  __forceinline bool operator () (Type *obj) const
  {
    (obj->*_member)();
    return false;
  }
};

/// creative function for CallVoidObjMemberAdapt
template <class Type>
CallVoidObjMemberAdapt<Type> CallVoidObjMember(void (Type::*func)() const)
{
  return CallVoidObjMemberAdapt<Type>(func);
}

/// creative function for CallVoidObjMemberAdapt
template <class Type>
CallVoidObjMemberAdaptNC<Type> CallVoidObjMember(void (Type::*func)())
{
  return CallVoidObjMemberAdaptNC<Type>(func);
}


/*-----
Examples:

AutoArray<int> array;
array.Add(1);
array.Add(3);
array.Add(2);
array.Add(4);
ForEachA(array,Print);

CLRefList<LinkedInt> list;
list.Insert(new LinkedInt(1));
list.Insert(new LinkedInt(3));
list.Insert(new LinkedInt(2));
list.Insert(new LinkedInt(4));
ForEachC(list,PrintLinkedInt);

*/

#endif
