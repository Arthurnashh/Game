#ifdef _MSC_VER
#pragma once
#endif

#ifndef __MATRIXA_H__
#define __MATRIXA_H__

template <class Type>
class Vector 
{
protected:
	Type * _pVector;
	unsigned int _nDim;
public:
	Vector():_nDim(0), _pVector(0) {};
	~Vector() {delete [] _pVector;};

	int Init(unsigned int nDim);
	void Negative(unsigned int nDim);
	void MoveUp(unsigned int iFromIndex, unsigned int iToIndex);
	void MoveDown(unsigned int iFromIndex, unsigned int iToIndex);

	Type& operator[](unsigned int i) const {ASSERT(_nDim > i);return _pVector[i];};

	Type * GetPointer() {return _pVector;};
	unsigned int GetDim() {return _nDim;};

	const Vector<Type>& operator=(const Vector<Type>& cVector);
};

template <class Type>
int Vector<Type>::Init(unsigned int nDim)
{
	_nDim = nDim;
	delete [] _pVector;
	_pVector = new Type[_nDim]; 
  if (!_pVector)
    return MARINA_FAILURE;

	memset(_pVector, 0x0, nDim * sizeof(Type));

	return MARINA_OK;
}

template <class Type>
void Vector<Type>::Negative(unsigned int nDim)
{
	ASSERT(_nDim >= nDim);

	for(unsigned int i = 0; i < nDim; i++)
		_pVector[i] = - _pVector[i];
}

template <class Type>
void Vector<Type>::MoveUp(unsigned int iFromIndex, unsigned int iToIndex)
{
	ASSERT(iToIndex > iFromIndex);
	ASSERT(iToIndex < _nDim);

	Type fTemp = _pVector[iFromIndex];

	memmove(_pVector + iFromIndex, _pVector + iFromIndex + 1, (iToIndex - iFromIndex) * sizeof(Type));
	
	_pVector[iToIndex] = fTemp;
}

template <class Type>
void Vector<Type>::MoveDown(unsigned int iFromIndex, unsigned int iToIndex)
{
	ASSERT(iToIndex < iFromIndex);
	ASSERT(iFromIndex < _nDim);

	Type fTemp = _pVector[iFromIndex];

	memmove(_pVector + iToIndex + 1, _pVector + iToIndex, (iFromIndex - iToIndex) * sizeof(Type));
	
	_pVector[iToIndex] = fTemp;
}

template <class Type>
const Vector<Type>& Vector<Type>::operator=(const Vector<Type>& cVector)
{
	delete [] _pVector;

	_nDim = cVector._nDim;
	_pVector = new Type[_nDim];

	memcpy(_pVector, cVector._pVector, sizeof(Type) * _nDim);

	return *this;
}

template <class Type>
class Matrix  // Symetric matrix !!! positive semidefinite.
{
protected:
	Type * _pMatrix;
	unsigned int _nDim;
public:

	Matrix():_nDim(0), _pMatrix(NULL) {};
	~Matrix() {delete [] _pMatrix;};
	int Init(unsigned int nDim);

	void SetMemberValue(unsigned int nRow, unsigned int nCollumn, const Type& Value) {_pMatrix[nRow * _nDim + nCollumn] = Value;}
	void AddMemberValue(unsigned int nRow, unsigned int nCollumn, const Type& Value) {_pMatrix[nRow * _nDim + nCollumn] += Value;}
	void SwitchCoordinates(unsigned int nCoord1, unsigned int nCoord2);
	void MoveDown(unsigned int iFromCoord1, unsigned int iToCoord1);
	void MoveUp(unsigned int iFromCoord1, unsigned int iToCoord1);
	void GetColumn(Vector<marina_real>& cColumn, unsigned int iColumn, unsigned int nLength);
	void CombinateColumns(unsigned int iDestColumn, unsigned int iSourceColumn, marina_real fCoef);
    void CombineCoordinates(unsigned int iCoord1, unsigned int iCoord2, marina_real * pfCombMatrix); // Combines two coordinates according to Matrix
	void RemoveCoordinate(unsigned int iCoord);
	Type RowTimesVector(unsigned int iRow, Vector<marina_real>& cForces, unsigned int nRows);
    Type RowTimesVector(unsigned int iRow, Vector<marina_real>& cVector, unsigned int iFrom, unsigned int iTo);
	const Vector<marina_real>& MultiplyByShortVector(Vector<marina_real>& cOutput, Vector<marina_real>& cInput, unsigned int nDim);	
	const Vector<marina_real>& MultiplyByShortVectorFromTo(Vector<marina_real>& cOutput, Vector<marina_real>& cInput, unsigned int nDim,
		unsigned int iFromCoord, unsigned int iToCoord);	

