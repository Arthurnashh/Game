
#ifndef _POSACCESS_DUMMY_HPP
#define _POSACCESS_DUMMY_HPP

void __cdecl GlobalShowMessage(int,char const *,...)
{
}

int __cdecl MemoryFreeBlocks(void) {return 0;}
int __cdecl MemoryUsed(void){return 0;}

class Object;
class LODShapeWithShadow;

Object *NewObject(RString type,RString shape)
{
	// unable to load proxy
	return NULL;
}

Object *NewObject(LODShapeWithShadow *shape, int id)
{
	return NULL;
}

Object *NewProxyObject(RString name)
{
	return NULL;
}

DWORD GlobalTickCount()
{
	return 0;
}

enum IDS;

RString LocalizeString(int ids)
{
	return "Error - no stringtable";
}

RString LocalizeString(IDS ids)
{
	return "Error - no stringtable";
}

RString LocalizeString(const char *ids)
{
	return ids;
}

RString GetUserParams() {return "";}

#endif