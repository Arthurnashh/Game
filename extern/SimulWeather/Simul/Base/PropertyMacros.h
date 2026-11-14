#ifndef SIMUL_BASE_PROPERTYMACROS_H
#define SIMUL_BASE_PROPERTYMACROS_H


#include <iostream>
#ifdef __GNUC__
// NOTE: gcc expects spaces after the commas in macros. No reason why.
#ifndef __COUNTER__
	#define __COUNTER__ __LINE__
#endif
    #define __FUNCTION__ __FUNC__
// also, gcc does NOT handle the pasting operator ## well. avoid if possible.
#endif
#ifdef _MSC_VER	// Using Visual Studio? Then this pragma works:
# pragma warning(push)
# pragma warning(disable:4284) // odd return type for operator->
#endif
// Note: __COUNTER__ is a Microsoft compiler macro that increments each time it is used.
// From gcc version 4.3,  __COUNTER__ also works in gcc!

template<class T> struct StreamingHelperTemplate
{
	static std::ostream &StreamOut(std::ostream &os, const T &t)
	{
		os.write((const char*)(&t), sizeof(t));
		return os;
	}
	static std::istream &StreamIn(std::istream &is, T &t)
	{
		is.read((char*)(&t), sizeof(t));
		return is;
	}
};

template<> struct StreamingHelperTemplate<std::string>
{
	static std::ostream &StreamOut(std::ostream &os, const std::string &t)
	{
		size_t len=t.length();
		os.write((const char*)(&len),		sizeof(size_t));
		if(len)
			os.write((const char*)(t.c_str()),	(std::streamsize)(len*sizeof(char)));
		return os;
	}
	static std::istream &StreamIn(std::istream &is, std::string &t)
	{
		size_t len=0;
		is.read((char*)(&len),sizeof(size_t));
		if(!len)
			return is;
		char *temp_str=new char[len];
		is.read(temp_str,(std::streamsize)(len*sizeof(char)));
		t=temp_str;
		delete [] temp_str;
		return is;
	}
};

template<> struct StreamingHelperTemplate<bool>
{
	static std::ostream &StreamOut(std::ostream &os, const bool &t)
	{
		bool temp=t;
		unsigned len=sizeof(temp);
		os.write((const char*)(&temp), len);
		return os;
	}
	static std::istream &StreamIn(std::istream &is, bool &t)
	{
		bool temp;
		unsigned len=sizeof(temp);
		is.read((char*)(&temp), len);
		t=temp;
		return is;
	}
};

// overloaded in META_EndProperties	
inline void StreamOutAutoProperties(std::ostream &)		
{															
}															
inline void StreamInAutoProperties(std::istream &)				
{															
}								

inline void StreamOutAutoPropertiesState(std::ostream &)		
{															
}															
inline void StreamInAutoPropertiesState(std::istream &)				
{															
}

template<class T> void META_Limit(T &t, T minimum, T maximum)
{
	if(t<minimum)
		t=minimum;
	if(t>maximum)
		t=maximum;
}		

template<class T, T minimum, T maximum> void META_StaticLimit(T &t)
{
	if(t<minimum)
		t=minimum;
	if(t>maximum)
		t=maximum;
}

#define META_DeclareMaxAndMin(type, propname, minimum, maximum)			\
	public:																\
		/*! Get the largest value of propname for this class.*/			\
		static type GetMaximumOf##propname()							\
		{																\
		return (maximum);												\
		}																\
	public:																\
		/*! Get the smallest value of propname for this class.*/		\
		static type GetMinimumOf##propname()							\
		{																\
			return (minimum);											\
		}

#if !defined(DOXYGEN)
#define META_DeclareEnforceRange(type, propname, minimum, maximum)	\
	protected:														\
		void EnforceRangeOf##propname()								\
		{															\
			META_Limit<type>(propname, minimum, maximum);			\
		}															\
		META_DeclareMaxAndMin(type, propname, minimum, maximum)
		

#define META_PropertyOfTypeGetAndSet(type, propname, info)								\
	protected:																			\
		type propname;																	\
	public:																				\
		/*! Get the value of propname (info) */											\
		const type &Get##propname() const												\
		{																				\
			return propname;															\
		}																				\
		/*! Set the value of propname (info) */											\
		void Set##propname(const type &value)											\
		{																				\
			propname=value;																\
		}

