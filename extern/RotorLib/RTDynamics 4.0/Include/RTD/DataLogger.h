/*
(c) Copyright 2008, RTDynamics
All Rights Reserved.

Product:        RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/

#pragma once

#include "RTD/BotBase.h"
#include "RTD/Common.h"

namespace RTD
{
    class PIDController;
    class PIDControllerD;

//
// The following typedefs are used to maintain multiplatformness
// of data logging. Currently this type of typedefs for other
// types are not required somewhere else in RTD. So it is defined
// and used here only.
//
#ifdef WIN32
    // Microsoft Types
    typedef __int32                 RTDInt32;
    typedef unsigned __int32        RTDUInt32;
#elif defined(__linux)
    // Linux Types (stdint.h)
    #include <stdint.h>
    typedef int32_t         RTDInt32;
    typedef uint32_t        RTDUInt32;
#endif

    enum BinaryFileFormatChunkType
    {
        BINARYFILEFORMATCHUNKTYPE_HEADER            =   0,
        BINARYFILEFORMATCHUNKTYPE_DATACHANNELDESC   =   1,
        BINARYFILEFORMATCHUNKTYPE_SAMPLEARRAY       =   2,
        BINARYFILEFORMATCHUNKTYPE_TIMEMARKER        =   3,
        BINARYFILEFORMATCHUNKTYPE_POSMARKER         =   4,

        NO_BINARYFILEFORMATCHUNKTYPES
    };

    class RTDYNAMICS_EXPORT DataLogger
    {
    public:
        struct Channel
        {
			Channel(): 
				m_source(NULL),
				m_index(0)
			{}

            bool                m_enabled;
            std::deque<double>  m_data;
            std::string         m_channelName;
			const RTDVariable*	m_source;
			unsigned int		m_index;
        };

		

        struct TimeMarkerDesc
        {
            RTDUInt32    chunkType ;

            float        time ;

            RTDUInt32    notesSize ;
            char         notes[1024];

            TimeMarkerDesc( smReal t, const std::string& n) ;
        };

        struct PosMarkerDesc
        {
            RTDUInt32    chunkType ;

            float        time ;

			float        posX ;
            float        posY ;
            float        posZ ;

            RTDUInt32    notesSize ;
            char         notes[1024];

            PosMarkerDesc( smReal t, const std::string& n, smReal x, smReal y, smReal z) ;
        };

        enum FileFormat
        {
            FileFormat_Ascii,
            FileFormat_BinaryVersion1,

            No_FileFormats
        };

        DataLogger();
        virtual ~DataLogger();

		virtual unsigned int    addBotChannels( const BotBase* bot );
		virtual unsigned int	addSimVarUserChannels( const SimulationVariableUser* svu );
		virtual unsigned int    addChannels( const RTDVariable* simvar );
        virtual unsigned int    addChannel( const std::string& name );
        virtual unsigned int    addPIDChannels(  const PIDController* pid , const std::string& prefix="" );
        virtual unsigned int    addPIDChannels( const PIDControllerD* pid , const std::string& prefix="");


		template<typename T>
		unsigned int	addData( unsigned int index , const RTD::DynamicControllerParameter<T>& param , T valueWhenUnspecified )
		{
			if ( param.isSpecified() )
			{
				T val = param;
				return addData( index , val );
			}
			
			if (param.getReturnDefaultWhenUnspecified() && param.hasDefault())
			{
				return addData( index , param.defaultValue() );
			}

			return addData( index , valueWhenUnspecified );
		}

		template<typename T>
		unsigned int	addDCPSingleMemberData( unsigned int index , const RTD::DynamicControllerParameter<T>& param , unsigned int memberIndex , T valueWhenUnspecified )
		{
			if ( param.isSpecified() )
			{
				T val = param;
				return addData( index , val[(int)memberIndex] );
			}
			
			if (param.getReturnDefaultWhenUnspecified() && param.hasDefault())
			{
				return addData( index , param.defaultValue()[(int)memberIndex] );
			}

			return addData( index , valueWhenUnspecified[(int)memberIndex] );
		}
		
        virtual unsigned int    addData( unsigned int index , const double& data );
        virtual unsigned int    addData( unsigned int index , const smVec2r& data );
        virtual unsigned int    addData( unsigned int index , const smVec3r& data );
        virtual unsigned int    addData( unsigned int index , const smVec2d& data );
        virtual unsigned int    addData( unsigned int index , const smVec3d& data );
        virtual unsigned int    addData( unsigned int index ,PIDController* pid );
        virtual unsigned int    addData( unsigned int index , PIDControllerD* pid );

        virtual void            addTimeMarker( smReal time, const std::string& notes);
        virtual void            addPosMarker( smReal time, const std::string& notes, smVec3r xyz );

		smDouble				getData( unsigned int channelIndex , unsigned int frame ) const;
		unsigned int			getNumFrames( unsigned int channelIndex  ) const;

        std::string     getChannelName(unsigned int index) const;
        bool            getChannelIndex( const std::string& name , unsigned int& index ) const;

        virtual void    newFrame();
        virtual void    endFrame();


        const std::string   getFileName() const {   return m_logFileName;   }

        virtual void    start(  const std::string& fileName ,
                                unsigned int downsamplingWindowSize = 1,
                                const std::vector< std::string >& activeChannels  = std::vector< std::string >(),
                                FileFormat binaryFileFormat = FileFormat_BinaryVersion1 );

		bool			isPaused()	{	return m_isPaused;	}
		virtual void    pause();
		virtual void    contin();

        virtual void    stop();

        bool            isLogging() const;

		/**
			Loads a log file and returns its contents in \e channelNames and \e channelData
			variables.

			The method returns true if loading and reading is completed without problems.
		*/
		static bool	loadBinary1Log( const std::string& file , 
										std::vector<std::string>& channelNames,
										std::vector< std::vector<double> >&	channelData );

		enum CompareLogsResult
		{
			CHANNEL_COUNTS_DIFFER = 1,
				CHANNEL_NAMES_DIFFER = 2,
				CHANNEL_CONTENTS_DIFFER =4,
				CHANNELS_IDENTICAL = 8
		};

		static std::string	compareLogs(	std::vector< std::string> & channelNames1,
											std::vector< std::vector<double> > &	channelData1,
											std::vector< std::string> & channelNames2,
											std::vector< std::vector<double> > &	channelData2,
											const std::vector<std::string>& ignoredChannels );									

    protected:
		bool					m_isPaused;

        std::vector< Channel* >      m_channels;
        std::vector< TimeMarkerDesc >   m_timeMarkers;
        std::vector< PosMarkerDesc >   m_posMarkers;


        FILE*                   m_logFile;
        std::string             m_logFileName;
        unsigned int            m_frameNo;
        unsigned                m_downsamplingWindowSize;
        FileFormat              m_formatType ;

        virtual void        createLogFile( const std::string& fileName );
        virtual void        saveLog();
        virtual void        saveAsciiLog();
        virtual void        saveBinary1Log();
		virtual void		updateChannelsWithSource();

    };



}
