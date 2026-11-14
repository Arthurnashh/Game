#ifdef _MSC_VER
#pragma once
#endif

#ifndef __DANTZIG_OLD_H__
#define __DANTZIG_OLD_H__

#include "marina_old.hpp"

#define TOLERANCE 0.01f
/** 
* A - matrix zones.
*/
typedef enum {
	EMZ_CLAMPED = 0, //< Clmaped forces
	EMZ_NOTCLAMPED = 1, //<  NotClamped normal forces
	EMZ_FRICTIONNOTCLAMPED = 2,//<  NotClamped friction forces
	EMZ_SOLVING = 3,   //<  Currently solving force
	EMZ_UNSOLVED = 4,//<  Buffer with forces not yet solved
	EMZ_FRICTIONFROMNOTCLAMPED = 5,//<  Friction forces belonging to NotClamped normal forces
    EMZ_FRICTIONNOTCLAMPED2 = 6,
	EMZ_NUMBER = 7
} EMATRIXAZONES;

/* // Not used at the moment
typedef enum {
	EMA_NONE,
	EMA_FRICTIONLESS,
	EMA_FRICTION,
	EMA_FRICTIONDIAGONAL,
	EMA_FRICTIONLESSDIAGONAL
} EMATRIXATYPE;*/ 

/** 
* Types of forces.
*/
typedef enum {
	EFT_DYNAMIC = 0, //< not used at the moment
	EFT_STATIC = 1, //< Normal force
	EFT_FRICTION = 2 //< Friction force
} EFORCETYPE;

/** 
* A Matrix coordinate index description.
* A Matrix coordinate indexes correspond to forces(columns) and acceleration(rows). Typically force and acceleration corresponding
* to one coordinate index have the same direction. Only in a situation when a normal force is clamped and a friction forces are not
* clamped, the direction of normal force is combination of its direction and direction of friction.
* The structure _TCOLUMNHEADER collects information collected with coordinate index. The order of forces and accelerations in A-Matrix is
* changed during algorithm iterations.
*/
typedef struct _TCOLUMNHEADER {
	EFORCETYPE eForceType;  //< Force type.
	unsigned int iDirIndex; //< Index of force direction corresponding to the dir field in M_ContactPoint. (0 - normal, 1,2 - tangential)
	unsigned int iMatrixAIndex;  //< Actual index in matrix.
	unsigned int iSourceMatrixAIndex; //< Index in source matrix.

	unsigned int iContactPointID; //< Index of contact point.
	unsigned int iContactPedestalID;//< Index of contact pedestal.

	unsigned int iNotUseInGlobalCycle; //< Do not use force in the cycle with current number
	unsigned int iLastChangeLocalCycle; 
	_TCOLUMNHEADER * pBrotherForcesHeaders[2]; /*< If iDirIndex == 0, the brother forces are friction forces.
											    * In the other cases the first brother force is a normal force and the second is 
											    * the complementar friction force.
                                                */
/*	Vector3 cNotClampedDir;*/           /*< Just first two coordinates from vector are used. 
									   * If the friction (in the contact point connected with this coordinate)is NotClamped, cNotClampedDir 
                                       * is direction of the friction in a coordinate system composed by 1 and 2 direction mentioned in the dir 
                                       * member of the corresponding M_ContactPoint structure.
                                       * Otherwise MUST BE zero vector.									   
                                       */
    unsigned int iNotClampedIndex;

    Vector3 cStartDir;                /*< Valid just for friction forces. If cStartDir != VZero, cStartDir is a prefered direction for the friction force.
                                       * It is not garranted that friction force will be allways in this direction at the algorithm's end.
                                       */
	marina_real fSlidingFrictionCoef; //< Sliding friction coeficient (0..1)
    BOOL bDeadMouse;                  // If TRUE behave like column does not exist.   
} TCOLUMNHEADER, *PTCOLUMNHEADER;

TypeIsSimple(TCOLUMNHEADER);
TypeIsSimple(PTCOLUMNHEADER);

/**
 * Dantzig Algorithm
 *
 */
class DantzigAlgorithm 
{
protected:
	
	//unsigned int _nCollisions;
	
    unsigned int _nNumberOfForces;   //< Number of forces (A-Matrix dimension).
    unsigned int _nNumberOfBodies;   //< Number of forces (A-Matrix dimension).
	unsigned int _pnLastIndexInZone[EMZ_NUMBER]; //< 1 - based indexes. 

	unsigned int _iLocalCycle;       //< Local cycle number. Update: after each nonzero step in DriveToZero
	unsigned int _iGlobalCycle;		 //< Global cycle number. Update: after successfull run of DriveToZero.
	
