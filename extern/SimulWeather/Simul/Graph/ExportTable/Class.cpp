
#include "Class.h"
// C4512 - "assignment operator could not be generated". Boost classes cause this warning.
// Also C4127 - "conditional expression is constant"
#pragma warning(push)
#pragma warning(disable:4512)
#pragma warning(disable:4127)
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#pragma warning(pop)
#include "Simul/Base/RealTimeTypes.h"
#include "Simul/Base/SmartPtr.h"
#include "Simul/Graph/ExportTable/ExportTable.h"
#include "Simul/Graph/Meta/Node.h"

using namespace simul;
using namespace base;
using namespace graph::exporttable;
using namespace boost::algorithm;
using namespace std;
extern bool ShowInfo;

ExportClass::ExportClass()
{
	completeObjectLocator=0;
	numVirtualFunctions=0;
	ConvertToString.reset();
	ConvertFromString.reset();
	vftable=NULL;
	table_size=0;
}

void ExportClass::FillInBaseClasses(const RTTICompleteObjectLocator *col)
{
	if(!col)
		col=completeObjectLocator;
	if(!col)
		return;
	if(name=="ParticleSystemManager")
	{
		std::string test;
	}
	const RTTIClassHierarchyDescriptor *chd=GetRTTIClassHierarchyDescriptor(col);
	for(unsigned i=1;i<chd->numBaseClasses;i++)
	{
		const RTTIBaseClassDescriptor* bcd=chd->GetBaseClass(i);
		const type_info *ti=GetRTTITypeInfo(bcd);//(bcd)->pTypeDescriptor;
		if(!ti)
			continue;
		if(!ti->name())
			continue;
		string undec=ti->name();
		if(undec=="")
			continue;
		const RTTICompleteObjectLocator *col2=GetRTTICompleteObjectLocator(*((int**)ti));
		if(col2)
		{
			boost::algorithm::replace_all(undec,"class ","");
			boost::algorithm::replace_all(undec,"struct ","");
			if(!GetBaseClass(undec))
				AddBaseClass(undec,true);
			GetBaseClass(undec)->offset=bcd->Where.mdisp;
		}
		if(bcd->numContainedBases)
			i+=bcd->numContainedBases;	// skip over so not counting them twice.
	}
	// now non-immediate bases:
	for(unsigned i=1;i<chd->numBaseClasses;i++)
	{
		const RTTIBaseClassDescriptor* bcd=chd->GetBaseClass(i);
		const type_info *ti=GetRTTITypeInfo(bcd);//(bcd)->pTypeDescriptor;
		if(!ti)
			continue;
		if(!ti->name())
			continue;
		string undec=ti->name();
		if(undec=="")
			continue;
		boost::algorithm::replace_all(undec,"class ","");
		boost::algorithm::replace_all(undec,"struct ","");
		if(GetBaseClass(undec))
			continue;
		AddBaseClass(undec,false);
		const RTTICompleteObjectLocator *col2=GetRTTICompleteObjectLocator(*((int**)ti));
		if(col2)
		{
			GetBaseClass(undec)->immediate=false;
			GetBaseClass(undec)->offset=bcd->Where.mdisp;
		}
	}
}

void ExportClass::CheckForActionFunction(simul::base::RefPtr<ExportFunction> EF)
{
	// Is this an action function?
	if((EF->name.substr(0,8)=="DoAction"))
	{
		std::string ac_name=EF->name.substr(8,EF->name.length()-8);
		Action &A=actions[ac_name];
		A.name=ac_name;
		A.Function=EF;
		std::string getordfn="GetOrdinalOf"+ac_name;
		if(functions.find(getordfn)!=functions.end())
		{
			A.GetOrdinalFunction=functions[getordfn];
		}
	}
}

