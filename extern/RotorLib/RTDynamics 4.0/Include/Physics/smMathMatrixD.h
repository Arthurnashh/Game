//
// This file is generated from template file DinaPhysics/Templates/smMathMatrix_Templ.h on Mon Mar 29 19:31:28 2010
//

#pragma once

#include "smMatrixBased.h"


/**     smMathMatrixD encapsulates matrix functions which are adjusted according to mathematical
*       matrix order. Therefore the methods in the class can be used with material from literature
*       unlike smMatrix.
*
*       smMathMatrixD is a template class to support both single and double precision floating point
*       types.
*/
class smMathMatrixD : public smMatrixBased
{
public:
    smMathMatrixD(smDouble a11,smDouble a12,smDouble a13,smDouble a14,
                smDouble a21,smDouble a22,smDouble a23,smDouble a24,
                smDouble a31,smDouble a32,smDouble a33,smDouble a34,
                smDouble a41,smDouble a42,smDouble a43,smDouble a44):
    smMatrixBased( a11, a12, a13, a14,
                 a21, a22, a23, a24,
                 a31, a32, a33, a34,
                 a41, a42, a43, a44)

        {

        }

    smMathMatrixD():smMatrixBased(  0,0,0,0,
                                0,0,0,0,
                                0,0,0,0,
                                0,0,0,0)

    {

    }

        static void makeRotMatrix( Mat4& dst, const smDouble& h, const smDouble& p, const smDouble& r )
        {
                smDouble ch, sh, cp, sp, cr, sr, srsp, crsp, dstp ;

                if ( h == 0.0f ) {
                        ch = 1.0;
                        sh = 0.0;
                }
                else {
                        smDouble hRad = smMath::degToRad( h );
                        sh = smMath::sin( hRad );
                        ch = smMath::cos( hRad );
                }

                if ( p == 0.0f ) {
                        cp = 1.0;
                        sp = 0.0;
                }
                else {
                        smDouble pRad = smMath::degToRad( p );
                        sp = smMath::sin( pRad );
                        cp = smMath::cos( pRad );
                }

                if ( r == 0.0f ) {
                        cr   = 1.0;
                        sr   = 0.0;
                        srsp = 0.0;
                        dstp = 0.0;
                        crsp = sp;
                }
                else {
                        smDouble rRad = smMath::degToRad( r ) ;
                        sr   = smMath::sin( rRad );
                        cr   = smMath::cos( rRad );
                        srsp = sr * sp;
                        crsp = cr * sp;
                        dstp = sr * cp;
                }

                dst[ 0 ][ 0 ] = (smDouble)(  ch * cr - sh * srsp );
                dst[ 0 ][ 1 ] = (smDouble)( -sh * cp );
                dst[ 0 ][ 2 ] = (smDouble)(  sr * ch + sh * crsp );
                dst[ 0 ][ 3 ] =  0;

                dst[ 1 ][ 0 ] = (smDouble)( cr * sh + srsp * ch );
                dst[ 1 ][ 1 ] = (smDouble)( ch * cp );
                dst[ 1 ][ 2 ] = (smDouble)( sr * sh - crsp * ch );
                dst[ 1 ][ 3 ] =  0;

                dst[ 2 ][ 0 ] = (smDouble)( -dstp );
                dst[ 2 ][ 1 ] = (smDouble)(  sp );
                dst[ 2 ][ 2 ] = (smDouble)(  cr * cp );
                dst[ 2 ][ 3 ] =  0;

                dst[ 3 ][ 0 ] =  0.0f;
                dst[ 3 ][ 1 ] =  0.0f;
                dst[ 3 ][ 2 ] =  0.0f;
                dst[ 3 ][ 3 ] =  1.0f;
        }

        static void makeRotMat( Mat4& dst, smAngleInDeg angleDeg, smVec3d ax )
        {
                ax.normalize();

                smDouble temp_angle =  smMath::degToRad( angleDeg )  ;
                smDouble s = static_cast<smDouble> ( sin ( temp_angle ) );
                smDouble c = static_cast<smDouble> ( cos ( temp_angle ) );
                smDouble t = 1.0f - c ;

                dst[ 0 ][ 0 ] = t * ax[ 0 ] * ax[ 0 ] + c ;
                dst[ 1 ][ 0 ] = t * ax[ 0 ] * ax[ 1 ] + s * ax[ 2 ] ;
                dst[ 2 ][ 0 ] = t * ax[ 0 ] * ax[ 2 ] - s * ax[ 1 ] ;
                dst[ 3 ][ 0 ] = 0.0f ;

                dst[ 0 ][ 1 ] = t * ax[ 1 ] * ax[ 0 ] - s * ax[ 2 ] ;
                dst[ 1 ][ 1 ] = t * ax[ 1 ] * ax[ 1 ] + c ;
                dst[ 2 ][ 1 ] = t * ax[ 1 ] * ax[ 2 ] + s * ax[ 0 ] ;
                dst[ 3 ][ 1 ] = 0.0f ;

                dst[ 0 ][ 2 ] = t * ax[ 2 ] * ax[ 0 ] + s * ax[ 1 ] ;
                dst[ 1 ][ 2 ] = t * ax[ 2 ] * ax[ 1 ] - s * ax[ 0 ] ;
                dst[ 2 ][ 2 ] = t * ax[ 2 ] * ax[ 2 ] + c ;
                dst[ 3 ][ 2 ] = 0.0f ;

                dst[ 0 ][ 3 ] = 0.0f ;
                dst[ 1 ][ 3 ] = 0.0f ;
                dst[ 2 ][ 3 ] = 0.0f ;
                dst[ 3 ][ 3 ] = 1.0f ;
        }

