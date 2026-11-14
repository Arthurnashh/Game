/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RTD/Base.h"
#include "RTD/Named.h"
#include "RTD/Common.h"
#include "RTD/Math.h"

namespace RTD
{
	class SimulationVariableUser;

	class RTDYNAMICS_EXPORT RTDVariable
	{
	public:
		enum DataType
		{
			GENERIC = 0,
				REAL,
				DOUBLE,
				INT,
				UNSIGNED_INT,
				BOOL,
				VEC2R,
				VEC3R,
				VEC4R,
				VEC2D,
				VEC3D,
				VEC4D,
				STRING,
				REAL_DATA_HISTORY,
				DOUBLE_DATA_HISTORY,
				VEC2R_DATA_HISTORY,
				VEC2D_DATA_HISTORY,
				VEC3R_DATA_HISTORY,
				VEC3D_DATA_HISTORY,
				VEC4R_DATA_HISTORY,
				VEC4D_DATA_HISTORY,
				ENUM,
				NUM_TYPES
		};

		RTDVariable();
		RTDVariable( const std::string& name  );
		RTDVariable( const std::string& name , const std::vector<std::string>& memberDescriptions );

		virtual DataType				getDataType() const			{	return m_dataType;	}
		std::string						getMemberDescription( unsigned int i ) const;
        int     						getMemberIndex( const std::string& memberDesc ) const;

		virtual std::string				getDataTypeName() const;
        static RTDVariable::DataType    getDataTypeFromName( const std::string& dataTypeName );

		const std::string&				getName() const				{	return m_name;	}
		virtual std::string 			getPathAndName() const	=	0;

		virtual void					getLoggableData( std::vector<double>& data) const;


		virtual std::vector<unsigned char>		toBinary() const = 0;
		virtual bool							fromBinary( const std::vector<unsigned char>& data ) = 0; 

	protected:
		std::vector< std::string >		m_memberDescriptions;
		DataType						m_dataType;
		std::string						m_name;
	};

	/** Base class of state variables. */
	class RTDYNAMICS_EXPORT SimulationVariableBase : public RTDVariable
	{
	public:
		enum VariableType
		{
			UNDEFINED = 1,
				INPUT = 2,
				OUTPUT = 4,
				STATE = 8
		};

		
		SimulationVariableBase( SimulationVariableUser* owner , const std::string& name );
		SimulationVariableBase( SimulationVariableUser* owner , const std::string& name , const std::vector<std::string>& memberDescriptions );
		SimulationVariableBase();

		virtual ~SimulationVariableBase();

		virtual void					init( SimulationVariableUser* owner , const std::string& name );

		virtual void					init( SimulationVariableUser* owner , const std::string& name , const std::vector<std::string>& memberDescriptions );


		virtual std::string 			getPathAndName() const;
		

		const SimulationVariableUser*	getOwner() const			{	return m_owner;	}
		SimulationVariableUser*			getOwner()					{	return m_owner;	}

		
		virtual VariableType			getVariableType() const		{	return m_variableType;	}

		
		virtual std::string				getVariableTypeName() const;
		
		virtual void					printValue( bool printDataType , bool printVariableType ,  bool printFullPath ) const = 0;
			
		bool							isInitialized() const		{	return m_isInitialized;	}									
		
		virtual void					reset() = 0;



		virtual SimulationVariableBase*	clone() = 0;


		virtual std::string		toString() const { return "NotSupported";}

		virtual bool			fromString( const std::string& str ){ return true; }; 

		virtual	bool			copyContentAndState( const SimulationVariableBase* rhs ) 
		{
			m_isInitialized	=	rhs->m_isInitialized;
			m_hasDefault	=	rhs->m_hasDefault;
			return true;
		}

	protected:
		
		SimulationVariableUser*	m_owner;		
		VariableType			m_variableType;
		bool					m_isInitialized;
		bool					m_hasDefault;

