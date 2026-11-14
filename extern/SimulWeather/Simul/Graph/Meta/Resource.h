#ifndef SIMUL_META_RESOURCE_H
#define SIMUL_META_RESOURCE_H

#include "Simul/Graph/Meta/NodeVisitor.h"
#include "Simul/Graph/Meta/NodeStack.h"
#include <map>
#include <assert.h>

namespace simul
{
	namespace graph
	{
		namespace meta
		{
			//! A resource user is a base for classes that use a particular kind of resource, specified by \param resourceInterfaceType.
			template<class resourceInterfaceType> class ResourceUser
			{
				resourceInterfaceType *resource;
			public:
				ResourceUser()
				{
					resource=NULL;
				}
				virtual ~ResourceUser(){}
				//! Get the resource interface class we're using.
				resourceInterfaceType *GetResourceInterface()
				{
					return resource;
				}
				//! Get the resource interface class we're using.
				const resourceInterfaceType *GetResourceInterface() const
				{
					return resource;
				}
				//! Set the resource interface class to use.
				virtual void SetResourceInterface(resourceInterfaceType *res)
				{
					resource=res;
				}
			};
			//! A template for a class (that should also inherit simul::graph::meta::Group) that descendant nodes can use as a resource.

			//! Resource is a node that looks through its children for nodes that are ResourceUsers of the correct type
			//! It stores a map of them.
			//! For as long as the resource is connected, resource users can call ResourceUser<resourceType>::GetResource() to
			//! obtain the resource's interface.
			template<class resourceInterfaceType> class Resource
			{
			protected:
				struct DataTypeWrapper
				{
					unsigned count;
				};
			public:
				virtual void test(){}
				typedef std::map<simul::graph::meta::Node*,DataTypeWrapper* > selection_map;
				selection_map selectionMap;

				//! Inform this parent that a child has been added, and recurse up the graph
				//! In this case, we call InitNode
				void ChildAdded(simul::graph::meta::Node *node,resourceInterfaceType *res)
				{
					ResourceUser<resourceInterfaceType> *user=dynamic_cast<ResourceUser<resourceInterfaceType>*>(node);
					if(!user)
						return;
					//if(!si->ShouldIncludeNode(node,(dataType*)NULL))
					//	return;
					if(selectionMap.find(node)!=selectionMap.end())
					{
						selectionMap[node]->count++;
						return;
					}
					DataTypeWrapper *p=new DataTypeWrapper;
					p->count=1;
					selectionMap[node]=p;
					user->SetResourceInterface(res);
				}
				void ChildRemoved(simul::graph::meta::Node *node,simul::graph::meta::NodeStack &)
				{
					ResourceUser<resourceInterfaceType> *user=dynamic_cast<ResourceUser<resourceInterfaceType>*>(node);
					if(!user)
						return;
					typename selection_map::iterator i=selectionMap.find(node);
					if(i!=selectionMap.end())
					{
						i->second->count--;
						if(!i->second->count)
						{
							selectionMap.erase(i);
							user->SetResourceInterface(NULL);
						}
					}
				}
			};
			//! TrackerResource: a resource that also tracks its users with a class
			template<class resourceInterfaceType,class dataType> class TrackerResource
			{
			protected:
				struct DataTypeWrapper
				{
					dataType data;
					unsigned count;
				};
			public:
				typedef std::map<simul::graph::meta::Node*,DataTypeWrapper* > selection_map;
				selection_map selectionMap;

				//! Inform this parent that a child has been added, and recurse up the graph
				//! In this case, we call InitNode
				void ChildAdded(simul::graph::meta::Node *node,resourceInterfaceType *res)
				{
					ResourceUser<resourceInterfaceType> *user=dynamic_cast<ResourceUser<resourceInterfaceType>*>(node);
					if(!user)
						return;
					//if(!si->ShouldIncludeNode(node,(dataType*)NULL))
					//	return;
					if(selectionMap.find(node)!=selectionMap.end())
					{
						selectionMap[node]->count++;
						return;
					}
					DataTypeWrapper *p=new DataTypeWrapper;
					dataType *dt=&(p->data);			// Add it to the list
					p->count=1;
					selectionMap[node]=p;
					user->SetResourceInterface(res);
				}
				void ChildRemoved(simul::graph::meta::Node *node,simul::graph::meta::NodeStack &)
				{
					ResourceUser<resourceInterfaceType> *user=dynamic_cast<ResourceUser<resourceInterfaceType>*>(node);
					if(!user)
						return;
					typename selection_map::iterator i=selectionMap.find(node);
					if(i!=selectionMap.end())
					{
						i->second->count--;
						if(!i->second->count)
						{
							dataType *dt=&(i->second->data);
							selectionMap.erase(i);
							user->SetResourceInterface(NULL);
						}
					}
				}
			};

		}
	}
}
#endif
