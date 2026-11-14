/*
(c) Copyright 2004, MachSim
All Rights Reserved.

Product:		DinaPhysics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of MachSim
*/
#pragma once

#include <stdio.h>

#include "smMath.h"
#include "smVec3r.h"
#include "smTypes.h"
#include "smVecTypes.h"

#include "Util/smNamed.h"
#include "Util/smEventGeneratorTrait.h"

class smPrinter ;

/**
*	The smMat4 is the backbone of smMatrix.
*
*	smMat4[r][c] is like this
*			Col0	Col1	Col2	Col3
*	row0	00		01		02		03
*	row1	10		11		12		13
*	row2	20		21		22		23
*	row3	30		31		32		33
*
*/
typedef float smMat4[ 4 ][ 4 ];

class smMatrix : public smEventGeneratorTrait
{
public:	// Inner types
    enum FloatArrayToMatrixMethod
    {
        MAKE_ROWS,
        MAKE_COLUMNS
    };

public:
    smMat4 m_matrix;

    /**
    *	Constructor of smMatrix doesn't initialize internal data. This is
    *	due to performance reasons. Call ::makeIdent() after construction if you
    *	really need a clean matrix.
    */
	        		smMatrix				()						{ }
	        		smMatrix				( const smMatrix& m )	{ set( m.m_matrix );  }
	explicit		smMatrix				( const smMat4 m )		{ set( m );			  }
					smMatrix				(	float a11,float a12,float a13,float a14,
												float a21,float a22,float a23,float a24,
												float a31,float a32,float a33,float a34,
												float a41,float a42,float a43,float a44);


    virtual			~smMatrix				()						{}

    /**	Sets the content of the matrix using the given source smMat4.    */
    void			set						( const smMat4 src ) ;
    /**	Static method allows copying the contents of a smMat4 to another.	*/
    static void		set						( smMat4 dst, const smMat4 src ) ;
    /**
    *	Sets the content of the matrix using the given source float array.
    *	Array must contain 16 floats.
    *
    *	If method is MAKE_ROWS, the first 4 float forms the first row and so on.
    *	If method is MAKE_COLUMNS, the first 4 float forms the first column and so on.
    *	( For conversion matrices from ODE use MAKE_COLUMNS ).
    */
	// Tested
	void			set						( const float* v, FloatArrayToMatrixMethod method = MAKE_ROWS ) ;
	void			set						( const double* v, FloatArrayToMatrixMethod method = MAKE_ROWS ) ;
	void			setXYZ					( const smReal& x, const smReal& y, const smReal& z ) ;
	void			setXYZ					( const smPositionVec& xyz ) ;
	void			setScale				( const smReal& x, const smReal& y, const smReal& z ) ;

    //
	//	Addition & Subtraction
	//
    static void		add						( smMat4 dst, const smMat4 src ) ;
    static void		add						( smMat4 dst, const smMat4 src0, const smMat4 src1 ) ;
    void			add						( const smMatrix& m1 ) ;
    void			add						( const smMatrix& m1, const smMatrix& m2 ) ;
    static void		sub						( smMat4 dst, const smMat4 a , const smMat4 b ) ;
    static void		sub						( smMat4 dst, const smMat4 src ) ;
    void			sub						( const smMatrix& m1 ) ;
    void			sub						( const smMatrix& m1, const smMatrix& m2 ) ;


    /** Makes a coordinate matrix which contains translations and rotations.	*/
	// Tested
	void			makeCoordMat			( float x, float y, float z, smAngleInDeg h, smAngleInDeg p, smAngleInDeg r ) ;
	void			makeCoordMat			( const smPositionVec& xyz, const smAngleDVec& hpr ) ;


	//
	//	Rotation
	//
    void			makeRotMat				( smAngleDVec hpr ) ;
    
	// Create a rotation matrix in OpenGL convention
	void			makeRotMat				( smAngleInDeg h, smAngleInDeg p, smAngleInDeg r ) ;

	// Create a rotation matrix in standard mathematical convention (i.e. usable with column vectors)
	void			makeRotMatMath				( smAngleInDeg h, smAngleInDeg p, smAngleInDeg r ) ;

    /**
	*	Builds a rotation matrix which can be used to rotate a vector.
    *	Used in the following manner : vec x RotMat
    *	In other words : (v0 , v1, v2, 1 ) x < Mat4x4 >
    *	This is the OpenGL way of interpreting vectors and vector multiplications
    *	This is NOT the mathematical interpretation of vectors. See smMathMatrix::makeRotMat.
	*/
	// Tested
    void			makeRotMat				( smAngleInDeg angleDeg, smVec3r ax );

    //
	//	Scaling
	//
    static void		makeScaleMat			( smMat4 m, float x, float y, float z ) ;
    static void		makeScaleMat			( smMat4 m, const smVec3r xyz ) ;
    void			makeScaleMat			( const smVec3r xyz ) ;
    void			makeScaleMat			( float x, float y, float z ) ;

    //
	//	Translation
	//
    static void		makeTransMat			( smMat4 m, const smVec3r xyz ) ;
    static void		makeTransMat			( smMat4 m, float x, float y, float z ) ;
    void			makeTransMat			( const smVec3r xyz ) ;
    void			makeTransMat			( float x, float y, float z  );

