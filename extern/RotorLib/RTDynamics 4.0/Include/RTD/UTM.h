#pragma once

#include "RTD/EarthModelWGS84.h"

namespace RTD
{
    enum ReferenceEllipsoid
    {
        RE_Airy  = 1,
        RE_AustralianNational,
        RE_Bessel_1841,
        RE_Bessel_1841_Nambia,
        RE_Clarke_1866,
        RE_Clarke_1880,
        RE_Everest,
        RE_Fischer_1960_Mercury,
        RE_Fischer_1968,
        RE_GRS_1967,
        RE_GRS_1980,
        RE_Helmert_1906,
        RE_Hough,
        RE_International,
        RE_Krassovsky,
        RE_Modified_Airy,
        RE_Modified_Everest,
        RE_Modified_Fischer_1960,
        RE_South_American_1969,
        RE_WGS_60,
        RE_WGS_66,
        RE_WGS_72,
        RE_WGS_84
    };

    /** UTM provides several static methods for conversions from geographic coordinates to
    *   UTM ( Universal Transverse Mercator coordinate system ) and the other way around.
    */
    class RTDYNAMICS_EXPORT UTM
    {
    public:
        typedef EarthModelWGS84::Double Double ;

        /** Computes the UTM coordinates of the position specified with the given
        *   latitude and longitude
        *
        *   @param  Lat     latitude in radians [in]
        *   @param  Lon     longitude in radians [in]
        *
        *   @param  UTMNorthing UTM northing coordinate in meters [out]
        *   @param  UTMEasting  UTM easting coordinate in meters [out]
        *   @param  UTMZone     String value of the UTM zone [out]
        *
        */
        static void LLtoUTM(const Double Lat, const Double Lon,
                            Double &UTMNorthing, Double &UTMEasting, std::string& UTMZone);

        /** Computes the geographic coordinates of the position specified with the given
        *   UTM coordinates
        *
        *   @param  UTMNorthing UTM northing coordinate in meters [in]
        *   @param  UTMEasting  UTM easting coordinate  in meters [in]
        *   @param  UTMZone     String value of the UTM zone [in]
        *   @param  re          Reference ellipsoid (usually WGS84) [in]
        *
        *   @param  Lat         latitude in radians [out]
        *   @param  Lon         longitude in radians [out]
        *
        *
        */
        static void UTMtoLL(const Double UTMNorthing, const Double UTMEasting,
                            const std::string& UTMZone, ReferenceEllipsoid re,
                            Double& Lat,  Double& Lon);

        /** This version of LLtoUTM is deprecated.
        *   @param  ReferenceEllipsoid reference ellipsoid id.
        *   @param  Lat             latitude in degrees
        *   @param  Lon             longitude in degrees
        *   @param  UTMNorthing     UTM northing coordinate in meters
        *   @param  UTMEasting      UTM easting coordinate  in meters
        *   @param  UTMZone         String value of the UTM zone
        */
        static void LLtoUTM(int ReferenceEllipsoid,
                            const Double Lat, const Double Lon,
                            Double &UTMNorthing, Double &UTMEasting, char* UTMZone);

        /** This version of UTMtoLL is deprecated. */
        static void UTMtoLL(int ReferenceEllipsoid,
                            const Double UTMNorthing, const Double UTMEasting,
                            const char* UTMZone, Double& Lat,  Double& Long );
    };

}