#define META_PropertyOfTypeGetSetAndCall(type, propname, call_on_set)					\
	protected:																			\
		type propname;																	\
	public:																				\
		/*! Get the value of propname.*/												\
		const type &Get##propname() const												\
		{																				\
			return propname;															\
		}																				\
		/*! Set the value of propname, then call call_on_set().  */						\
		void Set##propname(const type &value)											\
		{																				\
			if(propname==value)															\
				return;																	\
			propname=value;																\
			call_on_set();																\
		}

#define META_ValueOfTypeGetAndSet(type, propname)										\
	protected:																			\
		type propname;																	\
	public:																				\
		/*! Get the value of propname.*/												\
		type Get##propname() const														\
		{																				\
			return propname;															\
		}																				\
		/*! Set the value of propname.*/												\
		void Set##propname(type value)													\
		{																				\
			propname=value;																\
		}																				\
		/*! Set the value of propname.*/												\
		void Init##propname(type value)													\
		{																				\
			propname=value;																\
		}

#define META_ValueOfTypeGetSetAndCall(type, propname, call_on_set)						\
	protected:																			\
		type propname;																	\
	public:																				\
		/*! Get the value of propname.*/												\
		type Get##propname() const														\
		{																				\
			return propname;															\
		}																				\
		/*! Set the value of propname, then call call_on_set().  */						\
		void Set##propname(type value)													\
		{																				\
			if(propname==value)															\
				return;																	\
			propname=value;																\
			call_on_set();																\
		}																				\
		/*! Set the value of propname, then call call_on_set().  */						\
		void Init##propname(type value)													\
		{																				\
			propname=value;																\
			call_on_set();																\
		}

#define META_ValueOfTypeGetSetAndCall2(type, propname, call_on_set1, call_on_set2)		\
	protected:																			\
		type propname;																	\
	public:																				\
		/*! Get the value of propname.*/												\
		type Get##propname() const														\
		{																				\
			return propname;															\
		}																				\
		/*! Set the value of propname, then call call_on_set1() and call_on_set2(). */	\
		void Set##propname(type value)													\
		{																				\
			if(propname==value)															\
				return;																	\
			propname=value;																\
			call_on_set1();																\
			call_on_set2();																\
		}																				\
		/*! Set the value of propname, then call call_on_set1() and call_on_set2(). */	\
		void Init##propname(type value)													\
		{																				\
			propname=value;																\
			call_on_set1();																\
			call_on_set2();																\
		}

#define META_ValueRangeOfTypeGetAndSet(type, propname, minimum, maximum)					\
	protected:																				\
		type propname;																		\
	public:																					\
		/*! Get the value of propname.*/													\
		type Get##propname() const															\
		{																					\
			return propname;																\
		}																					\
		META_DeclareMaxAndMin(type, propname, minimum, maximum)								\
		/*! Set the value of propname, then call call_on_set1() and call_on_set2(). */		\
		void Set##propname(type value)														\
		{																					\
			if(value<minimum)																\
				value=minimum;																\
			if(value>maximum)																\
				value=maximum;																\
			if(propname==value)																\
				return;																		\
			propname=value;																	\
		}																					\
		/*! Set the value of propname, then call call_on_set(). */		\
		void Init##propname(type value)														\
		{																					\
			propname=value;																	\
		}


#define META_ValueRangeOfTypeGetSetAndCall(type, propname, call_on_set, minimum, maximum)	\
	protected:																				\
		type propname;																		\
	public:																					\
		/*! Get the value of propname.*/													\
		type Get##propname() const															\
		{																					\
			return propname;																\
		}																					\
		META_DeclareMaxAndMin(type, propname, minimum, maximum)								\
		/*! Set the value of propname, then call call_on_set1() and call_on_set2(). */		\
		void Set##propname(type value)														\
		{																					\
			if(value<minimum)																\
				value=minimum;																\
			if(value>maximum)																\
				value=maximum;																\
			if(propname==value)																\
				return;																		\
			propname=value;																	\
			call_on_set();																	\
		}																					\
		/*! Set the value of propname, then call call_on_set(). */							\
		void Init##propname(type value)														\
		{																					\
			propname=value;																	\
			call_on_set();																	\
		}


#else
#define META_DeclareEnforceRange(type, propname, minimum, maximum)

#define META_PropertyOfTypeGetAndSet(type, propname, info)								\
	public:																				\
		/*! Get the value of propname (info).*/											\
		const type &Get##propname() const;												\
		/*! Set the value of propname (info).*/											\
		void Set##propname(const type &value);

