/*
(c) Copyright 2009, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RTD/Common.h"
#include "RTD/World.h"
#include "RTD/WorldParticipant.h"
#include "RTD/ParamDb.h"
#include <algorithm>



namespace RTD
{
	class BotBase;
	class VehicleBase ;
	class BotControllerBase;
	class DynamicControllerParameterBase;
	class DataLogger;

	

	/**
		Base class of RTD controllers.
		
		Bots and controllers form the basic control and steering framework of RTDynamics 
		simulation library. Additional information on this subject can be found in "Bots and Controllers" section
		of the Programmer's Guide.
	*/
	class RTDYNAMICS_EXPORT BotControllerBase : public SimulationVariableUser
	{
		friend class BotBase;
		friend class DynamicControllerParameterBase; 

	public:
		/** When controller state is STATE_NOT_MASTER, controller is not the master controller. 
			\see Bots and Controllers section in Programmer's Guide  */
		static	const std::string	STATE_NOT_MASTER;

		/** When controller state is STATE_EXECUTING, controller is currently executing its
			control algorithm.
			\see Bots and Controllers section in Programmer's Guide  */
		static	const std::string	STATE_EXECUTING;

		/** When controller state is STATE_ENDED, controller finished its task.
			\see Bots and Controllers section in Programmer's Guide  */
		static	const std::string	STATE_ENDED;
		

		/** Constructor of the class. \e controlledBot is the bot to which this controller
			will be attached. \e name is the name of the controller. */
		BotControllerBase( BotBase* controlledBot ,  const std::string& name );

		/** Destructor of the class. */
		virtual ~BotControllerBase();

		/** Returns the name of the controller. */
		const std::string&	getName() const					{	return m_controllerName;	}

		/** Returns the Bot to which this controller is attached. */
		BotBase*			getControlledBot() const		{	return m_controlledBot;	}


		/**
			This method is called by the bot when this controller is activated.
			
			Bots trigger the update() of the master controller only. The Master triggers its 
			slaves, which, in turn, trigger their own slaves. If a Controller is not updated
			in a frame but gets updated in the next frame, it is considered activated. In this
			case initialize() method is called.

			If the BotControllerBase::update is overridden, new implementation must call 
			BotControllerBase::update() to make sure initialize() method is called properly.
		*/
		virtual void	initialize()				=	0;

		/** RTD::BotControllerBase::update is re-implemented in derived Controller classes to implement 
			custom controller logic. This method will be called regulary during simulation by the RTDynamics 
			controller framework.
		
			This method should be called by all overriding implementations otherwise last 
		    update time will not be updated accordingly and wasUpdatedLastFrame() method will not 
			function properly. Overriding update() methods should call BotControllerBase::update() 
			at the beginning.

			In update() both s_lastUpdateTime and s_wasUpdatedPreviousFrame member variables are set. In case
			s_wasUpdatedPreviousFrame is set to false (i.e. in previous frame the controller was not updated. )
			initialize() method is called automatically.
		*/
		virtual void	update( smReal deltaTime );

		/**
			This method is called by the bot when a controller is not master anymore.
		*/
		virtual void	finalize()					=	0;

		/** Resets the controller. Overriding methods should call this method. */
		virtual void	reset();

		/** This method returns true in case the controller was updated (its update method was called)
			in the previous frame. It works both for master and slave controllers. False is returned if the controller
			was not updated neither as master nor as slave.

			Note: BotControllerBase::update() method has to be called within overriding update() methods
				  for this method to function properly .
		*/	
		virtual bool	wasUpdatedPreviousFrame() const;

		/** This method is used to read static parameters of the controller from a string. The string
			contains the parameters in XML format.
			
			This method is called by the Bot class during XML file loading. Overriding methods shouldn't
			call this method.*/
		virtual bool	readParamStr( const std::string& parameterString , std::string& error );

		/** Returns true if all dynamic parameters of a controller are specified. */
		virtual bool	areAllDynamicParamsSpecified() const;

		/** Returns true if all dynamic parameters of a controller are either specified or
			they would return a valid default value (DynamicControllerParam::getReturnDefaultWhenUnspecified() and 
			DynamicControllerParam::hasDefault() are both true).			
		*/
		virtual bool	areAllDynamicParamsSpecifiedOrDefault() const;

		/** Sets all dynamic parameters to their default values. If a dynamic parameter
		    does not have a default, it is set unspecified.*/
		virtual void	setAllDynamicParamsToDefault();


		/** Returns status string of the controller. See Bots and Controllers section
			in the Programmer's Guide for more information on controller states. */			
		const std::string&	getStatusString() const		{	return s_statusString.getValue();	}

		/** 
			This function returns the DataLogger object used by this controller. Controller uses the 
			data logger of the bot it is connected to.
		*/
		RTD::DataLogger*	getDataLogger();

		/**
			Returns the number of dynamic controller parameters that belong to this controller.
		*/
		unsigned int		getNumDynamicParameters() const		{	return (unsigned int) m_dynamicParameters.getNum(); }

		/**
			Returns the dynamic controller parameter at the given index. If a controller with given index does not exists,
			NULL is returned.

			This method can be used to iterate dynamic controller parameters of a controller.

			@see getNumDynamicParameters()
		*/
		DynamicControllerParameterBase*			getDynamicParameter( unsigned int index );

		/**
			Returns the dynamic controller parameter at the given index. If a controller with given index does not exists,
			NULL is returned.

			This method can be used to iterate dynamic controller parameters of a controller.

			@see getNumDynamicParameters()
		*/
		const DynamicControllerParameterBase*	getDynamicParameter( unsigned int index ) const;

	protected:
		/** This variable is updated by BotControllerBase::update(). */
		StateVariableDouble		s_lastUpdateTime;

		/** This variable is updated by BotControllerBase::update(). */
		StateVariableBool		s_wasUpdatedPreviousFrame;

		StateVariableString		s_statusString;

		std::string		m_controllerName;
		BotBase*		m_controlledBot;


		smManager<DynamicControllerParameterBase>	m_dynamicParameters;
		smManager<BotControllerBase>				m_slaveControllers;

	};

	/**
		Base class of RTD bots.
		
		Bots and controllers form the basic control and steering framework of RTDynamics 
		simulation library. Additional information on this subject can be found in 
		"Bots and Controllers" section of the Programmer's Guide.		
	*/
	class RTDYNAMICS_EXPORT BotBase : public RTD::WorldParticipant , public SimulationVariableUser
	{
		friend class BotControllerBase;
	public:

		/** Constructor of the class. 
		
			Note that \e name must be unique. If there is another bot with the same name
			within the same world, the newly created bot will not be added to the world.
			Even if an entity is deleted, its name remains reserved until World::reset()
			is called.
		*/
		BotBase( RTD::VehicleBase* ac, const std::string& name , const std::string& parameterFile );

		/** Updates the bot by advancing the simulation time \e deltaTime seconds. For real-time applications
			\e deltaTime is the amount of time that passed since the last frame. 
			
			Calling this method directly is not necessary. RTD::World class calls this method as a part 
			of its update() method.
		*/
		virtual void						update( float deltaTime );

		/** Returns the current master controller. If there is no master controller, NULL is returned. */
		virtual const BotControllerBase*	getMasterController() const;
		
		/** Returns the current master controller. If there is no master controller, NULL is returned. */
		virtual BotControllerBase*			getMasterController();

		/** Calling this method sets the master controller to none, which causes the bot not to do anything to 
			steer the vehicle under control. */
		virtual void						setMasterToNone();

		/** Sets the master controller. */
		virtual	void						setMasterController( BotControllerBase* masterController  );

		/** Resets the bot. reset() sets the master controller to NULL and resets all controllers of the bot
		*	by calling their reset() methods.
		*
		*	If this method is reimplemented, overriding implementation should call this method. 
		*/
		virtual void						reset();

		/** This method will return true if the bot is correctly created and configured. For example, a 
			syntax error in the XML configuration file would result in a configuration error and 
			isBotCorrectlyConfigured() would return false. */
		bool								isBotCorrectlyConfigured() const {	return m_isBotCorrectlyConfigured; }

		/**
			Returns true if current master controller is different than the one at the last update.
			It simply compares m_prevMasterController to m_masterController. m_prevMasterController
			is updated by the BotBase::update() method just before the method exits.
		*/
		virtual bool					didMasterChange() const;				

		/** Returns the name of the bot. */
		const std::string&				getName() const				{ return m_name;	}

		/**	Returns the vehicle controlled by this bot.	*/
		RTD::VehicleBase*				getVehicle();

		/**	Returns the vehicle controlled by this bot.	*/
		const RTD::VehicleBase*			getVehicle() const;


		/** Returns the DataLogger object that is used by this bot. This data logger object is the
			same as the one used by the vehicle under control. */
		RTD::DataLogger*				getDataLogger();

		/** Returns the DataLogger object that is used by this bot. This data logger object is the
			same as the one used by the vehicle under control. */
		const RTD::DataLogger*			getDataLogger() const;

		
		/** Returns the status string of the master controller. If there is no master controller
			an empty string is returned. */
		const std::string&				getStatusString() const;

		/** This method makes a vertical line intersection with the terrain to find the height of the terrain at the given position. 
			\e pos is in world coordinates and its unit is meters. pos[2] (z component) is ignored. Result of the query is returned in 
			\e result again in meters. If the query fails (e.g. no terrain available at given position) false is returned. 

			This method uses TerrainQueryCallback object of the vehicle under control. If a callback is
			not attached to the vehicle, the method returns always false. */
		virtual bool					terrainHeight( const smVec3r& pos ,  smReal& result );

		/** This method makes a line intersection with terrain. \e startPnt and \e endPnt define the end points of the intersector line
			in world coordinates. They are in meters. Result is returned in \e result, which is also in world coordinates and
			uses meters as unit. If the query fails (e.g. no intersection point found) false is returned. 

			This method uses TerrainQueryCallback object of the vehicle under control. If a callback is
			not attached to the vehicle, the method returns always false. */
		virtual bool					terrainIntersect( const smVec3r& startPnt , const smVec3r& endPnt , smVec3r& result );

		/** Calculates and returns the altitude above terrain in meters. If
		 *  terrainHeight() doesn't return any results (e.g. if there is no
		 *  terrain under the vehicle, 0 is returned. */
		virtual smReal 					getAltitudeAboveTerrain() ;

		unsigned int					getNumControllers() const	{ return (unsigned int)m_controllers.getNum(); }

		/** Returns the controller whose name is given in \e controllerName. If the bot does not contain such a controller,
			NULL is returned. */
		const BotControllerBase*				getControllerByName( const std::string& controllerName ) const;

		/** Returns the controller whose name is given in \e controllerName. If the bot does not contain such a controller,
			NULL is returned.*/
		BotControllerBase*						getControllerByName( const std::string& controllerName );

		/**
			Returns the controller by index. If a controller with given index does not exists, NULL is returned.
		*/
		BotControllerBase*						getControllerByIndex(int index);

		/**
			Returns the controller by index. If a controller with given index does not exists, NULL is returned.
		*/
		const BotControllerBase*				getControllerByIndex(int index) const;

		/**
			Finds the index of a given controller. If \e controller is not a controller of this bot, -1 is returned. 
		*/
		int										findControllerIndex( const BotControllerBase* controller ) const;

		/**
			This method is used to take a snapshot of the current dynamic controller parameters of this bot. \e paramInfo
			is a human readable string that contains the desired information.

			Note that, this method is for debugging purposes only. Please use binary versions of serializeDynamicControllerParameters() 
			and deserializeDynamicControllerParameters() if you would like to save/load the dynamic controller parameters.
		*/
		virtual void							serializeDynamicControllerParameters( std::string& paramInfo );


		/**
			This method is used to take a snapshot of the current dynamic controller parameters of this bot. \e paramInfo
			is a byte array that contains the desired information.

			@see deserializeDynamicControllerParameters()
		*/
		virtual void							serializeDynamicControllerParameters( std::vector<unsigned char>& paramInfo );

		/**
			This method is used to restore the dynamic controller parameters of the bot from a byte array that contains the parameter information. 			
			\e paramInfo is the byte array that contains the information.

			@see serialize()
		*/
		virtual bool							deserializeDynamicControllerParameters( const std::vector<unsigned char>& paramInfo, std::vector<unsigned char>& remainder );


		/**
			This method is used to take a snapshot of the current state of this bot. \e stateInfo
			is a human readable string that contains the state information.

			Note that, this method is for debugging purposes only. If you would like to save/load
			bot state, please use binary versions of serialize() and deserialize().
		*/
		virtual void							serialize( std::string& stateInfo );


		/**
			This method is used to take a snapshot of the current state of this bot. \e stateInfo
			is a a byte array that contains the state information.

			@see deserialize()
		*/
		virtual void							serialize( std::vector<unsigned char>& stateInfo );

		/**
			This method is used to restore the state of the bot from a byte array that contains the state information. 			
			\e stateInfo is the byte array that contains the state information.

			@see serialize()
		*/
		virtual bool							deserialize( const std::vector<unsigned char>& stateInfo);


		/**
			Returns the time that passed since the last BotBase::reset() call. Returned value is in seconds.
		*/
		double									getTimeSinceReset() const	{ return s_timeSinceReset.getValue(); }
	protected:
	
		RTD::VehicleBase*				m_vehicle;

		std::string						m_name;
	
		bool							m_isBotCorrectlyConfigured;

		virtual ~BotBase( void );

		/** This method is called before the master controller is updated. Developers can reimplement this
			method.*/
		virtual void					preManeuverUpdate( smReal deltaTime ) {}

		/** This method is called after the master controller is updated. Developers can reimplement this
			method. */
		virtual void					postManeuverUpdate( smReal deltaTime ) {}

		/**
			This method is made protected to prevent that the name that is given in the constructor is
			changed later. Bot names are also unique ID's and changing them might create problems.
		*/
		virtual void setName( const std::string& name )  {}
		
	private:
		StateVariableDouble				s_timeSinceReset;

		StateVariableInt				s_masterController;
		StateVariableInt				s_prevMasterController;
		StateVariableInt				s_controllerToFinalize;
		StateVariableInt				s_controllerToInitialize;		

		smManager<BotControllerBase>	m_controllers;

		std::string						m_emptyString;

	};

