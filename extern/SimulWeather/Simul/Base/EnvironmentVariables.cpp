#include "precomp.h"

#include "Simul/Base/EnvironmentVariables.h"
#ifdef __BORLANDC__
#include <stdlib.h>
#define _putenv putenv
#endif
#if !defined(SN_TARGET_PS3)&&!defined(__GNUC__)
#include <direct.h>
#endif 
#include <errno.h>
 
#ifdef MSC_VER
#pragma warning(disable:4748)
#endif

using namespace simul::base;
std::string EnvironmentVariables::GetSimulEnvironmentVariable(const std::string &name)
{
#ifdef WIN32
	size_t sizeNeeded;
	char buffer[1];
	errno_t err=getenv_s(&sizeNeeded,
				buffer,
				1,
				name.c_str());
	std::string ret="";
	if(err!=0&&sizeNeeded>0)
	{
		char *txt=new char[sizeNeeded];
		err=getenv_s(&sizeNeeded,
				txt,
				sizeNeeded,
				name.c_str()); 
		if(err!=EINVAL)
			ret=(txt);
		delete [] txt;
	}
	return ret;
#else
	name;
	return std::string("");
#endif
}
std::string EnvironmentVariables::SetSimulEnvironmentVariable(const std::string &name,const std::string &value)
{
#ifdef WIN32
	std::string str=name+"=";
	str+=value;
	_putenv(str.c_str());
#else
	name;
	value;
#endif
	return value;
}

const std::string &EnvironmentVariables::GetWorkingDirectory()
{
	static std::string str;
#ifdef WIN32
	char buffer[_MAX_PATH];
	if(_getcwd(buffer,_MAX_PATH))
	{
		str=buffer;
	}
	else
		str="";
#endif
	return str;
}

void EnvironmentVariables::SetWorkingDirectory(std::string dir)
{
#ifdef WIN32
	_chdir(dir.c_str());
#endif
}

std::string EnvironmentVariables::AppendSimulEnvironmentVariable(const std::string &name,const std::string &value)
{
	std::string str=name+"=";
#ifdef WIN32
	str+=value;
		str+=";";
	str+=getenv(name.c_str());
	_putenv(str.c_str());
#else
	name;
	value;
#endif
	return str;
}
