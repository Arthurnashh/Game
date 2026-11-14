#ifndef SIMUL_META_RESOURCE_GROUP_H
#define SIMUL_META_RESOURCE_GROUP_H
#include "Simul/Graph/Meta/Group.h"
#include "Simul/Graph/Meta/Resource.h"
#include <map>
#include <assert.h>

namespace simul
{
	namespace graph
	{
		namespace meta
		{
			template<class resourceInterfaceType> class ResourceGroup
				:public simul::graph::meta::Group
				,resourceInterfaceType
				,public Resource<resourceInterfaceType>
			{
			public:
				virtual void ChildAdded(simul::graph::meta::Node *node,simul::graph::meta::NodeStack &path)
				{
					resourceInterfaceType *rit=dynamic_cast<resourceInterfaceType*>(this);
					Resource<resourceInterfaceType>::ChildAdded(node,this);
					simul::graph::meta::Group::ChildAdded(node,path);
				}
				virtual void ChildRemoved(Node *node,simul::graph::meta::NodeStack &path)
				{
					Resource<resourceInterfaceType>::ChildRemoved(node,path);
					simul::graph::meta::Group::ChildRemoved(node,path);
				}
			};
			template<class resourceInterfaceType1,class resourceInterfaceType2> class ResourceGroup2
				:public simul::graph::meta::Group
				,public resourceInterfaceType1
				,public resourceInterfaceType2
				,public Resource<resourceInterfaceType1>
				,public Resource<resourceInterfaceType2>
			{
			public:
				virtual void ChildAdded(simul::graph::meta::Node *node,simul::graph::meta::NodeStack &path)
				{
					Resource<resourceInterfaceType1>::ChildAdded(node,this);
					Resource<resourceInterfaceType2>::ChildAdded(node,this);
					simul::graph::meta::Group::ChildAdded(node,path);
				}
				virtual void ChildRemoved(Node *node,simul::graph::meta::NodeStack &path)
				{
					Resource<resourceInterfaceType1>::ChildRemoved(node,path);
					Resource<resourceInterfaceType2>::ChildRemoved(node,path);
					simul::graph::meta::Group::ChildRemoved(node,path);
				}
			};
		}
	}
}

#endif // ifndef SIMUL_META_RESOURCE_H