// Please note the use of "this->" for class members. This is required by g++.
#define DYNAMIC_CONTROLLER_PARAM_DECL(name,datatype,RTDdatatype)	name() { this->m_dataType = RTDdatatype; }\
														virtual DynamicControllerParameterBase* clone() const {	return new name(*this,true);	}\
														name&  operator=( const datatype& rhs )\
														{\
															this->m_value = (datatype) rhs;\
															this->m_isSpecified = true;\
															return *this;\
														}\
														name& operator=( const name& rhs )\
														{\
															if ( rhs.m_isSpecified )\
															{\
																this->m_value	=	rhs.m_value;\
																this->m_isSpecified = true;\
															}\
															else if ( rhs.m_returnDefaultWhenUnspecified )\
															{\
																if ( rhs.m_hasDefault )\
																{\
																	this->m_value = rhs.m_default;\
																	this->m_isSpecified = true;\
																}\
																else\
																{\
																	RTD::validateAndWarn(this->hasDefault() , "Right hand side parameter does not have a default but m_returnDefaultWhenUnspecified is set true." , "RTD::DynamicControllerParameter::operator=()," + this->getPathAndName() + " ."  );\
																}\
															}\
															else\
															{\
																this->m_isSpecified = false;\
																if ( this->m_returnDefaultWhenUnspecified && this->m_hasDefault)\
																	this->m_value = this->m_default;\
															}\
															return *this;\
														}\
														public:\
											

