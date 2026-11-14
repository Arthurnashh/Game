#ifndef SIMUL_EXPORTTABLE_PROPERTY_H
#define SIMUL_EXPORTTABLE_PROPERTY_H

#include <string>
#include <iostream>
#include <vector>
#include <map>
#include "./Export.h"
#include "Simul/Base/RefPtr.h"
#include "Function.h"
#pragma warning(push)
// C4251 can be ignored in Visual C++ if you are deriving from a type in the Standard C++ Library
#pragma warning(disable:4251)
namespace simul
{
	namespace graph
	{
		namespace exporttable
		{
			typedef std::map<std::string,simul::base::RefPtr<ExportFunction> > VirtualFnMap;

			struct EXPORTTABLE_EXPORT BaseClass
			{
				BaseClass()
				{
					offset=0;
					vftable=0;
					immediate=false;
				}
				int offset;
				const void* vftable;
				bool immediate;
				std::vector<void*> unallocated_virtual_functions;
				VirtualFnMap virtual_function_map;
			};

			struct EXPORTTABLE_EXPORT Property
			{
				Property();
				std::string name;
				std::string type;
				simul::base::RefPtr<ExportFunction> GetMinimumFunction;
				simul::base::RefPtr<ExportFunction> GetMaximumFunction;
				simul::base::RefPtr<ExportFunction> GetFunction;
				simul::base::RefPtr<ExportFunction> SetFunction;
				simul::base::RefPtr<ExportFunction> GetInfoFunction;
				simul::base::RefPtr<ExportFunction> GetOrdinalFunction;
				bool declared_in_header;
				bool open;
				bool use_reference;
				bool limits;
			};
		}
	}
}
#pragma warning(pop)
#endif