	void Sqr(Matrix<Type>& cOutput, unsigned int nRows);

	Type * GetPointer() {return _pMatrix;};
	unsigned int GetDim() {return _nDim;};
	Type GetMemberValue(unsigned int nRow, unsigned int nColumn) const {return _pMatrix[nRow * _nDim + nColumn];};

	const Matrix<Type>& operator=(const Matrix<Type>& cMatrix);
};

template <class Type>
int Matrix<Type>::Init(unsigned int nDim)
{
	delete [] _pMatrix;
	_nDim = nDim;
	_pMatrix = new Type[_nDim * _nDim]; 
  if (!_pMatrix)
    return MARINA_FAILURE;
	memset(_pMatrix, 0x0, nDim * nDim * sizeof(Type));

	return MARINA_OK;
}

template <class Type>
void Matrix<Type>::SwitchCoordinates(unsigned int nCoord1, unsigned int nCoord2)
{
	ASSERT(nCoord1 < _nDim);
	ASSERT(nCoord2 < _nDim);

	// Switch rows
	Type * pTemp = new Type[_nDim];

	memcpy(pTemp, _pMatrix + nCoord1 * _nDim, _nDim * sizeof(Type));
	memcpy(_pMatrix + nCoord1 * _nDim, _pMatrix + nCoord2 * _nDim, _nDim * sizeof(Type));
	memcpy(_pMatrix + nCoord2 * _nDim, pTemp, _nDim * sizeof(Type));

	delete [] pTemp;

	// Switch columns
	for(unsigned int i = 0; i < _nDim; i++)
	{
		Type Temp = _pMatrix[i * _nDim + nCoord1];
		_pMatrix[i * _nDim + nCoord1] = _pMatrix[i * _nDim + nCoord2];
		_pMatrix[i * _nDim + nCoord2] = Temp;
	}
}

template <class Type>
void Matrix<Type>::MoveUp(unsigned int iFromCoord, unsigned int iToCoord)
{
	ASSERT(iFromCoord < iToCoord);
	ASSERT(iToCoord < _nDim);

	// move rows
	Type * pTemp = new Type[_nDim];

	memcpy(pTemp, _pMatrix + iFromCoord * _nDim, _nDim * sizeof(Type));
	memmove(_pMatrix + iFromCoord * _nDim, _pMatrix + (iFromCoord + 1) * _nDim, 
		(iToCoord - iFromCoord) * _nDim * sizeof(Type));
	memcpy(_pMatrix + iToCoord * _nDim, pTemp, _nDim * sizeof(Type));

	delete [] pTemp;

	// move columns
	pTemp = _pMatrix + iFromCoord;
	for(unsigned int i = 0; i < _nDim; i++)
	{
		Type Temp = *pTemp;
		memmove(pTemp, pTemp + 1, (iToCoord - iFromCoord) * sizeof(Type));
		pTemp[iToCoord - iFromCoord] = Temp;
		pTemp += _nDim;
	}
}

