#include "precomp.h"

#include "Referenced.h"
#include "ObserverPtr.h"
#include <set>


using namespace simul::base;
typedef std::set<Observer*> ObserverSet;

Referenced::Referenced():_refCount(0),observers(0)
{
}

void Referenced::AddObserver(Observer* observer)
{
    if (!observers)
		observers = new ObserverSet;
    if (observers)
		static_cast<ObserverSet*>(observers)->insert(observer);
}

void Referenced::RemoveObserver(Observer* observer)
{
    if (observers)
		static_cast<ObserverSet*>(observers)->erase(observer);
}

void Referenced::DeleteObservers()
{
	if(observers)
	{
		ObserverSet* os = static_cast<ObserverSet*>(observers);
		for(ObserverSet::iterator itr = os->begin();
			itr != os->end();
			++itr)
		{
			(*itr)->objectDeleted(this);
		}
		delete os;
		observers = 0;
	}
}