	BOOL _bAllowDynamicContact; //< Not used at the moment.
	
	Matrix<marina_real> _cMatrixA;  //< Matrix A. 
	Matrix<marina_real> _cSourceMatrixA; //< Source Matrix A without changes in coordinates.

	Vector<marina_real> _cVectorB;   //< Vector B.
 
	Vector<marina_real> _cForces;    //< Actual forces.
	Vector<marina_real> _cAccelerations; //< Actual accelerations.

	AutoArrayWithMoves<PTCOLUMNHEADER> _cMappingToColumnID; //< Array is mapping current coordinate indexes to their descriptions.
	AutoArray<TCOLUMNHEADER> * _pcColumnHeaders; //< Coordinate descriptions.
    
    AutoArray<PTCOLUMNHEADER> _cReleaseList;

    BOOL _bLUDecompositionValid; //< The clamped part of Ma trix A was not changed from last calculation so old LU decomposition can be used;
   
                                       
	
    // BackUp
 /*
   unsigned int _pnLastIndexInZoneBackUp[EMZ_NUMBER]; //< 1 - based indexes.
    Matrix<marina_real> _cMatrixABackUp;  //< Matrix A. 

    Vector<marina_real> _cForcesBackUp;    //< Actual forces.
	Vector<marina_real> _cAccelerationsBackUp; //< Actual accelerations.

	AutoArray<PTCOLUMNHEADER> _cMappingToColumnIDBackUp; //< Array is mapping current coordinate indexes to their descriptions.
*/
	//M_ContactArray& _cContactPoints; //< Contact points.
	//M_ContactPedestalArray& _cContactPedestals; //< Contact pedestals.

    unsigned int _iCalledCalculateDiffs; // For profiling
    unsigned int _iMatrixSize;           // For profiling
    unsigned int _iLineDep;              // For profiling

    /** 
     * Function prepares A matrix and B vector and it initializates all staff needed in algorithm.
     * @param bNothingToDo - (out) If TRUE terminate no simulation needed.
     */
	virtual int Initialize(BOOL &bNothingToDo) = 0; 

    /**
     * Not used at the moment.
     */
	BOOL ProjectBIntoA(); 

    /**
     * Function makes next iteration.
     * @return TRUE - all is O.K., FALSE - unbound stream found
     */
	BOOL ResolveNextCollision(); 

    /**
     * Function makes first iteration.
     * @return TRUE - do more iterations, FALSE - no force need to be compensated. Finish the process.
     */
	BOOL ResolveFirstCollision();

    /**
     * Function calculates DiffForces and DiffAccelerations. (It solves algebra problem).
     * @param cDiffForces - (out) DiffForces
     * @param cDiffAccel - (out) DiffAccelerations
     * @param bPositive - (in) TRUE - new force must be positive, FALSE new force must be negative
     */
	void CalculateDiffs(Vector<marina_real>& cDiffForces, Vector<marina_real>& cDiffAccel, BOOL bPositive); 

    /**
     * Function resolves force. 
     * @param iMatrixIndex - (in) Coordinate index (force) in Matrix A. 
     * @param bNewForces - (in) TRUE - do next iteration, resolve force(try to compensate acceleration),
     *            FALSE - release force, lead force to zero.
     * @return TRUE - all is O.K., FALSE - unbound stream found
     */
	BOOL DriveToZero(unsigned int iMatrixIndex, 
			         BOOL bNewForce); 
private:
    BOOL DriveToZeroEnd(TCOLUMNHEADER &tColumnHeader, BOOL bFrictionClamped);
    void DriveToZeroCFromToNC(TCOLUMNHEADER &tColumnHeaderActual, TCOLUMNHEADER &tColumnHeaderMove);
    BOOL DriveToZeroRelease(TCOLUMNHEADER &tColumnHeaderRelease);
    void DriveToZeroReleaseEnd(TCOLUMNHEADER &tColumnHeaderRelease);
    void BackUp();
    void Restore();
protected:

    /**
     * Function replaces column in Matrix A with its corresponding original column from source Matrix A. 
     * @param iIndex - (in) Coordinate index in Matrix A.
     */
	void RevertColumnInMatrixA(unsigned int iIndex);

