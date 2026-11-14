#ifndef SIMUL_GRAPH_META_TRACKERMACROS_H
#define SIMUL_GRAPH_META_TRACKERMACROS_H

#include <iostream>
#include "Simul/Graph/Meta/Tracker.h"
#include "Simul/Graph/Meta/SingleTracker.h"
#include "Simul/Graph/Meta/PathTracker.h"
#include "Simul/Graph/Meta/Client.h"
//# pragma warning(push)
//# pragma warning(disable:4284) // odd return type for operator->
#if defined(DOXYGEN)|| defined(__GNUC__)
	#define BEGIN_TRACKERS
	#define META_ACCUMULATE_TRACKER_HANDLING(tracker_name)
	#define META_ACCUMULATE_PATH_TRACKER_HANDLING(tracker_name)
	#define DECLARE_TRACKER(owner_class,interface_class,tracker_class,tracker_name)
	#define DECLARE_CLIENT_TRACKER(owner_class,interface_class,client_interface,tracker_class,tracker_name)
	#define DECLARE_SINGLE_CLIENT_TRACKER(owner_class,interface_class,client_interface,tracker_name)
	#define DECLARE_TRACKER2(owner_class,interface_class,tracker_name)
	#define DECLARE_SIMPLE_TRACKER(owner_class,interface_class,tracker_name)
	#define DECLARE_SINGLE_TRACKER(owner_class,interface_class,tracker_name)
	#define DECLARE_PATH_TRACKER(owner_class,interface_class,tracker_class,tracker_name)
	#define DECLARE_GENERIC_TRACKER(owner_class)
	#define END_TRACKERS
	#define DEFAULT_TRACKER_HANDLING
	#define DEFAULT_TRACKER_TIMESTEP
#else

// we want to accumulate a function that saves each property.
#define BEGIN_TRACKERS																										\
	template<class T> bool ShouldIncludeNode(simul::graph::meta::Node *,T *){return true;}									\
	template<class T> void InitNode(simul::graph::meta::Node *,T *){}														\
	template<class T> void InitPathToNode(simul::graph::meta::Node *,T *,const simul::graph::meta::NodeStack &){}			\
	template<class T> void HandleNode(simul::graph::meta::Node *,T *){}														\
	template<class T> void RemovingNode(simul::graph::meta::Node *,T *){}													\
	template<class T> void ModifyingNode(simul::graph::meta::Node *,T *){}													\
	template<int N> void Templ_AccumulateChildAdded(simul::graph::meta::Node *,const simul::graph::meta::NodeStack &){}		\
	template<int N> void Templ_AccumulateChildRemoved(simul::graph::meta::Node *,const simul::graph::meta::NodeStack &){}	\
	template<int N> void Templ_AccumulateChildModified(simul::graph::meta::Node *){}	\
	template<int N> void Templ_AccumulateTimeStep(){}			


#define META_ACCUMULATE_TRACKER_HANDLING(tracker_name)										\
	template<> void Templ_AccumulateChildAdded<__COUNTER__>(simul::graph::meta::Node *n,const simul::graph::meta::NodeStack &path)	\
	{																						\
		Templ_AccumulateChildAdded<__COUNTER__-9>(n,path);									\
		tracker_name.ChildAdded(this,n);													\
	}																						\
	template<> void Templ_AccumulateChildRemoved<__COUNTER__>(simul::graph::meta::Node *n,const simul::graph::meta::NodeStack &path)	\
	{																						\
		Templ_AccumulateChildRemoved<__COUNTER__-9>(n,path);								\
		tracker_name.ChildRemoved(this,n);													\
	}																						\
	template<> void Templ_AccumulateChildModified<__COUNTER__>(simul::graph::meta::Node *n)	\
	{																						\
		Templ_AccumulateChildModified<__COUNTER__-9>(n);								\
		tracker_name.ChildModified(this,n);													\
	}																						\
	template<> void Templ_AccumulateTimeStep<__COUNTER__>()									\
	{																						\
		Templ_AccumulateTimeStep<__COUNTER__-9>();											\
		tracker_name.UpdateSelection(this);													\
	}

#define META_ACCUMULATE_PATH_TRACKER_HANDLING(tracker_name)									\
	template<> void Templ_AccumulateChildAdded<__COUNTER__>(simul::graph::meta::Node *n,const simul::graph::meta::NodeStack &path)	\
	{																						\
		Templ_AccumulateChildAdded<__COUNTER__-9>(n,path);									\
		tracker_name.ChildAdded(this,n,path);												\
	}																						\
	template<> void Templ_AccumulateChildRemoved<__COUNTER__>(simul::graph::meta::Node *n,const simul::graph::meta::NodeStack &path)	\
	{																						\
		Templ_AccumulateChildRemoved<__COUNTER__-9>(n,path);								\
		tracker_name.ChildRemoved(this,n,path);													\
	}																						\
	template<> void Templ_AccumulateChildModified<__COUNTER__>(simul::graph::meta::Node *n)	\
	{																						\
		Templ_AccumulateChildModified<__COUNTER__-9>(n);								\
		tracker_name.ChildModified(this,n);													\
	}																						\
	template<> void Templ_AccumulateTimeStep<__COUNTER__>()									\
	{																						\
		Templ_AccumulateTimeStep<__COUNTER__-9>();											\
		tracker_name.UpdateSelection(this);													\
	}