	private:
		/** operator= is undefined for the SimulationVariableBase because it does not contain any actual data. */
		SimulationVariableBase&  operator=( const SimulationVariableBase& rhs )								
		{				
			assert(0);
			return *this;												
		}	

	};
 
#define SIMULATION_VARIABLE_CLASS( VALUETYPE , SUFFIX , TYPE )\
	class SimulationVariable##SUFFIX : public SimulationVariableBase\
	{																															\
		public:																													\
			SimulationVariable##SUFFIX( SimulationVariableUser* owner , const std::string& name , VALUETYPE value ):			\
			SimulationVariableBase( owner , name )																				\
			{																													\
				m_dataType = TYPE;																								\
				m_isInitialized = true;																							\
				m_hasDefault = true;																							\
				m_value = new VALUETYPE;																						\
				m_defaultValue = new VALUETYPE;																					\
				*m_value = value;																								\
				*m_defaultValue = value;																						\
			}																													\
																																\
			SimulationVariable##SUFFIX( SimulationVariableUser* owner , const std::string& name ,								\
										const std::vector<std::string>& memberDescriptions, VALUETYPE value ):					\
			SimulationVariableBase( owner , name ,memberDescriptions )															\
			{																													\
				m_dataType = TYPE;																								\
				m_isInitialized = true;																							\
				m_hasDefault = true;																							\
				m_value = new VALUETYPE;																						\
				m_defaultValue = new VALUETYPE;																					\
				*m_value = value;																								\
				*m_defaultValue = value;																						\
			}																													\
																																\
			SimulationVariable##SUFFIX( SimulationVariableUser* owner , const std::string& name):								\
			SimulationVariableBase( owner , name ) 																				\
			{																													\
				m_dataType = TYPE;																								\
				m_value = new  VALUETYPE;																						\
				m_defaultValue = new  VALUETYPE;																				\
				m_isInitialized = false;																						\
				m_hasDefault = false;																							\
			}																													\
																																\
			SimulationVariable##SUFFIX( SimulationVariableUser* owner , const std::string& name,  								\
										const std::vector<std::string>& memberDescriptions ):									\
			SimulationVariableBase( owner , name , memberDescriptions )															\
			{																													\
				m_dataType = TYPE;																								\
				m_value = new  VALUETYPE;																						\
				m_defaultValue = new  VALUETYPE;																				\
				m_isInitialized = false;																						\
				m_hasDefault = false;																							\
			}																													\
																																\
			SimulationVariable##SUFFIX():																						\
			SimulationVariableBase() 																							\
			{																													\
				m_dataType = TYPE;																								\
				m_value = new VALUETYPE;																						\
				m_defaultValue = new  VALUETYPE;																				\
				m_isInitialized = false;																						\
				m_hasDefault = false;																							\
			}																													\
																																\
			virtual ~SimulationVariable##SUFFIX()																				\
			{																													\
				delete m_value;																									\
				delete m_defaultValue;																							\
			}																													\
																																\
			virtual void init(	SimulationVariableUser* owner , const std::string& name , VALUETYPE value )						\
			{																													\
				SimulationVariableBase::init( owner , name  );																	\
				m_isInitialized = true;																							\
				m_hasDefault = true;																							\
				*m_value = value;																								\
				*m_defaultValue = value;																						\
			}																													\
																																\
			virtual void init(	SimulationVariableUser* owner , const std::string& name ,										\
								const std::vector<std::string>& memberDescriptions , VALUETYPE value )							\
			{																													\
				SimulationVariableBase::init( owner , name ,memberDescriptions );												\
				m_isInitialized = true;																							\
				m_hasDefault = true;																							\
				*m_value = value;																								\
				*m_defaultValue = value;																						\
			}																													\
																																\
			const VALUETYPE& getValue()	 const																					\
			{																													\
				if ( m_isInitialized == false )																					\
					RTD::getPrinter()->fatal( "SimulationVariable::getValue: trying to get value of an uninitialized state variable (%s)." , getName().c_str());\
																																\
																																\
				return (const VALUETYPE&)*m_value;																				\
																																\
			}																													\
																																\
			VALUETYPE& getValue()																								\
			{																													\
				if ( m_isInitialized == false )																					\
					RTD::getPrinter()->fatal( "SimulationVariable::getValue: trying to get value of an uninitialized state variable (%s)." , getName().c_str());\
																																\
																																\
				return (VALUETYPE&)*m_value;																					\
																																\
			}																													\
																																\
			const VALUETYPE& getDefaultValue()	 const																			\
			{																													\
				if ( m_hasDefault == false )																					\
					RTD::getPrinter()->fatal( "SimulationVariable::getDefaultValue: trying to get default value of a state variable that does not have default (%s)." , getName().c_str());\
																																\
				return (const VALUETYPE&)*m_defaultValue;																		\
																																\
			}																													\
																																\
			VALUETYPE& getDefaultValue()																						\
			{																													\
				if ( m_hasDefault == false )																					\
					RTD::getPrinter()->fatal( "SimulationVariable::getDefaultValue: trying to get default value of a state variable that does not have default (%s)." , getName().c_str());\
																																\
				return (VALUETYPE&)*m_defaultValue;																				\
																																\
			}																													\
																																\
			virtual void reset()																								\
			{																													\
				*m_value = (VALUETYPE)getDefaultValue();																		\
			}																													\
																																\
			void	setValue(const VALUETYPE& v)			{	*m_value = (VALUETYPE)v; m_isInitialized = true;	}			\
																																\
			virtual void printValue(bool printDataType , bool printVariableType, bool printFullPath  ) const					\
			{																													\
				if ( printFullPath )																							\
				{																												\
					std::cout << getPathAndName();																				\
				}																												\
				else																											\
				{																												\
					std::cout << m_name;																						\
				}																												\
																																\
				if ( printVariableType )																						\
				{																												\
					switch ( m_variableType )																					\
					{																											\
					case UNDEFINED:																								\
						std::cout << "(Undefined)";																				\
						break;																									\
					case INPUT:																									\
						std::cout << "(Input)";																					\
						break;																									\
					case STATE:																									\
						std::cout << "(State)";																					\
						break;																									\
					case OUTPUT:																								\
						std::cout << "(Output)";																				\
						break;																									\
					}																											\
				}																												\
																																\
				if ( printDataType )																							\
					std::cout << "(" << getDataTypeName() << ")=";																\
																																\
				std::cout << *m_value << std::endl;																				\
			}																													\
																																\
			SimulationVariable##SUFFIX&  operator=( VALUETYPE rhs )																\
			{																													\
				*m_value = (VALUETYPE)rhs;																						\
				m_isInitialized = true;																							\
				return *this;																									\
			}																													\
																																\
			SimulationVariable##SUFFIX&  operator=( const SimulationVariable##SUFFIX& rhs )										\
			{																													\
				*m_value = (VALUETYPE)rhs.getValue();																			\
				m_isInitialized = true;																							\
				return *this;																									\
			}																													\
																																\
			operator VALUETYPE() const																							\
			{																													\
				return (VALUETYPE)getValue();																					\
			}																													\
																																\
			operator VALUETYPE&()																								\
			{																													\
				return (VALUETYPE&)getValue();																					\
			}																													\
																																\
			virtual	bool	copyContentAndState( const SimulationVariableBase* rhs )											\
			{																													\
				const SimulationVariable##SUFFIX* tmp = dynamic_cast<const SimulationVariable##SUFFIX*>( rhs );\
				if ( tmp )\
				{\
					if ( SimulationVariableBase::copyContentAndState( rhs ) == false ) return false;\
					*m_value = *tmp->m_value;\
					*m_defaultValue = *tmp->m_defaultValue;\
					return true;\
				}\
				return false;\
			}																													\
																																\
			virtual bool			fromString( const std::string& str );														\
			virtual std::string		toString() const;																			\
			virtual std::vector<unsigned char>		toBinary() const;															\
			virtual bool							fromBinary( const std::vector<unsigned char>& data );						\
			virtual void	getLoggableData( std::vector<double>& data) const;													\
																																\
		protected:																												\
			VALUETYPE*		m_value;																							\
			VALUETYPE*		m_defaultValue;																						\
																																\
	};																															\
																																\