#define DYNAMIC_CONTROLLER_PARAM_DECL_WITH_COPY(name,datatype,RTDdatatype, ancestor) DYNAMIC_CONTROLLER_PARAM_DECL(name,datatype,RTDdatatype)\
																					 name( const name& rhs , bool copyNameAndPath):ancestor(rhs,copyNameAndPath)\
																					 {\
																					 }\
				 																	 name( const name& rhs  ):ancestor(rhs)\
																					 {\
																					 }\

/**
	Base class for Dynamic Controller Parameters.

	More information on Dynamic Controller Parameters can be found in Bots and Controllers section
	of the Programmer's Guide.
*/
class RTDYNAMICS_EXPORT DynamicControllerParameterBase : public RTDVariable
	{
	public:
		/** 
			Constructor of the class. A newly created dynamic parameter is unspecified by default. 			
		*/
		DynamicControllerParameterBase():
		  RTDVariable(),
		  m_hasDefault( false ),
		  m_isSpecified( false ),
		  m_returnDefaultWhenUnspecified( false ),
		  m_owner( NULL ),
		  m_path("")
		{
		}


		/**
			Copy constructor. An important thing to note is that the copy does \e not
			belong to the same owner as the original, instead it is a free parameter
			(no owner). Only the content is copied but not the ownership. 
		*/
		DynamicControllerParameterBase( const DynamicControllerParameterBase& rhs , bool copyNameAndPath  )
		{			
			m_owner			=	NULL; 
			
			if ( copyNameAndPath )
			{
				m_name			=	rhs.m_name;
				m_path			=	rhs.m_path;
			}
			copyContentAndState( &rhs );

		}

		DynamicControllerParameterBase( const DynamicControllerParameterBase& rhs   )
		{			
			m_owner			=	NULL; 
			m_name			=	rhs.m_name;
			m_path			=	rhs.m_path;
			copyContentAndState( &rhs );
		}

	  /** Destructor of the class. */
		virtual ~DynamicControllerParameterBase()
		{
			if ( m_owner )
				m_owner->m_dynamicParameters.remove( this );
		}

		/** Returns the name of the parameter. */
		const std::string&	getName() const	{	return m_name;	}

		/** Returns the path of the parameter in the parameter tree. */
		const std::string&	getPath() const	{	return m_path;	}

		/** Returns the full name and path of the parameter. */
		std::string 		getPathAndName() const	{	return m_path + "/" + m_name;	}

		/** Returns true if the parameter is specified. */
		bool	isSpecified() const			{	return m_isSpecified;	}

		/** Using this method the parameter can be set "unspecified". */
		void	setUnspecified()			{	m_isSpecified = false;	}

		/** Returns true if the parameter has a default value. */
		bool	hasDefault() const								{	return m_hasDefault;	}
		
		/** Removes the default value of the parameter. */
		void	removeDefault()									{	m_hasDefault = false;	}
		
		/** Enables/disables returning the default value in case parameter is not specified. */
		void	setReturnDefaultWhenUnspecified( bool enable )	{	m_returnDefaultWhenUnspecified = enable;	}
		
		/** Returns true if the returning default value in case parameter is unspecified is enabled. */
		bool	getReturnDefaultWhenUnspecified() const			{	return m_returnDefaultWhenUnspecified ;	}

		/** Sets the parameter to its default value if the default value is already set. */
		virtual void	resetToDefault() = 0;


		/** Creates a clone of the dynamic parameter. Newly created parameter will not belong to any controller.*/
		virtual DynamicControllerParameterBase*	clone() const	=	0;

		/** Returns a human readable string that contains state and value information of this dynamic controller parameter. 
		
			@see toString()
			@see toBinary()
			@see fromBinary()
		*/
		virtual std::string		toString() const { return "NotSupported NotSupported";}
		
		/** Sets the state and value of this dynamic controller parameter from a user provided string. Note that
			conversion from/to string is subject to floating point errors. For exact conversions see toBinary() and
			fromBinary().
		
			@see toString()
			@see toBinary()
			@see fromBinary()
		*/
		virtual bool			fromString( const std::string& str ){ return true; }; 


		/** Returns a byte array that contains state and value information of this dynamic controller parameter. 
						
			@see toString()
			@see toBinary()
			@see fromBinary()

		*/
		virtual std::vector<unsigned char>		toBinary() const	{	return std::vector<unsigned char>(); }

		/** Sets the state and value of this dynamic controller parameter from a user provided byte array.
		
			@see toString()
			@see toBinary()		
		*/		
		virtual bool							fromBinary( const std::vector<unsigned char>& data )	{		return true;	}

		/**
			Copies content and state of \e rhs to this dynamic controller parameter.
		*/
		virtual	bool			copyContentAndState( const DynamicControllerParameterBase* rhs )
		{
			// Copying contents and state. Note that, owner is not set, also the name is not changed.
			m_isSpecified	=	rhs->m_isSpecified;
			m_hasDefault	=	rhs->m_hasDefault;
			m_returnDefaultWhenUnspecified=	rhs->m_returnDefaultWhenUnspecified;		
			return true;
		}

	protected:
		bool	m_isSpecified;
		bool	m_hasDefault;
		bool	m_returnDefaultWhenUnspecified;		
		std::string m_path;

		BotControllerBase* m_owner;

		virtual void registerToController( BotControllerBase* controller )
		{
			controller->m_dynamicParameters.add( this );
		}


	};


	/**
		Dynamic Controller Parameter template class definiton. This template
		can be used to create dynamic parameters of different types.

		More information on Dynamic Controller Parameters can be found in Bots and Controllers section
		of the Programmer's Guide.
	*/
	template<typename T>
	class DynamicControllerParameter : public DynamicControllerParameterBase
	{
	public:

		 DynamicControllerParameter( const DynamicControllerParameter& rhs , bool copyNameAndPath ):
			DynamicControllerParameterBase( rhs , copyNameAndPath ),
			m_default( rhs.m_default ),
			m_value( rhs.m_value )
		{
			
		}

		DynamicControllerParameter( const DynamicControllerParameter& rhs ):
			DynamicControllerParameterBase( rhs ),
			m_default( rhs.m_default ),
			m_value( rhs.m_value )
		{
			
		}

		DYNAMIC_CONTROLLER_PARAM_DECL( DynamicControllerParameter<T> , T , this->GENERIC );

		/** Returns the value of the parameter. When a value or default can not be returned
			an error message is given and application exits.
		
			More information on Dynamic Controller Parameters can be found in Bots and Controllers section
			of the Programmer's Guide.
		*/
		const T&	value() const
		{
			if ( m_isSpecified )
			{
				return m_value;
			}
			else if ( m_returnDefaultWhenUnspecified && m_hasDefault)
			{
				m_value = m_default;
				return m_value;
			}
			else
			{
				validateAndExit(isSpecified() , "Trying to read the value of an unspecified parameter." , "RTD::DynamicControllerParameter::value()," + getPathAndName() + " .");
			}

			return m_value;
		}


		/** Returns the value of the parameter. When a value or default can not be returned
			an error message is given and application exits.

			This method is especially useful if the data type is not a basic type, for example a smVec3r.
			Lets say myParam is a dynamic controller parameter of type smVec3r. If we would like to access
			the first value of the vector, we would write myParam[0], however, C++ compiler will not accept
			this code. In this case we can write myParam.value()[0].
		
			More information on Dynamic Controller Parameters can be found in Bots and Controllers section
			of the Programmer's Guide.
		*/
		T&	value()
		{
			if ( m_isSpecified )
			{
				return m_value;
			}
			else if ( m_returnDefaultWhenUnspecified && m_hasDefault)
			{
				m_value = m_default;
				return m_value;
			}
			else
			{
				validateAndExit(isSpecified() , "Trying to access to the value of an unspecified parameter." , "RTD::DynamicControllerParameter::value()," + getPathAndName() + " .");
			}

			return m_value;
		}



		/** Type casting operator. This operator calls value() method so error cases
			that are defined for value() also applies here. */
		operator T() const
		{
			return value();
		}

		/** Returns the default value. */
		T&		defaultValue()									{	return m_default;	}
		
		/** Returns the default value. */
		const T&	defaultValue() const						{	return m_default;	}
		
		/** Initializes the parameter. Note that, init() sets the default value of the parameter but not the value of the parameter. Unless 
			a value is explicitly assigned to a dynamic controller parameter, it is "Unspecified". This is the default state.			
			\param name   Name of the parameter
			\param owner  Owner of the parameter. If the parameter doesn't have to have an owner, init() method shouldn't be called.
			\param def    Default value of the parameter
			\param returnDefaultWhenUnspecified   If this argument is true, when the parameter is not specified, the default value will be returned.
												  If it is false then returning the default is disabled. 
   	    */
		void	init( const std::string& name , BotControllerBase* owner, const T& def  , bool returnDefaultWhenUnspecified )	
		{	
			validateAndWarn( owner!=NULL , "NULL pointer is not acceptable as owner." , "RTD::DynamicControllerParameter::init()," + getPathAndName() + " .");
			validateAndWarn( m_owner==NULL , "Parameter already initialized." , "RTD::DynamicControllerParameter::init()," + getPathAndName() + " .");

			m_owner = owner;
			registerToController(m_owner);

			m_default = def; 
			m_hasDefault= true; 
			m_returnDefaultWhenUnspecified = returnDefaultWhenUnspecified;	
			m_name =  name;
			m_path =  owner->getName();
		}

		/** Initializes the parameter. Note that after the init() the parameter is in "Unspecified" state, i.e. does not have a value.
			Unless a value is explicitly assigned to a dynamic controller parameter, it is "Unspecified".
			\param name   Name of the parameter
			\param owner : Owner of the parameter. If the parameter doesn't have to have an owner, init() method shouldn't be called.
   	    */
		void	init( const std::string& name , BotControllerBase* owner )	
		{	
			validateAndWarn( owner!=NULL , "NULL pointer is not acceptable as owner." , "RTD::DynamicControllerParameter::init()," + getPathAndName() + " .");
			validateAndWarn( m_owner==NULL , "Parameter already initialized." , "RTD::DynamicControllerParameter::init()," + getPathAndName() + " .");

			m_owner = owner;
			registerToController(m_owner);

			m_hasDefault= false; 
			m_returnDefaultWhenUnspecified = false;		
			m_name =  name;
			m_path =  owner->getName();
		}

		/** Sets the default value. */
		void	setDefault(  const T& def  )	
		{
			m_default = def; 
			m_hasDefault= true; 
		}

		/** Resets the parameter's value to its default. If a default is not specified an error occurs and application
			exists. */ 
		virtual void	resetToDefault()
		{
			validateAndWarn(hasDefault() , "Parameter does not have a default." , "RTD::DynamicControllerParameter::resetToDefault()");
			m_value = m_default;
			m_isSpecified= true;
		}

		virtual	bool	copyContentAndState( const DynamicControllerParameterBase* rhs ) 
		{			
			const DynamicControllerParameter<T>* rhs_casted = dynamic_cast< const DynamicControllerParameter<T>* >( rhs );
			if ( rhs_casted )
			{
				DynamicControllerParameterBase::copyContentAndState( rhs );
				m_value = rhs_casted->m_value;
				m_default =  rhs_casted->m_default;					
				return true;
			}

			return false;
		}

	protected:

		T			m_default;
		mutable T	m_value;
	};


	template<typename T>
	class DynamicControllerParameterBasicType : public DynamicControllerParameter<T>
	{
	public:		
		DYNAMIC_CONTROLLER_PARAM_DECL_WITH_COPY( DynamicControllerParameterBasicType<T> , T , this->GENERIC , DynamicControllerParameter<T>  );


	};


	template<typename T>
	class DynamicControllerParameterEnum : public DynamicControllerParameter<T>
	{
	public:		
		DYNAMIC_CONTROLLER_PARAM_DECL_WITH_COPY( DynamicControllerParameterEnum<T> , T , this->ENUM ,DynamicControllerParameter<T> );

		virtual void getLoggableData( std::vector<double>& data) const
		{
			data.clear();
			if ( this->isSpecified() || (this->getReturnDefaultWhenUnspecified() && this->hasDefault()) )
			{
				data.push_back( (double) this->value() );
			}
			else
			{
				data.push_back( 0.0 );
			}
		}

		virtual std::vector<unsigned char>		toBinary() const
		{
			std::vector<unsigned char> result;
			if ( this->isSpecified()  )
			{
				result.push_back(1);
			}
			else
			{
				result.push_back(0);
			}
			
			if ( this->isSpecified() || (this->getReturnDefaultWhenUnspecified() && this->hasDefault()) )
			{
				unsigned int tmp = this->m_value;
				appendDataToByteArray<unsigned int>(tmp,result);
			}

			return result;	
			
		}
		
		virtual bool	fromBinary( const std::vector<unsigned char>& data )
		{
			if ( data.size() != sizeof(this->m_value)+1 )
			{
				printf("RTD::DynamicControllerParameterEnum::fromBinary: Size mismatch (%s)!n" , this->getPathAndName().c_str() );
				return false;
			}
		
			unsigned int tmp;
			getDataFromByteArray<unsigned int>( data , 1 , tmp );
		
			if ( data[0] == 1 )
			{
				*this = (T)tmp;
			}
			else if ( data[0] == 0 )
			{
				*this = (T)tmp;
				this->setUnspecified();	
			}
			else
			{
				printf("RTD::DynamicControllerParameterEnum::fromBinary: Specified/Unspecified flag is not 0 or 1!n");
				return false;
			}
		
			return true;
		}

		virtual std::string		toString() const
		{
			std::ostringstream ostr;
			ostr.precision( 30 );

			if ( this->isSpecified()  )
			{
				ostr << "Specified";
			}
			else
			{
				ostr << "Unspecified";
			}
			
			if ( this->isSpecified() || (this->getReturnDefaultWhenUnspecified() && this->hasDefault()) )
				ostr << " " << (unsigned int ) this->value();

			return ostr.str();	
		}

		virtual bool			fromString( const std::string& str )
		{
			std::istringstream istr(str);
			
			std::string specifiedStr;
			
			istr >> specifiedStr;

			if ( specifiedStr == "Specified" )
			{
				// Note: We need the temp here because if we access value() (i.e. istr >> value() ) of an
				// currently unspecified variable it will give an assert.
				unsigned int tmp;
				istr >> tmp;
				*this = (T)tmp;
			}
			else if (  specifiedStr == "Unspecified" )
			{
				// Note: We need the temp here because if we access value() (i.e. istr >> value() ) of an
				// currently unspecified variable it will give an assert.
				unsigned int tmp;
				istr >> tmp;
				*this = (T)tmp;
				this->setUnspecified();
			}
			else
			{
				return false;
			}
			
			return true;
		}

	};


