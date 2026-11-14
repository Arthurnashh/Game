/*
(c) Copyright 2004, MachSim
All Rights Reserved.

Product:		DinaPhysics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of MachSim
*/
#pragma once


#include <assert.h>
#include "float.h"

//
// This header requires that it includes no smIntegralType derived
// classes.
//
#include "Util/smPrinter.h"
#include "Util/smMacros.h"
#include "Util/smUtil.h"
#include "Physics/smPrecision.h"
#include "Physics/smOperatorMacros.h"
#include "Util/smEventGeneratorTrait.h"


#define SMINTEGRALTYPE_COMMON(theClass,type,min,max,def) \
    SMINTEGRALTYPE_COMMON_EQUALITY_OPERATOR(theClass) \
    SMINTEGRALTYPE_COMMON_CTORS(theClass,type,min,max,def) \
    SMINTEGRALTYPE_COMMON_CTORS_OPERATORS_DECL(theClass)

//
// Common constructors
//
#define SMINTEGRALTYPE_COMMON_CTORS(theClass,type,min,max,def) \
public:\
theClass()\
{\
    init( type, def, min, max ) ;\
}\
theClass( double value )\
{\
    init( type, def, min, max ) ;\
    setVal( static_cast<smReal>(value) ) ;\
}\
theClass( float value )\
{\
    init( type, def, min, max ) ;\
    setVal( static_cast<smReal>(value) ) ;\
}\
theClass( int value )\
{\
    init( type, def, min, max ) ;\
    setVal( static_cast<smReal>(value) ) ;\
}\
theClass( smReal _min, smReal _max, smReal _def )\
{\
    init( type, _def, _min, _max) ;\
}


//
// = operators
//
#define SMINTEGRALTYPE_COMMON_EQUALITY_OPERATOR(theClass) \
public:\
theClass & operator=(const int &value ) \
{ \
    setVal( static_cast<smReal>(value) ) ;\
    return *this ;\
}\
theClass & operator=(const bool &value ) \
{ \
    setVal( static_cast<smReal>(value) ) ;\
    return *this ;\
}\
theClass & operator=(const smReal &value ) \
{ \
    setVal( value ) ;\
    return *this ;\
}\
theClass & operator=(const theClass &right)\
{\
    m_type	=	right.getType() ;\
    setVal( right.val() ) ;\
    m_min	=	right.getMin();\
    m_max	=	right.getMax();\
    m_def	=	right.getDef();\
    return *this ;\
}

/**

*/
class smIntegralType : public smEventGeneratorTrait
{
public:

    SMINTEGRALTYPE_COMMON(smIntegralType, SM_UNDEFINED, 0.0f, 0.0f, 0.0f);

public:

    // DO NOT FORGET TO UPDATE UNIT SYSTEM ALSO
    enum Type
    {

        SM_UNDEFINED = 0,
            //--------------
            SM_ACCEL,
            SM_ACCELVEC,
            SM_ALTITUDE,
            SM_ANGLE,
            SM_ANGLED,
            SM_ANGULARACCEL,
            SM_ANGULARRATE,
            SM_ANGULARMOMENTUM,
            SM_BOOLEAN,
            SM_COEF,
            SM_DENSITY,
            SM_DIMENSION,
            SM_FORCE,
            SM_FORCEVEC,
            SM_FRICTION,
            SM_FLOAT,
            SM_FREQUENCY,
            SM_HEIGHT,
            SM_INERTIA,
            SM_LENGTH,
            SM_LINEARMOMENTUM,
            SM_MASS,
            SM_MASSFLOWRATE,
            SM_MOMENTUM,
            SM_MOMENTUMVEC,
            SM_HALF_NEGATIVE_POSITIVE_COEFFICIENT,
            SM_NEGATIVE_POSITIVE_COEFFICIENT,
            SM_NEGATIVE_FLOAT,
            SM_PERCENT,
            SM_POSITION,
            SM_POSITIVE_COEFFICIENT,
            SM_POSITIVE_FLOAT,
            SM_POWER,
            SM_PRESSURE,
            SM_RPM,
            SM_SPEED,
            SM_SURFACE,
            SM_TEMPERATURE,
            SM_THROTTLE,
            SM_TIME,
            SM_TORQUE,
            SM_VELOCITY,
            SM_VOLUME,
            SM_VOLTAGE,
            SM_FILENAME,
            //--------------
            SM_TYPE_NUMTYPES
    };

public:
    /** */
    virtual ~smIntegralType();

