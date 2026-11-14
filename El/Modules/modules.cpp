#include <El/elementpch.hpp>
#include "modules.hpp"

#pragma warning(disable:4073)
#pragma init_seg(lib)

//! Priority queue of registration items
class RegistrationList : public CLList<RegistrationItem>
{
public:
	//! Add item in priority queue
	void RegisterModule(RegistrationItem *item);
	//! Init all modules
	void InitModules();
	//! Done all modules
	void DoneModules();

	/*!
	\todo Move this support for reverse loop in CLList
	*/
	RegistrationItem *End() const
	{
		return static_cast<RegistrationItem *>(CLDLink::next);
	}
	static RegistrationItem *Backward(RegistrationItem *item)
	{
		return static_cast<RegistrationItem *>(item->CLDLink::next);
	}
	bool NotStart(RegistrationItem *item) const
	{
		return item != (CLDLink *)this;
	}

} GRegistrationList INIT_PRIORITY_HIGH;

void RegistrationList::RegisterModule(RegistrationItem *item)
{
	for (RegistrationItem *walk = Start(); NotEnd(walk); walk = Advance(walk))
	{
		if (item->level <= walk->level)
		{
			InsertBefore(walk, item);
			return;
		}
	}

	// no existing object has higher level
	// we have the highest and we need to be added at the list end
	Add(item);
}

void RegistrationList::InitModules()
{
	for (RegistrationItem *walk = Start(); NotEnd(walk); walk = Advance(walk))
	{
		if (walk->initFunction) walk->initFunction();
	}
}

void RegistrationList::DoneModules()
{
	for (RegistrationItem *walk = End(); NotStart(walk); walk = Backward(walk))
	{
		if (walk->initFunction) walk->initFunction();
	}
}

void RegisterModule(RegistrationItem *item)
{
	GRegistrationList.RegisterModule(item);
}

void InitModules()
{
	GRegistrationList.InitModules();
}

void DoneModules()
{
	GRegistrationList.DoneModules();
}
