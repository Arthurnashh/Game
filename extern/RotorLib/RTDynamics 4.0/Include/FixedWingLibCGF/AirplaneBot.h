/*
(c) Copyright 2009, RTDynamics
All Rights Reserved.

Product:        RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/

#pragma once
#include "RTD/BotBase.h"
#include "FixedWingLibCGF/SimplifiedAirplane.h"
#include "FixedWingLibCGF/FollowRouteController.h"
#include "FixedWingLibCGF/BasicFlightController.h"
#include "FixedWingLibCGF/BasicLowLevelController.h"
#include "FixedWingLibCGF/VelocityController.h"
#include "FixedWingLibCGF/FlightPathController.h"
#include "FixedWingLibCGF/PositionController.h"
#include "FixedWingLibCGF/LoiterController.h"
#include "FixedWingLibCGF/BankLeftRightController.h"
#include "FixedWingLibCGF/RaceTrackController.h"
#include "FixedWingLibCGF/LandingController.h"
#include "FixedWingLibCGF/TakeoffController.h"
#include "FixedWingLibCGF/BasicTaxiController.h"

#ifdef RTDYNAMICS_EXPORT_MILITARY_FEATURES
#include "FixedWingLibCGF/DiveBombingController.h"
#include "FixedWingLibCGF/GroundAvoidanceController.h"
#include "FixedWingLibCGF/HardTurnController.h"
#include "FixedWingLibCGF/ImmelmannController.h"
#include "FixedWingLibCGF/STurnController.h"
#include "FixedWingLibCGF/FollowEntityController.h"
#include "FixedWingLibCGF/SplitSController.h"
#include "FixedWingLibCGF/PursuitController.h"
#include "FixedWingLibCGF/ExtensionController.h"
#include "FixedWingLibCGF/HighYoyoController.h"
#include "FixedWingLibCGF/LowYoyoController.h"
#include "FixedWingLibCGF/LiftVectorTurnController.h"
#include "FixedWingLibCGF/BreakTurnController.h"
#include "FixedWingLibCGF/HeadOnController.h"
#include "FixedWingLibCGF/Degree90Controller.h"
#include "FixedWingLibCGF/HighSpeedGunJinkController.h"
#include "FixedWingLibCGF/LowSpeedGunJinkController.h"
#include "FixedWingLibCGF/ReversalController.h"
#include "FixedWingLibCGF/BarrelRollController.h"
#include "FixedWingLibCGF/ScissorController.h"
#include "FixedWingLibCGF/DisengEgressController.h"
#include "FixedWingLibCGF/VTOLPositionController.h"
#include "FixedWingLibCGF/VTOLVelocityController.h"
#include "FixedWingLibCGF/VTOLLandingController.h"
#include "FixedWingLibCGF/VTOLTakeoffController.h"
#endif

namespace FixedWingLibCGF
{

    /**
		This class implements a bot that can fly airplanes. A bot can be thought of as a virtual pilot.

		@see Bots and Controllers section in the Programmer's Guide.
		@see FixedWingLib CGF section in the Programmer's Guide.
    */
    class RTDYNAMICS_EXPORT AirplaneBot : public RTD::BotBase
    {
    public:
        /**
			Constructor of the class. 
		
			After the bot is created, it flies the airplane straight and level from its current position and at its current 
			altitude and heading.
		
			@param  airplane        The airplane which is to be controlled by this bot.
        
			@param  name            A unique name for the bot. If there is another bot with the same name
									within the same world, the newly created bot will not be added to the world.
									Note that, even if an entity is deleted, its name remains reserved until World::reset()
									is called.

			@param  parameterFile   Configuration file which contains the parameters of the bot. The format of this 
									configuration file is explained in the FixedWingLib CGF section of the Programmer's 
									Guide.
        */
        AirplaneBot( SimplifiedAirplane* airplane , const std::string& name , const std::string& parameterFile   );

		/**
			This method calls the RTD::BotBase::reset() and resets all controllers. After that, it orders the bot to 
			fly the airplane straight and level from its current position and at its current altitude and heading.
		*/
		virtual void reset();

        /**
            @name Utilities
        */

        //@{
        /** Returns the airplane controlled by this bot. */
        SimplifiedAirplane* getVehicle();

        /** Returns the airplane controlled by this bot. */
        const SimplifiedAirplane* getVehicle() const;
        //@}



        /**
            @name Queries For Current Physical State
        */

        /** Returns the velocity vector of the airplane in m/s. */
        virtual void    getVelocity( smVec3r& v ) const;

        /** Returns the acceleration vector of the airplane in m/s^2. */
        virtual void    getAcceleration( smVec3r& a ) const;

        /** Returns the position of the airplane in meters.  */
        virtual void    getPosition( smVec3r& p ) const;

        /** Returns the attitude of the airplane. Returned
            vector contains heading,pitch and roll angles in degrees.
        */
        virtual void    getAttitude( smVec3r& hpr ) const;

        /** Returns the attitude of the airplane as a rotation matrix. Returned matrix is in mathematical
        *   convention.
        */
        virtual void    getAttitude( smMatrix& mat ) const;


        /** Returns the angular velocity of the airplane in degrees/s. */
        virtual void    getRotVelocity( smVec3r& rv ) const;

        /** Returns the angular acceleration of the airplane in degrees/s^2. */
        virtual void    getRotAcceleration( smVec3r& ra ) const;

        //@}

        /** @name Direction Vectors
		*		  These methods return vectors pointing to the
        *         front, right and up of the vehicle respectively. Returned vectors
		*		  are in world coordinates.
        */
        //@{
        virtual void    getForwardVector( smVec3r& forward ) const;
        virtual void    getSidewardVector( smVec3r& sideward ) const;
        virtual void    getUpwardVector( smVec3r& upward ) const;
        //@}
     


        /** This method can be used to set the control inputs directly. \e lateralStick, \e longStick and \e pedals
        *   must be between -1 and 1. \e throttle and \e airbrakes must be between 0 and 1.
        */
        virtual void    directControl( smReal lateralStick , smReal longStick , smReal pedals , smReal throttle , smReal airbrakes );


        /**
            @name Command Interface : Low Level Commands

			Low-level commands set the Basic Low Level Controller as the master. They are used to 
			perform simple actions such as rolling to a certain bank angle or setting the throttle.

			Detailed information regarding the low level commands can be found in the "Bot Commands"
			section of the "FixedWingLib CGF" chapter in the Programmer's Guide.
        */

        //@{

        virtual void    lowLevelControl(const BasicLowLevelController::Params& params )
        {
            setMasterController( getBasicLowLevelController() );
            getBasicLowLevelController()->m_dynParams = params;
        }

        virtual void    llcSetRollAngle( smReal angle , bool isRelative ,  smReal rateFactor );
        virtual void    llcSetPitchAngle( smReal angle , bool isRelative ,  smReal rateFactor );
        virtual void    llcSetSpeed( smReal speedKts );
        virtual void    llcSetThrottle( smReal throttle );
        virtual void    llcSetAirbrakeStatus( smReal percentage );

		//@}

        /**
            @name Command Interface : High Level Commands

			High-level commands make use of different controllers and they are used to perform relatively complex 
			tasks that might require multiple consecutive maneuvers (e.g. following a route) or coordination 
			of multiple pilot inputs (e.g. coordinated turn).

			Detailed information regarding the high level commands can be found in the "Bot Commands"
			section of the "FixedWingLib CGF" chapter in the Programmer's Guide.
        */
        //@{
		virtual void	velocityHold( const VelocityController::Params& params );
		virtual void	basicFlight( const BasicFlightController::Params& params );
        virtual void	levelFlight();
		virtual void	bankLeftRight( const BankLeftRightController::Params& param );
		virtual void    flyTo( const PositionController::Params& params );
		virtual void    followRoute( FollowRouteController::Params& params );                
		virtual void    loiter( const LoiterController::Params& params ) ;
		virtual void    flyRaceTrack( const RaceTrackController::Params& params ) ;		              
		virtual void    land( const LandingController::Params& params ) ;		              
		virtual void    takeOff( const TakeoffController::Params& params );
        virtual void    taxiAlongRoute( const TaxiRouteController::Params& params );
        virtual void    basicTaxi( const BasicTaxiController::Params& params );
        virtual void    taxiTo( const TaxiPositionController::Params& params );
        //@}


		#ifdef RTDYNAMICS_EXPORT_MILITARY_FEATURES

		/** @name Common Military Maneuvers.        
			
			Following maneuvers are available only if at least one of Air Combat Maneuvers or 
			Ground Attack Maneuvers features are enabled in the license.

			@param   params   A simple class which contains the dynamic parameters of the maneuver.
		*/
        //@{
		virtual void    cmmGroundAvoidance  ( const GroundAvoidanceController::Params& params ) ;				
		virtual void    cmmFollowEntity		( const FollowEntityController::Params& params ) ;
		virtual void    cmmSTurn            ( const STurnController::Params& params ) ; 
		virtual void    cmmVTOLHoverTo      ( const VTOLPositionController::Params& params ) ; 
		virtual void    cmmVTOLLand		    ( const VTOLLandingController::Params& params ) ; 
		virtual void    cmmVTOLTakeoff      ( const VTOLTakeoffController::Params& params ) ; 

        //@}


		/** @name Air-to-ground attack maneuvers.        
			
			Following maneuvers are available only if Ground Attack Maneuvers feature is enabled in the license.
		
			@param   params   A simple class which contains the dynamic parameters of the maneuver.
		*/
        //@{
        
		virtual void atgDiveBombing( const DiveBombingController::Params& params )
		{
			if ( getMasterController() == m_diveBombingController )
				setMasterController( NULL );

			setMasterController( m_diveBombingController );
			m_diveBombingController->m_dynParams = params ;
		}
		//@}

		
        /** @name Air Combat Maneuvers
		
			Following maneuvers are available only if Air Combat Maneuvers feature is enabled
			in the license.

			@param   params   A simple class which contains the dynamic parameters of the maneuver.
		**/
        //@{
        
        virtual  void	acmHardTurn         ( const HardTurnController::Params& params ) ;                
        virtual  void	acmImmelmann        ( const ImmelmannController::Params& params ) ;
        virtual  void	acmSplitS           ( const SplitSController::Params& params ) ;
        virtual  void	acmPursuit          ( const PursuitController::Params& params ) ;        
		virtual  void	acmHighSpeedGunJink ( const HighSpeedGunJinkController::Params& param );
		virtual  void	acmLowSpeedGunJink  ( const LowSpeedGunJinkController::Params& param );
        virtual  void	acmExtension        ( ) ;
        virtual  void	acmHighYoyo         ( const HighYoyoController::Params& param );
        virtual  void	acmBreakTurn        ( const BreakTurnController::Params& param );
        virtual  void	acmLiftVectorTurn   ( const LiftVectorTurnController::Params& param );
        virtual  void	acmLowYoyo          ( const LowYoyoController::Params& param );
        virtual  void	acmHeadOn           ( const HeadOnController::Params& param );
        virtual  void	acmDegree90         ( const  Degree90Controller::Params& param );
        virtual  void	acmDisengEgress     ( const DisengEgressController::Params& param );
        virtual  void	acmReversal         ( const ReversalController::Params& param );
        virtual  void	acmBarrelRoll       ( const  BarrelRollController::Params& params ) ;
        virtual  void	acmScissor          ( const  ScissorController::Params& params ) ;
        //@}
		#endif
	
        virtual void    accelerateDecelerate( smReal speedKts ) ;
 

       /** @name Const and non-const getter methods for controllers
         *  
         *  The getter methods can be used to get a pointer to a specific controller.
         **/
        //@{
        VelocityController*         getVelocityController()     {   return m_velocityController; }
        PositionController*         getPositionController()     {   return m_positionController; }
        BasicLowLevelController*    getBasicLowLevelController(){   return m_basicLowLevelController; }
        FollowRouteController*      getFollowRouteController()  {   return m_followRouteController;}
        BasicFlightController*      getBasicFlightController()  {   return m_basicFlightController;}
        FlightPathController*       getFlightPathController()   {   return m_flightPathController;}
        LoiterController*           getLoiterController()       { return m_loiterController ; }
		RaceTrackController*        getRaceTrackController()    { return m_raceTrackController ; }
		BankLeftRightController*    getBankLeftRightController(){ return m_bankLeftRightController ; }
		LandingController*			getLandingController()		{ return m_landingController ; }
		TakeoffController*			getTakeoffController()		{ return m_takeoffController ; }        
        BasicTaxiController*		getBasicTaxiController()	{ return    m_basicTaxiController ; }
        TaxiPositionController*		getTaxiPositionController()	{ return	m_taxiPositionController ; }
        TaxiRouteController*		getTaxiRouteController()	{ return    m_taxiRouteController ; }

		

		#ifdef RTDYNAMICS_EXPORT_MILITARY_FEATURES
		DiveBombingController*		getDiveBombingController()		{ return m_diveBombingController;}        
        HardTurnController*         getHardTurnController()         { return m_hardTurnController ; }
        GroundAvoidanceController*  getGroundAvoidanceController()  { return m_groundAvoidanceController ; }        
        ImmelmannController*        getImmelmannController()        { return m_immelmannController ; }
        SplitSController*           getSplitSController()           { return m_splitSController ; }
        STurnController*            getSTurnController()            { return m_sturnController ; }
        FollowEntityController*     getFollowEntityController()		{ return m_followEntityController ; }
        PursuitController*          getPursuitController()          { return m_pursuitController ; }
        ExtensionController*        getExtensionController()        { return m_extensionController ; }        
		HighYoyoController*			getHighYoyoController()			{ return m_highYoyoController;	}
		LowYoyoController*			getLowYoyoController()			{ return m_lowYoyoController;	}
		LiftVectorTurnController*	getLiftVectorTurnController()	{ return m_liftVectorTurnController;	}
		BreakTurnController*	    getBreakTurnController()	    { return m_breakTurnController;	}
		HeadOnController*	        getHeadOnController()	        { return m_headOnController;	}
		Degree90Controller*	        getDegree90Controller()	        { return m_degree90Controller;	}
		DisengEgressController*	    getDisengEgressController()	    { return m_disengEgressController;	}
		HighSpeedGunJinkController*	getHighSpeedGunJinkController()	{ return m_highSpeedGunJinkController;	}
		LowSpeedGunJinkController*	getLowSpeedGunJinkController()	{ return m_lowSpeedGunJinkController;	}
		ReversalController*	        getReversalController()	        { return m_reversalController;	}
		BarrelRollController*	    getBarrelRollController()	    { return m_barrelRollController;	}
		ScissorController*	        getScissorController()	        { return m_scissorController;	}

		VTOLPositionController*		getVTOLPositionController()    { return m_VTOLPositionController;	}
		VTOLVelocityController*		getVTOLVelocityController()    { return m_VTOLVelocityController;	}
		VTOLLandingController*		getVTOLLandingController()     { return m_VTOLLandingController;	}
		VTOLTakeoffController*		getVTOLTakeoffController()     { return m_VTOLTakeoffController;	}

		#endif

		
        const VelocityController*         getVelocityController() const   {   return m_velocityController; }
        const PositionController*         getPositionController() const   {   return m_positionController; }
        const BasicLowLevelController*    getBasicLowLevelController() const   {   return m_basicLowLevelController; }
        const FollowRouteController*      getFollowRouteController()const {   return m_followRouteController;}
        const BasicFlightController*      getBasicFlightController() const{   return m_basicFlightController;}
        const FlightPathController*       getFlightPathController() const {   return m_flightPathController;}
		const RaceTrackController*        getRaceTrackController() const        { return m_raceTrackController ; }
        const LoiterController*           getLoiterController() const           { return m_loiterController ; }
        const BankLeftRightController*    getBankLeftRightController() const    { return m_bankLeftRightController ; }
		const LandingController*		  getLandingController() const			{ return m_landingController ; }
		const TakeoffController*		  getTakeoffController() const			{ return m_takeoffController ; }
        const BasicTaxiController*		  getBasicTaxiController() const		{ return    m_basicTaxiController ; }
        const TaxiPositionController*	  getTaxiPositionController() const	    { return	m_taxiPositionController ; }
        const TaxiRouteController*		  getTaxiRouteController() const		{ return    m_taxiRouteController ; }

		#ifdef RTDYNAMICS_EXPORT_MILITARY_FEATURES
		const DiveBombingController*	  getDiveBombingController() const{   return m_diveBombingController;}
		const HardTurnController*         getHardTurnController() const         { return m_hardTurnController ; }
        const GroundAvoidanceController*  getGroundAvoidanceController() const  { return m_groundAvoidanceController ; }        
        const ImmelmannController*        getImmelmannController() const        { return m_immelmannController ; }
        const SplitSController*           getSplitSController() const           { return m_splitSController ; }
        const STurnController*            getSTurnController() const            { return m_sturnController ; }
        const FollowEntityController*       getFollowEntityController() const       { return m_followEntityController ; }
        const PursuitController*          getPursuitController() const          { return m_pursuitController ; }
        const ExtensionController*        getExtensionController() const        { return m_extensionController ; }        
		const HighYoyoController*		  getHighYoyoController() const			{ return m_highYoyoController;	}
		const LowYoyoController*		  getLowYoyoController() const			{ return m_lowYoyoController;	}
		const LiftVectorTurnController*   getLiftVectorTurnController() const 	{ return m_liftVectorTurnController;	}
		const BreakTurnController*	      getBreakTurnController() const	    { return m_breakTurnController;	}
		const HeadOnController*	          getHeadOnController()	const           { return m_headOnController;	}
		const Degree90Controller*	      getDegree90Controller() const         { return m_degree90Controller;	}
		const DisengEgressController*	  getDisengEgressController() const 	{ return m_disengEgressController;	}
		const HighSpeedGunJinkController* getHighSpeedGunJinkController() const	{ return m_highSpeedGunJinkController;	}
		const LowSpeedGunJinkController*  getLowSpeedGunJinkController() const	{ return m_lowSpeedGunJinkController;	}
		const ReversalController*	      getReversalController() const         { return m_reversalController;	}
		const BarrelRollController*	      getBarrelRollController() const	    { return m_barrelRollController;	}
		const ScissorController*	      getScissorController() const	        { return m_scissorController;	}

		const VTOLPositionController*	  getVTOLPositionController() const	    { return m_VTOLPositionController;	}
		const VTOLVelocityController*	  getVTOLVelocityController() const	    { return m_VTOLVelocityController;	}
		const VTOLLandingController*	  getVTOLLandingController() const	    { return m_VTOLLandingController;	}
		const VTOLTakeoffController*	  getVTOLTakeoffController() const	    { return m_VTOLTakeoffController;	}
		#endif
        //@}

    protected:
        /**
            Destructor of the class. The destructor is protected. Use remove() method to delete an instance of this class.
        */
        virtual ~AirplaneBot( void );

        VelocityController*					m_velocityController;
        PositionController*				    m_positionController;
        BasicLowLevelController*		    m_basicLowLevelController;
        FollowRouteController*				m_followRouteController;
        BasicFlightController*				m_basicFlightController;
        FlightPathController*				m_flightPathController;
		LoiterController*           		m_loiterController;
		BankLeftRightController*			m_bankLeftRightController;
		RaceTrackController*        		m_raceTrackController;
		LandingController*					m_landingController;
		TakeoffController*					m_takeoffController ;
        BasicTaxiController*				m_basicTaxiController ;
        TaxiPositionController*				m_taxiPositionController ;
        TaxiRouteController*				m_taxiRouteController ;

		#ifdef RTDYNAMICS_EXPORT_MILITARY_FEATURES
		DiveBombingController*				m_diveBombingController;
		GroundAvoidanceController*  		m_groundAvoidanceController ;
        HardTurnController*         		m_hardTurnController;                
        ImmelmannController*        		m_immelmannController;
        SplitSController*           		m_splitSController;
        STurnController*            		m_sturnController;
        FollowEntityController*       		m_followEntityController;
        PursuitController*          		m_pursuitController;
        ExtensionController*        		m_extensionController;		
		HighYoyoController*					m_highYoyoController;
		LowYoyoController*					m_lowYoyoController;
		LiftVectorTurnController*			m_liftVectorTurnController;
		BreakTurnController*	    		m_breakTurnController;
		HeadOnController*	        		m_headOnController;
		Degree90Controller*	        		m_degree90Controller;
		DisengEgressController*	    		m_disengEgressController;
		HighSpeedGunJinkController* 		m_highSpeedGunJinkController;
		LowSpeedGunJinkController*  		m_lowSpeedGunJinkController;
		ReversalController*         		m_reversalController;
		BarrelRollController*       		m_barrelRollController;
		ScissorController*          		m_scissorController;
		VTOLPositionController*				m_VTOLPositionController;
		VTOLVelocityController*				m_VTOLVelocityController;
		VTOLLandingController*				m_VTOLLandingController;
		VTOLTakeoffController*				m_VTOLTakeoffController;
		#endif


        virtual void                    update( smReal deltaTime );

        virtual bool                    load( const std::string& parameterFile , std::string& error  );
    };
}