#define META_PropertyOfTypeGetSetAndCall(type, propname, call_on_set)					\
	public:																				\
		/*! Get the value of propname.*/												\
		const type &Get##propname() const;												\
		/*! Set the value of propname.*/												\
		void Set##propname(const type &value);

#define META_ValueOfTypeGetAndSet(type, propname)										\
	public:																				\
		/*! Get the value of propname.*/												\
		type Get##propname() const;														\
		/*! Set the value of propname.*/												\
		void Set##propname(type value);

#define META_ValueOfTypeGetSetAndCall(type, propname, call_on_set)						\
	public:																				\
		/*! Get the value of propname.*/												\
		type Get##propname() const;														\
		/*! Set the value of propname.*/												\
		void Set##propname(type value);

#define META_ValueOfTypeGetSetAndCall2(type, propname, call_on_set1, call_on_set2)		\
	public:																				\
		/*! Get the value of propname.*/												\
		type Get##propname() const;														\
		/*! Set the value of propname.*/												\
		void Set##propname(type value);

#define META_ValueRangeOfTypeGetAndSet(type, propname, minimum, maximum)				\
	public:																				\
		/*! Get the value of propname.*/												\
		type Get##propname() const;														\
		/*! Set the value of propname.*/												\
		void Set##propname(type value);

#define META_ValueRangeOfTypeGetSetAndCall(type, propname, call_on_set, minimum, maximum)	\
	public:																					\
		/*! Get the value of propname.*/													\
		type Get##propname() const;															\
		/*! Set the value of propname.*/													\
		void Set##propname(type value);

#endif
			
// we want to accumulate a function that saves each property.
#if !defined(__GNUC__) && !defined(DOXYGEN)
#define META_BeginPropertiesOfType(proptype)													\
	private: template<int N> void Templ_AccumulateSetInitial##proptype()						\
	{																							\
	}																							\
	private: template<int N> void Templ_AccumulateStreamOut##proptype(std::ostream &) const		\
	{																							\
	}																							\
	private: template<int N> void Templ_AccumulateStreamIn##proptype(std::istream &)			\
	{																							\
	}

