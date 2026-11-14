#include "RealTimeTypes.h"

using namespace simul::graph::exporttable;

const RTTICompleteObjectLocator *simul::graph::exporttable::GetRTTICompleteObjectLocator(int *vftable)
{
	int *loc_ptr=vftable-1;
	const RTTICompleteObjectLocator *col=(RTTICompleteObjectLocator *)(*loc_ptr);
	return col;
}

const char *simul::graph::exporttable::GetTypeName(void *class_ptr)
{
	void *vftable=(void *)(*((int*)class_ptr));
	static std::string name;
	name="";
	int *loc_ptr=(int*)vftable-1;
	const RTTICompleteObjectLocator *col=(RTTICompleteObjectLocator *)(*loc_ptr);
	if(!col)
		return name.c_str();
	if(!col->pTypeDescriptor)
		return name.c_str();
	name=col->pTypeDescriptor->name();
	size_t pos=name.find_last_of("::");
	if(pos<name.length())
		name=name.substr(pos+1,name.length()-pos-1);
	pos=name.find(std::string("class "));
	if(pos<name.length())
		name=name.substr(pos+6,name.length()-pos-6);
	return name.c_str();
}