#define DEFINE_DCP_BASICTYPE( name , datatype, RTDdatatype )	class RTDYNAMICS_EXPORT name	:	public DynamicControllerParameterBasicType<datatype>\
																{\
																	public:\
																		DYNAMIC_CONTROLLER_PARAM_DECL_WITH_COPY( name , datatype ,RTDdatatype,DynamicControllerParameterBasicType<datatype> );\
																		virtual void getLoggableData( std::vector<double>& data) const;\
																		virtual std::vector<unsigned char>		toBinary() const;\
																		virtual bool							fromBinary( const std::vector<unsigned char>& data );\
																		virtual std::string		toString() const;\
																		virtual bool			fromString( const std::string& str );\
																};\



DEFINE_DCP_BASICTYPE( DynamicControllerParameterReal	, smReal		, REAL )
DEFINE_DCP_BASICTYPE( DynamicControllerParameterDouble  , smDouble		, DOUBLE)
DEFINE_DCP_BASICTYPE( DynamicControllerParameterString  , std::string	, STRING )
DEFINE_DCP_BASICTYPE( DynamicControllerParameterBool	, bool			, BOOL)
DEFINE_DCP_BASICTYPE( DynamicControllerParameterInt		, int			, INT)
DEFINE_DCP_BASICTYPE( DynamicControllerParameterUInt	, unsigned int	, UNSIGNED_INT )
DEFINE_DCP_BASICTYPE( DynamicControllerParameterVec2r	, smVec2r		, VEC2R )
DEFINE_DCP_BASICTYPE( DynamicControllerParameterVec3r	, smVec3r		, VEC3R )
DEFINE_DCP_BASICTYPE( DynamicControllerParameterVec4r	, smVec4r		, VEC4R )
DEFINE_DCP_BASICTYPE( DynamicControllerParameterVec2d	, smVec2d		, VEC2D )
DEFINE_DCP_BASICTYPE( DynamicControllerParameterVec3d	, smVec3d		, VEC3D )
DEFINE_DCP_BASICTYPE( DynamicControllerParameterVec4d	, smVec4d		, VEC4D )





}