	//
	//	Get methods of the matrix
	//
	smVec3r			getXYZ					( ) const ;
	void			getXYZ					( smReal& x, smReal& y, smReal& z ) const ;
	void			getXYZ					( smVec3r& xyz ) const ;
	void			getScale  				( smReal& x, smReal& y, smReal& z ) const ;
	void			getScale				( smVec3r& xyz ) const ;
    bool			getHPR					( smAngleInDeg& heading, smAngleInDeg& pitch, smAngleInDeg& roll ) const ;
    bool			getHPR					( smAngleDVec& hpr ) const ;

	/** Returns the rotation as HPR in degrees.	*/
    bool			getHPRInZXYConvention	( smAngleInDeg& rfZAngle, smAngleInDeg& rfXAngle, smAngleInDeg& rfYAngle) const ;

    /**
	*	Returns the vector for the y-axis of possibly rotated coord sys matrix
    *	Method will correctly work only if matrix is orthogonal.
	*/
	// Tested
    void			getDirection			( smVec3r& dir ) const ;

	/** Returns a const pointer to the internal smMat4 float array. */
    const float*	get						() const	{	return &m_matrix[ 0 ][ 0 ];    }
    float*			get						()			{	return &m_matrix[ 0 ][ 0 ];    }

    /** Fills the given smMat4 to represent the current matrix.		*/
    void			get						( smMat4 m ) ;

	/**
	*	Pre/Post operations and multiplications
	*/
    static void		multMatrices			( smMat4 dst, const smMat4 m1, const smMat4 m2 ) ;
    void			mult					( const smMatrix& m1, const smMatrix& m2 ) ;
    static void		preMultMatrices			( smMat4 dst, const smMat4 src ) ;
    static void		postMultMatrices		( smMat4 dst, const smMat4 src ) ;
    void			postMult				( const smMatrix& m ) ;
    void			postScale				( const smVec3r& v ) ;
    void			postScale				( float xs, float ys, float zs ) ;
    void			postTrans				( const smVec3r& v ) ;
    void			postTrans				( float x, float y, float z ) ;
    void			preMult					( const smMatrix& m )    {	smMatrix::preMultMatrices( m_matrix , m.m_matrix );	}
    void			preScale				( const smVec3r& v ) ;
    void			preScale				( float xs, float ys, float zs ) ;
    void			preTrans				( const smVec3r& v ) ;
    void			preTrans				( float x, float y, float z ) ;

	//
	//	Make Identity matrix
	//
    static void		makeIdent( smMat4 dst ) ;
    void			makeIdent				( void ){	smMatrix::makeIdent( m_matrix );	}

    /** Inverts the given src matrix and stores result in this matrix.	*/
	// Tested
    bool			invert					( const smMatrix& src ) ;
    static bool		invert					( smMat4 dst, const smMat4 src ) ;

    /** Transpose the given matrix. Note: dst can be same as source because internally a temporary matrix is used.	*/
	// Tested
    static void		transpose				( smMat4 dst, const smMat4 src ) ;
	/** Transposes the current matrix. */
    void			transpose				( void )					{	smMatrix::transpose( m_matrix, m_matrix );	}
	/** Sets the current matrix with the transpose of the given matrix m. */
	void			transpose				( const smMatrix& m )		{	smMatrix::transpose( m_matrix , m.m_matrix ) ;	}

	//
	//	Overloaded Operators
	//
    bool			operator!=				( const smMatrix& m ) const ;
    smMatrix&		operator*=				( const smMatrix& m ) ;
    smMatrix&		operator=				( const smMatrix& m ) ;
    bool			operator==				( const smMatrix& m ) const ;
	// Note: all [] operators must match the canonical type of the original
	// operator, which says: object &operator [] (int x)'
    float*			operator[]				( int i ) ;
    const float*	operator[]				( int i ) const ;
					operator float*			( ) ;
					operator const float*	( ) const ;
	// @tricky: The implementation of this operator has to be here since when put into .cpp linker breaks.
	inline smMatrix operator*				( const smMatrix& m ) const
	{
		smMatrix r;
		r.mult( *this, m );
		return r;
	}
	inline smVec3r   operator*				( const smVec3r& v ) const
	{
		smVec3r vec;
		vec.transform3x3( *this, v ) ;
		return vec;
	}

	inline smVec4r   operator*				( const smVec4r& v ) const
	{
		smVec4r vec;
		vec.transform4x4( *this, v ) ;
		return vec;
	}

/*	inline smVec4r   operator*				( const smVec4r& v ) const
	{
		smVec4r vec;
		vec.transform4x4( *this, v ) ;
		return vec;
	}*/

    //
	//	Printing
	//
	static void		print					( smMatrix& m, std::string prefix = " " ) ;
    static void		print					( smMat4 m, std::string prefix ) ;
	void			print					( std::string prefix = " " ) ;
	void			print					( smPrinter* printer, std::string prefix = ":", bool hprAndXyz = false )  const;



    /**
    *	Check if matrix has any element which is not finite
    *	Returns true if there is a infinite element.
    *	The position of the element is [i,j]
    */
    bool			isAnyInfiniteElements	( int& i, int& j );

    /**
    *	Convert this matrix from a row major matrix to
    *	column major matrix or vice versa.
    */
    void			rowToColumnMajor		( ) ;
};

