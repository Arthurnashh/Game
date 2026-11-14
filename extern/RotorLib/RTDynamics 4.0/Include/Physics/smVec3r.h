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

#include "smVec3rBase.h"

/**
* This is a specialization of the smVec-Class for 3 dimensions.
*
* The specialization is necessary because the number of parameters of the
* set-method depends on the dimension of the coordinate.
*/

class smVec3r : public smVec3rBase
{

public:

    /**
    * Constructor.
    */
    smVec3r();

    /**
    * Constructor with coordinate values.
    *
    * @param aX the x-component of the coordinate
    * @param aY the y-component of the coordinate.
    * @param aZ the z-component of teh coordinate.
    */
    smVec3r( const smReal &aX, const smReal &aY, const smReal &aZ );

    /**
    * Sets coordinate.
    *
    * @param aX the x-component of the coordinate.
    * @param aY the y-component of the coordinate.
    * @param aZ the z-component of teh coordinate.
    */
    void set( const smReal &aX, const smReal &aY, const smReal &aZ );

    /**
    * Gets x-component of the coordinate.
    *
    * @return the x-component.
    */
    const smReal& x()  const;
    smReal& x();

    /**
    * Gets y-component of the coordinate.
    *
    * @return the y-component.
    */
    const smReal& y()  const;
    smReal& y();

    /**
    * Gets z-component of the coordinate.
    *
    * @return the z-component.
    */
    const smReal& z()  const;
    smReal& z();

    /**
    * Computes heading and pitch in degrees of
    * this vector.
    */
    void getHP(  smAngleInDeg& h, smAngleInDeg& p )  const ;

    /**
    * Get the squared length of this vector (or squared distance from the
    * origin if the vec is being treated as a point).
    */
    smReal sqrLength()  const ;

    /**
    * Get the length of this vector (or distance from the origin if the
    * vec is being treated as a point).
    */
    smReal length()  const ;

    /**
    * Normalize this vector
    */
    void normalize() ;

    /**
    * Distance from *this* to a point
    */
    smReal distance(  const smVec3r& v ) const;

    /**
    * Get the squared distance between this point and v.
    */
    smReal sqrDistance(  const smVec3r& v ) const ;

    /** Multiply the given vector with the given matrix as  v*M and assign results to this vector. */
    void transform3x3	( const smVec3r v, const smMatrix& m );

    /** Multiply the given vector with the given matrix as  M*v and assign results to this vector. */
    void transform3x3	( const smMatrix& m, const smVec3r v );

    /**
    * Returns true if this vector is equal to v within the
    * tolerance specified by tol.
    */
    /*bool  almostEqual(  const smVec3r& v, smReal tol ) const;*/

	/** Computes the projection of v on the d. */
	// Ref:Eberly p603
	static smVec3r project( const smVec3r& v, const smVec3r& d );
	/** Computes the projection of this vector on the d */
	// Ref:Eberly p603
	smVec3r project( const smVec3r& d );

    /**
    *
    *
    *
    */
    void scale ( const smVec3r& aOperand );
    void scale ( const smReal& x, const smReal& y, const smReal& z );
    void scale ( const smReal& aValue );

    void add ( const smVec3r& aOperand );
    void add ( const smReal& x, const smReal& y, const smReal& z );
    void add ( const smReal& aValue );

    void sub ( const smVec3r& aOperand );
    void sub ( const smReal& x, const smReal& y, const smReal& z );
    void sub ( const smReal& aValue );


    /**
    * Addition operator.
    *
    * @param aOperand the operand.
    *
    * @return result.
    */
    smVec3r operator+( const smVec3r& aOperand ) const;
    smVec3r operator+( const int&  aOperand ) const;
    smVec3r operator+( const smReal& aOperand ) const;

    /**
    * Subtraction operator.
    *
    * @param aOperand the operand.
    *
    * @return result.
    */
    smVec3r operator-( const smReal& aOperand ) const;
    smVec3r operator-( const smVec3r& aOperand ) const;

    /**
    * Scalar multiplication operator.
    *
    * @param aOperand the operand.
    *
    * @return result.
    */
	smVec3r operator*( const smReal& aOperand ) const;
    smVec3r operator*( const smVec3r& aOperand ) const;

    /** Dot product */
    smReal  dot( smVec3r right ) const;

    /** Compues the cross product and assigns the results to this vector. */
    void    cross( smVec3r left, smVec3r right );

    /**
    * Scalar division operator.
    *
    * @param aOperand the operand.
    *
    * @return result.
    */
    smVec3r operator/( const smReal& aOperand ) const;
    smVec3r operator/( const smVec3r& aOperand ) const;

    /**
    * Equality check operator.
    *
    * @param aOperand the operand.
    *
    * @return result.
    */
    bool operator==( const smVec3r& aOperand  ) const;
    bool operator!=( const smVec3r& aOperand  ) const;
};

#pragma warning( pop )