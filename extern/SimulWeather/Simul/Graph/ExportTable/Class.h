#ifndef SIMUL_EXPORTTABLE_CLASS_H
#define SIMUL_EXPORTTABLE_CLASS_H

#include <string>
#include <iostream>
#include <vector>
#include <map>
#include "./Export.h"
#include "Simul/Base/RefPtr.h"
#include "Function.h"
#include "Property.h"
#pragma warning(push)
// C4251 can be ignored in Visual C++ if you are deriving from a type in the Standard C++ Library
#pragma warning(disable:4251)
namespace simul
{
	namespace base
	{
		struct RTTICompleteObjectLocator;
		struct RTTIBaseClassDescriptor;
	}
	namespace graph
	{
		namespace exporttable
		{
			struct EXPORTTABLE_EXPORT Action
			{
				std::string name;
				simul::base::RefPtr<ExportFunction> Function;
				simul::base::RefPtr<ExportFunction> GetOrdinalFunction;
			};

			typedef std::map<std::string,simul::base::RefPtr<ExportFunction> > FnMap;
			typedef std::map<std::string,BaseClass> BaseClassMap;
			typedef std::map<std::string,Property> PropertyMap;
			typedef std::map<std::string,Action> ActionMap;

			struct EXPORTTABLE_EXPORT Category
			{
				std::string name;
				simul::base::RefPtr<ExportFunction> BeginOrdinalFunction;
				simul::base::RefPtr<ExportFunction> EndOrdinalFunction;
				std::vector<Property*> properties;
			};

			typedef std::map<std::string,Category> CategoryMap;

			struct EXPORTTABLE_EXPORT ExportClass
			{
				ExportClass();
				void Sort();
				std::string name;
				void *vftable;					// if it has only one vftable for its bases
				size_t table_size;
				std::string module;
				FnMap functions;
				ActionMap actions;
				CategoryMap categories;
				std::vector<Property*> sorted_properties;
				std::vector<Category*> sorted_categories;
				unsigned numVirtualFunctions;
				const struct simul::base::RTTICompleteObjectLocator *completeObjectLocator;
				simul::base::RefPtr<ExportFunction> ConvertToString;	// Take a const ptr to this class,return an std::string
				simul::base::RefPtr<ExportFunction> ConvertFromString;	// Take an std::string and a ptr to a class instance,convert the string to class values
				simul::base::RefPtr<ExportFunction> ConstFromString;	// Take an std::string and a ptr to a class instance,convert the string to class values
				const PropertyMap &GetProperties() const {return properties;}
				Property *GetProperty(const std::string &prop_name);
				void FillInBaseClasses(const simul::base::RTTICompleteObjectLocator *col);
				void CheckForActionFunction(simul::base::RefPtr<ExportFunction> EF);
				void CheckForPropertyFunction(simul::base::RefPtr<ExportFunction> EF);
				void MatchVirtualFunctions();
				void CompleteVirtualFunctions();
				void RemoveTransmittedVirtuals(std::string base_name);
				simul::base::RefPtr<ExportFunction> FindFunctionInClass(void*proc);
				int GetNumBaseClasses() const;
				BaseClass *AddBaseClass(const std::string &name,bool immed);
				BaseClass *GetBaseClass(const std::string &name);
				BaseClass *GetBaseClass(int b);
				const BaseClass *GetBaseClass(const std::string &name)const;
				const BaseClass *GetBaseClass(int b)const;
				const std::string &GetBaseClassName(int b) const;
				PropertyMap properties;
				BaseClassMap base_class_map;
			private:
			};

			typedef std::map<std::string,ExportClass> ClassMap;
		}
	}
}
#endif