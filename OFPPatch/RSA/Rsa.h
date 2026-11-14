// rsa.h

#ifndef RSA_H
#define RSA_H

extern "C" {

#include "mpi.h"

typedef struct
{
    MPI n;	    /* modulus */
    MPI e;	    /* exponent */
} RSA_public_key;


typedef struct
{
    MPI n;	    /* public modulus */
    MPI e;	    /* public exponent */
    MPI d;	    /* exponent */
    MPI p;	    /* prime  p. */
    MPI q;	    /* prime  q. */
    MPI u;	    /* inverse of p mod q. */
} RSA_secret_key;

void publ(MPI output, MPI input, RSA_public_key *pkey);
void secr(MPI output, MPI input, RSA_secret_key *skey);
void generate(RSA_secret_key *sk, unsigned nbits);

}

#endif // RSA_H
