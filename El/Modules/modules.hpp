#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MODULES_HPP
#define _MODULES_HPP

/*!
\patch_internal 1.50 Date 4/2/2002 by Jirka
- Added: modules initialization logic
*/

#include <Es/Containers/cachelist.hpp>

typedef void ModuleFunction();

//! Element of module list
struct RegistrationItem : public CLDLink
{
	int level;
	ModuleFunction *initFunction;
	ModuleFunction *doneFunction;

	RegistrationItem(int l, ModuleFunction *init, ModuleFunction *done)
	{
		level = l; initFunction = init; doneFunction = done;
	}
};

void RegisterModule(RegistrationItem *item);
void InitModules();
void DoneModules();

#define REGISTER_MODULE(name, level, init, done) \
static struct RegisterModule##name : public RegistrationItem \
{ \
	RegisterModule##name() : RegistrationItem(level, init, done) \
	{ \
		::RegisterModule(this); \
	} \
}	GRegisterModule##name;

// use when no done function
#define INIT_MODULE(name, level) \
	void InitModule##name(); \
	REGISTER_MODULE(name, level, InitModule##name, NULL) \
	void InitModule##name()

#define REGISTER_PLUGIN(level, init, done) \
extern "C" __declspec(dllexport) void RegisterPlugin(int &retLevel, ModuleFunction *&retInit, ModuleFunction *&retDone); \
__declspec(dllexport) void RegisterPlugin(int &retLevel, ModuleFunction *&retInit, ModuleFunction *&retDone) \
{ \
	retLevel = level; retInit = init; retDone = done; \
}

#define INIT_PLUGIN(level) \
	void InitPlugin(); \
	REGISTER_PLUGIN(level, InitPlugin, NULL) \
	void InitPlugin()

#endif
