#pragma once

#include "RTD/Common.h"
#include "Physics/smVecTypes.h"
#include "Physics/smMathMatrixD.h"

namespace RTD
{
    /** EarthModelWGS84 provides constants and static methods which mathematically model
    *   the WGS84 earth model as defined by NIMA (US National Geospatial-intelligence Agency).
    *
    *   This class used by the TraitGeographicCoordinate (a parent class for most aircraft models
    *   such as GenHeli300).
    *
    *   Most methods in this class use double's to eliminate floating point errors.
    *
    *   Geographic coordinates (latitute and longitude ) are always in radians and can have negative or
    *   positive values.
    */
    class RTDYNAMICS_EXPORT EarthModelWGS84
    {
    public:
        typedef double          Double ;
        typedef smVec3d         VecD ;

        /** The four defining constants of the WGS84 ellipsoid as defined by NIMA.
        *   Units:
        *       Semi-major axis [m]
        *       Flattening []
        *       Earth’s Gravitational Constant m^3/s^2
        *       Angular Velocity of the Earth radians/s
        */
        static const Double sSemiMajorAxis ;
        static const Double sFlattening;
        static const Double sEarthsGravitionalConstant;
        static const Double sAngularVelocityOfEarth;

        /** Derived Constants
        *   Units
        *       sSemiMinorAxis (b)      [m]
        *       sFirstEccentricity(e)   []
        */
        static const Double sSemiMinorAxis;
        static const Double sFirstEccentricity;
        static const Double sFirstEccentricitySquared;

        /** Computes the so called Radius of Curvature at the given latitude.
        *   @param  lat geodetic latitude in radians
        *
        */
        static Double radiusOfCurvature( Double lat ) ;

        /**
        *   @param  lat geodetic latitude in radians
        *   @param  lon longitude in radians
        *   @param  alt altitude (height above geodetic surface) in meters
        */
        static VecD toXYZ( const Double& lat, const Double& lon, const Double& alt );

        /**
        *   @param  x   ECEF x position in meters
        *   @param  y   ECEF y position in meters
        *   @param  z   ECEF z position in meters
        *
        *   returns a 3d Vector:
        *       0th component is geodetic latitude [rad]
        *       1th component is lontitude [rad]
        *       2th component is geodetic altitude
        *
        *   This method can handle altitudes other than 0, since it implements an
        *   iterative solver.
        */
        static VecD toLLA( const Double& x, const Double& y, const Double& z ) ;

        /** llaToWCS converts the given geographic position to a position
        *   in the World Coordinate System (WCS). Please note that the
        *   origin of the WCS is called the surface center and can be set
        *   with World::setLocalCoordinateSystem method.
        *
        *   @param  lla             [in] latitude [rad], longitude [rad], altitude
        *                           [m]
        *   @param  llaSurfCenter   [in] local coordinate center. Set and get with
        *                           World::setLocalCoordinateSystem and
        *                           World::getLocalCoordinateSystem methods.
        *   @param  xyzWCS          [out] position in world coordinate system.
        */
        static void llaToWCS( const smVec3d& lla, const smVec3d& llaSurfCenter, smVec3d& xyzWCS) ;

        /** Converts the given degree to string formatted as:
        *       DD° MM' SS.SS" [N|S|W|E]
        */
        static void dmsToString( int& deg, int& min, Double& sec, std::string& strOut, bool isLat ) ;

        /** Converts the given latitute to string formatted as:
        *       DD° MM' SS.SS" [N|S]
        */
        static void latToString( int& lat, int& min, Double& sec, std::string& strOut ) ;

        /** Converts the given longitude to string formatted as:
        *       DD° MM' SS.SS" [W|E]
        */
        static void lonToString( int& lat, int& min, Double& sec, std::string& strOut ) ;

        /** Returns a string which is suitable for displaying latitude and longitude on screen.
        *   For example:
        *       Lat: DD° MM' SS.SS" ,Lon: DD° MM' SS.SS" , Alt: 2331 m
		*
        *   @param  lla             [in] latitude [rad], longitude [rad], altitude
        *                           [m]
        */
        static std::string llaToString( const smVec3d& lla ) ;

        /** Converts the given decimal representation of a geographic coordinate (latitude or longitude)
        *   to integer degrees, integer minutes and double seconds.
        */
        static void toDegreesMinutesSeconds( const Double& degIn, int& degOut, int& minOut, Double& secOut ) ;

        /** Transforms the given vector in ECEF frame to a vector in NED frame 
		*	which is positioned on the given latitude and longitude.
        *   
        *   @param  lat             [in] latitude [rad] position of the NED frame on earth surface
        *   @param  lon             [in] longitude [rad] position of the NED frame on earth surface
        *   @param  xyzInECEF       [in] vector in ECEF frame
        *   @param  xyzOutNED       [out] vector in NED frame
        */
        static void ECEFToNED( const Double& lat, const Double& lon, const VecD& xyzInECEF, VecD& xyzOutNED ) ;

		/** Transforms the given vector in NED frame ,which is positioned on 
		*	the given latitude and longitude, to a vector in NED frame.
        *   @param  lat             [in] latitude [rad] position of the NED frame on earth surface
        *   @param  lon             [in] longitude [rad] position of the NED frame on earth surface
        *   @param  xyzInNED        [in] vector in NED frame
        *   @param  xyzOutECEF      [out] vector in ECEF frame
        */
        static void NEDToECEF( const Double& lat, const Double& lon, const VecD& xyzInNED, VecD& xyzOutECEF ) ;

        /** Returns the ECEF coord. system to NED coord. system transformation matrix.
        *   for the given latitude and longitude, both in radians.
        */
        static void getECEFToNEDTransformationMatrix( const Double& lat, const Double& lon, smMathMatrixD::Mat4& m ) ;

        static void getNEDToECEFTransformationMatrix( const Double& lat, const Double& lon, smMathMatrixD::Mat4& m ) ;

        /** Latitude and longitude are always represented in radians.
        *   Latitude is in range [-pi/2,+pi/2] and longitude in [-pi,+pi]
        *   This methods checks if lat. and lon. are in ranges. If not
        *   returns false.
        */
        static bool checkLL( const Double& lat, const Double& lon ) ;
    };

}