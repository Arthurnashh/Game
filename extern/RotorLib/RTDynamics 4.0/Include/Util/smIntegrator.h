/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:		RotorLib
Subsystem:		RLDynamics

The content of this work is propriatery and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "Physics/smPrecision.h"
#include "Physics/smVecTypes.h"

/**
*	Multi-method integrator.
*
*/
class smIntegrator
{
public:
    enum Type
    {
        AB4,	// AB4 - Adams-Bashforth, fourth order
        AB3,	// AB3 - Adams-Bashforth, third order
        AB2,	// AB2 - Adams-Bashforth, second order
        AM3,	// AM3 - Adams-Moulton, third order
        EULER,  // EULER - Euler
        TRAPZ	// TRAPZ - Trapezoidal
    };

    /** 
        @param type Integration method to be used
      
        @param timestep the integration time step in seconds

        @param vTDeriv a reference to the current value of the time derivative of
        the quantity being integrated (i.e. if vUVW is being integrated
        vTDeriv is the current value of vUVWdot)

        @param vLastArray an array of previously calculated and saved values of
        the quantity being integrated (i.e. if vUVW is being integrated
        vLastArray[0] is the past value of vUVWdot, vLastArray[1] is the value of
        vUVWdot prior to that, etc.)

        @return the current, incremental value of the item integrated to add to the
        previous value.
    */
    static smReal integrate(Type type, smReal timestep, smReal vTDeriv, smReal *vLastArray)
    {
        smReal vResult;

        switch (type)
        {
            case AB4:
                vResult = (timestep/24.0f)*(  55.0f * vTDeriv
                    - 59.0f * vLastArray[0]
                    + 37.0f * vLastArray[1]
                    -  9.0f * vLastArray[2] );
                    vLastArray[2] = vLastArray[1];
                    vLastArray[1] = vLastArray[0];
                    vLastArray[0] = vTDeriv;
                    break;
            case AB3:
                vResult = (timestep/12.0f)*(  23.0f * vTDeriv
                    - 16.0f * vLastArray[0]
                    +  5.0f * vLastArray[1] );
                    vLastArray[1] = vLastArray[0];
                    vLastArray[0] = vTDeriv;
                    break;
            case AB2:
                vResult = (timestep/2.0f)*( 3.0f * vTDeriv - vLastArray[0] );
                vLastArray[0] = vTDeriv;
                break;
            case AM3:
                vResult = (timestep/12.0f)*(  5.0f * vTDeriv
                    + 8.0f * vLastArray[0]
                    - 1.0f * vLastArray[1] );
                    vLastArray[1] = vLastArray[0];
                    vLastArray[0] = vTDeriv;
                    break;
            case EULER:
                vResult = timestep * vTDeriv;
                break;
            case TRAPZ:
                vResult = (timestep*0.5f) * (vTDeriv + vLastArray[0]);
                vLastArray[0] = vTDeriv;
                break;
        }

        return vResult;
    }

};

/** smVecIntegrator can be used to integrate a 3D vector.	*/
class smVecIntegrator
{
public:
	smVecIntegrator()
	{ reset() ; }

	void setVector( smVec3r& vec )
	{ m_vector = &vec; }

	struct PrevValues
	{
		smReal m_prevValueArray[3] ;
	};

	void integrate( smIntegrator::Type type,smVec3r vTDeriv, smReal timestep )
	{
		for( int i = 0 ; i < 3 ; i++)
			(*m_vector)[i] = (*m_vector)[i] + smIntegrator::integrate( type, timestep, vTDeriv[i], m_prevValues[i].m_prevValueArray ) ;
	}

	void reset()
	{
		for( int i = 0 ; i < 3 ; i++)
		{
			m_prevValues[i].m_prevValueArray[0] = 0.0f ;
			m_prevValues[i].m_prevValueArray[1] = 0.0f ;
			m_prevValues[i].m_prevValueArray[2] = 0.0f ;
		}
	}

	smVec3r*   m_vector ;
	PrevValues m_prevValues[3] ;
};
