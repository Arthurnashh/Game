/*
(c) Copyright 2007, RTDynamics
All Rights Reserved.

Product:        RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once


#include "RTD/Common.h"
#include "Physics/DinaPhysics.h"
#include "Physics/smMathMatrixD.h"

namespace RTD
{
    /** Initialize RTD. This function shall be called before using
    *   any other classes or functions from the RTD SDK.
    *
    *   @param  notificationLevel   can be used to specify the amount of log messages
    *                               RTDynamics library will output. smPrinter::SMNOT_ALWAYS
    *                               prints all messages while smPrinter::SMNOT_FATAL will only
    *                               print fatal issues.
    *   @param  toStdErr            when true, the log messages are printed to the stderr.
    *   @param  logFileName         if a non-empty string is given, the log
    *                               messages are printed to the file with the
    *                               given name.
    *
    *   The RTDynamics library log is printed with the smPrinter class which you can get with the getPrinter() call.
    *   smPrinter class allows finer control of the logging.
    */
    RTDYNAMICS_EXPORT void      init            ( smPrinter::NotifyLevel notificationLevel = smPrinter::SMNOT_ALWAYS
                                                  , bool toStdErr = true
                                                  , const std::string& logFileName ="") ;

    /** Shutdown RTD        */
    RTDYNAMICS_EXPORT void      shutdown        ( ) ;

    /** Unlock RTD using a customer code.   */
    RTDYNAMICS_EXPORT void      unlock          (   unsigned char codeByte1,
                                                    unsigned char codeByte2,
                                                    unsigned char codeByte3,
                                                    unsigned char codeByte4,
                                                    unsigned char codeByte5,
                                                    unsigned char codeByte6 );

    /** Shortcut to the global RTD printer. */
    RTDYNAMICS_EXPORT smPrinter*    getPrinter  ( ) ;

    /** Used by example applications to get home directory of the installation. */
    RTDYNAMICS_EXPORT std::string   getHomeDir ( ) ;

    /** Used by example applications to get home directory of the installation. */
    RTDYNAMICS_EXPORT std::string   getHomeDirFromRegistry ( );

    /** Used by example applications to get the data directory of the installation. */
    RTDYNAMICS_EXPORT std::string   getDataDir ( ) ;

    /** Used by example applications to get the 3D model directory of the installation. */
    RTDYNAMICS_EXPORT std::string   get3DDataDir ( ) ;

    /** Converts a station given aircraft design coordinates to body coordinate system.
    *
    *   Design coordinate system vector is defined as:
    *
    *   [0] fuselage    points from nose to tail (origin placed at the aircraft nose tip)
    *   [1] buttline    points to right (origin located at the aircraft plane of symmetry)
    *   [2] waterline   points to up (origin located on the ground)
    *
    *   @param  CoGInDCS    position of center of gravity in design coordinate system
    *   @param  vecInDCS    station in design coordinates system
    */
    typedef smVec3r DesignCoordinates ;
    smVec3r designCoordSysToBodyCoordSys( const DesignCoordinates& CoGInDCS, const DesignCoordinates& vecInDCS ) ;

    /** If the given statement is false, validateAndWarn prints a warning
     *  message using the RTD Printer (see RTD::getPrinter()). If statement
     *  is true, no action will be taken.
     *  
     *  @param  statement statement to be verified.
     *  @param  what    name of the variable or statement which is being
     *                  validated as a std::string. E.g. " mVariable > 0"
     *  @param  where   name of the scope: the function or file name where
     *                  this validation takes place. E.g. "MyClass::setVariable"
     */
    RTDYNAMICS_EXPORT bool validateAndWarn(bool statement, const std::string& what, const std::string& where );

    /** If the given statement is false, validateAndExit prints a warning
    *   message using the RTD Printer (see RTD::getPrinter()), pops up a message
    *   box and finally aborts the application. If statement is true, no action
    *   will be taken.
    *  
    *   @param  statement statement to be verified.
    *   @param  what    name of the variable or statement which is being
    *                   validated as a std::string. E.g. " mVariable > 0"
    *   @param  where   name of the scope: the function or file name where this
    *                   validation takes place. E.g. "MyClass::setVariable"
     */
    RTDYNAMICS_EXPORT bool validateAndExit(bool statement, const std::string& what, const std::string& where );

    /** If the given statement is false, validateAndThrowException throws an
    *   std::string as an exception. The exception can be caught with a simple
    *   "catch( const std::string& error)" phrase. If statement is true,no
    *   action will be taken.
    *  
    *   @param  statement statement to be verified.
    *   @param  what    name of the variable or statement which is being
    *                   validated as a std::string. E.g. " mVariable > 0"
    *   @param  where   name of the scope: the function or file name where this
    *                   validation takes place. E.g. "MyClass::setVariable"
    */
    RTDYNAMICS_EXPORT void validateAndThrowException(bool statement, const std::string& what, const std::string& where );

	/** Converts the given vector in NED frame to WCS frame. */
    RTDYNAMICS_EXPORT void NEDToWCS( const smVec3d& in, smVec3d& out ) ;
    RTDYNAMICS_EXPORT void NEDToWCS( const smVec3r& in, smVec3r& out ) ;

	/** Converts the given vector in WCS frame to NED frame. */
    RTDYNAMICS_EXPORT void WCSToNED( const smVec3d& in, smVec3d& out ) ;
    RTDYNAMICS_EXPORT void WCSToNED( const smVec3r& in, smVec3r& out ) ;

	/** Sets the given matrix as NED to WCS transformation matrix. */
    RTDYNAMICS_EXPORT void getNEDToWCSTransformationMatrix( smMathMatrixD::Mat4& m ) ;

	/** Sets the given matrix as WCS to NED transformation matrix. */
    RTDYNAMICS_EXPORT void getWCSToNEDTransformationMatrix( smMathMatrixD::Mat4& m ) ;


    /**
    @name File encryption and decryption

    Following utility functions can be used to encrypt and decrypt files. It is possible to encrypt configuration files and distribute
    them in a non-human-readable form. When a vehicle configuration file is encrypted, it must have ecfg extension. Constructors of Vehicle
    classes are able to recognize ecfg files and decrypt them automatically. However, before loading ecfg files, the password that is 
    used for encryption must be set using setECFGPassword().

    */
    //@{

    /**
        \brief Encrypts a binary stream with a given password.
    */
	void RTDYNAMICS_EXPORT encrypt( const std::vector<unsigned char>& in , const std::string& passwordStr , std::vector<unsigned char>& out );

    /**
        \brief Decrypts an encrypted binary stream with a given password.
    */
	void RTDYNAMICS_EXPORT decrypt( const std::vector<unsigned char>& in , const std::string& passwordStr ,  std::vector<unsigned char>& out  );
	
	/**
        \brief Loads a file and decrypts it with a given password.
    */
	bool RTDYNAMICS_EXPORT loadEncryptedTextFile( const std::string& fileName , const std::string& password , std::string& decodedString );

    /**
        \brief Encrypts a given string with the given password and saves to a file.
    */
	void RTDYNAMICS_EXPORT saveEncryptedTextFile( const std::string& fileName ,  const std::string& stringToEncrypt , const std::string& password );
	
    /**
        \brief Creates an encrypted version of a file.
    */
	bool RTDYNAMICS_EXPORT encryptFile( const std::string& inputFileName  ,  const std::string& outputFileName , const std::string& password );

	/**
        \brief This method is used to set a system wide decryption password. Whenever an ecfg file is used as a configuration file for a vehicle, this 
        password is used to decrypt the file.
    */
	void RTDYNAMICS_EXPORT setECFGPassword( const std::string& password );

	/**
        \brief Retrieves the currently set ecfg password.
    */
	std::string RTDYNAMICS_EXPORT getECFGPassword();
    //@}

}

/*! \mainpage Welcome to RTDynamics Simulation Products Reference Guide
*
*<p>This reference guide is a complete guide for programmers who are
*integrating RTDynamics simulation products to their applications.
*
*Note that this is an API reference documentation only. Refer to evalution
*guide to find out the most relevant classes and flight models for your application
*if you are dealing with RotorLib or FixedWingLib for the first time.
*
*Additionally Programmer's Guide is authored by RotorLib and FixedWingLib developers
*and provides the conceptual background, tutorials and other important details.
*/