    /**
     * Function changes coordinate system for friction forces. 
     * @param tHeaderFriction1 - (in) Description of coordinate index with one of the friction forces.
     * @param cNewDir1 - (in) Direction in which must be friction axe corresponding to tHeaderFriction1. 
     *                   Just first two coordinates of vector are used. That are coordinates of new axe in coordinate 
     *      `            system build up by 1 and 2 direction mentioned in the dir member of the corresponding 
     *                   M_ContactPoint structure.
     */
    void ChangeFrictionAxes(TCOLUMNHEADER &tHeaderFriction1, const Vector3& cNewDir1);
	
    /**
     * Function calculates maximal step till first needed change between Clamped and NotClamped.
     * @param fSMin - (out) maximal step. If fSMin is negative, maximal step is infinite.
     * @param iCollision - (out) Coordinate index in Matrix A corresponding to fSMin
     * @param cDiffForces - (in) DiffForces
     * @param cDiffAccel - (in) DiffAccelerations
     * @return not used at the moment
     */
	BOOL CalculateMaximalAllowedStep( marina_real& fSMin, unsigned int &iCollision, 
		Vector<marina_real>& cDiffForces, Vector<marina_real>& cDiffAccel);

    /**
     * Function calculates maximal step till friction force reach it maximal value (normal force x friction coef).
     * @param fSMin - (out) maximal step. If fSMin is negative, maximal step is infinite.
     * @param tHeaderNormal - (in) Coordinate index description of normal force corresponding to current contact point
     * @param cDiffForces - (in) DiffForces
     */
	void CalculateMaximalAllowedStepForFrictionPoint(marina_real& fSMin, const TCOLUMNHEADER &tHeaderNormal, const Vector<marina_real>& cDiffForces);

	
    /**
     * Function switches position of two coordinates between themselfs. 
     * It does not cares about updates in Matrix Zones ... 
     * @param iColl1 - (in) Index of coordinate 1 in Matrix A.
     * @param iColl2 - (in) Index of coordinate 2 in Matrix A.
     */
	void SwitchCollisions(unsigned int iColl1, unsigned int iColl2);

     /**
     * Function moves coordinate into requested zone. 
     * @param iIndex - (in) Coordinate index in Matrix A.
     * @param iZone - (in) Destination zone.
     * @param iFromZone - (in) Zone, in which have to start searching for coordinate. (Just an optimalization.) 
     */
	void MoveToZone(unsigned int iIndex, unsigned int iZone, unsigned int iFromZone = 0);

    /**
     * Function moves coordinate into requested zone. 
     * It does some extra work comparing to MoveToZone. For instance: If normal force is moved between clamped
     * corresponding friction forces are moved into FrictionFromNotClamped zone. See source code for details.
     * @param iIndex - (in) Coordinate index in Matrix A.
     * @param iZone - (in) Destination zone.
     * @param iFromZone - (in) Zone, in which have to start searching for coordinate. (Just an optimalization.) 
     * @see MoveToZone
     */
	void MoveToZoneEx(unsigned int iIndex, unsigned int iToZone, unsigned int iFromZone = 0);

    /**
     * Function recalculates forces. 
     * It solves -VectorB = A ClampedForces(unknown).
     */
	void RecalculateClamped();

    /** 
     * Function applies calculated forces onto bodies.
     * @param cForces - (in) Forces.
     */
	virtual void ApplyCalculatedForces( const Vector<marina_real>& cForces) = 0;
	
    
    //void ApplyCalculatedCorrectionForces( Vector<marina_real>& cForces);
	//void ApplyCorrectionForces( marina_real fSimulationTime);

    /**
     * Not used int the moment
     */
	virtual BOOL ChooseNewPedestalPoint(unsigned int iIndex) = 0;

    /** 
     * Function frees memory dynamicaly allocated during algorithm run.
     */
	void Done(); 


    // Debug functions
	void Serialize();
	virtual void SerializeChild(FILE *fArchive) = 0;

    /** 
     * Function checks if clamped part of Matrix is regular.
     * @return TRUE - matrix is regular, FALSE - matrix is singular
     */
    BOOL IsMatrixARegular();

    BOOL VerifySituation();

    virtual Vector3& ContactPointDir(const unsigned int iContactPointID, const unsigned int iIndex) = 0;
public:

    /** 
     * Function performs the Dantzig algorithm and applies calculated forces.
     * @return Not used at the moment.
     */
	int Resolve();


	DantzigAlgorithm(): 	
	_bAllowDynamicContact(TRUE), _nNumberOfForces(0), _iLocalCycle(0), _iGlobalCycle(0), 
    _iCalledCalculateDiffs(0), _iLineDep(0), _iMatrixSize(0),    
	_pcColumnHeaders(NULL) {}; 

	~DantzigAlgorithm() {Done();};
};

#endif //__DANTZIG_H__
