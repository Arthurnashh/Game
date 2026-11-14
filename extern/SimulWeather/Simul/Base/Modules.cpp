#include "precomp.h"

#include "Simul/Base/Modules.h"
using namespace simul::base;

static std::map<std::string,void*> modules;

void simul::base::AddModule(const char *name,void *hinstDLL)
{
	modules[name]=hinstDLL;
}
void *simul::base::GetModule(const char *name)
{
	if(modules.find(name)!=modules.end())
		return modules[name];
	return  NULL;
}
size_t simul::base::GetNumModules()
{
	return modules.size();
}

const char *simul::base::GetModuleName(size_t i)
{
	std::map<std::string,void*>::const_iterator it=modules.begin();
	while(i)
	{
		it++;
		i--;
	}
	return it->first.c_str();
}
void simul::base::UnloadModule(const char *name)
{
	if(modules.find(name)==modules.end())
		return;
	modules.erase(name);
}