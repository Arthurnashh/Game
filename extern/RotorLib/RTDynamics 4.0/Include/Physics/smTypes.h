/*
(c) Copyright 2004, MachSim
All Rights Reserved.

Product:		DinaPhysics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of MachSim
*/
#pragma once

// Inclusion of stdio is needed for gcc 2.95.3
#include <stdio.h>

//  @Tricky : All method which are implemented in this header shall include the 'inline' modifier.
//  Otherwise link will first warn and than fail when linking in a model dll.
#include "Physics/smIntegralType.h"
#include "Physics/smMath.h"

class smAngleD ;
class smAccel ;
class smMass ;
class smTime ;

typedef	smReal	smAngleInDeg ;
typedef	smReal	smAngleInRad ;
typedef bool	smBool ;

class smAccel : public smIntegralType
{
    SMINTEGRALTYPE_COMMON( smAccel, SM_ACCEL, -SM_REAL_MAX, SM_REAL_MAX, 0.0f ) ;
};

class smAltitude : public smIntegralType
{
    SMINTEGRALTYPE_COMMON(  smAltitude, SM_ALTITUDE, -SM_REAL_MAX, SM_REAL_MAX, 0.0f ) ;
};

class smAngle : public smIntegralType
{
    SMINTEGRALTYPE_COMMON(  smAngle, SM_ANGLE, 0.0f, smMath::sTwoPI, 0.0f ) ;

    /**
    * This operator converts an angle in degrees
    * to radians easily.
    */
    virtual operator smAngleD() ;
    virtual operator smAngleD()  const ;

    smAngle & operator=( const smAngleD &value ) ;

    smAngleD toDegrees()  const ;
};

class smAngleD : public smIntegralType
{
    SMINTEGRALTYPE_COMMON( smAngleD, SM_ANGLED, 0.0f, 360.0f, 0.0f ) ;

    /**
    * This operator converts an angle in degrees
    * to radians easily.
    */
    virtual operator smAngle() ;
    virtual operator smAngle()  const ;

    smAngleD & operator=( const smAngle &value ) ;

    smAngle toRadians()  const ;
};

class smAngularRate : public smIntegralType
{
    SMINTEGRALTYPE_COMMON( smAngularRate, SM_ANGULARRATE, -SM_REAL_MAX, SM_REAL_MAX, 0.0f );
};


class smAngularAccel : public smIntegralType
{
    SMINTEGRALTYPE_COMMON( smAngularAccel, SM_ANGULARACCEL, -SM_REAL_MAX, SM_REAL_MAX, 0.0f );
};

class smDensity : public smIntegralType
{
    SMINTEGRALTYPE_COMMON(  smDensity, SM_DENSITY, 0.0f, SM_REAL_MAX, 1.0f ) ;
};

/**
*   smDimension represents the dimensions of physical object such as width, length and height.
*/
class smDimension : public smIntegralType
{
    SMINTEGRALTYPE_COMMON(  smDimension, SM_DIMENSION, 0.0f, SM_REAL_MAX, 1.0f );
};

class smForce : public smIntegralType
{
    SMINTEGRALTYPE_COMMON(  smForce, SM_FORCE, -SM_REAL_MAX, SM_REAL_MAX, 0.0f ) ;

    // smForce specific
    smAccel operator/( const smMass &value ) ;
    smAccel operator/( const smMass &value )  const ;
};


class smFrequency : public smIntegralType
{
    SMINTEGRALTYPE_COMMON( smFrequency, SM_FREQUENCY, 0.0f, SM_REAL_MAX, 0.0f );

    /**
    * Automatically convert frequency to period
    */
    virtual smTime toPeriod() ;
    virtual smTime toPeriod()  const ;
};


class smHeight : public smIntegralType
{
    SMINTEGRALTYPE_COMMON( smHeight, SM_HEIGHT, 0.0f, SM_REAL_MAX, 0.0f );
};


class smInertia : public smIntegralType
{    
    SMINTEGRALTYPE_COMMON( smInertia, SM_INERTIA, 0.0f, SM_REAL_MAX, 0.0f );
};

class smMass : public smIntegralType
{
    SMINTEGRALTYPE_COMMON(  smMass, SM_MASS, 0.0f, SM_REAL_MAX, 0.0f ) ;

    // smMass specific
    smForce operator*( const smAccel &value )
    {
        smForce temp ;

        if (  smMath::isFinite(  value.m_value  )  == 1 )
            temp = smForce(  m_value * value.m_value  );
        else
            printf( "smMass float* operator received infinity or NaN.n" );

        return temp ;
    }

    smForce operator*( const smAccel &value )  const
    {
        smForce temp ;

        if (  smMath::isFinite( value.m_value  )  == 1 )
            temp = smForce(  m_value * value.m_value  );
        else
            printf( "smMass float* operator received infinity or NaN.n" );

        return temp ;
    }

};

class smMassFlowRate : public smIntegralType
{
    SMINTEGRALTYPE_COMMON(  smMassFlowRate, SM_MASSFLOWRATE, -SM_REAL_MAX, SM_REAL_MAX, 0.0f ) ;
};

class smMomentum : public smIntegralType
{
    SMINTEGRALTYPE_COMMON( smMomentum, SM_MOMENTUM, -SM_REAL_MAX, SM_REAL_MAX, 0.0f );
};