#define META_AccumulateStreaming(type, propname, proptype)											\
		static unsigned GetOrdinalOf##propname()													\
		{																							\
			return __COUNTER__;																		\
		}																							\
		template<> void Templ_AccumulateSetInitial##proptype<__COUNTER__>()							\
		{																							\
			Templ_AccumulateSetInitial##proptype<__COUNTER__-8>();									\
		}																							\
		template<> void Templ_AccumulateStreamOut##proptype<__COUNTER__>(std::ostream &os)	const	\
		{																							\
			Templ_AccumulateStreamOut##proptype<__COUNTER__-8>(os);									\
			StreamingHelperTemplate<type>::StreamOut(os, propname);									\
		};																							\
		template<> void Templ_AccumulateStreamIn##proptype<__COUNTER__>(std::istream &is)			\
		{																							\
			Templ_AccumulateStreamIn##proptype<__COUNTER__-8>(is);									\
			type temp##propname;																	\
			StreamingHelperTemplate<type>::StreamIn(is, temp##propname);							\
			Set##propname(temp##propname);															\
		};

					
#define META_AccumulateStreamingAndInit(type, propname, proptype, initial)							\
		static unsigned GetOrdinalOf##propname()													\
		{																							\
			return __COUNTER__;																		\
		}																							\
		template<> void Templ_AccumulateSetInitial##proptype<__COUNTER__>()							\
		{																							\
			Templ_AccumulateSetInitial##proptype<__COUNTER__-8>();									\
			Init##propname(initial);																\
		};																							\
		template<> void Templ_AccumulateStreamOut##proptype<__COUNTER__>(std::ostream &os)	const	\
		{																							\
			Templ_AccumulateStreamOut##proptype<__COUNTER__-8>(os);									\
			StreamingHelperTemplate<type>::StreamOut(os, propname);									\
		};																							\
		template<> void Templ_AccumulateStreamIn##proptype<__COUNTER__>(std::istream &is)			\
		{																							\
			Templ_AccumulateStreamIn##proptype<__COUNTER__-8>(is);									\
			type temp##propname;																	\
			StreamingHelperTemplate<type>::StreamIn(is, temp##propname);							\
			Set##propname(temp##propname);															\
		};

#else
#define META_BeginPropertiesOfType(proptype)
#define META_AccumulateStreaming(type, propname, proptype)
#define META_AccumulateStreamingAndInit(type, propname, proptype, initial)
	
#endif

#if !defined(DOXYGEN) && !defined(__GNUC__)
#define META_EndPropertiesOfType(proptype)												\
	public:																				\
	void StreamOutAutoProperties##proptype(std::ostream &os) const						\
	{																					\
		Templ_AccumulateStreamOut##proptype<__COUNTER__-4>(os);							\
	}																					\
	void StreamInAutoProperties##proptype(std::istream &is)								\
	{																					\
		Templ_AccumulateStreamIn##proptype<__COUNTER__-3>(is);							\
	}																					\
	void InitializeProperties##proptype()												\
	{																					\
		Templ_AccumulateSetInitial##proptype<__COUNTER__-8>();							\
	}

#else
#if !defined(DOXYGEN)
#define META_EndPropertiesOfType(proptype)												\
	public:																				\
	void StreamOutAutoProperties##proptype(std::ostream &os) const						\
	{																					\
	}																					\
	void StreamInAutoProperties##proptype(std::istream &is)								\
	{																					\
	}																					\
	void InitializeProperties##proptype()												\
	{																					\
	}
#else
#define META_EndPropertiesOfType(proptype)	
#endif


#endif

#ifndef DOXYGEN
#define META_Info(propname, info)														\
	static const char *GetInfoFor##propname()											\
	{																					\
		return info;																	\
	}
#else
#define META_Info(propname, info)

#endif

#define VIRTUAL_ValueGet(type, propname, info)					\
	public:														\
		/*! info */												\
		/*! \name Property: type propname	*/					\
		/*@{*/													\
		/*! Get the value of propname (info) */					\
		virtual type Get##propname() const = 0;					\
		META_Info(propname, info)								\
		/*@}*/

#define VIRTUAL_ValueSet(type, propname, info)					\
	public:														\
		/*! info */												\
		/*! \name Property: type propname	*/					\
		/*@{*/													\
		/*! Set the value of propname (info). */				\
		virtual void Set##propname(type value) = 0;				\
		META_Info(propname, info)								\
		/*@}*/

#define VIRTUAL_ValueGetAndSetNOINFO(type, propname)			\
	public:														\
		/*! info */												\
		/*! \name Property: type propname	*/					\
		/*@{*/													\
		/*! Get the value of propname. */						\
		virtual type Get##propname() const = 0;					\
		/*! Set the value of propname. */						\
		virtual void Set##propname(type value) = 0;				\
		/*@}*/

#define VIRTUAL_ValueGetAndSet(type, propname, info)			\
	public:														\
		/*! info */												\
		/*! \name Property: type propname	*/					\
		/*@{*/													\
		/*! Get the value of propname (info) */					\
		virtual type Get##propname() const = 0;					\
		/*! Set the value of propname (info) */					\
		virtual void Set##propname(type value) = 0;				\
		META_Info(propname, info)								\
		/*@}*/

#define VIRTUAL_ReferenceGetAndSet(type, propname, info)		\
	public:														\
		/*! info */												\
		/*! \name Property: type propname	*/					\
		/*@{*/													\
		/*! Get the value of propname (info) */					\
		virtual const type &Get##propname() const = 0;			\
		/*! Set the value of propname (info) */					\
		virtual void Set##propname(const type &value) = 0;		\
		META_Info(propname, info)								\
		/*@}*/


#if !defined(DOXYGEN) && !defined(__GNUC__)

#define META_PassOnStreaming(proptype)																\
		template<> void Templ_AccumulateSetInitial##proptype<__COUNTER__>()							\
		{																							\
			Templ_AccumulateSetInitial##proptype<__COUNTER__-8>();									\
		}																							\
		template<> void Templ_AccumulateStreamOut##proptype<__COUNTER__>(std::ostream &os)	const	\
		{																							\
			Templ_AccumulateStreamOut##proptype<__COUNTER__-8>(os);									\
		};																							\
		template<> void Templ_AccumulateStreamIn##proptype<__COUNTER__>(std::istream &is)			\
		{																							\
			Templ_AccumulateStreamIn##proptype<__COUNTER__-8>(is);									\
		};

	#define META_Category(category, proptype)												\
	/*! \name Category: category	*/														\
		public:																				\
			static unsigned BeginOrdinalOfCategory##category()								\
			{																				\
				return __COUNTER__;															\
			}																				\
			META_PassOnStreaming(proptype)

	#define META_EndCategory(category, proptype)											\
		public:																				\
			static unsigned EndOrdinalOfCategory##category()								\
			{																				\
				return __COUNTER__;															\
			}																				\
			META_PassOnStreaming(proptype)
#else
	
	#define META_PassOnStreaming(proptype)		

	#define META_Category(category, proptype) \
	/*! \name Category: category	*/

		
	#define META_EndCategory(category, proptype) \
	/*! \name General	*/

#endif

#define META_PropertyCategory(category)		META_Category(category, Definition)

#define META_EndPropertyCategory(category)	META_EndCategory(category, Definition)

#define META_StateCategory(category)		META_Category(category, State)

#define META_EndStateCategory(category)		META_EndCategory(category, State)

#define META_PropertyOfType(type, propname, proptype, info)									\
		/*! info */																			\
		/*! \name Property: type propname	*/												\
		/*@{*/																				\
		META_PropertyOfTypeGetAndSet(type, propname, info)									\
		META_AccumulateStreaming(type, propname, proptype)									\
		META_Info(propname, info)															\
		/*@}*/ 

#define META_ValuePropertyOfType(type, propname, proptype, info)							\
		/*! info */																			\
		/*! \name Property: type propname	*/												\
		/*@{*/																				\
		META_ValueOfTypeGetAndSet(type, propname)											\
		META_AccumulateStreaming(type, propname, proptype)									\
		META_Info(propname, info)															\
		/*@}*/ 

#define META_ValueRangePropertyOfType(type, propname, init, minimum, maximum, proptype, info)	\
		/*! info */																				\
		/*! \name Property: type propname	*/													\
		/*@{*/																					\
		META_ValueRangeOfTypeGetAndSet(type, propname, minimum, maximum)						\
		META_AccumulateStreamingAndInit(type, propname, proptype, init)							\
		META_Info(propname, info)																\
		/*@}*/ 

#define META_ValueRangePropertyOfTypeWithSetCall(type, propname, minimum, maximum, proptype, call_on_set, info)	\
		/*! info */																								\
		/*! \name Property: type propname	*/																	\
		/*@{*/																									\
		META_ValueRangeOfTypeGetSetAndCall(type, propname, call_on_set, minimum, maximum)						\
		META_AccumulateStreaming(type, propname, proptype)														\
		META_Info(propname, info)																				\
		/*@}*/
			

#define META_PropertyOfTypeWithSetCall(type, propname, proptype, call_on_set, info)				\
		/*! info */																				\
		/*! \name Property: type propname	*/													\
		/*@{*/																					\
		META_PropertyOfTypeGetSetAndCall(type, propname, call_on_set)							\
		META_AccumulateStreaming(type, propname, proptype)										\
		META_Info(propname, info)																\
		/*@}*/ 

#define META_ValuePropertyOfTypeWithSetCall(type, propname, proptype, call_on_set, info)		\
		/*! info */																				\
		/*! \name Properfty: type propname	*/													\
		/*@{*/																					\
		META_ValueOfTypeGetSetAndCall(type, propname, call_on_set)								\
		META_AccumulateStreaming(type, propname, proptype)										\
		META_Info(propname, info)																\
		/*@}*/ 

#define META_OpenPropertyOfType(type, propname, proptype, info)									\
		/*! info */																				\
		/*! \name Property: type propname	*/													\
		/*@{*/																					\
		META_PropertyOfTypeGetAndSet(type, propname, info)										\
		META_AccumulateStreaming(type, propname, proptype)										\
		META_Info(propname, info)																\
	public:																						\
		type &Get##propname()																	\
		{																						\
			return propname;																	\
		}																						\
		/*@}*/ 

#define META_BeginProperties												META_BeginPropertiesOfType(Definition)
#define META_BeginStates													META_BeginPropertiesOfType(State)

#define META_ReferenceProperty(type, propname, info)						META_PropertyOfType(type, propname, Definition, info)
#define META_ReferencePropertyWithSetCall(type, propname, set_call, info)	META_PropertyOfTypeWithSetCall(type, propname, Definition, set_call, info)
#define META_PropertyWithSetCall(type, propname, set_call, info)			META_PropertyOfTypeWithSetCall(type, propname, Definition, set_call, info)
#define META_ValueProperty(type, propname, info)							META_ValuePropertyOfType(type, propname, Definition, info)
#define META_ValueRangeProperty(type, propname, init, minimum, maximum, info)		META_ValueRangePropertyOfType(type, propname, init, minimum, maximum, Definition, info)
#define META_ValuePropertyWithSetCall(type, propname, set_call, info)		META_ValuePropertyOfTypeWithSetCall(type, propname, Definition, set_call, info)
#define META_ValueRangePropertyWithSetCall(type, propname, minimum, maximum, set_call, info)	\
																			META_ValueRangePropertyOfTypeWithSetCall(type, propname, minimum, maximum, Definition, set_call, info)

#define META_EndProperties													META_EndPropertiesOfType(Definition)
#define META_EndStates														META_EndPropertiesOfType(State)

#define META_ReferenceState(type, propname, info)							META_PropertyOfType(type, propname, State, info)
#define META_ReferenceStateWithSetCall(type, propname, set_call, info)		META_PropertyOfTypeWithSetCall(type, propname, State, set_call, info)
#define META_ValueStateWithSetCall(type, propname, set_call, info)			META_ValuePropertyOfTypeWithSetCall(type, propname, State, set_call, info)
#define META_ValueState(type, propname, info)								META_ValuePropertyOfType(type, propname, State, info)
#define META_ValueRangeState(type, propname, init, minimum, maximum, info)		META_ValueRangePropertyOfType(type, propname, init, minimum, maximum, State, info)
#define META_ValueRangeStateWithSetCall(type, propname, minimum, maximum, set_call, info)		\
																			META_ValueRangePropertyOfTypeWithSetCall(type, propname, minimum, maximum, State, set_call, info)
#define META_OpenState(type, propname, info)								META_OpenPropertyOfType(type, propname, State, info)

#define VIRTUAL_Value(type, propname)															\
		VIRTUAL_ValueGetAndSet(type, propname)

#define Macro_EnableRecursiveStreaming(parent_node_class)										\
		virtual std::ostream &RecursiveStreamOut(std::ostream &os, bool include_subgraph) const	\
		{																						\
			parent_node_class::RecursiveStreamOut(os, include_subgraph);						\
			StreamOutAutoPropertiesDefinition(os);												\
			return StreamOut(os);																\
		}																						\
		virtual std::istream &RecursiveStreamIn(std::istream &is, bool include_subgraph)		\
		{																						\
			parent_node_class::RecursiveStreamIn(is, include_subgraph);							\
			StreamInAutoPropertiesDefinition(is);												\
			return StreamIn(is);																\
		}																						\
		virtual std::ostream &RecursiveStateStreamOut(std::ostream &os, bool include_subgraph) const	\
		{																								\
			parent_node_class::RecursiveStateStreamOut(os, include_subgraph);							\
			StreamOutAutoPropertiesState(os);															\
			return StateStreamOut(os);																	\
		}																								\
		virtual std::istream &RecursiveStateStreamIn(std::istream &is, bool include_subgraph)			\
		{																								\
			parent_node_class::RecursiveStateStreamIn(is, include_subgraph);							\
			StreamInAutoPropertiesState(is);															\
			return StateStreamIn(is);																	\
		}
#ifndef DOXYGEN

	#define DECLARE_PropertyAction(action_name,info)				\
		public:														\
			static unsigned GetOrdinalOf##action_name()				\
			{														\
				return __COUNTER__;									\
			}														\
			META_PassOnStreaming(Definition)						\
			void DoAction##action_name();



	#define DECLARE_StateAction(action_name,info)					\
		public:														\
			static unsigned GetOrdinalOf##action_name()				\
			{														\
				return __COUNTER__;									\
			}														\
			META_PassOnStreaming(State)								\
			void DoAction##action_name();

#else

	#define DECLARE_PropertyAction(action_name,info)				\
		public:														\
		/*! info */													\
		/*! \name Action: action_name */							\
		/*@{*/														\
			void DoAction##action_name();							\
		/*@}*/


	#define DECLARE_StateAction(action_name,info)					\
		public:														\
		/*! info */													\
		/*! \name Action: action_name */							\
		/*@{*/														\
			void DoAction##action_name();							\
		/*@}*/


#endif

#define DECLARE_Action(action_name,info)						\
	public:														\
		/*! info */												\
		/*! \name Action: action_name */						\
		/*@{*/													\
		void DoAction##action_name();							\
		/*@}*/ 

#define VIRTUAL_Action(action_name)								\
		/*! info */												\
		/*! \name Action: action_name */						\
		/*@{*/													\
	virtual void DoAction##action_name() = 0;					\
		/*@}*/ 

#define IMPLEMENT_Action(class_name, action_name)				\
	void class_name::DoAction##action_name()

#ifdef _MSC_VER
	#pragma warning(pop)
#endif
#endif

