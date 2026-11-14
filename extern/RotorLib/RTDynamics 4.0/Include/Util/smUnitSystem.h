/*
(c) Copyright 2004, MachSim
All Rights Reserved.

Product:		DinaPhysics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of MachSim
*/
#pragma once


#include "Util/smManager.h"
#include "Util/smNamed.h"
#include "Util/smException.h"
#include "Physics/smIntegralType.h"

// Inclusion of stdio is needed for gcc 2.95.3
#include <stdio.h>

/*
An individual unit ( e.g meter, kilogram )
*/
class smUnit
{
    public:
    explicit smUnit() {
    } ;
    virtual ~smUnit() {
    } ;
    public :
    /**
    *
    */
    smIntegralType::Type m_type ;
    /**
    * A text symbol of the unit. E.g m/s, kg.m/s^2
    */
    std::string    m_symbol ;
    /**
    * Scale and offset to convert the unit from the
    * metric system to your specific unit system.
    *
    * E.g to define the feet unit use
    *  m_offset = 0.0
    *  m_scale  = 0.33
    */
    smReal     m_offset ;
    smReal     m_scale ;
};
/**
* The UnitSystem is a collection of units. By default UnitSystem
* is a metric system.
*/
class smUnitSystem  : public smManager<smUnit>, public smNamed
{
    public: // Types
    class InfiniteValueDuringConversion : public smException
    {
        public:
        InfiniteValueDuringConversion( std::string msg ) : smException( msg ) {} ;
        std::string	getExceptionName()
        {
            return "InfiniteValueDuringConversion" ;
        }
    };
    public:
    smUnitSystem ( std::string name );
    virtual ~smUnitSystem();
    /**
    * Add a new unit to list of units.
    */
    smUnit* addUnit(smIntegralType::Type type, std::string symbol, smReal scale, smReal offset ) ;
    smUnit* findUnitByType( smIntegralType::Type type ) ;
    /**
    * Given a metric system unit, value is converted to this
    * unit system. It is required that the unit is defined.
    */
    smReal toThis (smIntegralType::Type type, smReal val) ;

    /**
    * Converts the value to the metric system using the offset
    * and scale defined for the unit. It is required that the unit
    * is defined in this smUnitSystem.
    */
    smReal toMetric (smIntegralType::Type type, smReal val) ;

    /**
    * Convert a value to the given unit system
    */
    smReal toUnitSystem ( smUnitSystem& fromUSys, smIntegralType::Type type, smReal val ) ;

    /**
    * conversionFinityCheck checks if value exceeds boundaries of
    * smReal. If this is the case, SM_REAL_MAX or -SM_REAL_MAX is returned.
    *
    * Throws std::string exception when SM_REAL_MAX is exceeded during conversion.
    */
    smReal conversionFinityCheck( smReal origVal, smReal finalVal ) ;
    std::string getUnitSymbol( smIntegralType::Type type ) ;
    /**
    * Prints the units defined in this smUnitSystem
    */
    void printContents( FILE* file = stderr ) ;
    /**
    * Makes this unit system a metric system
    */
    void makeMetric() ;

    /**
    * Makes this unit system an British system
    */
    void makeBritish() ;
};