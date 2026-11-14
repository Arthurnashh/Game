//----------------------------------------------------------------------
//
//  randomJames.h
//
//  Random number generator by F. James
//  adopted by Phil Linttell, James F. Hickling & Josef Pelikan
//
//  6.6.2000
//
//  Copyright (c) 2000 by Josef Pelikan, MFF UK Prague
//      http://cgg.ms.mff.cuni.cz/~pepca/
//
//----------------------------------------------------------------------

#ifndef _RANDOMJAMES_H
#define _RANDOMJAMES_H

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

class RandomJames {

protected:

    double u[97], c, cd, cm;
    int i97, j97;
    double prepMean, prepVar, norMul, norAdd;
    int prepRep;

public:

    bool ok;                                // false == error

    RandomJames ( int ij =1802, int kl =9373 );

    void reset ( int ij =1802, int kl =9373 );
        // deterministic sequence restart
    void randomize ();
        // undeterministic sequence (uses system time)
    bool test ();
        // test the random generator

    double uniformNumber ();
        // uniform random number from [0,1]
    void uniformNumbers ( double *vec, int len );
        // vector of uniform random numbers from [0,1]

    double normalNumber ( double mean, double variance, int rep =4 );
        // random number from normal (Gaussian) distribution
    void normalNumbers ( double mean, double variance, int rep, double *vec, int len );
        // vector of random numbers from normal (Gaussian) distribution

protected:

    inline void checkNormal ( double mean, double variance, int rep );

    };

//----------------------------------------------------------------------

#endif
