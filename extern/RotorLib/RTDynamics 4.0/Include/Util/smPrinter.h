/*
(c) Copyright 2004, MachSim
All Rights Reserved.

Product:		DinaPhysics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of MachSim
*/
#pragma once

#include <string>
#include <deque>
#include <stdio.h>

#include <Util/smManager.h>
#include <Util/smSingleton.h>
#include <Util/smException.h>

/**
*	smPrinter is general purpose extensible printing
*	class. Users of class can add Listener derived
*	classes to the list of print listeners. In this case
*	Any print is forwarded to the listeners in this list.
*
*	smStdErrPrinter is the classic printer which prints all messages
*	to stderr. User can add any other classes, e.g. GUI consoles, log
*	file printers.
*
*	smPrinter doesn't have any listeners by default.
*/
class smPrinter
{
public:
    enum NotifyLevel
    {
        SMNOT_DEBUG,
        SMNOT_INFO,
        SMNOT_WARNING,
        SMNOT_FATAL,
		SMNOT_ALWAYS
    };

    struct PrintRecord
    {
        NotifyLevel		m_level ;
        std::string		m_msg ;
    } ;

    /**
    *	Abstract base class for listeners of smPrinter
    *	messages.
    */
    class Listener
    {
    public:
        virtual void print( const std::string& msg ) = 0 ;
        virtual void print( const char* msg ) = 0 ;
    };

    public:
        /**
        *	By default, notification level is debug.
        *	A stderr printer is also created.
        *
        */
        explicit smPrinter() ;
        virtual ~smPrinter() ;

        /**
        *	Printing and notification methods
        */
        void		setNotificationLevel(NotifyLevel notLevel);
        NotifyLevel	getNotificationLevel();

		/** Toggle automatic append of \n for each print function.	*/
		void		setAutoEndLine( bool autoAddLine ) ;
		bool		getAutoEndLine() const ;

		void		setAutoPrefix( bool autoPrefix ) ;
		bool		getAutoPrefix( ) const ;

        void print		( NotifyLevel notLevel, const char *format, ... ) ;
        void print		( NotifyLevel notLevel, std::string msg ) ;
        void print		( std::string msg, NotifyLevel notLevel = SMNOT_INFO ) ;
        void printLow	( NotifyLevel notLevel, std::string msg ) ;

        void addPrintListener	( Listener* listener ) ;
        void remPrintListener	( Listener* listener ) ;
        void remAllPrintListeners	( bool deleteListeners = true ) ;

        /**
        *   The following functions forward the given strings to the print listeners
        *   which can then print to console or write to file.
        *
        *   These methods are subject to print level filtering.
        */
        void always	(const char *format, ...) ;
        void fatal	(const char *format, ...) ;
        void warn	(const char *format, ...) ;
        void info	(const char *format, ...) ;
        void debug	(const char *format, ...) ;
        void always	(const std::string&) ;
        void fatal	(const std::string&) ;
        void warn	(const std::string&) ;
        void info	(const std::string&) ;
        void debug	(const std::string&) ;


        void					setPrintRecordSize ( size_t size ) ;
        size_t					getPrintRecordSize () { return m_printRecordSize ; }
        size_t					getPrintRecordCurrentSize () { return m_printRecord.size() ; }
        PrintRecord&			getPrintRecord ( int idx ) { return m_printRecord[idx] ; }
        void					clearPrintRecord () { m_printRecord.clear() ; }

		/** Enables beeping. Beep sound will be produced only when a message with beepLevel and above comes. */
		void beep( bool status, NotifyLevel beepLevel ) ;

		/** Pops up a message box. If the parent is NULL, the focus will not be on the message box.
		*	A valid parent window allows the focus to be on the message box.
		*/
		void setMessageBox( bool status, NotifyLevel msgBoxLevel ) ;
		static void messageBox( const std::string& title, const std::string& message, void* parent = NULL ) ;

    private:
        smManager<Listener>				m_printListenerMan ;
        NotifyLevel						m_notLevel;

        std::deque<PrintRecord>			m_printRecord ;
		size_t							m_printRecordSize ;

		bool							m_autoEndLine ;
		bool							m_autoPrefix ;

		bool							m_beep ;
		NotifyLevel						m_beepLevel ;

		bool							m_messageBox ;
		NotifyLevel						m_messageBoxLevel ;
};

/**
*	Default print listener is smStdErrPrinter
*	Prints to console.
*/
class smStdErrPrinter : public smPrinter::Listener
{
public:
    void print( const std::string& msg ) ;
    void print( const char* msg );
};


/**
*	Non buffering log file printer, this file printer
*   is slow because it flushes the buffer with every log entry.
*/
class smLogFilePrinter : public smPrinter::Listener
{
public:
    smLogFilePrinter( std::string fileName ) ;

    virtual ~smLogFilePrinter(  );
    void print( const std::string& msg );
    void print( const char* msg ) ;
    const std::string getFileName() const { return m_fileName; } ;

private:
    FILE*       m_file ;
    std::string m_fileName ;
};