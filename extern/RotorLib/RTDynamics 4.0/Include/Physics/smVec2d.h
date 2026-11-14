//
// This file is generated from template file DinaPhysics/Templates/smVec2_Templ.h on Mon Mar 29 19:31:28 2010
//

/*
(c) Copyright 2004, MachSim
All Rights Reserved.

Product:		DinaPhysics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of MachSim
*/
#pragma once

// Prevents 4267 warning for this file. It is ok, we can ignore this warning here:
// warning C4267: 'argument' : conversion from 'size_t' to 'int', possible loss of data
// for operator[].
#pragma warning( push )
#pragma warning( disable:4267 )
#pragma warning( disable:4786 )

#include "Physics/smVec2dBase.h"

/**
* This is a specialization of the smVec-Class for 2 dimensions.
*
* The specialization is necessary because the number of parameters of the
* set-method depends on the dimension of the coordinate.
*/

class smVec2d : public smVec2dBase
{

public:

    /**
    * Constructor.
    */
    smVec2d();

    /**
    * Constructor with coordinate values.
    *
    * @param aX the x-component of the coordinate
    * @param aY the y-component of the coordinate.
    */
    smVec2d( const smDouble  &aX, const smDouble  &aY );

    /**
    * Sets coordinate.
    *
    * @param aX the x-component of the coordinate.
    * @param aY the y-component of the coordinate.
    */
    void set( const smDouble  &aX, const smDouble  &aY );

    /**
    * Gets x-component of the coordinate.
    *
    * @return the x-component.
    */
    const smDouble & x()  const;
    smDouble & x();

    /**
    * Gets y-component of the coordinate.
    *
    * @return the y-component.
    */
    const smDouble & y()  const;
    smDouble & y();

    /**
    * Addition operator.
    *
    * @param aOperand the operand.
    *
    * @return result.
    */
    smVec2d operator+( const smVec2d& aOperand ) const;

    /**
    * Substraction operator.
    *
    * @param aOperand the operand.
    *
    * @return result.
    */
    smVec2d operator-( const smVec2d& aOperand ) const;

    /**
    * Scalar multiplication operator.
    *
    * @param aOperand the operand.
    *
    * @return result.
    */
    smVec2d operator*( const smDouble&  aOperand ) const;

    /**
    * Scalar division operator.
    *
    * @param aOperand the operand.
    *
    * @return result.
    */
    smVec2d operator/( const smDouble&  aOperand ) const;

	/**
	* Equality check operator.
	*
	* @param aOperand the operand.
	*
	* @return result.
	*/
    bool operator==( const smVec2d& aOperand  ) const;
	bool operator!=( const smVec2d& aOperand  ) const;

	/**
    * Get the squared length of this vector (or squared distance from the
    * origin if the vec is being treated as a point).
    */
    smDouble  sqrLength()  const ;

    /**
    * Get the length of this vector (or distance from the origin if the
    * vec is being treated as a point).
    */
    smDouble  length()  const ;


	/**
	* Distance from *this* to a point
	*/
	smDouble  distance(  const smVec2d& v ) const;

	/**
	* Get the squared distance between this point and v.
	*/
	smDouble  sqrDistance(  const smVec2d& v ) const ;
};

#pragma warning( pop )