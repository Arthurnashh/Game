//
// This file is generated from template file DinaPhysics/Templates/smVec4_Templ.h on Mon Mar 29 19:31:28 2010
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

#include "smVec4dBase.h"
#include "smVec3r.h"
#include "smVec3d.h"

/**
* This is a specialization of the smVec-Class for 4 dimensions.
*
* The specialization is necessary because the number of parameters of the
* set-method depends on the dimension of the coordinate.
*/

class smVec4d : public smVec4dBase
{

public:

	/**
	* Constructor.
	*/
	smVec4d();

	/**
	* Constructor.
	*/
	smVec4d( smVec3r&, smDouble fourthVal );
	smVec4d( smVec3d&, smDouble fourthVal );

    /**
    * Constructor with coordinate values.
    *
    * @param aX the x-component of the coordinate
    * @param aY the y-component of the coordinate.
    * @param aZ the z-component of the coordinate.
    * @param aW the w-component of the coordinate.
    */
    smVec4d( const smDouble &aX, const smDouble &aY, const smDouble &aZ, const smDouble &aW );

    /**
    * Sets coordinate.
    *
    * @param aX the x-component of the coordinate.
    * @param aY the y-component of the coordinate.
    * @param aZ the z-component of the coordinate.
    * @param aW the w-component of the coordinate.
    */
    void set( const smDouble &aX, const smDouble &aY, const smDouble &aZ, const smDouble &aW );

    /**
    * Gets x-component of the coordinate.
    *
    * @return the x-component.
    */
    const smDouble& x()  const;
    smDouble& x();

    /**
    * Gets y-component of the coordinate.
    *
    * @return the y-component.
    */
    const smDouble& y()  const;
    smDouble& y();

    /**
    * Gets z-component of the coordinate.
    *
    * @return the z-component.
    */
    const smDouble& z()  const;
    smDouble& z();

    /**
    * Gets w-component of the coordinate.
    *
    * @return the w-component.
    */
    const smDouble& w()  const;
    smDouble& w();

    /**
    * Addition operator.
    *
    * @param aOperand the operand.
    *
    * @return result.
    */
    smVec4d operator+( const smVec4d& aOperand ) const;

    /**
    * Subtraction operator.
    *
    * @param aOperand the operand.
    *
    * @return result.
    */
    smVec4d operator-( const smVec4d& aOperand ) const;

    /**
    * Scalar multiplication operator.
    *
    * @param aOperand the operand.
    *
    * @return result.
    */
    smVec4d operator*( smDouble aOperand ) const;

    /**
    * Scalar division operator.
    *
    * @param aOperand the operand.
    *
    * @return result.
    */
    smVec4d operator/( smDouble aOperand ) const;

    /** Dot product */
    smDouble  dot( smVec4d right ) const;

    /** Multiply the given vector with the given matrix as  M*v and assign results to this vector. */
    void transform4x4	( const smMatrix& m, const smVec4d v );
    
	/**
    * Get the squared length of this vector (or squared distance from the
    * origin if the vec is being treated as a point).
    */
    smDouble sqrLength()  const ;

    /**
    * Get the length of this vector (or distance from the origin if the
    * vec is being treated as a point).
    */
    smDouble length()  const ;
};

#pragma warning( pop )