template <class Type>
void Matrix<Type>::MoveDown(unsigned int iFromCoord, unsigned int iToCoord)
{
	ASSERT(iFromCoord > iToCoord);
	ASSERT(iFromCoord < _nDim);

	// move rows
	Type * pTemp = new Type[_nDim];

	memcpy(pTemp, _pMatrix + iFromCoord * _nDim, _nDim * sizeof(Type));
	memmove(_pMatrix + (iToCoord + 1) * _nDim, _pMatrix + iToCoord * _nDim, 
		(iFromCoord - iToCoord) * _nDim * sizeof(Type));
	memcpy(_pMatrix + iToCoord * _nDim, pTemp, _nDim * sizeof(Type));

	delete [] pTemp;

	// move columns
	pTemp = _pMatrix + iToCoord;
	for(unsigned int i = 0; i < _nDim; i++)
	{
		Type Temp = pTemp[iFromCoord - iToCoord];
		memmove(pTemp + 1, pTemp, (iFromCoord - iToCoord) * sizeof(Type));
		*pTemp = Temp;
		pTemp += _nDim;
	}
}

template <class Type>
void Matrix<Type>::GetColumn(Vector<marina_real>& cColumn, unsigned int iColumn, unsigned int nLength)
{
	ASSERT(iColumn < _nDim);
	ASSERT(nLength <= _nDim);

	for(unsigned int i = 0; i < nLength; i++)
	{
		cColumn[i] = _pMatrix[i * _nDim + iColumn];
	}
}

template <class Type>
Type Matrix<Type>::RowTimesVector(unsigned int iRow, Vector<marina_real>& cForces, unsigned int nRows)
{
	Type Output = 0;
	Type * pTemp = _pMatrix + iRow * _nDim;
	for(unsigned int i = 0; i < nRows; i++)
	{
		Output += (*pTemp) * cForces[i];		
		pTemp++;
	}
	return Output;
}

template <class Type>
Type Matrix<Type>::RowTimesVector(unsigned int iRow, Vector<marina_real>& cVector, unsigned int iFrom, unsigned int iTo)
{
	Type Output = 0;
	Type * pTemp = _pMatrix + iRow * _nDim;
	for(unsigned int i = iFrom; i < iTo; i++)
	{
		Output += (*pTemp) * cVector[i];		
		pTemp++;
	}
	return Output;
}


template <class Type>
const Vector<marina_real>& Matrix<Type>::MultiplyByShortVector(Vector<marina_real>& cOutput, Vector<marina_real>& cInput, unsigned int nDim)
{
	ASSERT(cOutput.GetDim() >= nDim);
	ASSERT(cInput.GetDim() >= nDim);

	for(unsigned int i = 0; i < nDim; i++)
	{
		cOutput[i] = 0;
		Type * pTemp = _pMatrix + i * _nDim;
		for(unsigned int j = 0; j < nDim; j++)
		{
			cOutput[i] += *pTemp * cInput[j];
			pTemp++;
		}
	}

	return cOutput;
}

template <class Type>
const Vector<marina_real>& Matrix<Type>::MultiplyByShortVectorFromTo(Vector<marina_real>& cOutput, Vector<marina_real>& cInput, unsigned int nDim,
		unsigned int iFromCoord, unsigned int iToCoord)
{
	ASSERT(cOutput.GetDim() >= iToCoord);
	ASSERT(cInput.GetDim() >= nDim);

	for(unsigned int i = iFromCoord; i <= iToCoord; i++)
	{
		cOutput[i] = 0;
		Type * pTemp = _pMatrix + i * _nDim;
		for(unsigned int j = 0; j < nDim; j++)
		{
			cOutput[i] += *pTemp * cInput[j];
			pTemp++;
		}
	}

	return cOutput;
}


template <class Type>
void Matrix<Type>::Sqr(Matrix<Type>& cOutput, unsigned int nRows)
{
	ASSERT(cOutput._nDim = nRows);

	/// Just for symetric matrix
	for(unsigned int i = 0; i < nRows; i ++)
		for(unsigned int j = i; j < nRows; j++)
		{
			Type * pTempI = _pMatrix + i * _nDim;
			Type * pTempJ = _pMatrix + j * _nDim;

			Type * pTemp = cOutput._pMatrix + i * nRows + j;
			*pTemp = 0;

			for(unsigned int k = 0; k < nRows; k++)
			{
				*pTemp += (*pTempI) * (*pTempJ);
				pTempI++; pTempJ++;
			}

			cOutput._pMatrix[j * nRows + i] = *pTemp;			
		}

}

