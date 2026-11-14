#ifndef SIMUL_META_GENERIC_TRACKER_H
#define SIMUL_META_GENERIC_TRACKER_H
#include "Simul/Graph/Meta/Group.h"
#include "Simul/Graph/Meta/NodeVisitor.h"
#include "Simul/Base/RealTimeTypes.h"
#include <map>
#include <assert.h>

namespace simul
{
	namespace graph
	{
		namespace meta
		{
			template<class ownerType> class GenericHandler:public simul::base::Referenced
			{
			public:
				GenericHandler():simul::base::Referenced()
				{
				}
				virtual ~GenericHandler()
				{
				}
				virtual void Init(ownerType *,simul::graph::meta::Node *)
				{
				}
				virtual void Removing(ownerType *,simul::graph::meta::Node *)
				{
				}
				virtual void Modifying(ownerType *,simul::graph::meta::Node *)
				{
				}
				virtual void Handle(ownerType *,simul::graph::meta::Node *)
				{
				}
			};
			//! GenericTracker: A template for a node that keeps a list of child nodes of arbitrary types.

			//! GenericTracker is a node that looks through its children for nodes of types that can be
			//! specified at a later time (nodeType), and keeps a map of them with corresponding instances
			//! of a helper type (dataType), which is also specified later.
			//! dataType contains information particular to the GenericTracker; while nodeType has no knowledge of Selectors
			//! that refer to it.
			//! When TimeStep is called, it iterates through the list calling  HandleNode(nodeType*,dataType*).
			template<class ownerType> class GenericTracker
			{
			protected:
				typedef simul::graph::meta::GenericHandler<ownerType> HandlerClass;
				struct DataTypeWrapper
				{
					simul::base::SmartPtr<HandlerClass > handler;
					unsigned count;
					simul::graph::meta::Node *node;
				};
				static std::string IdOf(const std::type_info *ti,simul::graph::meta::Node *node)
				{
					std::string id=ti->name();
					char txt[100];
					sprintf_s(txt,100,"%d",node);
					id+=txt;
					return id;
				}
			public:
				typedef std::map<std::string,simul::graph::meta::GenericHandler<ownerType> *(*)()> type_map;
				typedef std::map<std::string,DataTypeWrapper* > selection_map;
				selection_map selectionMap;		//! Map from the nodes to the CarSounds
				type_map typeMap;				//! Map from the nodes to the CarSounds

				HandlerClass* GetHandler(const std::string &type_name,int index=0)
				{
					int idx=0;
					for(selection_map::iterator i=selectionMap.begin();i!=selectionMap.end();i++)
					{
						HandlerClass *h=i->second->handler.get();
						if(i->first.find(type_name)<i->first.size())
						{
							if(idx==index)
								return h;
							idx++;
						}
					}
					return NULL;
				}
				void AddType(const std::string &type_name,simul::graph::meta::GenericHandler<ownerType> *(*creator_fn)())
				{
					typeMap[type_name]=creator_fn;
				}
				//! Inform this parent that a child has been added, and recurse up the graph
				//! In this case, we call InitNode
				virtual void ChildAdded(ownerType *si,simul::graph::meta::Node *node)
				{
					// Is this of a recognized type?
//					DWORD numBaseClasses=0;
					//simul::base::RTTIBaseClassDescriptor*const * pBaseClassArray=NULL;
					const simul::base::RTTICompleteObjectLocator *col=simul::base::GetRTTICompleteObjectLocatorFromClassPtr(node);
					const simul::base::RTTIClassHierarchyDescriptor *chd=simul::base::GetRTTIClassHierarchyDescriptor(col);

					//simul::base::GetBasesFromClassPtr(node,numBaseClasses,pBaseClassArray);
						bool result=false;
					std::string class_name="";
					for(DWORD i=0;i<chd->numBaseClasses;i++)
					{	
						const simul::base::RTTIBaseClassDescriptor *bcd=chd->GetBaseClass(i);
						std::type_info *ti=bcd->pTypeDescriptor;
						std::string name=ti->name();
						if(typeMap.find(name)!=typeMap.end())
						{
							class_name=name;
							result=true;
							std::string node_interface_id=IdOf(ti,node);
							if(selectionMap.find(node_interface_id)!=selectionMap.end())
							{
								selectionMap[node_interface_id]->count++;
								return;
							}
							DataTypeWrapper *p=new DataTypeWrapper;
							p->node=node;
							simul::graph::meta::GenericHandler<ownerType> *(*createfn)()=typeMap[class_name];
							p->handler=createfn();
							selectionMap[node_interface_id]=p;
							p->count=1;
							p->handler->Init(si,node);						// Initialize dt with info relevant to cn.
						}
					}
				}
				virtual void ChildRemoved(ownerType *si,simul::graph::meta::Node *node)
				{
					const type_info &ti=typeid(*(node));
					std::string class_name=ti.name();
					if(typeMap.find(class_name)==typeMap.end())
						return;
					std::string node_interface_id=IdOf(&ti,node);
					selection_map::iterator i=selectionMap.find(node_interface_id);
					if(i==selectionMap.end())
						return;
					i->second->count--;
					if(i->second->count)
						return;
					i->second->handler->Removing(si,node);
					selectionMap.erase(i);
				}
				virtual void ChildModified(ownerType *si,simul::graph::meta::Node *node)
				{
					const type_info &ti=typeid(*(node));
					std::string class_name=ti.name();
					if(typeMap.find(class_name)==typeMap.end())
						return;
					std::string node_interface_id=IdOf(&ti,node);
					selection_map::iterator i=selectionMap.find(node_interface_id);
					if(i==selectionMap.end())
						return;
					i->second->handler->Modifying(si,node);
				}
				void UpdateSelection(ownerType *si)
				{
					for(selection_map::iterator i=selectionMap.begin();i!=selectionMap.end();)
					{
						selection_map::iterator j=i;			// Do it this way in case we destroy i.
						j++;
						i->second->handler->Handle(si,i->second->node);			// Do whatever this GenericTracker does.
						i=j;
					}
				}
			};

		}
	}
}
#endif // ifndef SIMUL_META_SELECTOR_H

