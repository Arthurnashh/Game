//----------------------------------------------------------------------
//
//  randomJames.cpp
//
//  Random number generator by F. James
//  adopted by Phil Linttell, James F. Hickling & Josef Pelikan
//
//  12.2.2003
//
//  Copyright (c) 2000-2003 by Josef Pelikan, MFF UK Prague
//      http://cgg.ms.mff.cuni.cz/~pepca/
//
//----------------------------------------------------------------------

#include <El/elementpch.hpp>
#include <math.h>
#include <time.h>
#include "randomJames.h"

/************************************************************************
 This random number generator originally appeared in "Toward a Universal
 Random Number Generator" by George Marsaglia and Arif Zaman.
 Florida State University Report: FSU-SCRI-87-50 (1987)

 It was later modified by F. James and published in "A Review of Pseudo-
 random Number Generators"

 Converted from FORTRAN to C by Phil Linttell, James F. Hickling
 Management Consultants Ltd, Aug. 14, 1989.

 THIS IS THE BEST KNOWN RANDOM NUMBER GENERATOR AVAILABLE.
       (However, a newly discovered technique can yield
         a period of 10^600. But that is still in the development stage.)

 It passes ALL of the tests for random number generators and has a period
   of 2^144, is completely portable (gives bit identical results on all
   machines with at least 24-bit mantissas in the floating point
   representation).

 The algorithm is a combination of a Fibonacci sequence (with lags of 97
   and 33, and operation "subtraction plus one, modulo one") and an
   "arithmetic sequence" (using subtraction).

 On a Vax 11/780, this random number generator can produce a number in
    13 microseconds.
************************************************************************/

bool RandomJames::test ()
    // test the random generator
{
    reset(1802,9373);
    if ( !ok ) return false;
    double temp[1000];
    int i;
    for ( i = 0; i++ < 20; ) {
        uniformNumbers(temp,1000);
        if ( !ok ) return false;
        }
    uniformNumbers(temp,6);
    for ( i = 0; i < 6; i++ ) temp[i] = temp[i] * 4096.0 * 4096.0 + 0.5;
    return( (int)temp[0] ==  6533892 &&
            (int)temp[1] == 14220222 &&
            (int)temp[2] ==  7275067 &&
            (int)temp[3] ==  6172232 &&
            (int)temp[4] ==  8354498 &&
            (int)temp[5] == 10633180 );
}

/************************************************************************
 This is the initialization routine for the random number generator
 NOTE: The seed variables can have values between:    0 <= IJ <= 31328
                                                      0 <= KL <= 30081
 The random number sequences created by these two seeds are of sufficient
 length to complete an entire calculation with. For example, if several
 different groups are working on different parts of the same calculation,
 each group could be assigned its own IJ seed. This would leave each group
 with 30000 choices for the second seed. That is to say, this random
 number generator can create 900 million different subsequences -- with
 each subsequence having a length of approximately 10^30.

 Use IJ = 1802 & KL = 9373 to test the random number generator. The
 subroutine uniformNumber() should be used to generate 20000 random numbers.
 Then display the next six random numbers generated multiplied by 4096*4096
 If the random number generator is working properly, the random numbers
 should be:
           6533892.0  14220222.0   7275067.0
           6172232.0   8354498.0  10633180.0
************************************************************************/

void RandomJames::reset ( int ij, int kl )
    // deterministic sequence restart
{
    double s, t;
    int i, j, k, l, m;
    int ii, jj;

    if ( ij < 0 || ij > 31328 || kl < 0 || kl > 30081 ) {
        ok = false;
        return;
        }
    ok = true;

    i = (int)fmod(ij/177.0, 177.0) + 2;
    j = (int)fmod(ij      , 177.0) + 2;
    k = (int)fmod(kl/169.0, 178.0) + 1;
    l = (int)fmod(kl      , 169.0);

    for ( ii = 0; ii < 97; ii++ ) {
        s = 0.0;
        t = 0.5;
        for ( jj = 0; jj < 24; jj++ ) {
            m = (int)fmod( fmod(i*j,179.0)*k , 179.0 );
            i = j;
            j = k;
            k = m;
            l = (int)fmod( 53.0*l+1.0 , 169.0 );
            if ( fmod(l*m,64.0) >= 32 ) s = s + t;
            t *= 0.5;
            }
        u[ii] = s;
        }

    c  =   362436.0 / 16777216.0;
    cd =  7654321.0 / 16777216.0;
    cm = 16777213.0 / 16777216.0;

    i97 = 96;
    j97 = 32;
}