        /** Please note: src and dst shall not be the same.
		*       @param  m		matrix to be used in the transformation
        *       @param  src     Source vector
        *       @param  dst     Destination vector.
        */
        static void transform3x3( const Mat4& m, const smVec3d& src, smVec3d& dst )
        {
                dst[0] = m[0][0] * src[0] + m[0][1] * src[1] + m[0][2] * src[2] ;
                dst[1] = m[1][0] * src[0] + m[1][1] * src[1] + m[1][2] * src[2] ;
                dst[2] = m[2][0] * src[0] + m[2][1] * src[1] + m[2][2] * src[2] ;
        }

        static void print( Mat4 m, std::string prefix )
        {
                int i, j;

                printf( "%s\n", prefix.c_str() );
                for ( i = 0; i < 4; i++ )
                {
                        for ( j = 0; j < 4; j++ )
                        {
                                printf( "%.3f ", m[ i ][ j ] );
                        }
                        printf( "\n" );
                }
                printf( "\n" );
        }

        static void print( smVec3d v, std::string prefix )
        {
                printf( "%s\n", prefix.c_str() );
                for ( int i = 0; i < 3; i++ )
                {
                        printf( "%.3f ", v[ i ] );
                }
                printf( "\n" );
        }



        static void q2m(const smQuat& src, Mat4& dst)
        {
                // Source: Gamasutra, Rotating Objects Using Quaternions
                //
                //http://www.gamasutra.com/features/programming/19980703/quaternions_01.htm

                smDouble wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;

                // calculate coefficients
                x2 = src._v[0] + src._v[0];
                y2 = src._v[1] + src._v[1];
                z2 = src._v[2] + src._v[2];

                xx = src._v[0] * x2;
                xy = src._v[0] * y2;
                xz = src._v[0] * z2;

                yy = src._v[1] * y2;
                yz = src._v[1] * z2;
                zz = src._v[2] * z2;

                wx = src._v[3] * x2;
                wy = src._v[3] * y2;
                wz = src._v[3] * z2;

                // Note. Using original articles row/major order unlike smQuat::get(smMatrix)
                // Devrim Erdem 16.4.2006
                // Also matches Eberly p537
                dst[0][0] = 1.0f - (yy + zz);
                dst[0][1] = xy - wz;
                dst[0][2] = xz + wy;
                dst[0][3] = 0.0f;

                dst[1][0] = xy + wz;
                dst[1][1] = 1.0f - (xx + zz);
                dst[1][2] = yz - wx;
                dst[1][3] = 0.0f;

                dst[2][0] = xz - wy;
                dst[2][1] = yz + wx;
                dst[2][2] = 1.0f - (xx + yy);
                dst[2][3] = 0.0f;

                dst[3][0] = 0;
                dst[3][1] = 0;
                dst[3][2] = 0;
                dst[3][3] = 1;
        }

        static void m2q(const Mat4& src, smQuat& dst)
        {
                // Source: Gamasutra, Rotating Objects Using Quaternions
                //
                //http://www.gamasutra.com/features/programming/19980703/quaternions_01.htm
                // The article uses a column major (opengl compatible) src order.
                // Here I have changed the indeces to match row major matrices. As in
                // Eberly p 537.
                // Devrim Erdem 16.4.2006

                // smQuat uses smReal only, so we work with smReal.
                smReal  tr, s;
                smReal  tq[4];
                int    i, j, k;

                int nxt[3] = {1, 2, 0};

                tr = static_cast<smReal>( src[0][0] + src[1][1] + src[2][2] );

                // check the diagonal
                if (tr > 0.0f)
                {
                        s = (smReal) sqrt(tr + 1.0f);
                        dst._v[3] = s / 2.0f;
                        s = 0.5f / s;
                        dst._v[0] = static_cast<smReal>( (src[1][2] - src[2][1]) * s);
                        dst._v[1] = static_cast<smReal>( (src[2][0] - src[0][2]) * s);
                        dst._v[2] = static_cast<smReal>( (src[0][1] - src[1][0]) * s);

                        dst._v[0] = static_cast<smReal>( (src[2][1] - src[1][2]) * s);
                        dst._v[1] = static_cast<smReal>( (src[0][2] - src[2][0]) * s);
                        dst._v[2] = static_cast<smReal>( (src[1][0] - src[0][1]) * s);
                }
                else
                {
                        // diagonal is negative
                        i = 0;
                        if (src[1][1] > src[0][0])
                                i = 1;
                        if (src[2][2] > src[i][i])
                                i = 2;
                        j = nxt[i];
                        k = nxt[j];

                        s = (smReal) sqrt ((src[i][i] - (src[j][j] + src[k][k])) + 1.0f);

                        tq[i] = s * 0.5f;

                        if (s != 0.0f)
                                s = 0.5f / s;

                        tq[3] = static_cast<smReal>( (src[k][j] - src[j][k]) * s );
                        tq[j] = static_cast<smReal>( (src[j][i] + src[i][j]) * s );
                        tq[k] = static_cast<smReal>( (src[k][i] + src[i][k]) * s );

                        dst._v[0] = tq[0];
                        dst._v[1] = tq[1];
                        dst._v[2] = tq[2];
                        dst._v[3] = tq[3];
                }
        }


