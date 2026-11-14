/*
(c) Copyright 2004, MachSim
All Rights Reserved.

Product:		DinaPhysics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of MachSim
*/
#pragma once

//
// operators
//
// Do not use references since they are not good with hardcoded numbers.

#define SMINTEGRALTYPE_COMMON_CTORS_OPERATORS_IMPL(theType) \
theType theType::operator*(const int value )\
{ \
/*	s_validatorMngr->validate(this, smReal(value)) ;*/\
    \
    theType temp ;	\
    temp.m_value =	m_value * smReal(value) ;\
    return temp ;\
}\
\
theType theType::operator*(const int value ) const\
{ \
/*	s_validatorMngr->validate(this, smReal(value)) ;*/\
    \
    theType temp ;	\
    temp.m_value =	m_value * smReal(value) ;\
    return temp ;\
}\
\
theType theType::operator*(const smReal value )\
{ \
/*	s_validatorMngr->validate(this, value) ;*/\
    \
    theType temp ;	\
    temp.m_value =	m_value * smReal(value) ;\
    return temp ;\
}\
\
theType theType::operator*(const smReal value ) const\
{ \
/*	s_validatorMngr->validate(this, smReal(value)) ;*/\
    \
    theType temp ;	\
    temp.m_value =	m_value * smReal(value) ;\
    return temp ;\
}\
\
theType theType::operator*(const theType &value )\
{ \
/*	s_validatorMngr->validate(this, value) ;*/\
    \
    theType temp ;	\
    temp.m_value =	m_value * smReal(value) ;\
    return temp ;\
}\
\
theType theType::operator*(const theType &value ) const\
{ \
/*	s_validatorMngr->validate(this, value) ;*/\
    \
    theType temp ;	\
    temp.m_value =	m_value * smReal(value) ;\
    return temp ;\
}\
\
\
\
theType theType::operator+(const int value )\
{ \
/*	s_validatorMngr->validate(this, value) ;*/\
    \
    theType temp ;	\
    temp.m_value =	m_value + smReal(value) ;\
    return temp ;\
}\
\
theType theType::operator+(const int value ) const\
{ \
/*	s_validatorMngr->validate(this, value) ;*/\
    \
    theType temp ;	\
    temp.m_value =	m_value + smReal(value) ;\
    return temp ;\
}\
\
theType theType::operator+(const smReal value )\
{ \
/*    s_validatorMngr->validate(this, value) ;*/\
    \
    theType temp ;	\
    temp.m_value =	m_value + smReal(value) ;\
    return temp ;\
}\
\
theType theType::operator+(const smReal value ) const\
{ \
    /*    s_validatorMngr->validate(this, value) ;*/\
    \
    theType temp ;	\
    temp.m_value =	m_value + smReal(value) ;\
    return temp ;\
}\
\
theType theType::operator+(const theType &value )\
{ \
    /*s_validatorMngr->validate(this, dynamic_cast<const smIntegralType&>(value), smValidator::ADD) ;*/\
    \
    theType temp ;	\
    temp.m_value =	m_value + smReal(value) ;\
    return temp ;\
}\
\
theType theType::operator+(const theType &value ) const\
{ \
    /*s_validatorMngr->validate(this, dynamic_cast<const smIntegralType&>(value), smValidator::ADD) ;*/\
    \
    theType temp ;	\
    temp.m_value =	m_value + smReal(value) ;\
    return temp ;\
}\
\
\
\
theType theType::operator/(const int value )\
{ \
    /*    s_validatorMngr->validate(this, value) ;*/\
    \
    theType temp ;	\
    temp.m_value =	m_value / smReal(value) ;\
    return temp ;\
}\
\
theType theType::operator/(const int value ) const\
{ \
    /*    s_validatorMngr->validate(this, value) ;*/\
    \
    theType temp ;	\
    temp.m_value =	m_value / smReal(value) ;\
    return temp ;\
}\
\
theType theType::operator/(const smReal value )\
{ \
    /*    s_validatorMngr->validate(this, value) ;*/\
    \
    theType temp ;	\
    temp.m_value =	m_value / smReal(value) ;\
    return temp ;\
}	\
\
theType theType::operator/(const smReal value )	const\
{ \
    /*    s_validatorMngr->validate(this, value) ;*/\
    \
    theType temp ;	\
    temp.m_value =	m_value / smReal(value) ;\
    return temp ;\
}	\
\
theType theType::operator/(const theType &value )\
{ \
    /*    s_validatorMngr->validate(this, value) ;*/\
    \
    theType temp ;	\
    temp.m_value =	m_value / smReal(value) ;\
    return temp ;\
}\
\
theType theType::operator/(const theType &value ) const\
{ \
    /*    s_validatorMngr->validate(this, value) ;*/\
    \
    theType temp ;	\
    temp.m_value =	m_value / smReal(value) ;\
    return temp ;\
}\
\
\
\
theType theType::operator-(const int value )\
{ \
    /*    s_validatorMngr->validate(this, value) ;*/\
    \
    theType temp ;	\
    temp.m_value =	m_value - smReal(value) ;\
    return temp ;\
}\
\
theType theType::operator-(const int value ) const\
{ \
    /*    s_validatorMngr->validate(this, value) ;*/\
    \
    theType temp ;	\
    temp.m_value =	m_value - smReal(value) ;\
    return temp ;\
}\
\
theType theType::operator-(const smReal value )\
{ \
    /*    s_validatorMngr->validate(this, value) ;*/\
    \
    theType temp ;	\
    temp.m_value =	m_value - smReal(value) ;\
    return temp ;\
}	\
\
theType theType::operator-(const smReal value )	const\
{ \
/*	s_validatorMngr->validate(this, value) ;*/\
    \
    theType temp ;	\
    temp.m_value =	m_value - smReal(value) ;\
    return temp ;\
}	\
\
theType theType::operator-(const theType &value )\
{ \
    /*s_validatorMngr->validate(this, dynamic_cast<const smIntegralType&>(value), smValidator::SUB) ;*/\
    \
    theType temp ;	\
    temp.m_value =	m_value - smReal(value) ;\
    return temp ;\
}\
\
theType theType::operator-(const theType &value ) const\
{ \
    /*s_validatorMngr->validate(this, dynamic_cast<const smIntegralType&>(value), smValidator::SUB) ;*/\
    \
    theType temp ;	\
    temp.m_value =	m_value - smReal(value) ;\
    return temp ;\
}


#define SMINTEGRALTYPE_COMMON_CTORS_OPERATORS_DECL(theType) \
theType operator*(const int value ) ;\
theType operator*(const int value ) const ;\
theType operator*(const smReal value ) ;\
theType operator*(const smReal value ) const ;\
theType operator*(const theType &value ) ;\
theType operator*(const theType &value ) const ;\
theType operator+(const int value ) ;\
theType operator+(const int value ) const ;\
theType operator+(const smReal value ) ;\
theType operator+(const smReal value ) const ;\
theType operator+(const theType &value ) ;\
theType operator+(const theType &value ) const ;\
theType operator/(const int value ) ;\
theType operator/(const int value ) const ;\
theType operator/(const smReal value ) ;\
theType operator/(const smReal value )	const ;\
theType operator/(const theType &value ) ;\
theType operator/(const theType &value ) const ;\
theType operator-(const int value ) ;\
theType operator-(const int value ) const ;\
theType operator-(const smReal value ) ;\
theType operator-(const smReal value )	const ;\
theType operator-(const theType &value ) ;\
theType operator-(const theType &value ) const ;
