#ifndef SIMUL_BASE_REALTIMETYPES_H
#define SIMUL_BASE_REALTIMETYPES_H
#if defined(WIN32) || defined(WIN64)
#include <string>

#ifndef _CPPRTTI
#error RTTI Should be enabled if RealTimeTypes.h is included.
#endif

#if !defined(__GNUC__)
#include <typeinfo.h>		//C++ RTTI
#else
#include <typeinfo>			//C++ RTTI
#endif
#ifdef WIN64
typedef unsigned int ti_ptr;
#else
typedef type_info* ti_ptr;
#endif
#include "Simul/Base/Export.h"
#ifdef __GNUC__
	typedef std::type_info type_info;
#endif

typedef unsigned long DWORD;

namespace simul
{
	namespace base
	{
		struct RTTIClassHierarchyDescriptor;
		struct RTTIBaseClassDescriptor;
		#ifdef WIN64
			typedef unsigned int RTTIClassHierarchyDescriptor_ptr;
			typedef unsigned int RTTIBaseClassArray_ptr;
			typedef unsigned int RTTIBaseClassDescriptor_ptr;
		#else
			typedef RTTIClassHierarchyDescriptor* RTTIClassHierarchyDescriptor_ptr;
			typedef RTTIBaseClassDescriptor** RTTIBaseClassArray_ptr;
			typedef RTTIBaseClassDescriptor* RTTIBaseClassDescriptor_ptr;
		#endif
#pragma pack(push,1)
#pragma pack(1)
		/*
		RTTI (Run-Time Type Identification) is special compiler-generated information which is
		used to support C++ operators like dynamic_cast<> and typeid(), and also for C++ exceptions.
		Due to its nature, RTTI is only required (and generated) for polymorphic classes,
		i.e. classes with virtual functions.

		MSVC compiler puts a pointer to the structure called "Complete Object Locator" just before
		the vftable. The structure is called so because it allows compiler to find the location of
		the complete object from a specific vftable pointer (since a class can have several of them).
		*/
		struct RTTICompleteObjectLocator
		{
			DWORD signature;									//always zero ?
			DWORD offset;										//offset of this vtable in the complete class
			DWORD cdOffset;										//constructor displacement offset
			ti_ptr pTypeDescriptor;								//TypeDescriptor of the complete class
			RTTIClassHierarchyDescriptor_ptr pClassDescriptor;	//describes inheritance hierarchy
		};

		struct PMD
		{
			int mdisp;  //member displacement
			int pdisp;  //vbtable displacement
			int vdisp;  //displacement inside vbtable
		};

		//Base Class Array describes all base classes together with information which allows compiler to cast the derived class to any of them during execution of the _dynamic_cast_ operator. Each entry (Base Class Descriptor) has the following structure:

		struct RTTIBaseClassDescriptor
		{
			ti_ptr pTypeDescriptor;		//type descriptor of the class
			DWORD numContainedBases;		//number of nested classes following in the Base Class Array
			PMD Where;						//pointer-to-member displacement info
			DWORD attributes;				//flags, usually 0
		};

		//Class Hierarchy Descriptor describes the inheritance hierarchy of the class. It is shared by all COLs for a class.

		struct SIMUL_BASE_EXPORT RTTIClassHierarchyDescriptor
		{
			DWORD signature;      //always zero?
			DWORD attributes;     //bit 0 set = multiple inheritance, bit 1 set = virtual inheritance
			DWORD numBaseClasses; //number of classes in pBaseClassArray
			RTTIBaseClassArray_ptr pBaseClassArray;
			const RTTIBaseClassDescriptor* GetBaseClass(size_t i) const;
		};
		extern SIMUL_BASE_EXPORT const type_info* GetRTTITypeInfo(const RTTICompleteObjectLocator *col);
		extern SIMUL_BASE_EXPORT const type_info* GetRTTITypeInfo(const RTTIBaseClassDescriptor *col);
		
		//extern SIMUL_BASE_EXPORT void GetBasesFromClassPtr(const void *class_instance_ptr,DWORD &numBaseClasses,RTTIBaseClassDescriptor*const * &pBaseClassArray);
		extern SIMUL_BASE_EXPORT const void *GetVftableFromClassPtr(const void *class_instance_ptr);
		extern SIMUL_BASE_EXPORT const RTTICompleteObjectLocator *GetRTTICompleteObjectLocatorFromClassPtr(const void *class_instance_ptr);
		extern SIMUL_BASE_EXPORT const RTTICompleteObjectLocator *GetRTTICompleteObjectLocator(const int *vftable);
		extern SIMUL_BASE_EXPORT const RTTIClassHierarchyDescriptor *GetRTTIClassHierarchyDescriptor(const RTTICompleteObjectLocator *col);
		SIMUL_BASE_EXPORT const char *GetQualifiedTypeName(const void *class_ptr);//< Return the qualified type name of the class instance (not the pointer but what it points to).
		SIMUL_BASE_EXPORT const char *GetUnqualifiedTypeName(const void *class_ptr);//< Return the unqualified type name of the class instance (not the pointer but what it points to).
		SIMUL_BASE_EXPORT const char *GetQualifiedInheritancePath(const void *class_ptr,const char *root);//< Return the direct path of inheritance from "root" to this class, fully namespace-qualified, delimited by dots.
		SIMUL_BASE_EXPORT const char *GetQualifiedInheritancePathFromVftable(const void *vftable,const char *root);
		SIMUL_BASE_EXPORT const char *GetQualifiedInheritancePathFromCompleteObjectLocator(const RTTICompleteObjectLocator *,const char *root);
		extern SIMUL_BASE_EXPORT std::string PtrNameToTypeName(const char *typename_of_ptr);
#pragma pack(pop)
	}
}

/// This macro acts like a function that fills in a std::string \a name containing the fully-qualified
/// type name of the class parameter.
#define TYPE_NAME(Class)				\
	simul::base::PtrNameToTypeName(typeid((Class*)NULL).name())
#endif
#endif

