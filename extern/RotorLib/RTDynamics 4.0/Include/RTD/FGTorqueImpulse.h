/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:        RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "Util/smTimeBasedPulse.h"
#include "RTD/ForceGenerator.h"

namespace RTD
{
    /**
    *   Torque generates (only) torque on the body
    */
    class RTDYNAMICS_EXPORT FGTorqueImpulse : public ForceGenerator
    {
    private:
        smTimeBasedPulse mPulse ;
        smTorqueVec      m_torqueVec ;
    public:
        FGTorqueImpulse( const std::string& name , VehicleBase* vehicle ) ;
        virtual void  update( smTime timestep ) ;

        /**
        *   @param  torque      the torque vector
        *   @param  waitTime    specifies how long the pulse generator will wait for before starting the
        *                       signal
        *   @param  pulseTime   specifies how long the pulse will be generated
        *   @param  repeat      specifies how many times the pulse will be repeated
        */
        void setTorque( const smTorqueVec& torque, smReal waitTime = 0, smReal pulseTime=1, int repeat=1 );
    };
}