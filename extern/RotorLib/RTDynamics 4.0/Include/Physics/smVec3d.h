//
// This file is generated from template file DinaPhysics/Templates/smVec3_Templ.h on Mon Mar 29 19:31:28 2010
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

#include "smVec3dBase.h"

/**
* This is a specialization of the smVec-Class for 3 dimensions.
*
* The specialization is necessary because the number of parameters of the
* set-method depends on the dimension of the coordinate.
*/

class smVec3d : public smVec3dBase
{

public:

    /**
    * Constructor.
    */
    smVec3d();

    /**
    * Constructor with coordinate values.
    *
    * @param aX the x-component of the coordinate
    * @param aY the y-component of the coordinate.
    * @param aZ the z-component of teh coordinate.
    */
    smVec3d( const smDouble &aX, const smDouble &aY, const smDouble &aZ );

    /**
    * Sets coordinate.
    *
    * @param aX the x-component of the coordinate.
    * @param aY the y-component of the coordinate.
    * @param aZ the z-component of teh coordinate.
    */
    void set( const smDouble &aX, const smDouble &aY, const smDouble &aZ );

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
    * Computes heading and pitch in degrees of
    * this vector.
    */
    void getHP(  smAngleInDeg& h, smAngleInDeg& p )  const ;

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

    /**
    * Normalize this vector
    */
    void normalize() ;

    /**
    * Distance from *this* to a point
    */
    smDouble distance(  const smVec3d& v ) const;

    /**
    * Get the squared distance between this point and v.
    */
    smDouble sqrDistance(  const smVec3d& v ) const ;

    /** Multiply the given vector with the given matrix as  v*M and assign results to this vector. */
    void transform3x3	( const smVec3d v, const smMatrix& m );

    /** Multiply the given vector with the given matrix as  M*v and assign results to this vector. */
    void transform3x3	( const smMatrix& m, const smVec3d v );

    /**
    * Returns true if this vector is equal to v within the
    * tolerance specified by tol.
    */
    /*bool  almostEqual(  const smVec3d& v, smDouble tol ) const;*/

	/** Computes the projection of v on the d. */
	// Ref:Eberly p603
	static smVec3d project( const smVec3d& v, const smVec3d& d );
	/** Computes the projection of this vector on the d */
	// Ref:Eberly p603
	smVec3d project( const smVec3d& d );

    /**
    *
    *
    *
    */
    void scale ( const smVec3d& aOperand );
    void scale ( const smDouble& x, const smDouble& y, const smDouble& z );
    void scale ( const smDouble& aValue );

    void add ( const smVec3d& aOperand );
    void add ( const smDouble& x, const smDouble& y, const smDouble& z );
    void add ( const smDouble& aValue );

    void sub ( const smVec3d& aOperand );
    void sub ( const smDouble& x, const smDouble& y, const smDouble& z );
    void sub ( const smDouble& aValue );


    /**
    * Addition operator.
    *
    * @param aOperand the operand.
    *
    * @return result.
    */
    smVec3d operator+( const smVec3d& aOperand ) const;
    smVec3d operator+( const int&  aOperand ) const;
    smVec3d operator+( const smDouble& aOperand ) const;

    /**
    * Subtraction operator.
    *
    * @param aOperand the operand.
    *
    * @return result.
    */
    smVec3d operator-( const smDouble& aOperand ) const;
    smVec3d operator-( const smVec3d& aOperand ) const;

    /**
    * Scalar multiplication operator.
    *
    * @param aOperand the operand.
    *
    * @return result.
    */
	smVec3d operator*( const smDouble& aOperand ) const;
    smVec3d operator*( const smVec3d& aOperand ) const;

    /** Dot product */
    smDouble  dot( smVec3d right ) const;

    /** Compues the cross product and assigns the results to this vector. */
    void    cross( smVec3d left, smVec3d right );

    /**
    * Scalar division operator.
    *
    * @param aOperand the operand.
    *
    * @return result.
    */
    smVec3d operator/( const smDouble& aOperand ) const;
    smVec3d operator/( const smVec3d& aOperand ) const;

    /**
    * Equality check operator.
    *
    * @param aOperand the operand.
    *
    * @return result.
    */
    bool operator==( const smVec3d& aOperand  ) const;
    bool operator!=( const smVec3d& aOperand  ) const;
};

#pragma warning( pop )