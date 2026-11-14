/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:        RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RTD/Common.h"
#include "RTD/RTD.h"
#include "RTD/Globals.h"
#include "RTD/Named.h"

#define RTDYNAMICS_VERSION_MAJOR    /*[[[major]]]*/4/*[[[End]]]*/
#define RTDYNAMICS_VERSION_MINOR    /*[[[minor]]]*/0/*[[[End]]]*/
#define RTDYNAMICS_RELEASENAME  /*[[[releasename]]]*/"4.0.Beta5.Bohemia"/*[[[End]]]*/

namespace RTD
{
    /** Common Data Types for all RTD Classes.  */
    enum Quadrant
    {
        QDR_POSX_POSY,  // Front Right
        QDR_POSX_NEGY,  // Rear Right
        QDR_NEGX_POSY,  // Front Left
        QDR_NEGX_NEGY,  // Read Left
    };

    /**
    *   Lightweight base class for all other RTD classes.
    */
    class RTDYNAMICS_EXPORT Base
    {
    public:
        Base() {}
        virtual ~Base() {}
    };

#define Printable_Real( RE )    pr->debug( " " #RE "=\"%.2f\"", (smReal) RE );
#define Printable_Bool( BO )    pr->debug( " " #BO "=\"%.d\"", BO );
#define Printable_sm( SM )      pr->debug( " " #SM "=\"%.2f\"", SM.val() );
#define Printable_Vec2( VEC )   pr->debug( " " #VEC "=\"%.2f %.2f\"", (smReal) VEC[0], (smReal) VEC[1] );
#define Printable_Vec3( VEC )   pr->debug( " " #VEC "=\"%.2f %.2f %.2f\"", (smReal) VEC[0], (smReal) VEC[1], (smReal) VEC[2] );
#define Printable_Vec4( VEC )   pr->debug( " " #VEC "=\"%.2f %.2f %.2f %.2f\"", (smReal) VEC[0], (smReal) VEC[1], (smReal) VEC[2], (smReal) VEC[3] );
#define Printable_Quat( QU )    pr->debug( " " #QU "=\"%.2f %.2f %.2f %.2f\"", QU[0], QU[1], QU[2], QU[3] );
#define Printable_Matrix( MA )  pr->debug( " " #MA " " ); \
                                pr->debug( " "     " \"%.4f %.4f %.4f %.4f\"", MA[0][0], MA[0][1], MA[0][2], MA[0][3] ); \
                                pr->debug( " "     " \"%.4f %.4f %.4f %.4f\"", MA[1][0], MA[1][1], MA[1][2], MA[1][3] ); \
                                pr->debug( " "     " \"%.4f %.4f %.4f %.4f\"", MA[2][0], MA[2][1], MA[2][2], MA[2][3] ); \
                                pr->debug( " "     " \"%.4f %.4f %.4f %.4f\"", MA[3][0], MA[3][1], MA[3][2], MA[3][3] );

    /** Used abbreviations
    *       mps         meters per second
    *       fpmin       feet per second
    *       kmph        kilometers per hour
    *       mph         miles per hour
    *
    *   The Units class provides several conversion factors to convert
    *   metric system to the imperial unit system and the other way around.
    *
    *   This class only provides several constants, simply multiply the value
    *   with the appropriate constant to convert to the unit system of your
    *   choice. E.g.:
    *       altitude_in_meters = altitude_in_feet * RTD::Units::feet_to_meters
    */
    class RTDYNAMICS_EXPORT Units
    {
    public:
        static const smReal meters_to_feet ;
        static const smReal feet_to_meters ;

        static const smReal nmiles_to_meters ;
        static const smReal meters_to_nmiles ;

        static const smReal knots_to_mps ;
        static const smReal mps_to_knots ;

        static const smReal kmph_to_mps ;
        static const smReal mps_to_kmph ;

        static const smReal mph_to_kmph ;
        static const smReal kmph_to_mph ;

        static const smReal mps_to_fpmin ;
        static const smReal fpmin_to_mps ;

        static const smReal mps_to_fpsec ;
        static const smReal fpsec_to_mps ;

        static const smReal slugft2_to_kgm2 ;
        static const smReal kgm2_to_slugft2 ;

        static const smReal slugs_to_kg;
        static const smReal kg_to_slugs;

        static const smReal lbs_to_newton;
        static const smReal newton_to_lbs;

        static const smReal slugsfeet_to_kgm;
        static const smReal kgm_to_slugsfeet;

		static const smReal rpm_to_radsec;
		static const smReal radsec_to_rpm;

		static const smReal watt_to_HP;
		static const smReal HP_to_watt;

    };
}

/**
*   \namespace RTD
*
*   The RTD namespace contains all classes which are related to
*   real-time rigid body simulation of air vehicle.
*
*/