        static bool m2hpr(const Mat4& src, smVec3d& hpr)
        {
                if ( src[2][1] < 1.0 )
                {
                        if ( src[2][1] > -1.0 )
                        {
                                hpr[0] = smMath::atan2(-src[0][1],src[1][1]);
                                hpr[1] = smMath::asin(src[2][1]);
                                hpr[2] = smMath::atan2(-src[2][0],src[2][2]);
                                return true;
                        }
                        else
                        {
                                // WARNING.  Not unique.  ZA - YA = -atan(r02,r00)
                                hpr[0] = -smMath::atan2(src[0][2],src[0][0]);
                                hpr[1] = -smMath::radToDeg( smMath::sHalfPI ) ;
                                hpr[2] = 0.0f;
                                return false;
                        }
                }
                else
                {
                        // WARNING.  Not unique.  ZA + YA = atan2(r02,r00)
                        hpr[0] = smMath::atan2(src[0][2],src[0][0]);
                        hpr[1] = smMath::radToDeg( smMath::sHalfPI ) ;
                        hpr[2] = 0.0f;
                        return false;
                }
        }

        static void rotateVector( const smDouble& h, const smDouble& p, const smDouble& r, const smVec3d& src, smVec3d& dst )
        {
                Mat4 rotMat ;
                makeRotMatrix( rotMat, h, p, r );
                transform3x3( rotMat, src, dst );
        }

        void makeTransMat( const smVec3d& translation )
        {
                int i , j;

                for ( i = 0; i < 4; i++ )
                {
                        for ( j = 0; j < 4 ;j++ )
                        {
                                m_mat[ i ][ j ] = 0.0f ;
                        }
                }

                m_mat[0][0]     =       1.0f;
                m_mat[1][1]     =       1.0f;
                m_mat[2][2]     =       1.0f;
                m_mat[3][3]     =       1.0f;

                m_mat[0][3] = translation[0];
                m_mat[1][3] = translation[1];
                m_mat[2][3] = translation[2];
        }

        smMathMatrixD operator*( const smMathMatrixD& rhs ) const
        {
                smMathMatrixD result;

                for ( unsigned int j = 0; j < 4; j++ )
                {
                        for ( unsigned int i = 0; i < 4; i++ )
                        {
                                result[i][j] =  m_mat[i][0]* rhs[0][j]+
                                        m_mat[i][1]* rhs[1][j]+
                                        m_mat[i][2]* rhs[2][j]+
                                        m_mat[i][3]* rhs[3][j] ;
                        }
                }

                return result;
        }


        smVec3d   operator*( const smVec3d& rhs ) const
        {
                smVec3d result;

                result[0] = m_mat[0][0]*rhs[0]+ m_mat[0][1]*rhs[1]+ m_mat[0][2]*rhs[2];
                result[1] = m_mat[1][0]*rhs[0]+ m_mat[1][1]*rhs[1]+ m_mat[1][2]*rhs[2];
                result[2] = m_mat[2][0]*rhs[0]+ m_mat[2][1]*rhs[1]+ m_mat[2][2]*rhs[2];
        }

		smVec4d   operator*( const smVec4d& rhs ) const
		{
			smVec4d result;

			result[0] = m_mat[0][0]*rhs[0]+ m_mat[0][1]*rhs[1]+ m_mat[0][2]*rhs[2]+ m_mat[0][3]*rhs[3];
			result[1] = m_mat[1][0]*rhs[0]+ m_mat[1][1]*rhs[1]+ m_mat[1][2]*rhs[2]+ m_mat[1][3]*rhs[3];
			result[2] = m_mat[2][0]*rhs[0]+ m_mat[2][1]*rhs[1]+ m_mat[2][2]*rhs[2]+ m_mat[2][3]*rhs[3];
			result[3] = m_mat[3][0]*rhs[0]+ m_mat[3][1]*rhs[1]+ m_mat[3][2]*rhs[2]+ m_mat[3][3]*rhs[3];		
		}
};

