//
// This file is generated from template file DinaPhysics/Templates/smVecBase_Templ.h on Mon Mar 29 19:31:28 2010
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

#include <vector>

// Inclusion of stdio is needed for gcc 2.95.3	
#include <stdio.h>

#include "Physics/smMath.h"
#include "Physics/smPrecision.h"
#include "Physics/smTypes.h"
#include "Util/smPrinter.h"

#include "Util/smMacros.h"

class smMatrix ;

class smVec4dBase
{
public:
    /**
    * Symbols for element access.
    */
    enum{ NW_X=0, NW_Y, NW_Z, NW_W };

public:

    /**
    * Constructor.
     */
	smVec4dBase();

	/** Init the internal array with same value. */
	smVec4dBase( smDouble val );

    /**
    *
    * The coordinate is initialized by a given array, which must have
    * the same dimension as the coordinate. No size checking is done
    * to obtain maximum performance.
    *
    * @param aArray the array for initialization.
    */
    smVec4dBase( const smDouble aArray[] );

    /**
    * Conversion operator.
    *
    * Converts to array with entries of type 'smDouble'.
    */
    //        operator const smDouble*() const;
    operator smDouble*();

    /**
    * Accesses an element with range check.
    *
    * If the given index is out of range, the exception RangeError
    * is thrown.
    *
    * @param aIndex the element index.
    *
    * @return the element.
    */
    const smDouble& at( size_t aIndex )  const;
    smDouble& at( size_t aIndex );

	// Note: all [] operators must match the canonical type of the original
	// operator, which says: object &operator [] (int x)'
	smDouble&  operator[]  (  int idx )
	{ return at(  idx ); }
	const smDouble&  operator[]  ( int idx ) const
	{ return at(  idx ); }

    /**
    * Sets the components of the coordinate to the given value.
    *
    * @param aValue the value to be set.
    */
    void fill( const smDouble& aValue = smDouble() );


    /**
    * Addition assignment operator.
    *
    * @param aOperand the operand.
    *
    * @return result.
    */
    smVec4dBase& operator+=( const smVec4dBase& aOperand );
    smVec4dBase& operator+=( const int&      aOperand );
    smVec4dBase& operator+=( const smDouble&     aOperand );

    /**
    * Subtraction assignment operator.
    *
    * @param aOperand the operand.
    *
    * @return result.
    */
    smVec4dBase& operator-=( const smVec4dBase& aOperand );
    smVec4dBase& operator-=( const int&      aOperand );
    smVec4dBase& operator-=( const smDouble&     aOperand );

    /**
    * Scalar multiplication assignment operator.
    *
    * @param aOperand the operand.
    *
    * @return result.
    */
    smVec4dBase& operator*=( smDouble aOperand );
    smVec4dBase& operator*=( smVec4dBase& aOperand );

    /**
    * Scalar division assignment operator.
    *
    * @param aOperand the operand.
    *
    * @return result.
    */
    smVec4dBase& operator/=( smDouble aOperand );
    smVec4dBase& operator/=( smVec4dBase& aOperand );

	/**
	* Comparison
    * @param aOperand the operand.
    *
    * @return result.
	*/
	bool operator==( const smVec4dBase& aOperand  ) const;

    /**
    * Prints coordinate.
    *
    * @param the stream to which is printed.
    */
	void print(  std::string prefix = ":" )  const;
	void print(  smPrinter* printer, std::string prefix = ":" )  const;

    /**
    * Gets the coordinate array.
    *
    * @return the coordinate array.
    */
    const smDouble *getArray()  const;
    smDouble* getArray();

    /**
    * Gets dimension of the coordinate.
    *
    * @return the dimension of the coordinate.
    */
    static size_t getDim();

    /** reset the coordinates to initial values 
	*	Deprecated. Please use reset2.
	*/
    void reset()
    { fill(); }


	/** Returns a string representation of this vector. It is made available 
	*	for debugging purposes.
	*	getAsString is deprecated, please use toString
	*/
	std::string	getAsString( ) const ;
	std::string	toString( size_t prec=3, size_t minWidth=1 ) const ;

private:
    /**
    * Internal data structure
    */
    smDouble mArray[  4 ];
};

#pragma warning( pop )