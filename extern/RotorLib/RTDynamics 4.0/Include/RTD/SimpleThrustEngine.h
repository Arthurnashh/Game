/*
(c) Copyright 2006, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/

#pragma once

#include "RTD/EngineModelBase.h"


namespace RTD
{
	class FGSimpleThrust;

	/**
		A basic engine implementation which creates a thrust.
	*/
	class RTDYNAMICS_EXPORT SimpleThrustEngine :	public EngineModelBase
	{
	public:

		/**
			Configuration parameters of the engine.
		*/
		class RTDYNAMICS_EXPORT ConfigParams : public EngineModelBase::ConfigParams
		{
		public:
			/**
				Constructor of the class.
			*/
			ConfigParams();

			virtual ~ConfigParams();

			/** Maximum thrust in Newtons. */
			smReal	m_maxThrust;

			/** When throttle is changed the thrust produced by the engine changes as well.
				Using this parameter the speed of this change can be adjusted. The larger is
				reaction time the slower will the engine reach the target thrust value. */
			smReal	m_reactionTimeCoefficient;

			/**
				The position at which the engine is attached to the aircraft. Members of
				the vector are in meter.
			*/
			smVec3r	m_position;

			/**
				Direction of the engine. The thrust produced by the engine is in the opposite direction.
			*/
			smVec3r	m_direction;

			/**
				A utility method that parses a character stream which contains initialization
				parameters. The \e parameterString is in XML. If there is an error false is returned
				and \e error will contain the reason.
			*/
			virtual bool			readParamStr( const std::string& parameterString , std::string& error );
		};

		/**
			Creates an instance of SimpleThrustEngine.
			\param vehicle		:	The vehicle to which the engine is attached.
			\param name			:   Name of the engine instance.
			\param params		:	Configuration parameters of the engine.
		*/
		static	EngineModelBase*					createClass( RTD::VehicleBase* vehicle, const std::string& name , const ConfigParams* params  );

		/**
			Creates a ConfigParams structure which can be used to initialize an SimpleThrustEngine instance.
		*/
		static	EngineModelBase::ConfigParams*	createParameters();

		/**
			Constructor of the class.
			\param vehicle		:	The vehicle to which the engine is attached.
			\param name			:   Name of the engine instance.
			\param params		:	Configuration parameters of the engine.
		*/
		SimpleThrustEngine( RTD::VehicleBase* vehicle, const std::string& name , const ConfigParams* params );

		/**
			Destructor of the class.
		*/
		virtual ~SimpleThrustEngine();


		/** Returns the type of the engine. For this engine it returns "SimpleThrustEngine". */
		virtual std::string		getType() const;

		/** Updates the SimpleThrustEngine by advancing the simulation \e stepTime seconds. */
		virtual void	update( smTime stepTime );

		/** Resets the engine. */
		virtual void			reset();

		/** Returns the force generator. */
		virtual ForceGenerator*	getForceGenerator();

		/** Returns the max thrust that can be produced by this engine in Newtons. */
		smReal	getMaxThrust() const;

		/** Returns force produced by the engine in Newtons. */
		virtual	smReal	getForce() const;

		/**
			Returns the configuration parameters of the engine. 
		*/
		ConfigParams*	getConfigParams()				{	return &m_configParams;	}

		/**
			Returns the configuration parameters of the engine. 
		*/
		const ConfigParams*	getConfigParams() const		{	return &m_configParams;	}

	protected:
		RTD::FGSimpleThrust*	m_thruster;

		ConfigParams			m_configParams;

		virtual smDouble	getExternalTorque() const { return 0.0; }
		virtual smDouble	getRotationalInertia() const { return 0.0; }

	};

}