void ExportClass::CheckForPropertyFunction(simul::base::RefPtr<ExportFunction> EF)
{
	// Is this a property function?
	if((EF->name.substr(0,3)=="Get"||EF->name.substr(0,3)=="Set")
		&&EF->name.find("GetMaximumOf")>=EF->name.length()
		&&EF->name.find("GetMinimumOf")>=EF->name.length()
		&&EF->name.find("GetInfoFor")>=EF->name.length()
		&&EF->name.find("GetOrdinalOf")>=EF->name.length())
	{
		std::string prop_name=EF->name.substr(3,EF->name.length()-3);
		std::string other_function_name=EF->name;
		if(prop_name.find("HorizontalF")<prop_name.length())
		{
			other_function_name=EF->name;
		}
		if(EF->name.substr(0,3)=="Get")
		{
			other_function_name[0]='S';
			if(EF->param_types.size()==0)
			{
				simul::base::RefPtr<ExportFunction> get=EF;
				Property *P=GetProperty(prop_name);
				std::string type=get->return_type;
				boost::algorithm::replace_all(type,"class","");
				boost::algorithm::replace_all(type,"struct","");
				boost::algorithm::replace_all(type,"const","");
				boost::algorithm::replace_all(type,"&","");
				boost::algorithm::trim(type);
				P->type=type;
				P->GetFunction=EF;
				if(get->return_type.find("&")<get->return_type.size())
				{
					P->use_reference=true;
				}
			}
		}
		else
			other_function_name[0]='G';
		if(functions.find(other_function_name)!=functions.end())
		{
			simul::base::RefPtr<ExportFunction> get=EF;//&functions[member_name];
			simul::base::RefPtr<ExportFunction> set=functions[other_function_name];
			if(EF->name.substr(0,3)=="Set")
				std::swap(get,set);
			bool open=false;
			if(set->name=="multiple")
			{	
				other_function_name="void-"+get->name;
				set=functions[other_function_name];
				if(!set.get())
					return;
			}
			if(get->name=="multiple")
			{
				other_function_name=set->param_types[0]+"-"+other_function_name;
				boost::algorithm::replace_all(other_function_name,"class ","");
				boost::algorithm::replace_all(other_function_name,"struct ","");
				string non_const_function_name=other_function_name;
				boost::algorithm::replace_all(non_const_function_name,"const ","");
				if(functions.find(non_const_function_name)!=functions.end())
				{
					get=functions[non_const_function_name];
					open=true;
				}
				else
				{
					get=functions[other_function_name];
					get.reset(new ExportFunction);
				}
				if(!get.get())
					return;
			}
			if((set->proc||set->Virtual)&&(get->proc||get->Virtual))
			if(set->param_types.size()==1)
			{
				std::string get_return=get->return_type;
				std::string set_param=set->param_types[0];
				boost::algorithm::replace_all(set_param,"class ","");
				boost::algorithm::replace_all(set_param,"struct ","");

				string type=get_return;
				boost::algorithm::replace_all(type,"class","");
				boost::algorithm::replace_all(type,"struct","");
				boost::algorithm::replace_all(type,"const","");
				boost::algorithm::replace_all(type,"&","");
				boost::algorithm::trim(type);

				if(get_return!=set_param)
					if(get_return==type+" &"&&set_param==type+" const &")
						open=true;

				if(get_return==set_param||open)
				{
					Property *P=GetProperty(prop_name);
					P->type=type;
					P->open=open;
					P->GetFunction=get;
					P->SetFunction=set;
					if(get_return.find("&")<get_return.size())
					{
						P->use_reference=true;
					}
				}
			}
		}

		// now check for static limit functions:
		std::string getmaxfn="GetMaximumOf"+prop_name;
		std::string getminfn="GetMinimumOf"+prop_name;
		if(functions.find(getmaxfn)!=functions.end()&&
			functions.find(getminfn)!=functions.end())
		{
			Property *P=GetProperty(prop_name);
			P->limits=true;
			P->GetMinimumFunction=functions[getminfn];
			P->GetMaximumFunction=functions[getmaxfn];
		}
		std::string getinfofn="GetInfoFor"+prop_name;
		if(functions.find(getinfofn)!=functions.end())
		{
			Property *P=GetProperty(prop_name);
			P->GetInfoFunction=functions[getinfofn];
		}
		std::string getordfn="GetOrdinalOf"+prop_name;
		if(functions.find(getordfn)!=functions.end())
		{
			Property *P=GetProperty(prop_name);
			P->GetOrdinalFunction=functions[getordfn];
		}
	}
	if(EF->name.substr(0,22)=="BeginOrdinalOfCategory")
	{
		std::string category_name=EF->name.substr(22,EF->name.length()-22);
		Category &c=categories[category_name];
		c.name=category_name;
		c.BeginOrdinalFunction=EF;
	}
	if(EF->name.substr(0,20)=="EndOrdinalOfCategory")
	{
		std::string category_name=EF->name.substr(20,EF->name.length()-20);
		Category &c=categories[category_name];
		c.name=category_name;
		c.EndOrdinalFunction=EF;
	}
}
#pragma warning(disable:4189)
// Make the virtual function map for the base classes of C.
void ExportClass::MatchVirtualFunctions()
{
	// the vftables for the (immediate?) base classes are packed together.
	std::vector<const void*> vftables;
	for(int i=0;i<GetNumBaseClasses();i++)
	{
		BaseClass *base_class=GetBaseClass(i);
		if(base_class->vftable)
			vftables.push_back(base_class->vftable);
	}
	if(base_class_map.find("simul::base::Referenced")==base_class_map.end())
		return;
	// must be a ref class so it can delete itself.
	if(functions.find("Creator")==functions.end())
	{
		std::cerr<<"ExportClass::MatchVirtualFunctions: No Creator() function for class "<<this->name.c_str()<<", cannot complete reflection."<<std::endl;
		functions.clear();
		return;
	}
	size_t last_pos=name.find_last_of(string("::"));
	if(last_pos>=name.length())
		last_pos=0;
	else
		last_pos++;
	std::string destructor_name=name.substr(last_pos,name.length()-last_pos);
	destructor_name="~"+destructor_name;
	if(functions.find("Creator")==functions.end()||
		functions.find(destructor_name)==functions.end())
		return;
	ExportFunction *creator=functions["Creator"].get();
	if(creator->Static==false)
		return;
	int_or_ptr instance_ptr=creator->Invoke(NULL);
	simul::base::SmartPtr<simul::graph::meta::Node> smart_ptr=(simul::graph::meta::Node*)instance_ptr;
// But is this ACTUALLY the pointer to the object or just the simul::graph::meta::Node parent of it?
	BaseClass *node_base=GetBaseClass("simul::graph::meta::Node");
	if(instance_ptr)
		instance_ptr-=node_base->offset;
	// we have created an instance of the class.
	// now for each base class,find the adjusted pointer and read off the virtuals.
	if(instance_ptr)
	for(int i=0;i<GetNumBaseClasses();i++)
	{
		BaseClass *base_class=GetBaseClass(i);			// The base definition
		ExportClass *base_C=ExportTable::GetSingletonInstance().GetClass(GetBaseClassName(i));		// The actual class definition.

		int_or_ptr base_ptr=instance_ptr+base_class->offset;

		base_class->vftable=GetVftableFromClassPtr((void*)base_ptr);
		if(!base_class->vftable)
			continue;
		if(base_class->vftable==(const void*)0xbaadf00d)
			continue;

		void** vftable_iterator=(void**)base_class->vftable;
		void* virtual_proc=*(vftable_iterator);

		// For this base, make the list of unallocated virtual functions.
		base_class->unallocated_virtual_functions.clear();
		while(virtual_proc)
		{
			base_class->unallocated_virtual_functions.push_back(virtual_proc);
			vftable_iterator++;
			virtual_proc=*(vftable_iterator);
			bool onto_next=false;
			for(size_t j=0;j<vftables.size();j++)
			{
				if(vftable_iterator==vftables[j])
					onto_next=true;
			}
			if(onto_next)
				break;
		}
		// For each virtual in the base class, find whether it is implemented
		// in the derived (main) class or in an intermediate sub-class
		for(vector<void*>::iterator j=base_class->unallocated_virtual_functions.begin();
			j!=base_class->unallocated_virtual_functions.end();j++)
		{
			void *virtual_proc=*j;
			// what function is this?
			simul::base::RefPtr<ExportFunction> f=FindFunctionInClass(virtual_proc);
			if(!f.get())
			{
				// not implemented in the main class. What about other base classes?
//				int current_base=i;
				for(int k=i;k<GetNumBaseClasses();k++)
				{
// this class must be derived from the current base.
					ExportClass *test_class=ExportTable::GetSingletonInstance().GetClass(GetBaseClassName(k));
					if(!test_class)//->base_class_map.find(i->first)==classes[k->first].base_class_map.end())
						continue;
					// Is the current base a base of THIS CLASS?
					if(!test_class->GetBaseClass(GetBaseClassName(i)))
						continue;
					// Does this class implement the function?
					simul::base::RefPtr<ExportFunction> f_test=test_class->FindFunctionInClass(virtual_proc);
					if(!f_test.get())
						continue;
					f=f_test;
				}
				if(!f.get())
					continue;
			}
			base_class->virtual_function_map[f->name]=f;
			f->VirtualImplementation=true;
			if(base_C->functions.find(f->name)==base_C->functions.end())		// Pure virtual base function!
			{
				ExportFunction *EF=new ExportFunction;
				base_C->functions[f->name].reset(EF);
				EF->name=f->name;
				EF->Virtual=true;
				EF->return_type=f->return_type;
				EF->param_types=f->param_types;
				EF->proc=NULL;
				EF->Static=false;
			}
		}
		base_class->unallocated_virtual_functions.clear();
	}
#if 0
	if(instance_ptr)
	{
		BaseClass *referenced=GetBaseClass("simul::base::Referenced");
		int_or_ptr ref_ptr=instance_ptr+referenced->offset;
		ExportClass *ref_C=ExportTable::GetSingletonInstance().GetClass("simul::base::Referenced");
		ref_C->functions["intrusive_ptr_release"]->Invoke((void*)ref_ptr);//((void*)ref_ptr,(void*)NULL);
	}
#endif
}

