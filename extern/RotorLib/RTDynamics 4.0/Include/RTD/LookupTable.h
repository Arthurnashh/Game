/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RTD/Common.h"

namespace RTD
{

class RTDYNAMICS_EXPORT LookUpTable
{
public:
	LookUpTable();

	bool	readFromString( const char* content , std::string& error );

	bool	lookUp( double x , double y , double& result ) const;

	void	addColumn( double columnVal );
	void	addRow( double rowVal );
	
	void	setTableEntry( unsigned int row , unsigned int column, const double& val );
	bool	getTableEntry( unsigned int row , unsigned int column, double& val );

	unsigned int getNumColumns() const;
	unsigned int getNumRows() const;

	smDouble	getColumn( unsigned int col ) const;
	smDouble	getRow( unsigned int row ) const;

	bool		checkLimits( smDouble left , smDouble right , smDouble top , smDouble bottom  );

	void		clear();

	bool		getFirstLocalExtremeOfColumn( unsigned int column , smReal from, smReal to , bool searchLocalMaxima , int& result );

	std::string toString( ) const;

	std::vector<double>	getXAxis( ) const { return m_xAxis; }
	std::vector<double>	getYAxis( ) const { return m_yAxis; }

protected:
	std::vector<double>	m_xAxis;
	std::vector<double>	m_yAxis;	
	std::vector< std::vector< double> >	m_values;

	
	void findClosestRows( smDouble pnt , int& before, int& after  );

};

}