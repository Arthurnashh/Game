#include "Simul/Graph/ExportTable/ExportTable.h"
#include "Simul/Base/RealTimeTypes.h"
#include <iostream>
#include <fstream>
#include <assert.h>
#include <windows.h>

#include <ios>
#include <direct.h>
#pragma warning(disable:4748)
// C4512 - "assignment operator could not be generated". Boost classes cause this warning.
// Also C4127 - "conditional expression is constant"
#pragma warning(push)
#pragma warning(disable:4512)
#pragma warning(disable:4127)
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#pragma warning(pop)

#include "Simul/Graph/ExportTable/pedump.h"
#include "Simul/Base/StringFunctions.h"
#include "Simul/Base/Modules.h"
#include "CallBinding.h"

using namespace simul::base;
using namespace simul::graph::exporttable;
using namespace boost::algorithm;
using namespace std;
ExportTable ExportTable::instance;
bool ShowInfo=false;
typedef vector<string> string_vector;
ExportTable*callbackExportTable=NULL;

void export_callback(const char *test,void *proc)
{
	if(callbackExportTable)
		callbackExportTable->AddDefinition(test,proc);
}

void ExportTable::AddClassFromPointer(const void *class_instance_ptr)
{	
	const RTTICompleteObjectLocator *col=GetRTTICompleteObjectLocatorFromClassPtr(class_instance_ptr);
	const type_info *ti=GetRTTITypeInfo(col);//col->pTypeDescriptor;
	if(!ti)
		return;
	if(!ti->name())
		return;
	string undec=ti->name();
	boost::algorithm::replace_all(undec,"class ","");
	boost::algorithm::replace_all(undec,"struct ","");
	ExportClass *C=&(classes[undec]);
	C->name=undec;
	C->FillInBaseClasses(col);
}

void ExportTable::AddClass(const std::string &class_name)
{
	classes[class_name];
}

void ExportTable::AddClassFromTypeInfo(const type_info *ti)
{
	if(!ti)
		return;
	if(!ti->name())
		return;
	string undec=ti->name();
	boost::algorithm::replace_all(undec,"class ","");
	boost::algorithm::replace_all(undec,"struct ","");
	ExportClass *C=&(classes[undec]);
	const RTTICompleteObjectLocator *col=GetRTTICompleteObjectLocator(*((int**)ti));
	C->FillInBaseClasses(col);
}

/*
Vtable layout

Each vtable consists of the following parts,in the order listed.
All offsets in a vtable are of type ptrdiff_t.

"Virtual base offsets" are used to access the virtual bases of an
object and to non virtual bases when pointer adjustment is required  
for some overriden virtual function. Each entry is a displacement to  
a virtual base subobject from the location within the object of the  
vtable pointer that addresses this vtable. These entries are only
necessary if the class directly or indirectly inherits from virtual  
base classes,or if it overrides a virtual function defined in a base  
class and if  adjustment from the based to the derived class is
needed. The values can be positive or negative.

"Offset to top" holds the displacement to the top of the object from  
the location within the object of the vtable pointer that addresses  
this vtable. A negative value indicates the vtable pointer is part of  
an embedded base class subobject;otherwise it is zero. The offset
provides a way to find the top of the object from any base subobject  
with a vtable
pointer. This is necessary for dynamic_cast<void*> in particular.
The virtual pointer points to the "offset to top" field.

"Typeinfo pointer" points to the typeinfo object used for RTTI. All  
entries in each of the vtables for a given class point to the same
typeinfo object.

"Duplicate base information pointer" points to a table used to
perform runtime disambiguation of duplicate base classes for dynamic  
casts. The entries in each of the vtables for a given class do not
all point to the same table of duplicate base information.
*/

