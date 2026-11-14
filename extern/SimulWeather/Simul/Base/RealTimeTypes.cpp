#include "precomp.h"
#include "RealTimeTypes.h"
#include <iostream>
#include <vector>
#if defined(WIN32) || defined(WIN64)
#include <windows.h>
extern "C"
{
	NTSYSAPI
PVOID
NTAPI RtlPcToFileHeader(
		__in          PVOID PcValue,
		__out         PVOID* BaseOfImage
	);

}

static void replace_all(std::string& target, const std::string& to_find, const std::string& replacement )
{
   if (to_find.empty())
      return; 
   
   size_t replace_inc = replacement.length();
   for (size_t loc = target.find( to_find ); 
      loc != std::string::npos; 
      loc = target.find( to_find, loc + replace_inc ) )
   {
      target.replace( loc, to_find.length(), replacement );
   }
}
/*
void replace_all(std::string &name,const std::string &str1,const std::string &str2)
{
	std::string newname=name.replace(name.begin(),name.end(),str1,str2);
	while(name!=newname)
	{
		name=newname;
		newname=name.replace(name.begin(),name.end(),str1,str2);
	}
}*/
/*
dynamic_cast works by looking at the RTTI (run-time type info) associated with a type.
That RTTI has a bunch of pointers in it.  On x86, these pointers are just that, raw 32-bit
pointers.  But on 64-bit platforms, the pointers are actually 32-bit offsets which need to
be added to the base address of the DLL or EXE in which the RTTI resides to compute a true
64-bit pointer.  That addition shouldn't be causing any major perf problems, since it's
cheap and not too common.  But determining the module base address, which happens once per
dynamic_cast, could be expensive.  It's done via the API call RtlPcToFileHeader, which (in
the fast case) takes the loader lock and walks the list of loaded modules to find where the
RTTI data resides.
*/
#ifdef WIN64
	static void *Pointer(unsigned int offset,const void *vftable)
	{
		if(!offset)
			return 0;
		void *base=0;
		RtlPcToFileHeader(
				  const_cast< void  * >( vftable),
				  &base
				);
		unsigned __int64 i1=(unsigned __int64)base;
		unsigned __int64 i2=(unsigned __int64)offset;
		return (void*)(i1+i2);
	}
#else
	static const void *Pointer(const void* offset,const void *)
	{
		return offset;
	}
#endif
using namespace simul::base;
using namespace std;
struct string_and_counter
{
	string str;
	int counter;
};

const type_info* simul::base::GetRTTITypeInfo(const RTTICompleteObjectLocator *col)
{
	type_info* ti =(type_info* )Pointer(col->pTypeDescriptor,col);
	return ti;
}
const type_info* simul::base::GetRTTITypeInfo(const RTTIBaseClassDescriptor *bcd)
{
	type_info* ti =(type_info* )Pointer(bcd->pTypeDescriptor,bcd);
	return ti;
}

const void *simul::base::GetVftableFromClassPtr(const void *class_instance_ptr)
{
	if(!class_instance_ptr)
		return NULL;
#ifdef WIN64
	return (const void *)(*((const unsigned __int64*)class_instance_ptr));
#else
	return (const void *)(*((const int*)class_instance_ptr));
#endif
}

const RTTICompleteObjectLocator *simul::base::GetRTTICompleteObjectLocatorFromClassPtr(const void *class_instance_ptr)
{
	const int *vftable=(const int *)GetVftableFromClassPtr(class_instance_ptr);
	return GetRTTICompleteObjectLocator(vftable);
}
const RTTIBaseClassDescriptor* RTTIClassHierarchyDescriptor::GetBaseClass(size_t i) const
{
	const RTTIBaseClassDescriptor_ptr* bcda =(const RTTIBaseClassDescriptor_ptr* )Pointer(pBaseClassArray,this);
	const RTTIBaseClassDescriptor *bcd=(const RTTIBaseClassDescriptor* )Pointer(bcda[i],this);
	return bcd;
}

const RTTICompleteObjectLocator *simul::base::GetRTTICompleteObjectLocator(const int *vftable)
{
#ifdef WIN64
	const unsigned __int64 *loc_ptr=(const unsigned __int64 *)vftable-1;
#else
	const int *loc_ptr=vftable-1;
#endif
	if(!loc_ptr)
		return NULL;
	const RTTICompleteObjectLocator *col=(const RTTICompleteObjectLocator *)(*loc_ptr);//(Pointer((const void*)(*loc_ptr),(const void*)vftable));
	return col;
}

const RTTIClassHierarchyDescriptor *simul::base::GetRTTIClassHierarchyDescriptor(const RTTICompleteObjectLocator *col)
{
#ifdef WIN64
	const RTTIClassHierarchyDescriptor *chd=(const RTTIClassHierarchyDescriptor *)Pointer(col->pClassDescriptor,col);
	return chd;
#else
	return col->pClassDescriptor;
#endif
}

