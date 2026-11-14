#include <stdio.h>
#define _HAS_EXCEPTIONS 0
#if defined(_MSC_VER) && (_MSC_VER < 1300 )
    #include "mtl/mtl.h"    
    #include "mtl/lu.h"
#elif defined(_MSC_VER)
    #include "mtl_vs7/mtl.h"
    #include "mtl_vs7/lu.h"
#else
    #include "mtl/mtl.h"
    #include "mtl/lu.h"
#endif

using namespace mtl ;


typedef matrix<float, rectangle<>, dense<>, row_major>::type MatrixMTL;
typedef dense1D<float> VectorMTL;

MatrixMTL * pLU = NULL;
dense1D<int> * pvector = NULL;
unsigned int nN = 0; 

void LUSoln(const MatrixMTL &A, const VectorMTL &b, VectorMTL &x)
{
  // create LU decomposition
  //MatrixMTL LU(A.nrows(), A.ncols());
  //dense1D<int> pvector(A.nrows());

  delete pLU;
  pLU = new MatrixMTL(A.nrows(), A.ncols());

  delete pvector;
  pvector = new dense1D<int>(A.nrows());

  if (pLU != NULL && pvector != NULL)
  {
    copy(A, *pLU);
    lu_factorize(*pLU, *pvector);
        
    // solve
    lu_solve(*pLU, *pvector, b, x);
  }
}

void LUSolnOld( const VectorMTL &b, VectorMTL &x)
{
  // create LU decomposition
  //MatrixMTL LU(A.nrows(), A.ncols());
  
  if (pLU != NULL && pvector != NULL)
  {    
    // solve
    lu_solve(*pLU, *pvector, b, x);
  }
}

void SolveNxN(unsigned int n, unsigned int lda, const float * pMatrix, float * pVector)
{
	MatrixMTL cMatrix(n,n);
	VectorMTL cIn(n), cOut(n);

  nN = n;
  unsigned int iX = 0;	
  for(; iX < n; iX ++)
	{
		cIn[iX] = pVector[iX];
		for(unsigned int iY = 0; iY < n; iY++)
			cMatrix(iX,iY) = pMatrix[iX * lda + iY];
	}
	
	LUSoln(cMatrix, cIn, cOut);
	
	for(iX = 0; iX < n; iX ++)
	{
		pVector[iX] = cOut[iX];
	}
}

void SolveNxNOld(float * pVector)
{
    VectorMTL cIn(nN), cOut(nN);
    unsigned int iX = 0;
    for(; iX < nN; iX ++)
	{
		cIn[iX] = pVector[iX];
    }

    LUSolnOld(cIn, cOut);

    for(iX = 0; iX < nN; iX ++)
	{
		pVector[iX] = cOut[iX];
	}
}

void SolvingDone()
{
    delete pLU;
    pLU = NULL;
    delete pvector;
    pvector = NULL;
}