// Complete the virtual function map for the base classes whose virtuals are implemented in other bases.
void ExportClass::CompleteVirtualFunctions()
{
	//for(int i=0;i<GetNumBaseClasses();i++)
	{
		for(int j=0;j<GetNumBaseClasses();j++)
		{
			ExportTable::GetSingletonInstance().AddClass(GetBaseClassName(j));
		}

	}
}

void ExportClass::RemoveTransmittedVirtuals(string base_name)
{
	ExportClass *base_C=ExportTable::GetSingletonInstance().GetClass(base_name);
	if(base_class_map.find(base_name)==base_class_map.end())
		return;
	BaseClass *base_class=&base_class_map[base_name];
	// this base class has a list of unallocated virtuals.
	// The bases of this base have a smaller list.
	// REMOVE
	for(BaseClassMap::iterator i=base_C->base_class_map.begin();i!=base_C->base_class_map.end();i++)
	{
		ExportClass *new_base_C=ExportTable::GetSingletonInstance().GetClass(i->first);
		BaseClass *new_base_class=&i->second;
		if(!i->second.immediate)
			continue;
		if(!new_base_C->table_size)
			continue;
		for(size_t j=0;j<new_base_C->table_size;j++)
		{
			if(!base_class->unallocated_virtual_functions.size())
				break;
			new_base_class->unallocated_virtual_functions.push_back(base_class->unallocated_virtual_functions[0]);
			base_class->unallocated_virtual_functions.erase(base_class->unallocated_virtual_functions.begin());
		}
	}
}