const char *simul::base::GetQualifiedTypeName(const void *class_ptr)
{
#ifdef _CPPRTTI
	static std::string name;
	if(!class_ptr)
		return NULL;
	const void *vftable=GetVftableFromClassPtr(class_ptr);//(void *)(*((int*)class_ptr));
	if(!vftable)
		return NULL;
	name="";
#ifdef WIN64
	unsigned __int64 *loc_ptr=(unsigned __int64*)vftable-1;
#else
	int *loc_ptr=(int*)vftable-1;
#endif
	const RTTICompleteObjectLocator *col=(RTTICompleteObjectLocator *)(*loc_ptr);
	if(!col)
		return name.c_str();
	if(!col->pTypeDescriptor)
		return name.c_str();
	type_info* ti =(type_info* )Pointer(col->pTypeDescriptor,vftable);
	name=ti->name();
#ifdef _MSC_VER
	replace_all(name,"class ","");
#endif
	return name.c_str();
#else
	return NULL;
#endif
}
const char *simul::base::GetUnqualifiedTypeName(const void *class_ptr)
{
	static std::string name;
	name=GetQualifiedTypeName(class_ptr);
	size_t pos=name.find_last_of("::");
	if(pos<name.length())
		name=name.substr(pos+1,name.length()-pos-1);
	pos=name.find(std::string("class "));
	if(pos<name.length())
		name=name.substr(pos+6,name.length()-pos-6);
	return name.c_str();
}
std::string simul::base::PtrNameToTypeName(const char *typename_of_ptr)
{
	if(!typename_of_ptr)
	{
		cerr<<"simul::base::PtrNameToTypeName: Invalid type pointer."<<endl;
		return "invalid type";
	}
	std::string name=typename_of_ptr;
#ifdef _MSC_VER
	replace_all(name,"class ","");
	replace_all(name,"struct ","");
	replace_all(name," *","");
	replace_all(name,"*","");
#endif
	return name;
}
static string BuildInheritancePath(const RTTICompleteObjectLocator *col,const char *root);
const char *simul::base::GetQualifiedInheritancePath(const void *class_ptr,const char *root)
{
	// look at each base class: is any of them also inherited from "root"?
	// if it's more than one, return NULL: this only works for single inheritance.
	// traverse downward until we reach the root.
	const void *vftable=GetVftableFromClassPtr(class_ptr);
	return GetQualifiedInheritancePathFromVftable(vftable,root);
}

const char *simul::base::GetQualifiedInheritancePathFromVftable(const void *vftable,const char *root)
{
	const RTTICompleteObjectLocator *col=GetRTTICompleteObjectLocator((int*)vftable);
	static string path;
	path=BuildInheritancePath(col,root);
	if(path=="")
		return NULL;
	return path.c_str();
}

const char *simul::base::GetQualifiedInheritancePathFromCompleteObjectLocator(const RTTICompleteObjectLocator *col,const char *root)
{
	static string path;
	path=BuildInheritancePath(col,root);
	if(path=="")
		return NULL;
	return path.c_str();
}

static string BuildInheritancePath(const RTTICompleteObjectLocator *col,const char *root)
{
	string inheritance_path;
	vector<string_and_counter> path;
	RTTIClassHierarchyDescriptor *chd=(RTTIClassHierarchyDescriptor*)Pointer(col->pClassDescriptor,col);
	for(unsigned i=0;i<chd->numBaseClasses;i++)
	{
		const RTTIBaseClassDescriptor *bcd=chd->GetBaseClass(i);
		const std::type_info *ti=(std::type_info *)(Pointer((bcd)->pTypeDescriptor,col));
		std::string name=ti->name();
#ifdef _MSC_VER
		replace_all(name,"class ","");
		replace_all(name,"struct ","");
#endif
		string_and_counter X;
		X.str=name;
		X.counter=1+bcd->numContainedBases;
		path.insert(path.begin(),X);

		if(name==root)
			break;
		
		for(unsigned j=0;j<path.size();j++)
		{
			path[j].counter--;
			if(!path[j].counter)
			{
				path.erase(path.begin()+j);
				j--;
			}
		}
	}
	for(unsigned j=0;j<path.size();j++)
	{
		if(j)
			inheritance_path+=".";
		inheritance_path+=path[j].str;
	}

	return inheritance_path;
}
/*
void simul::base::GetBasesFromClassPtr(const void *class_instance_ptr,DWORD &numBaseClasses,
									   RTTIBaseClassDescriptor*const * &pBaseClassArray)
{
	if(!class_instance_ptr)
		return;
#ifdef WIN64
	const void *vftable=(const void *)(*((const unsigned __int64*)class_instance_ptr));
#else
	const void *vftable=(const void *)(*((const int*)class_instance_ptr));
#endif
	if(!vftable)
		return;
	const RTTICompleteObjectLocator *col=GetRTTICompleteObjectLocator((int*)vftable);
	if(!col)
		return;
	RTTIClassHierarchyDescriptor *chd=(RTTIClassHierarchyDescriptor*)Pointer(col->pClassDescriptor,vftable);
	numBaseClasses=chd->numBaseClasses;
	pBaseClassArray=chd->pBaseClassArray;
}*/
#endif