void ExportTable::AddDefinition(const string &Def,void *proc)
{
	std::string calling_convention;
	std::string access_keyword;
	std::string name;
	std::string params;
	std::string remainder;
	std::string return_type;
	std::string def=Def;
	std::string long_string="std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> >";
	string test=boost::algorithm::replace_all_copy(Def,long_string,"std::string");

	if(test.find("::`vftable")<test.length())
	{
		// IF this is a vftable of a derived class for a base class,it will contain a
		// null-terminated list of the function pointers.
		string_vector SplitVec;// #2: Search for tokens
		boost::algorithm::split(SplitVec,test,is_any_of("`"));// SplitVec=={ "hello abc","ABC","aBc goodbye" }
		remainder=SplitVec[0];
		string named_base_class;
		if(SplitVec.size()>2)
			if(SplitVec[2].length()>2)
				named_base_class=SplitVec[2].substr(0,SplitVec[2].length()-2);
		//remainder=test;
		string const_string=ExtractNextName(remainder);
		string class_name;
		size_t last_pos=remainder.find_last_of(string("::"));
		class_name=remainder.substr(0,last_pos-1);
		boost::algorithm::replace_all(class_name,"class ","");
		boost::algorithm::replace_all(class_name,"struct ","");
		ExportClass &C=classes[class_name];
		C.module=current_module;
		C.name=class_name;
		if(!proc)
			return;
		const RTTICompleteObjectLocator *col=GetRTTICompleteObjectLocator((int*)proc);
		C.completeObjectLocator=col;
		if(!col)
			return;
		const simul::base::RTTIClassHierarchyDescriptor* cd=GetRTTIClassHierarchyDescriptor(col);
		if(!cd)
			return;
		if(!cd->pBaseClassArray)
			return;
		//const simul::base::RTTIBaseClassDescriptor*const* bcd_array=GetRTTIBaseClassArray(cd);
		// A class with either one or zero bases (with virtual functions)
		// will not name the base class.
		if(named_base_class=="")
		{
			 def=Def;
		}
		// For each base class of the main class:
		int offset=0;
		for(unsigned i=1;i<cd->numBaseClasses;i++)
		{
			const RTTIBaseClassDescriptor* bcd=cd->GetBaseClass(i);//bcd_array[i];
			const type_info *ti=GetRTTITypeInfo(bcd);//(bcd)->pTypeDescriptor;
			// From type_info ti now get or create this class if necessary:
			if(!ti)
				continue;
			if(!ti->name())
				continue;
			string undec=ti->name();
			boost::algorithm::replace_all(undec,"class ","");
			boost::algorithm::replace_all(undec,"struct ","");
			ExportClass *base_class=&(classes[undec]);
			base_class->name=undec;
			if(undec=="")
				continue;
			const RTTICompleteObjectLocator *col2=GetRTTICompleteObjectLocator(*((int**)ti));
			if(!col2)
				continue;
			if(named_base_class==undec)
				offset=bcd->Where.mdisp;
		}
		// For class C,describe its base classes:
		C.FillInBaseClasses(col);
		BaseClass *this_base=NULL;
		if(named_base_class!="")
		{
			this_base=C.GetBaseClass(named_base_class);
			if(!this_base)
			{
				this_base=C.AddBaseClass(named_base_class,false);
			}
			this_base->offset=offset;//bcd->Where.mdisp;UNNECESSARY??
			//this_base->immediate=false;
			if(classes.find(named_base_class)==classes.end())
			{
				classes[named_base_class].module=current_module;// if we're CREATING it here!
				classes[named_base_class].name=named_base_class;
			}
			this_base->vftable=proc;
		}
		else
		{
			// apply this vftable to all the bases of this class
			for(int i=0;i<C.GetNumBaseClasses();i++)
			{
				C.GetBaseClass(i)->vftable=proc;
			}
			C.vftable=proc;
			C.table_size=0;
			void** vftable_iterator=(void**)proc;
			void* virtual_proc=*(vftable_iterator);
			while(virtual_proc)
			{
				vftable_iterator++;
				virtual_proc=*(vftable_iterator);
				C.table_size++;
			}
		}
		return;
	}

	// we should ALWAYS have a a calling convention.

	size_t cc_pos;
	if((cc_pos=test.find("__thiscall"))<test.length())
	{
		calling_convention="__thiscall";
	}
	else if((cc_pos=test.find("__stdcall"))<test.length())
	{
		calling_convention="__stdcall";
	}
	else if((cc_pos=test.find("__cdecl"))<test.length())
	{
		calling_convention="__cdecl";
		// but in Win64/x64, even non-static member functions
		// CLAIM to use __cdecl, although they actually use the Win64
		// calling convention, which is like __fastcall.
	}
	else
		return;

	string before_cc=test.substr(0,cc_pos);
	size_t end_cc=cc_pos+calling_convention.length();
	string after_cc=test.substr(end_cc,test.length()-end_cc);

	if(before_cc.find(": ")<before_cc.length())
		GetDelimited(before_cc,access_keyword,remainder,": ");
	else
		remainder=before_cc;

// We only store public functions:
	if(access_keyword!="public"&&access_keyword!=""&&
		(access_keyword!="protected"||after_cc.find("~")>=after_cc.length()))
		return;
	simul::base::RefPtr<ExportFunction> fn;
	fn.reset(new ExportFunction);
	fn->Virtual=false;
	fn->Static=false;

	std::string word=NextName(remainder);
	if(word=="virtual")
	{
		ExtractNextName(remainder);
		fn->Virtual=true;
		word=NextName(remainder);
	}
	else if(word=="static")
	{
		ExtractNextName(remainder);
		fn->Static=true;
		word=NextName(remainder);
	}
	return_type=trim_copy(remainder);

	boost::algorithm::replace_all(return_type,"class ","");
	boost::algorithm::replace_all(return_type,"struct ","");
	
	remainder=after_cc;
	GetDelimited(remainder,name,remainder,'(');
	boost::algorithm::trim(name);
	GetDelimited(remainder,params,remainder,')');
	fn->name=name;
	fn->proc=proc;
	fn->return_type=return_type;
	remainder=params;
	std::string this_param;
	while((this_param=ExtractDelimited(remainder,",",true))!="")
	{
		boost::algorithm::trim(this_param);
		if(this_param=="void")
			continue;
		boost::algorithm::replace_all(this_param,"class ","");
		boost::algorithm::replace_all(this_param,"struct ","");
		fn->param_types.push_back(this_param);
	}

	// No access keyword - it's not a member function
	if(access_keyword=="")
	{
		functions[test]=fn;
		fn->calling_convention=calling_convention;
#ifdef WIN64
		fn->calling_convention="win64_call";
#endif
//	if(calling_convention!="__thiscall"&&calling_convention!="win64_thiscall"&&!fn->Static)
	
	//	functions[test]=fn;
		// is this function a string-converter for a class?
		// ConvertToString function must:
		// be called "ConvertToString"
		// return a std::string
		// take a single parameter,being a const ptr to the class instance.
		// e.g. std::string ConvertToString(const classtype *)
		if(name=="ConvertToString"&&fn->return_type=="char const *"&&fn->param_types.size()==1)
		{
			std::string param=fn->param_types[0];
			if(param.find(" const *")<param.length())
			{
				std::string class_name=param;
				boost::algorithm::replace_all(class_name,"class ","");
				boost::algorithm::replace_all(class_name,"struct ","");
				boost::algorithm::replace_all(class_name," const *","");
				classes[class_name].ConvertToString=functions[test];
			}
		}
		// e.g. std::string ConvertFromString(const classtype *)
		if(name=="ConvertFromString"&&fn->return_type=="void"&&fn->param_types.size()==2)
		{
			std::string param1=fn->param_types[0];
			std::string param2=fn->param_types[1];
			if(param1.find("*")<param1.length()&&param2=="char const *")
			{
				std::string class_name=param1;
				boost::algorithm::replace_all(class_name," *","");
				boost::algorithm::replace_all(class_name,"class ","");
				boost::algorithm::replace_all(class_name,"struct ","");
				classes[class_name].ConvertFromString=functions[test];
			}
		}
		// e.g. std::string ConvertFromString(const classtype *)
		if(name=="ConstFromString"&&fn->return_type!="void"&&fn->param_types.size()==1)
		{
			std::string param1=fn->param_types[0];
			if(param1=="char const *")
			{
				std::string class_name=fn->return_type;
				boost::algorithm::replace_all(class_name," const &","");
				boost::algorithm::replace_all(class_name,"class ","");
				boost::algorithm::replace_all(class_name,"struct ","");
				classes[class_name].ConstFromString=functions[test];
			}
		}
		return;
	}
#ifdef WIN64
	if(fn->Static)
		calling_convention="win64_call";
	else
		calling_convention="win64_thiscall";
#endif
	fn->calling_convention=calling_convention;

	// first get the class name from the overall function name:
	// (we already know it's a __thiscall,so the last word before ::function()
	// is the class name,not part of the namespace)
	string class_name;
	size_t last_pos=name.find_last_of(string("::"));
	class_name=name.substr(0,last_pos-1);
	string member_name=name.substr(last_pos+1,name.length()-last_pos-1);
	
	ExportClass &C=classes[class_name];
	C.name=class_name;
	C.module=current_module;
	fn->name=member_name;
	if(C.functions.find(member_name)!=C.functions.end())
	{
		simul::base::RefPtr<ExportFunction> fn2=C.functions[member_name];
		// already a function with that name!
		if(fn2->name!="multiple")
		{
			if(fn2->return_type==fn->return_type&&
				fn2->param_types==fn->param_types)
				return;
			std::string member_name_2=fn2->return_type+"-"+fn2->name;
			C.functions[member_name_2]=fn2;
			if(fn2->name=="Creator")
				C.functions[member_name_2]=fn2;

			fn2->name="multiple";
			fn2->proc=NULL;
		}
		member_name=return_type+"-"+member_name;
	}
	C.functions[member_name]=fn;
}

