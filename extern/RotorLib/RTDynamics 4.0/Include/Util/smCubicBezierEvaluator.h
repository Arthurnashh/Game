/*
(c) Copyright 2004, MachSim
All Rights Reserved.

Product:		DinaPhysics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of MachSim
*/
#pragma once

#include <Util/smCubicBezier.h>

class smCubicBezierEvaluator
{
public:
	/** Result class is used by the evaluate methods to return the
	*	results of the evaluation.
	*/
	class Result
	{
	public:
		smCBVec3	m_point ;	// Position
		smCBVec3	m_speed ;	// 1st derivative
		smCBVec3	m_rate ;	// 2nd derivative
	};

	typedef	std::vector<Result>			ResultsList ;
	typedef	ResultsList::iterator		ResultsListIter ;
	typedef	ResultsList::const_iterator	ResultsListConstIter ;

protected:
	smCubicBezier&	m_curve ;

	//
	// State variables used for evaluation
	//
	// m_x is the total length evaluated on the current segment
	smCBReal	m_x ;
	
	// m_tx is the total length evaluated on the curve
	smCBReal	m_tx ;

	// m_s is current active segment of the curve
	size_t		m_s ;

	// m_sP is current (last) segment parameter(t) found by the
	// evaluator update method.
	smCBReal	m_sP ;

	bool		m_dbgPrint ;

	// Reparameterization lookup table for each segment
public: //@tmp
	std::vector<smGraph2D>	m_reparamLookUp ;

	smCBReal	m_epsilon ;

public:
	/**	Construct a new evaluator which uses the given curve.	*/
	smCubicBezierEvaluator( smCubicBezier& curve, bool dbgPrint = false ) ;

	void reset() ;

	const size_t& getCurSegment() const
	{ return m_s ; }

	void setCurSegment(const size_t& s)
	{ 
		smValidateAndWarn( s < m_curve.getNoSegments(), "s < m_curve.getNoSegments()", "smCubicBezierEvaluator::setCurSegment" );
		m_s = s ; 	
	}

	const smCBReal& getCurSegmentParam() const
	{ return m_sP ; }

	/**	saveErrorFunc can print the error func for the given s point
	*	on the curve. The results file can be used plot s vs F to see
	*   the root of the F function.
	*/
	void saveErrorFunc(std::string fileName, smCBReal s ) ;

	

	/** Bake would calculate the animation of object moving on the curve at the given "dx"
	*	interval and when requested with constant speed. The results are stored in a Results
	*	vector.
	*
	*   The results vector is cleared by the bake method before inserting new results.
	*/
	void bake( ResultsList& results, bool constantSpeed, const smCBReal& dx = 1.0 ) ;

	/** bakeToFile uses bake and stores the resulting points to a file. 
	*	See bake() for parameters.
	*
	*	The resulting file has the extension .bab which stands for baked bezier.
	*	This file includes 9 columns per line. The first 3 columns are positions
	*	in x,y,z axes, the next 3 are speed at that position and the last 3 are 
	*	accelerations.
	*
	*	@param	fileName	filename without any extensions. .bab string will be
	*						appended.
	*/
	void bakeToFile( std::string fileName, bool constantSpeed, const smCBReal& dx = 1.0 ) ;

	void evaluateWithDeltaX( const smCBReal& dx, bool constantSpeed, Result& res, bool& reachedCurveEnd ) ;
	void evaluate( smCBReal v, smCBReal dt, bool constantSpeed, Result& res, bool& reachedEndOfSeg ) ;
	void evaluate( smCBReal s, bool constantSpeed, Result& res, bool& reachedEndOfSeg ) ;
	
	/** getArcLength get the arc length either from the internal fast cache of this
	*	evaluator or directly from the curve.
	*/
	smCBReal getArcLength(	smCBReal t, 
							size_t segId, 
							bool fromCache = true ) ;

	/**	findConstantSpeedT does the arc length reparameterization for the given t. A new t
	*	value is returned which guarantees a constant speed motion on the curve.
	*/
	bool findConstantSpeedT(smCBReal tIn, 
							smCBReal& tOut,
							size_t segId, 
							smCBReal s, 							
							size_t maxLoops = 200, 
							bool newton = true ) ;

	std::vector<smGraph2D> computeReparamLookUpTable( size_t subdivisions ) ;
};