simul::base::RefPtr<ExportFunction> ExportClass::FindFunctionInClass(void *proc)
{
	for(FnMap::iterator i=functions.begin();i!=functions.end();i++)
	{
		if(i->second.get())
			if(i->second->proc==proc)
				return i->second;
	}
	simul::base::RefPtr<ExportFunction> F;
	for(BaseClassMap::iterator i=base_class_map.begin();i!=base_class_map.end();i++)
	{
		ExportClass *base=ExportTable::GetSingletonInstance().GetClass(i->first);
		simul::base::RefPtr<ExportFunction> f=base->FindFunctionInClass(proc);
		if(f.get())
			return f;
	}
	return F;
}

int ExportClass::GetNumBaseClasses() const
{
	return (int)base_class_map.size();
}

BaseClass *ExportClass::GetBaseClass(const std::string &name)
{
	BaseClassMap::iterator i=base_class_map.find(name);
	if(i==base_class_map.end())
	{
		return 0;
	}
	return &(i->second);
}

BaseClass *ExportClass::GetBaseClass(int b)
{
	BaseClassMap::iterator j=base_class_map.begin();
	for(int i=0;i<b;i++,j++)
	{
	}
	return &(j->second);
}


BaseClass *ExportClass::AddBaseClass(const std::string &n,bool immed)
{
	base_class_map[n].immediate=immed;
	return &base_class_map[n];
}