#define DECLARE_TRACKER(owner_class,interface_class,tracker_class,tracker_name)										\
	typedef Tracker<owner_class,interface_class,tracker_class> Type##tracker_name;									\
	Type##tracker_name tracker_name;																				\
	typedef Tracker<owner_class,interface_class,tracker_class>::selection_map::iterator iterator_##tracker_name;	\
	friend class Tracker<owner_class,interface_class,tracker_class>;												\
	META_ACCUMULATE_TRACKER_HANDLING(tracker_name)


#define DECLARE_CLIENT_TRACKER(owner_class,interface_class,client_interface,tracker_class,tracker_name)							\
	typedef simul::graph::meta::ClientTracker<owner_class,interface_class,client_interface,tracker_class> Type##tracker_name;	\
	Type##tracker_name tracker_name;																							\
	typedef simul::graph::meta::ClientTracker<owner_class,interface_class,client_interface,tracker_class>::selection_map::iterator iterator_##tracker_name;	\
	friend class simul::graph::meta::ClientTracker<owner_class,interface_class,client_interface,tracker_class>;					\
	META_ACCUMULATE_TRACKER_HANDLING(tracker_name)

#define DECLARE_SINGLE_CLIENT_TRACKER(owner_class,interface_class,client_interface,tracker_name)							\
	typedef simul::graph::meta::SingleClientTracker<owner_class,interface_class,client_interface> Type##tracker_name;	\
	Type##tracker_name tracker_name;																							\
	friend class simul::graph::meta::SingleClientTracker<owner_class,interface_class,client_interface>;					\
	META_ACCUMULATE_TRACKER_HANDLING(tracker_name)


#define DECLARE_TRACKER2(owner_class,interface_class,tracker_name)											\
	typedef SimpleTracker<owner_class,interface_class> Type##tracker_name;									\
	Type##tracker_name tracker_name;																		\
	typedef SimpleTracker<owner_class,interface_class>::selection_map::iterator iterator_##tracker_name;	\
	friend class SimpleTracker<owner_class,interface_class>;												\
	META_ACCUMULATE_TRACKER_HANDLING(tracker_name)

#define DECLARE_SIMPLE_TRACKER(owner_class,interface_class,tracker_name)									\
	typedef SimpleTracker<owner_class,interface_class> Type##tracker_name;									\
	Type##tracker_name tracker_name;																		\
	friend class SimpleTracker<owner_class,interface_class>;												\
	META_ACCUMULATE_TRACKER_HANDLING(tracker_name)

#define DECLARE_SINGLE_TRACKER(owner_class,interface_class,tracker_name)							\
	typedef simul::graph::meta::SingleTracker<owner_class,interface_class> Type##tracker_name;		\
	Type##tracker_name tracker_name;																\
	friend class simul::graph::meta::SingleTracker<owner_class,interface_class>;					\
	META_ACCUMULATE_TRACKER_HANDLING(tracker_name)

#define DECLARE_PATH_TRACKER(owner_class,interface_class,tracker_class,tracker_name)		\
	typedef PathTracker<owner_class,interface_class,tracker_class> Type##tracker_name;		\
	Type##tracker_name tracker_name;														\
	friend class PathTracker<owner_class,interface_class,tracker_class>;					\
	META_ACCUMULATE_PATH_TRACKER_HANDLING(tracker_name)

#define DECLARE_GENERIC_TRACKER(owner_class)																	\
	typedef simul::graph::meta::GenericTracker<owner_class> GenericTrackerType;									\
	GenericTrackerType generic_tracker;																			\
	typedef simul::graph::meta::GenericTracker<owner_class>::selection_map::iterator generic_tracker_iterator;	\
	friend class simul::graph::meta::GenericTracker<owner_class>;												\
	META_ACCUMULATE_TRACKER_HANDLING(generic_tracker)


#define END_TRACKERS															\
	void SelectorsChildAdded(simul::graph::meta::Node *n,simul::graph::meta::NodeStack &path)						\
	{																			\
		Templ_AccumulateChildAdded<__COUNTER__-8>(n,path);							\
	}																			\
	void SelectorsChildRemoved(simul::graph::meta::Node *n,simul::graph::meta::NodeStack &path)						\
	{																			\
		Templ_AccumulateChildRemoved<__COUNTER__-7>(n,path);							\
	}																			\
	void SelectorsChildModified(simul::graph::meta::Node *n)					\
	{																			\
		Templ_AccumulateChildModified<__COUNTER__-6>(n);						\
	}																			\
	void SelectorsUpdate()														\
	{																			\
		Templ_AccumulateTimeStep<__COUNTER__-5>();								\
	}


#define DEFAULT_TRACKER_HANDLING																\
	virtual void ChildAdded(simul::graph::meta::Node *node,simul::graph::meta::NodeStack &path)	\
	{																							\
		SelectorsChildAdded(node,path);															\
		simul::graph::meta::Group::ChildAdded(node,path);										\
	}																							\
	virtual void ChildRemoved(simul::graph::meta::Node *node,simul::graph::meta::NodeStack &path)									\
	{																							\
		SelectorsChildRemoved(node,path);														\
		simul::graph::meta::Group::ChildRemoved(node,path);											\
	}																							\
	virtual void ChildModified(simul::graph::meta::Node *node)									\
	{																							\
		SelectorsChildModified(node);														\
		simul::graph::meta::Group::ChildModified(node);											\
	}


#define DEFAULT_TRACKER_TIMESTEP																\
	virtual bool TimeStep(float dt)																\
	{																							\
		SelectorsUpdate();																		\
		Group::TimeStep(dt);																	\
	}

//# pragma warning(pop)
#endif
#endif
