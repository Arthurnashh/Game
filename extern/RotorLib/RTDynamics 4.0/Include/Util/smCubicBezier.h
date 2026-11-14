/*
(c) Copyright 2004, MachSim
All Rights Reserved.

Product:		DinaPhysics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of MachSim
*/
#pragma once

#include <algorithm>
#include <numeric>
#include <Physics/smVecTypes.h>
#include <Physics/smGraph2D.h>

#define NEWTON	1

typedef smReal	smCBReal ;
typedef smVec3r	smCBVec3 ;

class smCubicBezier
{
public:
	typedef	std::vector<smCBVec3>		PointList ;
	typedef	PointList::iterator			PointListIter ;
	typedef	PointList::const_iterator	PointListConstIter ;
	typedef	std::vector<smCBReal>		SegmentLengthsList ;

protected:
	PointList			m_pointList ;
	SegmentLengthsList	m_segLengths ;	
	smCBReal			m_totalLength ;

	bool				m_dbgPrint ;

	public:
	// The subdivisions used for length calculation.
	size_t				m_lengthSubDivs ;

	public:
	std::vector<smGraph2D*>	m_arcLengthCache ;


public:
	/** Contructs a new cubic bezier out of the given point list 
	*	and automatically calculates certain characteristic values of the
	*	curve such as its segment length list.
	*
	*	To create a closed curve, the point list shall be given accordingly.
	*	Please refer to close method of this class for details.
	*/
	smCubicBezier( const PointList& pointList, size_t subDivisions, bool debugPrints = false ) ;

	/** Returns the total length of the curve, which is the sum of lengths of 
	*	segments.
	*/
	smCBReal	getTotalLength()
	{ return m_totalLength ; }

	SegmentLengthsList	getSegLengths()
	{ return m_segLengths ; }

	smCBReal	getSegLength( size_t segId )
	{ return m_segLengths[segId] ; }

	size_t	getNoSegments()
	{ return m_segLengths.size() ; }

	const PointList&	getPointList() const
	{ return m_pointList ; }

	size_t	getSubdivisions()
	{ return m_lengthSubDivs ; }

	/**	Analytically measure the length of each segment
	*	by using speed at each point.
	*/
	SegmentLengthsList computeLengthsAnalytic( ) ;

	/**	Numerically compute the length of each segment
	*	simply by measuring distance between t and t+dt.
	*
	*	@param	step	The granularity of the measurement. Smaller dt values
	*					result more precise results.
	*/
	SegmentLengthsList computeLengthsNumeric( smCBReal step = 0.01f ) ;

	/**	Analytically calculate the speed of the specified point of the curve.	
	*	@param	t				Control variable in range [0,1]
	*	@param	segId			Segment selector
	*/
	smCBVec3 getSpeedVec( const smCBReal& t, const size_t& segId ) ;

	/** Analytically calculate the speed of the specified point of the curve.
	*	@param	t				Control variable in range [0,1]
	*	@param	p1,p2,p3	Points relative to p0. They must be computed as:
	*						 p1' = p1 - p0 
	*						 p2' = p2 - p0 
	*						 p3' = p3 - p0 
	*/
	static smCBVec3 getSpeedVec( const smCBReal& t, const smCBVec3& p1, const smCBVec3& p2, const smCBVec3& p3 ) ;

	/**	Analytically calculate the TangentRate of the specified point of the curve.	
	*	@param	t				Control variable in range [0,1]
	*	@param	segId			Segment selector
	*/
	smCBVec3 getTangentRateVec( const smCBReal& t, const size_t& segId ) ;

	/** Analytically calculate the TangentRate of the specified point of the curve.
	*	@param	t				Control variable in range [0,1]
	*	@param	p1,p2,p3	Points relative to p0. They must be computed as:
	*						 p1' = p1 - p0 
	*						 p2' = p2 - p0 
	*						 p3' = p3 - p0 
	*/
	static smCBVec3 getTangentRateVec( const smCBReal& t, const smCBVec3& p1, const smCBVec3& p2, const smCBVec3& p3 ) ;


	/** Evaluates a bezier curve segment, defined by the specified points at the
	*	given t.
	*
	*	@param	t				Control variable in range [0,1]
	*	@param	segID			Id of the segment to be evaluated
	*/
	smCBVec3 evaluate( smCBReal t, size_t segId ) ;
	
	/** Evaluates a bezier curve segment, defined by the specified points at the
	*	given t.
	*
	*	@param	t				Control variable in range [0,1]
	*	@param	p0,p1,p2,p3		Control points as 3d vectors
	*/
	static smCBVec3 evaluate( smCBReal t, smCBVec3 p0, smCBVec3 p1, smCBVec3 p2, smCBVec3 p3 ) ;

	
	/**	Computes the length of the arc on the given segment.
	*	@param	segId			segment id
	*	@param	t				bezier parameter
	*/
	smCBReal computeArcLength( size_t segId, smCBReal t ) ;

	/** The incCurveParam can be used to increment a t safely.
	*	This method will also handle the segId and if necessary change
	*	the t and segId to use the previous or next segment.
	*/
	void	incCurveParam( size_t& segId, smCBReal& t, const smCBReal& dt ) ;


	/** Calculates and returns the center of this curve. Control points are
	*	excluded.
	*/
	smVec3r getCenter();

	/** Find the bounds of the curve.
	*/
	void getBounds( smVec3r& minXYZ, smVec3r& maxXYZ ) ;

	/** Adds two new CPs and one P to the given list to make it a closed
	*	(hence looping) curve. Doesn't check if the curve is already closed
	*	or not.
	*/
	static bool close(PointList& pList);

	void print( smPrinter* pr, std::string prefix ) const ;
	void save( const std::string& filename ) const ;
};
