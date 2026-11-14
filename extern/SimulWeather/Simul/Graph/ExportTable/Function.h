#ifndef SIMUL_EXPORTTABLE_FUNCTION_H
#define SIMUL_EXPORTTABLE_FUNCTION_H

#include <string>
#include <iostream>
#include <vector>
#include <map>
#include "./Export.h"
#include "Simul/Base/RefPtr.h"
#pragma warning(push)
// C4251 can be ignored in Visual C++ if you are deriving from a type in the Standard C++ Library
#pragma warning(disable:4251)
#pragma warning(disable:4312)
namespace simul
{
	namespace graph
	{
		namespace exporttable
		{
#ifdef WIN64
			typedef unsigned __int64 int_or_ptr;
#else
			typedef unsigned long int_or_ptr;
#endif
			struct FunctionReturnValue
			{
				const FunctionReturnValue& operator=(int_or_ptr val)
				{
					ret_val=val;
					return *this;
				}
				const FunctionReturnValue& operator=(float val)
				{
					float_ret=val;
					return *this;
				}
				operator float() const
				{
					return float_ret;
				}
				operator __int64() const
				{
					return ret_val;
				}
				operator int() const
				{
					return (int)ret_val;
				}
				operator unsigned int() const
				{
					return (unsigned)ret_val;
				}
				operator int_or_ptr() const
				{
					return ret_val;
				}
				operator void *() const
				{
					return (void *)ret_val;
				}
				operator const void *() const
				{
					return (const void *)ret_val;
				}
			protected:
				int_or_ptr ret_val;
				float float_ret;
				friend struct ExportFunction;
			};
			struct EXPORTTABLE_EXPORT ExportFunction
			{
				ExportFunction();
				typedef std::vector<std::string> ParamVec;
				std::string name;
				void* proc;
				std::string return_type;
				ParamVec param_types;
				std::string calling_convention;
				bool Virtual;
				bool VirtualImplementation;
				bool Static;
				FunctionReturnValue Invoke(void* this_pointer,...) const;
			protected:
			};
		}
	}
}
#pragma warning(pop)
#endif