template <class Type>
void Matrix<Type>::CombinateColumns(unsigned int iDestColumn, unsigned int iSourceColumn, marina_real fCoef)
{
	ASSERT(iDestColumn < _nDim);
	ASSERT(iSourceColumn < _nDim);
	if (iDestColumn >= _nDim || iSourceColumn >= _nDim) return;

	// Use two independent pointers. The previous code indexed with
	// (iSourceColumn - iDestColumn), which underflows when the source
	// column comes before the destination and caused an access violation.
	Type * pDest = _pMatrix + iDestColumn;
	Type * pSource = _pMatrix + iSourceColumn;
	for(unsigned int iRows = 0; iRows < _nDim; iRows++)
	{
		*pDest += fCoef * (*pSource);
		pDest += _nDim;
		pSource += _nDim;
	}
}

template <class Type>
void Matrix<Type>::CombineCoordinates(unsigned int iCoord1, 
                                      unsigned int iCoord2, 
                                      marina_real * pfCombMatrix) //  must be unitary
{
    ASSERT(iCoord1 < _nDim);
	ASSERT(iCoord2 < _nDim);
    if (iCoord1 >= _nDim || iCoord2 >= _nDim || iCoord1 == iCoord2) return;
    ASSERT(IsNumericalZero(pfCombMatrix[0] * pfCombMatrix[2] + pfCombMatrix[1] * pfCombMatrix[3]));//  must be unitary
    
    // Columns. Use one pointer per coordinate: the old pTemp[iCoord2 - iCoord1]
    // underflows when iCoord2 < iCoord1 and reads out of bounds (crash in the
    // vehicle collision solver when friction axes are combined in that order).
    Type * pColumn1 = _pMatrix + iCoord1;
    Type * pColumn2 = _pMatrix + iCoord2;
    Type * pTempEnd = _pMatrix + _nDim * _nDim;
    Type In1, In2;

	for(; pColumn1 < pTempEnd; pColumn1 += _nDim, pColumn2 += _nDim)
	{
        In1 = (*pColumn1);
        In2 = (*pColumn2);
        (*pColumn1) = pfCombMatrix[0] * In1 + pfCombMatrix[1] * In2;
        (*pColumn2) = pfCombMatrix[2] * In1 + pfCombMatrix[3] * In2;
	}

    // Rows
    Type * pRow1 = _pMatrix + iCoord1 * _nDim;
    pTempEnd = pRow1 + _nDim;
    Type * pRow2 = _pMatrix + iCoord2 * _nDim;

    for(; pRow1 < pTempEnd; pRow1++,pRow2++)
	{
        In1 = (*pRow1);
        In2 = (*pRow2);

        (*pRow1) = pfCombMatrix[0] * In1 + pfCombMatrix[1] * In2;
        (*pRow2) = pfCombMatrix[2] * In1 + pfCombMatrix[3] * In2;
    }
}

template <class Type>
void Matrix<Type>::RemoveCoordinate(unsigned int iCoord)
{
	ASSERT(iCoord < _nDim);

	if (iCoord + 1 == _nDim)
		return; // trivial case

	// move rows
	memmove(_pMatrix + _nDim * iCoord, _pMatrix + _nDim * (iCoord + 1), (_nDim - iCoord - 1) * _nDim * sizeof(Type));

	// move columns
	Type * pTemp = _pMatrix + iCoord;
	for(unsigned int iRows = 0; iRows < _nDim; iRows++)
	{
		memmove(pTemp, pTemp + 1, (_nDim - iCoord - 1) * sizeof(Type));
		pTemp += _nDim;
	}
}

template <class Type>
const Matrix<Type>& Matrix<Type>::operator=(const Matrix<Type>& cMatrix)
{
	if (_nDim != cMatrix._nDim)
	{
		delete [] _pMatrix;
		_nDim = cMatrix._nDim;

		_pMatrix = new Type[_nDim * _nDim];
	}

	memcpy(_pMatrix, cMatrix._pMatrix, sizeof(Type) * _nDim * _nDim);

	return *this;
}
#endif

