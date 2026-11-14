#ifndef SIMUL_BASE_ENVIRONMENTVARIABLES_H
#define SIMUL_BASE_ENVIRONMENTVARIABLES_H
#include <string>
#include "Simul/Base/Export.h"
namespace simul
{
	namespace base
	{
		//! A namespace to manage environment variables.
		SIMUL_BASE_EXPORT_CLASS EnvironmentVariables
		{
		public:
		//! Get the named environment variable (Windows only, other platforms return an empty string.
			static std::string GetSimulEnvironmentVariable(const std::string &name);
		//! Set the named environment variable for this process.
			static std::string SetSimulEnvironmentVariable(const std::string &name,const std::string &value);
			static std::string AppendSimulEnvironmentVariable(const std::string &name,const std::string &value);
		//! Get the current directory.
			static const std::string &GetWorkingDirectory();
		//! Set the current working directory.
			static void SetWorkingDirectory(std::string);
		};
	}
}
#endif
