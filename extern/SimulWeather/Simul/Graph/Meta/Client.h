#ifndef SIMUL_META_CLIENT_H
#define SIMUL_META_CLIENT_H
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
			// A client user is a base for a tracked node with multiple clients.
			// The clientInterface is a base for the tracker's owner.
			template<class clientInterface> class ClientUser
			{
			public:
				ClientUser(){}
				virtual ~ClientUser(){}
				typedef std::map<clientInterface *, unsigned> client_list;
				client_list clients;
				virtual void AddClient(clientInterface *c)
				{
					if(clients.find(c)!=clients.end())
						clients[c]++;
					else
						clients[c]=1;
				}
				virtual void RemoveClient(clientInterface *c)
				{
					typename client_list::iterator it;
					it=clients.find(c);
					if(it!=clients.end())
						clients[c]--;
					if(!clients[c])
						clients.erase(it);
				}
			};
			template<class ownerType,class childInterface,class clientInterface,class dataType> class ClientTracker
			{
			protected:
				struct DataTypeWrapper
				{
					dataType data;
					unsigned count;
				};
			public:
				typedef std::map<simul::graph::meta::Node*, DataTypeWrapper* > selection_map;
				selection_map selectionMap;		//! Map from the nodes to the CarSounds
				//! Inform this parent that a child has been added, and recurse up the graph
				//! In this case, we call InitNode
				virtual void ChildAdded(ownerType *si,simul::graph::meta::Node *node)
				{
 					childInterface *nt=dynamic_cast<childInterface*>(node);
					if(!nt)
						return;
					if(!si->ShouldIncludeNode(node,(dataType*)NULL))
						return;
 					ClientUser<clientInterface> *cu=dynamic_cast<ClientUser<clientInterface>*>(node);
					clientInterface *ci=dynamic_cast<clientInterface*>(si);
					cu->AddClient(ci);
					if(selectionMap.find(node)!=selectionMap.end())
					{
						selectionMap[node]->count++;
						return;
					}
					DataTypeWrapper *p=new DataTypeWrapper;
					selectionMap[node]=p;
					dataType *dt=&(p->data);		// Add it to the list
					p->count=1;
					si->InitNode(node,dt);			// Initialize dt with info relevant to cn.
				}
				virtual void ChildRemoved(ownerType *si,simul::graph::meta::Node *node)
				{
					childInterface *nt=dynamic_cast<childInterface*>(node);
					if(!nt)
						return;
					typename selection_map::iterator it=selectionMap.find(node);
					if(it==selectionMap.end())
						return;
					it->second->count--;
					if(it->second->count)
						return;
 					ClientUser<clientInterface> *cu=dynamic_cast<ClientUser<clientInterface>*>(node);
					clientInterface *ci=dynamic_cast<clientInterface*>(si);
					cu->RemoveClient(ci);
					si->RemovingNode(node,&(it->second->data));
					delete selectionMap[node];
					selectionMap.erase(it);
				}
				virtual void ChildModified(ownerType *si,simul::graph::meta::Node *node)
				{
					childInterface *nt=dynamic_cast<childInterface*>(node);
					if(nt)
					{
						typename selection_map::iterator i=selectionMap.find(node);
						if(i==selectionMap.end())
							return;
						si->ModifyingNode(node,&(i->second->data));
					}
				}
				void UpdateSelection(ownerType *si)
				{
					typename selection_map::iterator it;
					for(it=selectionMap.begin();it!=selectionMap.end();)
					{
						typename selection_map::iterator jt=it;			// Do it this way in case we destroy i.
						jt++;
						si->HandleNode(it->first,&(it->second->data));			// Do whatever this ClientTracker does.
						it=jt;
					}
				}
			};