const ExportFunction *ExportTable::GetFunction(const std::string &function_name)
{
	if(functions.find(function_name)==functions.end())
		return NULL;
	return functions[function_name].get();
}

const ClassMap &ExportTable::GetClasses() const
{
	return classes;
}

ExportClass *ExportTable::GetClass(const std::string &class_name)
{
	if(classes.find(class_name)==classes.end())
		return NULL;
	ExportClass &c=classes[class_name];
	return &c;
}

ExportTable::ExportTable()
{
	initialized=false;
}

static const string &GetCurrentExeFilename()
{
	static string ret;
	static char fn[1000];
	HMODULE hModule=NULL;
	GetModuleFileName(
		hModule,
		fn,
		1000
		);
	ret=fn;
	return ret;
}
void ExportTable::Initialize()
{
	initialized=true;
	std::string Filename;
	Filename=GetCurrentExeFilename();
 	callbackExportTable=this;
	current_module=Filename.c_str();
#ifdef SIMUL_DLL
	// Because ExportTable.dll depends on SimulBase,it will already be resident.
	#ifdef _DLL
		#ifdef _DEBUG
			HandleLibrary("SimulBase_MDd.dll");
		#else
			HandleLibrary("SimulBase_MD.dll");
		#endif
	#else
		#ifdef _DEBUG
			HandleLibrary("SimulBase_MTd.dll");
		#else
			HandleLibrary("SimulBase_MT.dll");
		#endif
	#endif
#endif
	HandleLibrary(Filename.c_str());
	CheckForChanges();
}


