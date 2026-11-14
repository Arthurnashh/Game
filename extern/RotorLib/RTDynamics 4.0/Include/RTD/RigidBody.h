/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:        RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RTD/Base.h"
#include "RTD/World.h"
#include "RTD/ParamDb.h"

namespace RTD
{
    /** Static properties of a rigid body are collected in the RigidBody class. Dynamics properties
    *   of a body is stored in the RigidBodyState class.
    */
    class RTDYNAMICS_EXPORT RigidBody 
    {
    private:
        smMass      m_mass ;
        World*      m_world ;

        smMatrix    m_IBody ;
        smMatrix    m_IBodyInv ;

        /** Dimensions of the body.*/
        smDimensionVec  m_dims ;

    public:
        RigidBody( World* world, const std::string& name ) ;

        /** computeAndStoreInertia shall be called if it is needed to set the mass and the
        *   dimensions of the vehicle. This method will set the mass and the dimensions and also
        *   compute an inertia matrix which is needed for vehicle dynamics.
        *
        *   @param  mass        mass of body in kg
        *   @param  sizeInXYZ   dimensions of body in meters
        */
        void computeAndStoreInertia( smMass mass, smDimensionVec sizeInXYZ ) ;

        /** Compute and return the moments of inertia for a box.
        *   @param  mass        mass of body in kg
        *   @param  sizeInXYZ   dimensions of body in meters
        *   @param  moi         moments of inertia vector
        */
        static void computeInertiaVecOfABox( smMass mass, smDimensionVec sizeInXYZ, smVec3r& moi ) ;
        static void computeInertiaMatrixOfABox( smMass mass, smDimensionVec sizeInXYZ, smMatrix& moi ) ;

        /** setMassAndInertia can be used to set the inertia directly. Please
        *   note that RL uses a different coordinate system than the aerospace
        *   engineering literature, which uses the NED frame. So if you are reading
        *   the moments of inertia from an aerospace engineering data source, please
        *   do the following conversion:
        *
        *   @param  mass    mass of body in kg
        *   @param  Ixx     Iyy in aerospace engineering literature (in NED Frame)
        *   @param  Iyy     Ixx in aerospace engineering literature (in NED Frame)
        *   @param  Izz     Izz in aerospace engineering literature (in NED Frame)
        *   @param  Iyz     Ixz in aerospace engineering literature (in NED Frame)
        *
        *   All units are kg-m^2, if you have data in slug-ft^2, you can multiply the
        *   moments with Units::slugft2_to_kgm2 to convert to kg-m^2.
        *
        *   Note: inertial moments are always positive numbers.
        */
        void setMassAndInertia( smMass mass, smReal Ixx, smReal Iyy, smReal Izz, smReal Iyz ) ;
        void setMassAndInertia( smMass mass, smVec4r MoI ) ;

        void print( smPrinter* pr ) ;

        const smMass&   getMass() const
        { return m_mass; }

        const World*    getWorld() const
        { return m_world; }

        World*  getWorld()
        { return m_world; }

        const smMatrix& getInvBodyInertia() const
        { return m_IBodyInv; }

        const smMatrix& getBodyInertia() const
        { return m_IBody; }

        const smDimensionVec getDimensions() const
        { return m_dims ; }
    };
}