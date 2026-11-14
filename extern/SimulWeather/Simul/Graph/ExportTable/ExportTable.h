#ifndef SIMUL_EXPORTTABLE_EXPORTTABLE_H
#define SIMUL_EXPORTTABLE_EXPORTTABLE_H
#pragma warning(push)
#pragma warning(disable:4996)
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include "Simul/Base/RefPtr.h"
#include "./Export.h"
#include "Function.h"
#include "Property.h"
#include "Class.h"
// C4251 can be ignored in Visual C++ if you are deriving from a type in the Standard C++ Library
#pragma warning(disable:4251)

namespace simul
{
	namespace graph
	{
		namespace exporttable
		{
			class EXPORTTABLE_EXPORT ExportTable
			{
			public:
				ExportTable();
				static ExportTable &GetSingletonInstance();
				void LoadDynamicLibrary(const std::string &filename);
				void HandleLibrary(const std::string &filename);
				void UnloadDynamicLibrary(const std::string &name);
				void AddDefinition(const std::string &Def,void *proc);
				const ExportFunction *GetFunction(const std::string &function_name);
				ExportClass *GetClass(const std::string &class_name);
				const ClassMap &GetClasses() const;
//! Add a class to the class list just from its type_info. Does nothing if it's already there.
				void AddClassFromTypeInfo(const type_info *ti);
				void AddClassFromPointer(const void *class_instance_ptr);
				void AddClass(const std::string &class_name);
			protected:
				bool initialized;
				void Initialize();
				void CheckForChanges();
				std::map<std::string,void*> handled_modules;
				std::map<std::string,simul::base::RefPtr<ExportFunction> > functions;
				ClassMap classes;
				static ExportTable instance;
				~ExportTable();
				const void *FindIntermediateBaseVFTable(std::string derived,std::string main_base);
				void MatchVirtualFunctions();
				void MatchVirtualFunctions(const std::string &mod);
				void MakeProperties();
				std::string current_module;
			};
		}
	}
}


#pragma warning(pop)
#endif