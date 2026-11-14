//
// This file is generated from template file DinaPhysics/Templates/smMatrixBase_Templ.h on Mon Mar 29 19:31:28 2010
//

/*
(c) Copyright 2004, MachSim
All Rights Reserved.

Product:                DinaPhysics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of MachSim
*/
#pragma once

#include <stdio.h>
#include <string.h>

#include "Physics/smMatrix.h"
#include "Physics/smQuat.h"

/**     smMatrixBaser contains matrix data and matrix-order-convention independent functionality.
*       All methods in this class are independent of the order conventions (row major or column major).
*
*       Prefer to use this class and it's derived classes instead of the smMatrix class which offers
*       OpenGL only float matrix functionality.
*/
class smMatrixBaser
{
public:
        // When smReal is float, Mat4 is same as smMat4. It can
        typedef smReal Mat4[ 4 ][ 4 ];

protected:
        Mat4    m_mat ;

public:
                                smMatrixBaser                             ()                                              { }
                                smMatrixBaser                             ( const smMatrixBaser& m )        { set( m.m_mat );  }
        explicit                smMatrixBaser                             ( const Mat4& m )               { set( m );                       }

        explicit                smMatrixBaser                             (       smReal a11,smReal a12,smReal a13,smReal a14,
                                                                                                        smReal a21,smReal a22,smReal a23,smReal a24,
                                                                                                        smReal a31,smReal a32,smReal a33,smReal a34,
                                                                                                        smReal a41,smReal a42,smReal a43,smReal a44)
        {
                m_mat[0][0] = a11;      m_mat[0][1] = a12; m_mat[0][2] = a13; m_mat[0][3] = a14;
                m_mat[1][0] = a21;      m_mat[1][1] = a22; m_mat[1][2] = a23; m_mat[1][3] = a24;
                m_mat[2][0] = a31;      m_mat[2][1] = a32; m_mat[2][2] = a33; m_mat[2][3] = a34;
                m_mat[3][0] = a41;      m_mat[3][1] = a42; m_mat[3][2] = a43; m_mat[3][3] = a44;
        }

    virtual                     ~smMatrixBaser                            ()                                              {}

        static void copy( Mat4& dst, const Mat4& src )
        {
                memcpy( (void*) dst, (const void *) src, sizeof(smReal) * 16 );
        }
        void set( const Mat4& src )
        {
                copy( m_mat, src );
        }

        static bool invert( Mat4& dst, const Mat4& src )
        {
                smMatrixBaser tempDst;
                smMatrixBaser tempSrc( src );

                if ( tempDst.invert( tempSrc ) ) {
                        tempDst.get( dst );
                        return true;
                }
                else {
                        return false;
                }
        }

        /** Returns a const pointer to the internal smMat4 smReal array. */
    const smReal*          get                                             () const        {       return &m_mat[ 0 ][ 0 ];    }
    smReal*                        get                                             ()                      {       return &m_mat[ 0 ][ 0 ];    }

    /** Fills the given smMat4 to represent the current matrix.         */
    void                        get                                             ( Mat4 m )
        {
                int i, j;

                for ( i = 0; i < 4; i++ )
                {
                        for ( j = 0; j < 4; j++ )
                        {
                                m[ i ][ j ] = m_mat[ i ][ j ];
                        }
                }
        }

        void makeIdent ( void ){        smMatrixBaser::makeIdent( m_mat ); }

        // Static Method
        static void makeIdent( Mat4& dst )
        {
                int i , j;

                for ( i = 0; i < 4; i++ )
                {
                        for ( j = 0; j < 4 ;j++ )
                        {
                                dst[ i ][ j ] = 0.0f ;
                        }
                }
                dst[ 0 ][ 0 ] = 1.0f;
                dst[ 1 ][ 1 ] = 1.0f;
                dst[ 2 ][ 2 ] = 1.0f;
                dst[ 3 ][ 3 ] = 1.0f;
        }

        bool invert( const smMatrixBaser& src )
        {
                int i;
                Mat4 tmp;

                copy( tmp, src.m_mat ) ;
                makeIdent();

                for ( i = 0 ; i != 4 ; i++ ) {

                        smReal val = tmp[ i ][ i ] ;
                        int ind = i ;
                        int j;

                        for ( j = i + 1; j != 4 ;j++ ) {
                                if ( fabs ( tmp[ i ][ j ] ) > fabs( val ) ) {
                                        ind = j;
                                        val = tmp[ i ][ j ] ;
                                }
                        }

                        // Swap columns
                        if ( ind != i ) {
                                for ( j = 0; j != 4; j++ ) {
                                        smReal t ;
                                        t = m_mat[ j ][ i ];
                                        m_mat[ j ][ i ] = m_mat[ j ][ ind ];
                                        m_mat[ j ][ ind ] = t;
                                        t = tmp[ j ][ i ];
                                        tmp[ j ][ i ] = tmp[ j ][ ind ];
                                        tmp[ j ][ ind ] = t;
                                }
                        }


                        if ( fabs(val) <= FLT_EPSILON ) {
                                printf( "sm: ERROR - Singular matrix, no inverse!\n" ) ;

                                // Do something !
                                makeIdent();
                                return false;
                        }

                        smReal ival = 1.0f / val;

                        for ( j = 0; j != 4; j++ ) {
                                tmp[ j ][ i ]           *= ival;
                                m_mat[ j ][ i ] *= ival;
                        }

                        for ( j = 0; j != 4; j++ ) {

                                if ( j == i )
                                        continue ;

                                val = tmp[ i ][ j ] ;
                                int k;
                                for ( k = 0; k != 4; k++ ) {
                                        tmp[ k ][ j ] -= tmp[ k ][ i ] * val;
                                        m_mat[ k ][ j ] -= m_mat[ k ][ i ] * val;
                                }
                        }
                }
                return true;
        }


        smReal* operator[] ( int i )
        {
                return m_mat[i];
        }

        const smReal*      operator[] ( int i ) const
        {
                return m_mat[i];
        }


        smMatrixBaser& operator=( smMatrixBaser& m )
        {
                for ( unsigned int j = 0; j < 4; j++ )
                {
                        for ( unsigned int i = 0; i < 4; i++ )
                        {
                                m_mat[i][j] = m[i][j];
                        }
                }

                return *this;
        }

};
