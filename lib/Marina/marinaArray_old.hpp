#ifdef _MSC_VER
#pragma once
#endif

#ifndef __MARINAARRAY_OLD_HPP__
#define __MARINAARRAY_OLD_HPP__

#include <Es/Containers/array.hpp>

template <class Type>
class AutoArrayWithMoves : public AutoArray<Type>
{
public:
    typedef AutoArray<Type> base;
    typedef ConstructTraits<Type> CTraits;
    void MoveDown(unsigned int iFromIndex, unsigned int iToIndex);
    void MoveUp(unsigned int iFromIndex, unsigned int iToIndex);
};

template <class Type>
void AutoArrayWithMoves<Type>::MoveDown(unsigned int iFromIndex, unsigned int iToIndex)
{
	Assert(iFromIndex > iToIndex);
	Assert(iFromIndex < (unsigned int)_n);

	Type Temp = base::_data[iFromIndex];
    CTraits::InsertData(base::_data + iToIndex, iFromIndex - iToIndex + 1);	
    CTraits::CopyConstruct(base::_data[iToIndex], Temp);

    //memmove(_data + iToIndex + 1, _data + iToIndex, (iFromIndex - iToIndex) * sizeof(Type));
	//_data[iToIndex] = Temp;
}

template <class Type>
void AutoArrayWithMoves<Type>::MoveUp(unsigned int iFromIndex, unsigned int iToIndex)
{
	Assert(iFromIndex < iToIndex);
	Assert(iToIndex < (unsigned int)_n);

	Type Temp = base::_data[iFromIndex];
    CTraits::Destruct(base::_data[iFromIndex]);
    CTraits::DeleteData(base::_data + iFromIndex, iToIndex - iFromIndex + 1, 1);
    CTraits::CopyConstruct(base::_data[iToIndex], Temp);
    
	//memmove(_data + iFromIndex, _data + iFromIndex + 1, (iToIndex - iFromIndex)  * sizeof(Type));
	//_data[iToIndex] = Temp;
}

#endif

