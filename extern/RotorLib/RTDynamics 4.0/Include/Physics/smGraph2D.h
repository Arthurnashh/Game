/*
(c) Copyright 2004, MachSim
All Rights Reserved.

Product:		DinaPhysics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of MachSim
*/
#pragma once


#pragma warning(disable : 4786)
#include <vector>
#include <deque>
#include <assert.h>

#include "smTypes.h"
#include "smIntegralType.h"

class smUnitSystem ;

typedef smReal	smGpReal ;

/**
*
*
*
*/
class smGraphPoint
{
    public :
        smGraphPoint ( smGpReal px = 0.0f , smGpReal py = 0.0f , smGpReal pz = 0.0f ) { x = px; y = py ; z = pz ; } ;
        smGraphPoint ( const smGraphPoint &p ) { x = p.x; y = p.y; z = p.z; } ;

        virtual ~smGraphPoint() {} ;

    public:
        smGpReal x;
        smGpReal y;

public:
	smGpReal z;
};

/**
*
*
*/
class smGraph2D
{
public:
    smGraph2D	( size_t maxCount = 1800 );
    smGraph2D	( const smGraph2D &right );

    virtual ~smGraph2D();

public:
    /** @name File Access
    *
    */
    //@{
    bool	save ( std::string fileName ) ;
    bool	load ( std::string fileName ) ;
	bool    readString( const char* str, smGpReal scale=1, smGpReal offset=0, smGpReal conversionFactor = 1);
    //@}

    /**
    *	Graph Parameters Access
    */
    void setName			( const std::string& name )		{ m_name = name ;};
    void setXLabel			( const std::string& label )	{ m_xlabel = label ; } ;
    void setYLabel			( const std::string& label )	{ m_ylabel = label ; } ;

    /**	Bias and scale can be used to offset or scale the graph in 2nd dimension.	*/
    void setBias			( const smGpReal& bias )		{ m_bias = bias ;} ;
    void setScale			( const smGpReal& scale )		{ m_scale = scale ; } ;
    smGpReal getBias			( ) const { return m_bias ; } ;
    smGpReal getScale			( ) const { return m_scale ; } ;

    /**	Return the name of the graph. */
    const std::string& getName		( ) const { return m_name ; };

    /**	Return the x label of the graph. */
    const std::string& getXLabel	( ) const { return m_xlabel ; } ;

    /**	Return the y label of the graph. */
    const std::string& getYLabel	( ) const { return m_ylabel ; } ;

    /** Return the type for X and Y axes. */
    int    getXType			( ) const { return m_xtype ; } ;
    int    getYType			( ) const { return m_ytype ; } ;

    void   setLocalCount	( size_t localCount ) ;
    size_t getLocalCount	( ) { return m_localCount; } ;


    /**
    *	Functions below needs to check Unit Conversion
    */
    void setXType( int type, smUnitSystem	*xUnitSys = NULL )
                    {
                        m_xtype = (smIntegralType::Type) type ;
                        m_xUnitSys = xUnitSys ;
                    } ;

    void setYType  ( int type, smUnitSystem *yUnitSys = NULL )
                    {
                        m_ytype = (smIntegralType::Type) type ;
                        m_yUnitSys = yUnitSys ;
                    } ;


    /** @name Point management methods
    *
    */
    //@{
    smGraphPoint	getPoint	( size_t idx, smUnitSystem *guiUnitSys = NULL ) const ;
    void			addPoint	( smGraphPoint point, smUnitSystem *guiUnitSys = NULL ) ;
    void			insertPoint ( smGraphPoint point, size_t idx, smUnitSystem *guiUnitSys = NULL ) ;
    bool			updatePoint ( size_t idx, smGraphPoint point, smUnitSystem *guiUnitSys = NULL ) ;

    void    		clearPoints		() ;
    void			setMaxNumPoints ( size_t val ) ;
    size_t			getMaxNumPoints ( void ) const;
    size_t			getNumPoints	() const	{ return m_pointList.size() ; } ;
    bool			removePoint		( size_t idx ) ;
    //@}

    smGpReal getMeanValue		( int axes, smUnitSystem *guiUnitSys = NULL ) const;
    smGpReal getStdDeviation	( int axes, int flag = 0, smUnitSystem *guiUnitSys = NULL ) const ;

    smGpReal getMaxX	  ( smUnitSystem *guiUnitSys = NULL ) const;
    smGpReal getMaxY	  ( smUnitSystem *guiUnitSys = NULL ) const;
    smGpReal getMinX	  ( smUnitSystem *guiUnitSys = NULL ) const;
    smGpReal getMinY	  ( smUnitSystem *guiUnitSys = NULL ) const;

    smGpReal getLocalMaxX ( smUnitSystem *guiUnitSys = NULL ) ;
    smGpReal getLocalMaxY ( smUnitSystem *guiUnitSys = NULL ) ;
    smGpReal getLocalMinX ( smUnitSystem *guiUnitSys = NULL ) ;
    smGpReal getLocalMinY ( smUnitSystem *guiUnitSys = NULL ) ;

    /** Returns y data for a given x value. */
    bool	getXValue ( smGpReal inVal, std::vector<smGpReal> &outValList ) const ;
    bool	getYValue ( smGpReal inVal, std::vector<smGpReal> &outValList ) const ;

	smReal	getYValue ( smReal inVal , smReal defaultValue ) const ;

	bool	getYValue2 ( smReal inVal , smReal& result ) const ;

    /**
    *    Only returns the first matching pair
    */
    bool	getIndexFromPointX ( smGpReal inVal, size_t &prev, size_t &next ) ;

protected:

    std::string				m_name;
    std::string				m_xlabel;
    std::string				m_ylabel;

    smUnitSystem*			m_xUnitSys ;
    smUnitSystem*			m_yUnitSys ;

    smIntegralType::Type	m_xtype;
    smIntegralType::Type	m_ytype;

    smGpReal					m_bias;
    smGpReal					m_scale;

    smGpReal					m_maxX;
    smGpReal					m_maxY;
    smGpReal					m_minX;
    smGpReal					m_minY;

    size_t					    m_maxCount ;		// use to limit the points
    size_t					    m_localCount;
    smGpReal					m_localMaxX;
    smGpReal					m_localMaxY;
    smGpReal					m_localMinX;
    smGpReal					m_localMinY;

    std::deque<smGraphPoint>	m_pointList;
};
