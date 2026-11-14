/*
(c) Copyright 2007, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once




#include "RTD/RTD.h"
#include "RTD/MBDVehicle.h"


#include "RTD/FGWing.h"
#include "RTD/FGAirDrag200.h"
#include "RTD/FGTorque.h"
#include "RTD/FGGroundContactBase.h"
#include "RTD/EngineModelBase.h"
#include "RTD/Instruments.h"


namespace GroundLibDDM
{
	
	class RTDYNAMICS_EXPORT GenGroundVehicle100	: public RTD::MBDVehicle 
										
	{
	public:
		/**
		*	Configuration parameters of the vehicle.
		*/
		class RTDYNAMICS_EXPORT ConfigParams : public RTD::VehicleBase::ConfigParams
		{
			friend class GenGroundVehicle100;
		public:
			/** Constructor of the class.	*/
			ConfigParams();

			/** Configuration parameters of body drag model. */
			RTD::FGAirDrag200::ConfigParams		m_bodyDragParams;		

			/**
				A utility method that parses a character stream which contains initialization
				parameters. The \e parameterString is in XML. If there is an error false is returned
				and \e error will contain the reason.

				GenGroundVehicle100::ConfigParams::load() method uses this method to parse the parameter file.
			*/
			virtual bool	readParamStr( const std::string& parameterString , std::string& error );

		protected:
			/** Copy constructor of the class. Deep copying is not supported and can result in
				hard to catch bugs if used. Because of this copy constructor is disabled. */
			ConfigParams( const ConfigParams& rhs );

			/** Assignment operator of the class. Deep copying is not supported and can result in
				hard to catch bugs if used. Because of this copy assignment operator is disabled. */
			virtual ConfigParams&	operator=(const ConfigParams& rhs );

			/** Do not allow a ConfigParams instance on the stack. ConfigParams should be a pointer
				always. */
			virtual ~ConfigParams();

		};

	public:
		/**
			Constructor of the class.
			\param world	:	An instance of the RTD::World class which stores environmental information like wind or gravity.                                

			\param name			A unique name for the vehicle. If there is another vehicle with the same name
								within the same world, the newly created aircraft will not be added to the world.
								Note that, even if an entity is deleted, its name remains reserved until World::reset()
								is called.

			\param params	:	\e params stores the configuration parameters of the vehicle. The \e params
								pointer will \e not be deleted automatically.
		*/
		GenGroundVehicle100(	RTD::World* world,
					const std::string& name,
					ConfigParams* params  );

		/**
			Constructor of the class.
			\param world	:	An instance of the RTD::World class which stores environmental information like wind or gravity.                                

			\param name			A unique name for the vehicle. If there is another vehicle with the same name
								within the same world, the newly created aircraft will not be added to the world.
								Note that, even if an entity is deleted, its name remains reserved until World::reset()
								is called.

			\param parameterFile :	Name of an XML based configuration file. When this constructor is called, GenGroundVehicle100
                                    will be initialized with this configuration file. If there is a problem in reading the file,
									vehicle will be disabled.  

		*/
		GenGroundVehicle100( RTD::World* world , const std::string& name, const std::string& parameterFile );


		/**
			Updates the GenGroundVehicle100 by advancing the simulation \e stepTime seconds.
		*/
		virtual void	preUpdate( smTime stepTime ) ;
		virtual void	postUpdate( smTime stepTime ) ;

		/** Resets the class. */
		virtual void	reset() ;

		RTD::FGAirDrag200*			getAirDrag()				{	return m_airDrag;	}

		ConfigParams*				getConfigParams()			{	return m_configParams;	}
		const ConfigParams*			getConfigParams() const		{	return m_configParams;	}


		protected:
		ConfigParams*				m_configParams;

		RTD::FGAirDrag200*			m_airDrag ;

		unsigned int				m_firstDataLoggerChannelIndex;

		/** Destructor of the class. The destructor is intentionally declared as a protected destructor. Use remove() method to delete an instance of this class. */
		virtual ~GenGroundVehicle100( );

		void						createLoggingPrinter() ;
		virtual	void				create();

		
		virtual void				updateDataLogger();

	};
}

/**
*	\namespace GroundLibDDM
*
*   The GroundLibDDM namespace includes high fidelity vehicle driving dynamics
*   model (DDM) for vehicle simulation.
*/