void ExportTable::CheckForChanges()
{
#ifdef SIMUL_DLL
	for(size_t i=0;i<simul::base::GetNumModules();i++)
	{
		// HandleLibrary will make sure it's not already handled:
		HandleLibrary(simul::base::GetModuleName(i));
	}	
	MatchVirtualFunctions();
	MakeProperties();
#endif
}

// the closest class to derived that implements a table for main_base:
const void *ExportTable::FindIntermediateBaseVFTable(string derived,string main_base)
{
	ExportClass *C=&(classes[derived]);
	/// does any immediate base implement?
	for(int i=0;i<C->GetNumBaseClasses();i++)
	//for(BaseClassMap::iterator i=C->base_class_map.begin();i!=C->base_class_map.end();i++)
	{
		BaseClass *base_class=C->GetBaseClass(i);
		std::string base_name=C->GetBaseClassName(i);
		ExportClass *base_C=&(classes[base_name]);
		if(!base_class->immediate)
			continue;
		if(!base_C->GetBaseClass(main_base))
			continue;
		BaseClass *base_of_this=base_C->GetBaseClass(main_base);
		if(base_of_this->vftable)
			return base_of_this->vftable;
		const void *temp=FindIntermediateBaseVFTable(base_name,main_base);
		if(temp)
			return temp;
	}
	return NULL;
}
void ExportTable::MatchVirtualFunctions()
{
	for(ClassMap::iterator i=classes.begin();i!=classes.end();i++)
	{
		i->second.MatchVirtualFunctions();
	}
	for(ClassMap::iterator i=classes.begin();i!=classes.end();i++)
	{
		i->second.CompleteVirtualFunctions();
	}
}

void ExportTable::MatchVirtualFunctions(const std::string &mod)
{
	for(ClassMap::iterator i=classes.begin();i!=classes.end();i++)
	{
		if(i->second.module==mod)
			i->second.MatchVirtualFunctions();
	}
	for(ClassMap::iterator i=classes.begin();i!=classes.end();i++)
	{
		if(i->second.module==mod)
			i->second.CompleteVirtualFunctions();
	}
}