class smLinearMomentum : public smIntegralType
{
    SMINTEGRALTYPE_COMMON( smLinearMomentum, SM_LINEARMOMENTUM, -SM_REAL_MAX, SM_REAL_MAX, 0.0f );
};

class smAngularMomentum : public smIntegralType
{
    SMINTEGRALTYPE_COMMON( smAngularMomentum, SM_ANGULARMOMENTUM, -SM_REAL_MAX, SM_REAL_MAX, 0.0f );
};

class smPercent : public smIntegralType
{
    SMINTEGRALTYPE_COMMON( smPercent, SM_PERCENT, 0.0f, 100.0f, 0.0f );
};

class smPosition : public smIntegralType
{
    SMINTEGRALTYPE_COMMON(  smPosition, SM_POSITION, -SM_REAL_MAX, SM_REAL_MAX, 0.0f );
};


class smPower : public smIntegralType
{
    SMINTEGRALTYPE_COMMON( smPower, SM_POWER, 0.0f, SM_REAL_MAX, 0.0f );
};

class smPressure : public smIntegralType
{

    SMINTEGRALTYPE_COMMON(  smPressure, SM_PRESSURE, -SM_REAL_MAX, SM_REAL_MAX, 0.0f ) ;
};

class smRPM : public smIntegralType
{
    SMINTEGRALTYPE_COMMON( smRPM, SM_RPM, 0.0f, SM_REAL_MAX, 0.0f );
};

class smSpeed : public smIntegralType
{
    SMINTEGRALTYPE_COMMON(  smSpeed, SM_SPEED, -SM_REAL_MAX, SM_REAL_MAX, 0.0f ) ;
};

class smSurface : public smIntegralType
{
    SMINTEGRALTYPE_COMMON( smSurface, SM_SURFACE, 0.0f, SM_REAL_MAX, 0.0f );
};

class smTemperature : public smIntegralType  //## Inherits: <unnamed>%39184F9302F8
{
    SMINTEGRALTYPE_COMMON(  smTemperature, SM_TEMPERATURE, -SM_REAL_MAX, SM_REAL_MAX, 0.0f ) ;
};

/**
* smTime is in seconds.
*
*/
class smTime : public smIntegralType
{
    SMINTEGRALTYPE_COMMON(  smTime, SM_TIME, 0.0f, SM_REAL_MAX, 0.0f );

public:
    /**
    * On Linux, a float can not handle the content of time.
    * Therefore we use a double to deal with time.
    */
    typedef double  DoublePrec;
};


class smTorque : public smIntegralType
{
    SMINTEGRALTYPE_COMMON(  smTorque, SM_TORQUE, -SM_REAL_MAX, SM_REAL_MAX, 0.0f );
};


class smVolume : public smIntegralType
{
    SMINTEGRALTYPE_COMMON( smVolume, SM_VOLUME, 0.0f, SM_REAL_MAX, 0.0f );
};


class smFloat : public smIntegralType
{
    SMINTEGRALTYPE_COMMON(  smFloat, SM_UNDEFINED, -SM_REAL_MAX, SM_REAL_MAX, 0.0f ) ;
};

/**
* A special type for representing throttle. Min is -100.0 and
* max is 100.0.
**/

class smThrottle : public smIntegralType
{
    SMINTEGRALTYPE_COMMON(  smThrottle, SM_THROTTLE, 0.0f, -100.0f, 100.0f ) ;
};


/**
* smCoef is a specialized type. Although it is a physics type like
* others, it is implemented for general convenience use.
*
* It is limited in [0.0,1.0], therefore it is very useful in many
* applications.
*/

class smCoefPos : public smIntegralType
{
    SMINTEGRALTYPE_COMMON(  smCoefPos, SM_POSITIVE_COEFFICIENT, 0.0f, 1.0f, 0.0f ) ;
};

class smCoefNegPos : public smIntegralType
{
    SMINTEGRALTYPE_COMMON(  smCoefNegPos, SM_NEGATIVE_POSITIVE_COEFFICIENT, -1.0f, 1.0f, 0.0f ) ;
};

/** smHalfCoefNegPos is especially practical because it is length is 1. */
class smHalfCoefNegPos : public smIntegralType
{
	SMINTEGRALTYPE_COMMON(  smHalfCoefNegPos, SM_NEGATIVE_POSITIVE_COEFFICIENT, -0.5f, 0.5f, 0.0f ) ;
};

class smPosFloat : public smIntegralType
{
    SMINTEGRALTYPE_COMMON(  smPosFloat, SM_POSITIVE_FLOAT, 0.0f, SM_REAL_MAX, 0.0f ) ;
};

class smNegFloat : public smIntegralType
{
    SMINTEGRALTYPE_COMMON(  smNegFloat, SM_NEGATIVE_FLOAT, -SM_REAL_MAX, 0.0f, 0.0f ) ;
};

/**
* A special type for representing frictions. Min is 0.0 and
* max is not specified.
**/

class smFriction : public smIntegralType
{
    SMINTEGRALTYPE_COMMON(  smFriction, SM_FRICTION, 0.0f, SM_REAL_MAX, 0.0f ) ;
};

/**
*
*
**/
class smVoltage : public smIntegralType
{
    SMINTEGRALTYPE_COMMON(  smVoltage, SM_VOLTAGE, -SM_REAL_MAX, +SM_REAL_MAX, 0.0f ) ;
};