//! tracks the node with childInterface
			template<class ownerType,class childInterface,class clientInterface> class SingleClientTracker
			{
			protected:
				childInterface *tracked;
				simul::graph::meta::Node *tracked_node;
				unsigned path_count;
			public:
				SingleClientTracker()
				{
					tracked=NULL;
					tracked_node=NULL;
					path_count=0;
				}
				childInterface *GetTracked(){return tracked;}
				const childInterface *GetTracked() const {return tracked;}
				//! Inform this parent that a child has been added, and recurse up the graph
				//! In this case, we call InitNode
				virtual void ChildAdded(ownerType *si,simul::graph::meta::Node *node)
				{
					childInterface *nt=dynamic_cast<childInterface*>(node);
					if(!nt)
						return;
					if(!si->ShouldIncludeNode(node,nt))
						return;
					path_count++;
					if(tracked==nt)
						return;
					assert(!tracked);
					tracked=nt;
					tracked_node=node;
					si->InitNode(node,nt);							// Initialize dt with info relevant to cn.
 					ClientUser<clientInterface> *cu=dynamic_cast<ClientUser<clientInterface>*>(node);
					clientInterface *ci=dynamic_cast<clientInterface*>(si);
					cu->AddClient(ci);
				}
				virtual void ChildRemoved(ownerType *si,simul::graph::meta::Node *node)
				{
					childInterface *nt=dynamic_cast<childInterface*>(node);
					if(!nt)
						return;
					assert(tracked==nt);
					path_count--;
					if(path_count)
						return;
					// no more paths to this node!
					si->RemovingNode(node,nt);
					tracked=NULL;
					tracked_node=NULL;
 					ClientUser<clientInterface> *cu=dynamic_cast<ClientUser<clientInterface>*>(node);
					clientInterface *ci=dynamic_cast<clientInterface*>(si);
					cu->RemoveClient(ci);
				}
				virtual void ChildModified(ownerType *si,simul::graph::meta::Node *node)
				{
					childInterface *nt=dynamic_cast<childInterface*>(node);
					if(!nt)
						return;
					si->ModifyingNode(node,nt);
				}
				void UpdateSelection(ownerType *si)
				{
					if(tracked)
						si->HandleNode(tracked_node,tracked);
				}
			};

//! tracks the node with childInterface that has the shortest path to the parent - e.g. children
//! take precedence over grandchildren etc.
			template<class ownerType,class childInterface,class clientInterface> class NearestClientTracker
			{
			protected:
				childInterface *tracked;
				simul::graph::meta::Node *tracked_node;
				unsigned path_count;
			public:
				NearestClientTracker()
				{
					tracked=NULL;
					tracked_node=NULL;
					path_count=0;
				}
				childInterface *GetTracked(){return tracked;}
				const childInterface *GetTracked() const {return tracked;}
				//! Inform this parent that a child has been added, and recurse up the graph
				//! In this case, we call InitNode
				virtual void ChildAdded(ownerType *si,simul::graph::meta::Node *node,
					const simul::graph::meta::NodeStack &path)
				{
					childInterface *nt=dynamic_cast<childInterface*>(node);
					if(!nt)
						return;
					if(!si->ShouldIncludeNode(node,nt))
						return;
					path_count++;
					if(tracked==nt)
						return;
					assert(!tracked);
					tracked=nt;
					tracked_node=node;
					si->InitNode(node,nt);							// Initialize dt with info relevant to cn.
 					ClientUser<clientInterface> *cu=dynamic_cast<ClientUser<clientInterface>*>(node);
					clientInterface *ci=dynamic_cast<clientInterface*>(si);
					cu->AddClient(ci);
				}
				virtual void ChildRemoved(ownerType *si,simul::graph::meta::Node *node,
					const simul::graph::meta::NodeStack &path)
				{
					childInterface *nt=dynamic_cast<childInterface*>(node);
					if(!nt)
						return;
					assert(tracked==nt);
					path_count--;
					if(path_count)
						return;
					// no more paths to this node!
					si->RemovingNode(node,nt);
					tracked=NULL;
					tracked_node=NULL;
 					ClientUser<clientInterface> *cu=dynamic_cast<ClientUser<clientInterface>*>(node);
					clientInterface *ci=dynamic_cast<clientInterface*>(si);
					cu->RemoveClient(ci);
				}
				virtual void ChildModified(ownerType *si,simul::graph::meta::Node *node,
					const simul::graph::meta::NodeStack &path)
				{
					childInterface *nt=dynamic_cast<childInterface*>(node);
					if(!nt)
						return;
					si->ModifyingNode(node,nt);
				}
				void UpdateSelection(ownerType *si)
				{
					if(tracked)
						si->HandleNode(tracked_node,tracked);
				}
			};
			
		}
	}
}

#endif // ifndef SIMUL_META_CLIENT_H