void ExportTable::MakeProperties()
{
	for(ClassMap::iterator i=classes.begin();i!=classes.end();i++)
	{
		ExportClass &C=i->second;
		if(ShowInfo)
			cout<<endl<<"Properties for class "<<C.name<<endl;
		for(FnMap::iterator j=C.functions.begin();j!=C.functions.end();j++)
		{
			simul::base::RefPtr<ExportFunction> fn=j->second;
			if(fn.get())
			if(!fn->VirtualImplementation)
			{
				C.CheckForPropertyFunction(fn);
				C.CheckForActionFunction(fn);
			}
		}
		C.Sort();
	}
}

ExportTable &ExportTable::GetSingletonInstance()
{
	if(!instance.initialized)
		instance.Initialize();
	return instance;
}

void ExportTable::HandleLibrary(const string &filename)
{
	filename;
	if(handled_modules.find(filename)!=handled_modules.end())
	{
		return;
	}
	string path_filename;
	HMODULE hModule=GetModuleHandle(filename.c_str());
	// Are we SURE it's not loaded? what about the pointer?
	for(std::map<std::string,void*>::const_iterator i=handled_modules.begin();
		i!=handled_modules.end();i++)
	{
		if(hModule==(HMODULE)(i->second))
			return;
	}
	if(!hModule)
		return;
	char full_path[512];
	LPTSTR lpFilename=full_path;
	GetModuleFileName(
		hModule,
		lpFilename,
		512
		);
	path_filename=full_path;
	handled_modules[filename]=hModule;
	current_module=filename;
	DumpFile(hModule,(path_filename).c_str(),export_callback);
#if 0
	MatchVirtualFunctions(current_module);
	MakeProperties();
#endif
}

void ExportTable::LoadDynamicLibrary(const string &Filename)
{
	string_vector SplitVec;// Search for tokens
	boost::algorithm::split(SplitVec,Filename,is_any_of("/\\"));
	string filename=SplitVec[SplitVec.size()-1];
	string backslash_filename=boost::algorithm::replace_all_copy(Filename,"/","\\");

    HMODULE hModule=LoadLibraryEx(backslash_filename.c_str(),NULL,0);//was path_filename
	if(!hModule)
	{
		if(ShowInfo)
		{
			char buffer[100];
			DWORD error=GetLastError();
			FormatMessage(	FORMAT_MESSAGE_FROM_SYSTEM,
										NULL,
										error,
										LANG_USER_DEFAULT,
										buffer,
										100,
										NULL);
			cerr<<buffer<<endl;
		}
		return;
	}
	// We have loaded a new library. But it may have caused others to be loaded.
	// If they are Simul libraries,their dllMain functions will have added them to
	// the simul::base::modules list.

	// So now we handle ALL the libraries that are in modules but not in handled_modules.
	for(size_t i=0;i<simul::base::GetNumModules();i++)
	//for(std::map<std::string,void*>::const_iterator i=modules.begin();i!=modules.end();i++)
	{
		// HandleLibrary will make sure it's not already handled:
		HandleLibrary(simul::base::GetModuleName(i));
	}	
}
void ExportTable::UnloadDynamicLibrary(const string &name)
{
	void *hInstDLL=simul::base::GetModule(name.c_str());
	if(!hInstDLL)
		return;
	for(ClassMap::iterator i=classes.begin();i!=classes.end();i++)
	{
		if(i->second.module==name)
		{
			classes.erase(i);
			i=classes.begin();
		}
	}
	FreeLibrary((HMODULE)(hInstDLL));
	simul::base::UnloadModule(name.c_str());
}

ExportTable::~ExportTable()
{
}

/*bool Find(istream &is,const char *match)
{
	size_t len=strlen(match);
	size_t match_index=0;
	char c=0;
	unsigned index=0;
	static unsigned pos;
	while(!is.eof())
	{
		is.read(&c,sizeof(c));
	pos=is.tellg();
		index++;
		if(c==match[match_index])
		{
			match_index++;
			if(match_index==len)
				break;
		}
		else
			match_index=0;
	}
	if(match_index!=len)
		return false;
	return  true;
}
*/