    // Deprecated, do not use, use val(), it is shorter
    virtual smReal getValue() const
    {	return m_value ; }

    /** */
    virtual smReal val()
    {	return m_value ; }

    /** */
    virtual smReal val() const
    {	return m_value ; }

    /** */
    virtual int intVal() const
    {	return static_cast<int>( m_value ) ; }

    /** */
    virtual unsigned int uintVal() const
    {	return static_cast<unsigned int>( m_value ) ; }

    /** */
    smReal absVal() const ;

    /** */
    virtual bool isEqual( smReal refValue ) const ;

    /** */
    inline Type getType() const
    {	return m_type ;	}

    /** */
    inline smReal getMin() const
    {	return m_min ; 	}

    /** */
    inline smReal getMax() const
    {	return m_max ;	}

    /** */
    inline smReal getDef() const
    {	return m_def ; 	}

    /** */
    inline void setMinMaxDef( smReal min, smReal max, smReal def )
    {	setMin ( min ) ;	setMax ( max ) ;	setDef ( def ) ; }

    /** */
    inline void setDef( smReal def )
    {	m_def	=  def ;	setVal( def ) ; }

    /** */
    inline void setMin( smReal min )
    {	m_min	=	min ; }

    /** */
    inline void setMax( smReal max )
    {	m_max	=	max ; }

    /**
    *  @param   callValueChangedFunctors    Set this value to false when calling setVal from the () operator of
    *                                       a ValueChangedOperation functor class.
    */
    inline void             setVal ( smReal val, bool callValueChangedFunctors=true )
    {
		smReal oldValue = m_value ;
        m_value	=	val ;

        if ( callValueChangedFunctors )
			if ( m_value != oldValue )
				callEventHandlers ( smEventGeneratorTrait::VALUE_CHANGED ) ;
    }

    /**	Resets the value of this smIntegralType to the default.
	*	Deprecated. Please use reset().
    */
	void                    resetToDefault	()
	{ setVal( m_def ); 	}

	/** Resets the value of this smIntegralType to the default.
	*	Prefer this method over resetToDefault()
	*/
	void                    reset()
	{ 
		resetToDefault() ;
	}

    /** */
    void init( Type type, smReal def, smReal min, smReal max );

    /** */
    static std::string	getTypeText		( Type type ) ;

    /** */
    static Type			getTypeByText	( std::string name ) ;

    /** */
    std::string getTypeName() const
    {	return getTypeText( getType() ) ; }

    /** */
    void validate ();

    /** Print the value of this smIntegralType to the given printer.	*/
	void					print( smPrinter* pr = NULL, const std::string& prefix = std::string("") ) const ;

    /**
    *	Automatic conversion operator
    *	returns value when class is type casted to smReal. Using other types
    *	such as int, smReal will cause ambiguity problems with operators.
    */ 
	virtual operator smReal&()
	{	return m_value; };

	virtual operator const smReal&() const
	{	return m_value; };

    //
    // *= operators
    //
    smIntegralType & operator*=(const smReal &value ) ;
    smIntegralType & operator*=(const int &value ) ;

    //
    // += operators
    //
    smIntegralType & operator+=(const smReal &value ) ;
    smIntegralType & operator+=(const int &value ) ;

    //
    // -= operators
    //
    smIntegralType & operator-=(const smReal &value ) ;
    smIntegralType & operator-=(const int &value ) ;

    //
    // /= operators
    //
    smIntegralType & operator/=(const smReal &value ) ;
    smIntegralType & operator/=(const int &value ) ;

    //
    // == operators
    //
    bool operator==(const smIntegralType &right) const
    {
        if ( m_type == right.getType()	&&
            m_value == right.val()		&&
            m_min  == right.getMin()   &&
            m_max  == right.getMax()   &&
            m_def  == right.getDef()
            )
            return true ;
        else
            return false ;
    }
    bool operator==(const bool right) const
    {
        if ( m_value == (smReal) right )
            return true ;
        else
            return false ;
    }
    bool operator==(const int right) const
    {
        if ( m_value == (smReal) right )
            return true ;
        else
            return false ;
    }
    bool operator==(const smReal right) const
    {
        if ( m_value == (smReal) right )
            return true ;
        else
            return false ;
    }

	/** Prefer to use setVal to change the value, setVal() makes sure that all callback methods are
	*	called upon value change.
	*/
	smReal m_value;

protected:
    Type m_type;


    smReal m_min;
    smReal m_max;
    smReal m_def;
};

