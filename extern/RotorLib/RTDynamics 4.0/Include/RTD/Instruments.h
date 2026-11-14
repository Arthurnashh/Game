/*
    (c) Copyright 2006, RTDynamics
    All Rights Reserved.

    Product:        RTDynamics


    The content of this work is proprietary and confidential.Permission to use,copy,
    publish and modify this software and its documentation for any commercial and
    non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "Util/smFilter.h"
#include "RTD/RigidBodyStateBase.h"
#include "RTD/RigidBody.h"


namespace RTD
{
    class VehicleBase;

    class UnreliableMeasurementGenerator
    {
    public:

        // This lag smoothes out the random function.
        smLagFilter<smReal> m_measurementLag ;
        int m_count ;
        int m_interval ;
        smReal m_lastGenValue ;

    public:
        UnreliableMeasurementGenerator() : m_interval(10), m_lastGenValue(0)
        {
        }

        void reset( smReal prevValue )
        {
            m_measurementLag.setPrevValue( prevValue ) ;
            m_count = 0 ;
        }

        smReal generate( smReal range )
        {
            // Pick a new value every n steps
            if ( (m_count % m_interval) == 0 )
                m_lastGenValue = smMath::makeNormalizedPosRand() * range ;

            smReal measurement ;
            m_measurementLag.filter( m_lastGenValue, measurement, 0.95f ) ;

            m_count++ ;

            return measurement ;
        }
    };

    /** InsGenAirspeedIndicator100 is a simulation model of a common pitot-tube
    *   airspeed indicator. This model can simulate the following physical phenomena:
    *
    *       - Measurement of forward airspeed
    *       - Inaccurate measurement due to flight direction
    *       - Inaccurate measurement due to low airspeed
    *       - Effects of wind (e.g. blowing directly at the aircraft)
    *       - Effects of blocked inlets, ports ( e.g. due to pitot-heater not being used. )
    *
    *   Notes:
    *       Getter methods for various units are provided. See Getter Methods.
    *       For unit conversions the Units utility class provides several conversion constants.
    *       This class needs to be reset when the aircraft is reset.
    */
    class RTDYNAMICS_EXPORT InsGenAirspeedIndicator100
    {
        smVec3r m_pitotTubeDir ;

        /** The x,y,z location of the pitottube. */
        smVec3r m_pitotTubePos ;

        UnreliableMeasurementGenerator  m_unrelMeasGen ;

        smReal  m_accurateMeasurementFwdSpd ;
        smSmoothThresholdFilter m_timedLagFilter ;

        smReal  m_blockFactor ;

        smReal  m_IAS_mps ;     // m/s
        smReal  m_IAS_knots ;   // knots
        smReal  m_IAS_mph ;     // miles per hour

    public:
        InsGenAirspeedIndicator100()
        {
            setAccuracyParams( 20.0f * Units::knots_to_mps ) ;

            // Usually pitot tubes point fwd.
            m_pitotTubeDir.set(0,1,0) ;

            // and is placed somewhere forward
            m_pitotTubePos.set( 0, +1, 0.5f ) ;

            // Pitot-tube is not blocked by default.
            setBlockfactor(0.0f) ;
        }

        /** The update method shall be called once every simulation step.
        *
        *   @param  rbs         The RigidBodyStateBase of the vehicle
        *   @param  worldWind   The global wind in world coordinate system effecting on the aircraft.
        */
        virtual void    update( const RigidBodyStateBase& rbs, const smVec3r& worldWind = smVec3r() ) ;

        /** @name Getter Methods
        *   These getter methods return the simulated indicated airspeed in miles per hour,
        *   meters per second and knots.
        */
        //@{
        virtual smReal  getIndicatedAirspeed_mps()      {   return m_IAS_mps ; }
        virtual smReal  getIndicatedAirspeed_knots()    {   return m_IAS_knots ; }
        virtual smReal  getIndicatedAirspeed_mph()      {   return m_IAS_mph ; }
        //@}


        /** Pitot-tubes are known to be unreliable and very inaccurate for lower airspeeds
        *   (such as 20-30 knots.). With this method you can set this threshold and the airspeed
        *   which will be indicated below the threshold.
        *
        *   @param accurateMeasurementFwdSpd    Threshold airspeed [m/s]
        *
        */
        virtual void    setAccuracyParams( smReal accurateMeasurementFwdSpd )
        {
            m_accurateMeasurementFwdSpd = accurateMeasurementFwdSpd ;
            m_unrelMeasGen.reset( accurateMeasurementFwdSpd / 2.0f );
        }

        /** A value of 1.0 m_blockFactor indicated fully blocked pitot-tube. */
        virtual void    setBlockfactor( smReal blockFactor )
        { m_blockFactor = smMath::clamp( blockFactor, 0, 1.0f ); }

        /** Sets position of the pitot-tube according to aircraft center.*/
        virtual void    setPitotTubePos( const smVec3r pos )
        { m_pitotTubePos = pos ;}

        /** Reset. */
        virtual void    reset( const smReal& initVel )
        {
            m_timedLagFilter.reset() ;
            m_unrelMeasGen.reset( initVel );
        }
    };

    /** InsGenAltimeter100 is a simulation model of a common analog altimeter.
    *
    *   In addition to measuring the altimeter, this model can simulate the clogging
    *   of the static port. See setBlockFactor() method.
    *
    *   Notes:
    *       Getter methods for various units are provided. See Getter Methods.
    *       For unit conversions the Units utility class provides several conversion constants.
    *       This class needs to be reset when the aircraft is reset.
    */
    class RTDYNAMICS_EXPORT InsGenAltimeter100
    {
        smReal  m_blockFactor ;

        smReal  m_altitude_m ;
        smReal  m_altitude_ft;
        smReal  m_pressureAltitude_m ;

        smLagFilter<smReal> m_cloggingLag ;
    public:
		InsGenAltimeter100() : m_altitude_m(0), m_altitude_ft(0), m_pressureAltitude_m(0)
        {
            m_blockFactor = 0.0f;
        }

        /** The update method shall be called once every simulation step. */
        void    update( const RigidBodyStateBase& rbs, const World& world ) ;

        /** Reset. */
        void    reset( smReal z )
        { m_cloggingLag.setPrevValue(z) ; }

        /** Static port block factor. A value of 1.0 means the static port is blocked.*/
        void    setBlockFactor( smReal blockFactor )
        { m_blockFactor = smMath::clamp( blockFactor, 0, 1.0f ); }
        smReal  getBlockFactor(  )
        { return m_blockFactor ; }

        /** @name Getter Methods
        *   These getter methods return the simulated altituder in feet and meters.
        */
        //@{
        smReal  getAltitude_feet()  { return m_altitude_ft; }
        smReal  getAltitude_m() { return m_altitude_m; }
        smReal  getPressureAltitude_m () { return m_pressureAltitude_m; }
        //@}
    };

    /** InsGenVerticalSpeedIndicator100 is a simulation model of a common analog vertical
    *   speed indicator.
    *
    *   There is a lag associated with the reading on the VSI, and it may take a few seconds
    *   to stabilize when showing rate of climb or descent. This phenomena is modeled with a
    *   lag. See setLag() method.
    *
    *   Additionally, rough control technique and turbulence can further extend the lag
    *   period and cause erratic and unstable rate indications. This is called the secondary
    *   lag. See setSecondaryLag() method.
    *
    *   Static ports must be unobstructed. With a blocked static system, the VSI freezes at zero.
    *   It is possible to specify the level of clogging of the static port with the method
    *   setStaticPortBlockFactor().
    *
    *   Notes:
    *       Getter methods for various units are provided. See Getter Methods.
    *       For unit conversions the Units utility class provides several conversion constants.
    *       This class needs to be reset when the aircraft is reset.
    */
    class RTDYNAMICS_EXPORT InsGenVerticalSpeedIndicator100
    {

        smReal  m_vertSpeed_mps ;
        smReal  m_vertSpeed_fpm ;

        smLagFilter<smReal> m_standardLag ;
        smLagFilter<smReal> m_secondaryLag ;

        smTime  m_lag ;
        smReal  m_secondaryLagFactor ;
        smReal  m_staticPortBlockFactor ;

    public:
		InsGenVerticalSpeedIndicator100() : m_vertSpeed_mps(0),m_vertSpeed_fpm(0)
        {
            // By default, static port is not blocked.
            setStaticPortBlockFactor( 0.0f ) ;

            // Set default lag
            setLag( 0.5f ) ;

            // By default, there is no turbulence lag.
            setSecondaryLag( 0.0f ) ;
        }

        virtual ~InsGenVerticalSpeedIndicator100() {}

        /** The update method shall be called once every simulation step. */
        virtual void    update( const RigidBodyStateBase& rbs ) ;

        /** @name Getter Methods
        *   These getter methods return the simulated vertical speed,
        *   in feet per minute and meters per second.
        */
        //@{
        virtual smReal  getVerticalSpeed_fpm()  { return m_vertSpeed_fpm; }
        virtual smReal  getVerticalSpeed_mps()  { return m_vertSpeed_mps; }
        //@}

        /** @param  lag the standard lag of the VSI[0,1]. */
        virtual void    setLag( smReal lag )
        { m_lag = smMath::clamp( lag, 0, 1 ) ; }

        /** @param factor   A factor of 1.0 indicates a completely blocked static port.
        */
        virtual void    setStaticPortBlockFactor( smReal factor )
        { m_staticPortBlockFactor = factor ; }

        virtual void    reset( smReal initVelZ )
        {
            m_standardLag.setPrevValue( initVelZ ) ;
            m_secondaryLag.setPrevValue( initVelZ ) ;
        }

        /** @param  factor  A factor of 1.0 indicates a maximum turbulence and hence
        *                   causes measurement error.[0,1]
        */
        virtual void    setSecondaryLag( smReal factor )
        { m_secondaryLagFactor = smMath::clamp( factor, 0, 1 ) ; }
    };

    /** InsGenAttitudeIndicator100 is a simulation model of a common analog attitude
    *   indicator.
    */
    class RTDYNAMICS_EXPORT InsGenAttitudeIndicator100
    {
        smVec2r m_pr ;
    public:
        /** The update method shall be called once every simulation step. */
        virtual void    update( const RigidBodyStateBase& rbs ) ;

        /** Returns the attidute (pitch and bank) in degrees. */
        virtual void    getAttitude( smAngleD& pitch, smAngleD& bank )
        { pitch = m_pr[0] ; bank = m_pr[1] ; }
    };

    /** InsGenHeadingIndicator100 is a simulation model of a common analog heading
    *   indicator.
    */
    class RTDYNAMICS_EXPORT InsGenHeadingIndicator100
    {
        smAngleD    m_h ;
    public:
		InsGenHeadingIndicator100() : m_h(0)
		{
		}

        /** The update method shall be called once every simulation step. */
        virtual void    update( const RigidBodyStateBase& rbs ) ;

        /** Returns the heading in degrees. Heading rotations are clockwise positive. */
        virtual smAngleD    getHeading( )
        { return m_h ; }
    };

    /** InsGenTurnIndicator100 is a simulation model of a common analog turn
    *   indicator.
    *
    *   Use the getTurnRate() method to find the direction and rate of turn.
    *   For example a standard rate turn is 3° per second.
    *
    *   For the simulation of the inclinometer, getLoad() method can be used.
    *   The displacement of the ball can be than adjusted according to the load
    *   and its direction ( slipping or skidding ).
    */
    class RTDYNAMICS_EXPORT InsGenTurnIndicator100
    {
        smReal  m_turnRate ;
        smReal  m_load ;

    public:
        static const smReal s_standardRateTurn_radps ;

		InsGenTurnIndicator100() : m_turnRate(0), m_load(0)
		{
		}

        /** The update method shall be called once every simulation step. */
        void    update( const RigidBodyStateBase& rbs, const smVec3r& gravity, const smReal& mass) ;

        /** Returns the turn rate in degrees per second. */
        smReal  getTurnRate( )
        { return m_turnRate ; }

        /** Returns the load factor of the aircraft. */
        smReal  getLoad( )
        { return m_load ; }
    };

    /** TraitStdInstruments is a utility class which simply adds
    *   all the important instruments to an aircraft class at once.
    *   It also handles resetting the instruments.
    *
    */
    class RTDYNAMICS_EXPORT TraitStdInstruments
    {
        InsGenAirspeedIndicator100*         m_airspdInd ;
        InsGenAltimeter100*                 m_altimeter ;
        InsGenVerticalSpeedIndicator100*    m_vsi ;
        InsGenAttitudeIndicator100*         m_attInd ;
        InsGenHeadingIndicator100*          m_hdgInd ;
        InsGenTurnIndicator100*             m_turnInd ;

    public:
        TraitStdInstruments() ;

		virtual ~TraitStdInstruments() ;

        /** Update method */
        virtual void update( const RigidBodyStateBase& rbs, const World& world ) ;

        /** Factory method. Override to construct derived instrument classes. */
        virtual void makeInstruments() ;

        /** Resets all instruments at once. Call after RigidBodyStateBase is reset to new position and speeds.*/
        virtual void reset( const RigidBodyStateBase& rbs ) ;

        /** @name Getter methods. */
        //@{
        InsGenAirspeedIndicator100*         getAirspeedIndicator()      { return m_airspdInd ; }
        InsGenAltimeter100*                 getAltimeter()              { return m_altimeter ; }
        InsGenVerticalSpeedIndicator100*    getVSI()                    { return m_vsi; }
        InsGenAttitudeIndicator100*         getAttitudeIndicator()      { return m_attInd ; }
        InsGenHeadingIndicator100*          getHeadingIndicator()       { return m_hdgInd ; }
        InsGenTurnIndicator100*             getTurnIndicator()          { return m_turnInd ; }
        //@{
    };
}