const BaseClass *ExportClass::GetBaseClass(const std::string &name) const
{
	BaseClassMap::const_iterator i=base_class_map.find(name);
	if(i==base_class_map.end())
	{
		return 0;
	}
	return &(i->second);
}

const BaseClass *ExportClass::GetBaseClass(int b) const
{
	BaseClassMap::const_iterator j=base_class_map.begin();
	for(int i=0;i<b;i++,j++)
	{
	}
	return &(j->second);
}

const std::string &ExportClass::GetBaseClassName(int b) const
{
	BaseClassMap::const_iterator j=base_class_map.begin();
	for(int i=0;i<b;i++,j++)
	{
	}
	return j->first;
}

static bool pred(Property* P1,Property* P2)
{
	if(!P1->GetOrdinalFunction.get()||!P2->GetOrdinalFunction.get())
		return false;
	int_or_ptr i1=P1->GetOrdinalFunction->Invoke(NULL);
	int_or_ptr i2=P2->GetOrdinalFunction->Invoke(NULL);
	bool res=(i1<i2);
	return res;
}

static bool category_predicate(Category* C1,Category* C2)
{
	if(!C1->BeginOrdinalFunction.get()||!C2->BeginOrdinalFunction.get())
		return false;
	if(!C1->EndOrdinalFunction.get()||!C2->EndOrdinalFunction.get())
		return false;
	int_or_ptr b1=C1->BeginOrdinalFunction->Invoke(NULL);
	int_or_ptr b2=C2->BeginOrdinalFunction->Invoke(NULL);
	int_or_ptr e1=C1->EndOrdinalFunction->Invoke(NULL);
	int_or_ptr e2=C2->EndOrdinalFunction->Invoke(NULL);
	bool res=(b1<b2);
	if(b1<b2&&e1<e2)
	{
		if(ShowInfo)
			std::cerr<<"ERROR: Overlapping categories "<<C1->name<<" and "<<C2->name;
	}
	return res;
}

void ExportClass::Sort()
{
	sorted_properties.clear();
	for(PropertyMap::iterator j=properties.begin();j!=properties.end();j++)
	{
		sorted_properties.push_back(&j->second);
	}
	std::sort(sorted_properties.begin(),sorted_properties.end(),pred);// sort by ordinal
	sorted_categories.clear();
	for(CategoryMap::iterator j=categories.begin();j!=categories.end();j++)
	{
		Category *c=&j->second;
		if(!c->BeginOrdinalFunction.get())
		{
			if(ShowInfo)
				cerr<<"ERROR: Category "<<c->name<<" of class "<<name<<" has no start."<<endl;
			continue;
		}
		if(!c->EndOrdinalFunction.get())
		{
			if(ShowInfo)
				cerr<<"ERROR: Category "<<c->name<<" of class "<<name<<" has no end."<<endl;
			continue;
		}
		sorted_categories.push_back(c);
	}
	std::sort(sorted_categories.begin(),sorted_categories.end(),category_predicate);// sort by ordinal

	for(unsigned j=0;j<sorted_categories.size();j++)
	{
		Category *c=sorted_categories[j];
		c->properties.clear();
		int start=c->BeginOrdinalFunction->Invoke(NULL);
		int end=c->EndOrdinalFunction->Invoke(NULL);
		for(unsigned k=0;k<sorted_properties.size();k++)
		{
			Property *p=sorted_properties[k];
			if(p->GetOrdinalFunction.get())
			if((int)p->GetOrdinalFunction->Invoke(NULL)>start&&
				(int)p->GetOrdinalFunction->Invoke(NULL)<end)
			{
				c->properties.push_back(p);
			}
		}
	}
}

Property *ExportClass::GetProperty(const std::string &prop_name)
{
	PropertyMap::iterator i=properties.find(prop_name);
	if(i==properties.end())
	{
		properties[prop_name].name=prop_name;
		i=properties.find(prop_name);
	}
	return &(i->second);

}