#define TYPED_SIM_VARIABLE_CLASS( VALUETYPE , SUFFIX , TYPE , VARIABLETYPE , CLASSNAMEPREFIX  )									\
	class CLASSNAMEPREFIX##Variable##SUFFIX : public SimulationVariable##SUFFIX													\
	{																															\
		public:																													\
			CLASSNAMEPREFIX##Variable##SUFFIX( SimulationVariableUser* owner , const std::string& name , VALUETYPE value ):		\
			SimulationVariable##SUFFIX( owner , name,value )																	\
			{																													\
				m_variableType	= VARIABLETYPE;																					\
			}																													\
																																\
			CLASSNAMEPREFIX##Variable##SUFFIX( SimulationVariableUser* owner , const std::string& name):						\
			SimulationVariable##SUFFIX( owner , name )																			\
			{																													\
				m_variableType	= VARIABLETYPE;																					\
			}																													\
																																\
			CLASSNAMEPREFIX##Variable##SUFFIX( SimulationVariableUser* owner , const std::string& name,							\
											   const std::vector<std::string>& memberDescriptions, VALUETYPE value ):			\
			SimulationVariable##SUFFIX( owner , name,memberDescriptions,value )													\
			{																													\
				m_variableType	= VARIABLETYPE;																					\
			}																													\
																																\
			CLASSNAMEPREFIX##Variable##SUFFIX( SimulationVariableUser* owner , const std::string& name ,						\
											  const std::vector<std::string>& memberDescriptions):								\
			SimulationVariable##SUFFIX( owner , name , memberDescriptions )														\
			{																													\
				m_variableType	= VARIABLETYPE;																					\
			}																													\
																																\
			CLASSNAMEPREFIX##Variable##SUFFIX():																				\
			SimulationVariable##SUFFIX()																						\
			{																													\
				m_variableType	= VARIABLETYPE;																					\
			}																													\
																																\
			CLASSNAMEPREFIX##Variable##SUFFIX&  operator=( VALUETYPE rhs )														\
			{																													\
				*m_value =  (VALUETYPE)rhs;																						\
				m_isInitialized = true;																							\
				return *this;																									\
			}																													\
																																\
			CLASSNAMEPREFIX##Variable##SUFFIX&  operator=( const SimulationVariable##SUFFIX& rhs )								\
			{																													\
				*m_value = (VALUETYPE)rhs.getValue();																			\
				m_isInitialized = true;																							\
				return *this;																									\
			}																													\
																																\
			virtual SimulationVariableBase*	clone()    																			\
			{																													\
				CLASSNAMEPREFIX##Variable##SUFFIX* clonedObj = new CLASSNAMEPREFIX##Variable##SUFFIX;							\
				clonedObj->m_name = m_name;																						\
				clonedObj->m_owner = NULL;																						\
				clonedObj->m_dataType = m_dataType;																				\
				clonedObj->m_variableType = m_variableType;																		\
				clonedObj->m_isInitialized = m_isInitialized;																	\
				clonedObj->m_hasDefault = m_hasDefault;																			\
				*clonedObj->m_value = *m_value;																					\
				*clonedObj->m_defaultValue = *m_defaultValue;																	\
				return clonedObj;																								\
			}																													\
	};																															\





	SIMULATION_VARIABLE_CLASS( std::string , String , STRING )
	SIMULATION_VARIABLE_CLASS( bool , Bool , BOOL )
	SIMULATION_VARIABLE_CLASS( unsigned int , UInt , UNSIGNED_INT )
	SIMULATION_VARIABLE_CLASS( int , Int , INT )
	SIMULATION_VARIABLE_CLASS( smReal , Real , REAL )
	SIMULATION_VARIABLE_CLASS( smDouble , Double , DOUBLE )
	SIMULATION_VARIABLE_CLASS( smVec2r , Vec2r , VEC2R )
	SIMULATION_VARIABLE_CLASS( smVec3r , Vec3r , VEC3R )
	SIMULATION_VARIABLE_CLASS( smVec4r , Vec4r , VEC4R )
	SIMULATION_VARIABLE_CLASS( smVec2d , Vec2d , VEC2D )
	SIMULATION_VARIABLE_CLASS( smVec3d , Vec3d , VEC3D )
	SIMULATION_VARIABLE_CLASS( smVec4d , Vec4d , VEC4D )
	SIMULATION_VARIABLE_CLASS( RealDataHistory , RealDataHistory , REAL_DATA_HISTORY )
	SIMULATION_VARIABLE_CLASS( DoubleDataHistory , DoubleDataHistory , DOUBLE_DATA_HISTORY )
	SIMULATION_VARIABLE_CLASS( Vec2rDataHistory , Vec2rDataHistory , VEC2R_DATA_HISTORY )
	SIMULATION_VARIABLE_CLASS( Vec2dDataHistory , Vec2dDataHistory , VEC2D_DATA_HISTORY )
	SIMULATION_VARIABLE_CLASS( Vec3rDataHistory , Vec3rDataHistory , VEC3R_DATA_HISTORY )
	SIMULATION_VARIABLE_CLASS( Vec3dDataHistory , Vec3dDataHistory , VEC3D_DATA_HISTORY )
	SIMULATION_VARIABLE_CLASS( Vec4rDataHistory , Vec4rDataHistory , VEC4R_DATA_HISTORY )
	SIMULATION_VARIABLE_CLASS( Vec4dDataHistory , Vec4dDataHistory , VEC4D_DATA_HISTORY )



	TYPED_SIM_VARIABLE_CLASS( std::string , String , STRING , STATE , State)
	TYPED_SIM_VARIABLE_CLASS( bool , Bool , BOOL , STATE , State)
	TYPED_SIM_VARIABLE_CLASS( unsigned int , UInt , UNSIGNED_INT, STATE , State )
	TYPED_SIM_VARIABLE_CLASS( int , Int , INT, STATE , State )
	TYPED_SIM_VARIABLE_CLASS( smReal , Real , REAL, STATE , State )
	TYPED_SIM_VARIABLE_CLASS( smDouble , Double , DOUBLE , STATE , State)
	TYPED_SIM_VARIABLE_CLASS( smVec2r , Vec2r , VEC2R , STATE , State )
	TYPED_SIM_VARIABLE_CLASS( smVec3r , Vec3r , VEC3R , STATE , State )
	TYPED_SIM_VARIABLE_CLASS( smVec4r , Vec4r , VEC4R , STATE , State)
	TYPED_SIM_VARIABLE_CLASS( smVec2d , Vec2d , VEC2D , STATE , State)
	TYPED_SIM_VARIABLE_CLASS( smVec3d , Vec3d , VEC3D , STATE , State)
	TYPED_SIM_VARIABLE_CLASS( smVec4d , Vec4d , VEC4D , STATE , State )
	TYPED_SIM_VARIABLE_CLASS( RealDataHistory , RealDataHistory , REAL_DATA_HISTORY , STATE , State )
	TYPED_SIM_VARIABLE_CLASS( DoubleDataHistory , DoubleDataHistory , DOUBLE_DATA_HISTORY , STATE , State )
	TYPED_SIM_VARIABLE_CLASS( Vec2rDataHistory , Vec2rDataHistory , VEC2R_DATA_HISTORY , STATE , State )
	TYPED_SIM_VARIABLE_CLASS( Vec2dDataHistory , Vec2dDataHistory , VEC2D_DATA_HISTORY , STATE , State  )
	TYPED_SIM_VARIABLE_CLASS( Vec3rDataHistory , Vec3rDataHistory , VEC3R_DATA_HISTORY , STATE , State  )
	TYPED_SIM_VARIABLE_CLASS( Vec3dDataHistory , Vec3dDataHistory , VEC3D_DATA_HISTORY , STATE , State  )
	TYPED_SIM_VARIABLE_CLASS( Vec4rDataHistory , Vec4rDataHistory , VEC4R_DATA_HISTORY , STATE , State  )
	TYPED_SIM_VARIABLE_CLASS( Vec4dDataHistory , Vec4dDataHistory , VEC4D_DATA_HISTORY , STATE , State  )


	TYPED_SIM_VARIABLE_CLASS( std::string , String , STRING , OUTPUT , Output)
	TYPED_SIM_VARIABLE_CLASS( bool , Bool , BOOL , OUTPUT , Output)
	TYPED_SIM_VARIABLE_CLASS( unsigned int , UInt , UNSIGNED_INT, OUTPUT , Output )
	TYPED_SIM_VARIABLE_CLASS( int , Int , INT, OUTPUT , Output )
	TYPED_SIM_VARIABLE_CLASS( smReal , Real , REAL, OUTPUT , Output )
	TYPED_SIM_VARIABLE_CLASS( smDouble , Double , DOUBLE , OUTPUT , Output)
	TYPED_SIM_VARIABLE_CLASS( smVec2r , Vec2r , VEC2R , OUTPUT , Output )
	TYPED_SIM_VARIABLE_CLASS( smVec3r , Vec3r , VEC3R , OUTPUT , Output )
	TYPED_SIM_VARIABLE_CLASS( smVec4r , Vec4r , VEC4R , OUTPUT , Output)
	TYPED_SIM_VARIABLE_CLASS( smVec2d , Vec2d , VEC2D , OUTPUT , Output)
	TYPED_SIM_VARIABLE_CLASS( smVec3d , Vec3d , VEC3D , OUTPUT , Output)
	TYPED_SIM_VARIABLE_CLASS( smVec4d , Vec4d , VEC4D , OUTPUT , Output )
	TYPED_SIM_VARIABLE_CLASS( RealDataHistory , RealDataHistory , REAL_DATA_HISTORY , OUTPUT , Output )
	TYPED_SIM_VARIABLE_CLASS( DoubleDataHistory , DoubleDataHistory , DOUBLE_DATA_HISTORY , OUTPUT , Output )
	TYPED_SIM_VARIABLE_CLASS( Vec2rDataHistory , Vec2rDataHistory , VEC2R_DATA_HISTORY , OUTPUT , Output )
	TYPED_SIM_VARIABLE_CLASS( Vec2dDataHistory , Vec2dDataHistory , VEC2D_DATA_HISTORY , OUTPUT , Output  )
	TYPED_SIM_VARIABLE_CLASS( Vec3rDataHistory , Vec3rDataHistory , VEC3R_DATA_HISTORY , OUTPUT , Output  )
	TYPED_SIM_VARIABLE_CLASS( Vec3dDataHistory , Vec3dDataHistory , VEC3D_DATA_HISTORY , OUTPUT , Output  )
	TYPED_SIM_VARIABLE_CLASS( Vec4rDataHistory , Vec4rDataHistory , VEC4R_DATA_HISTORY , OUTPUT , Output  )
	TYPED_SIM_VARIABLE_CLASS( Vec4dDataHistory , Vec4dDataHistory , VEC4D_DATA_HISTORY , OUTPUT , Output  )

	TYPED_SIM_VARIABLE_CLASS( std::string , String , STRING , INPUT , Input)
	TYPED_SIM_VARIABLE_CLASS( bool , Bool , BOOL , INPUT , Input)
	TYPED_SIM_VARIABLE_CLASS( unsigned int , UInt , UNSIGNED_INT, INPUT , Input )
	TYPED_SIM_VARIABLE_CLASS( int , Int , INT, INPUT , Input )
	TYPED_SIM_VARIABLE_CLASS( smReal , Real , REAL, INPUT , Input )
	TYPED_SIM_VARIABLE_CLASS( smDouble , Double , DOUBLE , INPUT , Input)
	TYPED_SIM_VARIABLE_CLASS( smVec2r , Vec2r , VEC2R , INPUT , Input )
	TYPED_SIM_VARIABLE_CLASS( smVec3r , Vec3r , VEC3R , INPUT , Input )
	TYPED_SIM_VARIABLE_CLASS( smVec4r , Vec4r , VEC4R , INPUT , Input)
	TYPED_SIM_VARIABLE_CLASS( smVec2d , Vec2d , VEC2D , INPUT , Input)
	TYPED_SIM_VARIABLE_CLASS( smVec3d , Vec3d , VEC3D , INPUT , Input)
	TYPED_SIM_VARIABLE_CLASS( smVec4d , Vec4d , VEC4D , INPUT , Input )
	TYPED_SIM_VARIABLE_CLASS( RealDataHistory , RealDataHistory , REAL_DATA_HISTORY , INPUT , Input )
	TYPED_SIM_VARIABLE_CLASS( DoubleDataHistory , DoubleDataHistory , DOUBLE_DATA_HISTORY , INPUT , Input )
	TYPED_SIM_VARIABLE_CLASS( Vec2rDataHistory , Vec2rDataHistory , VEC2R_DATA_HISTORY , INPUT , Input )
	TYPED_SIM_VARIABLE_CLASS( Vec2dDataHistory , Vec2dDataHistory , VEC2D_DATA_HISTORY , INPUT , Input  )
	TYPED_SIM_VARIABLE_CLASS( Vec3rDataHistory , Vec3rDataHistory , VEC3R_DATA_HISTORY , INPUT , Input  )
	TYPED_SIM_VARIABLE_CLASS( Vec3dDataHistory , Vec3dDataHistory , VEC3D_DATA_HISTORY , INPUT , Input  )
	TYPED_SIM_VARIABLE_CLASS( Vec4rDataHistory , Vec4rDataHistory , VEC4R_DATA_HISTORY , INPUT , Input  )
	TYPED_SIM_VARIABLE_CLASS( Vec4dDataHistory , Vec4dDataHistory , VEC4D_DATA_HISTORY , INPUT , Input  )


	/** Base class of state variable users. */
	class RTDYNAMICS_EXPORT SimulationVariableUser : public Named , public Base
	{	
		friend class SimulationVariableBase;
	public:		
		SimulationVariableUser( const std::string& name , SimulationVariableUser* parent );
		virtual ~SimulationVariableUser();

		virtual void				reset();

		virtual void				addSimulationVariableUserChild( SimulationVariableUser* child );
		virtual void				removeSimulationVariableUserChild( SimulationVariableUser* child );
		bool						isSimulationVariableUserChild( SimulationVariableUser* svu ) const;

		SimulationVariableUser*			getParentSimulationVariableUser()		{	return m_stateVariableUserParent;	}
		const SimulationVariableUser*	getParentSimulationVariableUser() const	{	return m_stateVariableUserParent;	}
		

		SimulationVariableUser*			findChildSimulationVariableUser( const std::string& name ) ;
		const SimulationVariableUser*	findChildSimulationVariableUser( const std::string& name ) const;
		int							findChildSimulationVariableUserIndex( const std::string& name ) const;
		
		/** Returns the state variable with given name under this SimulationVariableUser. If a state variable with this name
			does not exist NULL is returned. */
		SimulationVariableBase*			findSimulationVariable( const std::string& pathAndName ,  bool errorWhenNotFound = false ) ;
		const SimulationVariableBase*	findSimulationVariable( const std::string& pathAndName , bool errorWhenNotFound = false ) const;



		/** Removes the state variable from this SimulationVariableUser's state variables list.*/
		void						removeSimulationVariable( SimulationVariableBase* sv  );
		

		/** Print all state variables of the SimulationVariableUser and its children in a tree form. */
		virtual void				printAllSimulationVariables( unsigned int indent = 0 ,  bool printDataType  = true ,bool printVariableType = true,  bool fullPath = false , unsigned int variableTypeMask = 0xffffffff ) const;

		virtual void				collectSimulationVariables( std::vector<SimulationVariableBase*>& vars, unsigned int variableTypeMask ); 		

		virtual void				collectSimulationVariables( std::vector<const SimulationVariableBase*>& vars, unsigned int variableTypeMask ) const; 		

		virtual void				acquireState( const SimulationVariableUser& rhs );


		
		smReal&				getSimulationVariableReal( const std::string& pathAndName ) ;
		smDouble&			getSimulationVariableDouble( const std::string& pathAndName );
		smVec2r&			getSimulationVariableVec2r( const std::string& pathAndName ) ;
		smVec3r&			getSimulationVariableVec3r( const std::string& pathAndName ) ;
		smVec4r&			getSimulationVariableVec4r( const std::string& pathAndName ) ;
		smVec2d&			getSimulationVariableVec2d( const std::string& pathAndName ) ;
		smVec3d&			getSimulationVariableVec3d( const std::string& pathAndName ) ;
		smVec4d&			getSimulationVariableVec4d( const std::string& pathAndName ) ;
				
		const smReal&		getSimulationVariableReal( const std::string& pathAndName ) const ;
		const smDouble&		getSimulationVariableDouble( const std::string& pathAndName ) const ;
		const smVec2r&		getSimulationVariableVec2r( const std::string& pathAndName ) const;
		const smVec3r&		getSimulationVariableVec3r( const std::string& pathAndName ) const;
		const smVec4r&		getSimulationVariableVec4r( const std::string& pathAndName ) const;
		const smVec2d&		getSimulationVariableVec2d( const std::string& pathAndName ) const;
		const smVec3d&		getSimulationVariableVec3d( const std::string& pathAndName ) const;
		const smVec4d&		getSimulationVariableVec4d( const std::string& pathAndName ) const;

		virtual void		serializeSimulationVariables( std::string& result );
		virtual void		serializeSimulationVariables( std::vector<unsigned char>& result );
		virtual bool		deserializeSimulationVariables( const std::vector<unsigned char>& serializedArray , std::vector<unsigned char>& remainingArray );


	protected:	
		SimulationVariableUser*					m_stateVariableUserParent;
		std::vector<SimulationVariableUser*>		m_stateVariableUserChildren;

		std::vector<SimulationVariableBase*>		m_stateVariables;

		SimulationVariableBase*			findSimulationVariableRec( const std::string& pathAndName , const std::string& recPathAndName , bool errorWhenNotFound) const ;
		void							collectSimulationVariablesRec( std::vector<SimulationVariableBase*>& vars, unsigned int variableTypeMask );
		void							collectSimulationVariablesRecConst( std::vector<const SimulationVariableBase*>& vars, unsigned int variableTypeMask ) const;
	};


    
    bool RTDYNAMICS_EXPORT setSimVariableValue( const smDouble& val , RTD::SimulationVariableBase* simvar , const int member );
    bool RTDYNAMICS_EXPORT getSimVariableValue( smDouble& val , const RTD::SimulationVariableBase* simvar , const int member  );
    

}