RandomJames::RandomJames ( int ij, int kl )
{
    reset(ij,kl);
    prepMean = 0.0;
    prepVar = -1.0;
    prepRep = 0;
}

double RandomJames::uniformNumber ()
    // uniform random number from [0,1]
{
    if ( !ok ) return 0.0;
    double uni = u[i97] - u[j97];
    if ( uni < 0.0 ) uni += 1.0;
    u[i97] = uni;
    if ( --i97 < 0 ) i97 = 96;
    if ( --j97 < 0 ) j97 = 96;
    if ( (c -= cd) < 0.0 ) c += cm;
    if ( (uni -= c) < 0.0 ) uni += 1.0;
    return uni;
}

void RandomJames::uniformNumbers ( double *vec, int len )
    // vector of uniform random numbers from [0,1]
{
    if ( !vec ) return;
    while ( len-- )
        *vec++ = uniformNumber();
}

inline void RandomJames::checkNormal ( double mean, double variance, int rep )
{
    if ( rep != prepRep || variance != prepVar || mean != prepMean ) {
        prepRep = rep;
        prepMean = mean;
        prepVar = variance;
        norMul = variance * sqrt( 12.0 / rep );
        norAdd = mean - rep * 0.5 * norMul;
        }
}

double RandomJames::normalNumber ( double mean, double variance, int rep )
    // random number from normal (Gaussian) distribution
{
    checkNormal(mean,variance,rep);
    int i;
    double sum = 0.0;
    for ( i = 0; i++ < rep; ) sum += uniformNumber();
    return( sum * norMul + norAdd );
}

void RandomJames::normalNumbers ( double mean, double variance, int rep, double *vec, int len )
    // vector of random numbers from normal (Gaussian) distribution
{
    if ( !vec ) return;
    while ( len-- )
        *vec++ = normalNumber(mean,variance,rep);
}

/************************************************************************
  I use the following procedure in TC to generate seeds:

  The sow() procedure calculates two seeds for use with the random number
  generator from the system clock.  I decided how to do this myself, and
  I am sure that there must be better ways to select seeds; hopefully,
  however, this is good enough.  The first seed is calculated from the values
  for second, minute, hour, and year-day; weighted with the second most
  significant and year-day least significant.  The second seed weights the
  values in reverse.
************************************************************************/

void RandomJames::randomize ()
    // undeterministic sequence (uses system time)
{
    struct tm *tm_now;
    double s_sig, s_insig, maxs_sig, maxs_insig;
    time_t secs_now;
    int s, m, h, d, s1, s2;

    time(&secs_now);
    tm_now = localtime(&secs_now);

    s = tm_now->tm_sec + 1;
    m = tm_now->tm_min + 1;
    h = tm_now->tm_hour + 1;
    d = tm_now->tm_yday + 1;

    maxs_sig   = 60.0 + 60.0/60.0 + 24.0/60.0/60.0 + 366.0/24.0/60.0/60.0;
    maxs_insig = 60.0 + 60.0*60.0 + 24.0*60.0*60.0 + 366.0*24.0*60.0*60.0;

    s_sig      = s + m/60.0 + h/60.0/60.0 + d/24.0/60.0/60.0;
    s_insig    = s + m*60.0 + h*60.0*60.0 + d*24.0*60.0*60.0;

    s1 = (int)(s_sig   / maxs_sig   * 31328.0);
    s2 = (int)(s_insig / maxs_insig * 30081.0);

    reset(s1,s2);
}
