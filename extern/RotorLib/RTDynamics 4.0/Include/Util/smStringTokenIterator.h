/*
(c) Copyright 2004, MachSim
All Rights Reserved.

Product:		DinaPhysics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of MachSim
*/
#pragma once

// gcc 2.x series compiler can not handle this class.
#define SMSTRINGTOKENITERATOR_AVAILABLE
#if defined(__linux)
#	if __GNUC__ < 3
#		undef SMSTRINGTOKENITERATOR_AVAILABLE
#	endif
#endif

#if defined(SMSTRINGTOKENITERATOR_AVAILABLE)

#include <string>
#include <vector>
#include <iterator>

/** From http://www.codeproject.com/string/stringsplitter.asp */
class smStringTokenIterator
    : public std::iterator<std::input_iterator_tag, std::string>
{
public:
    smStringTokenIterator() : mStr(0), mStart(0), mEnd(0) {}
    smStringTokenIterator(const std::string & str_, const char * seperator = " ") :
    mSeperator( seperator ),
        mStr(&str_),
        mEnd(0)
    {
        findNext();
    }
    smStringTokenIterator(const smStringTokenIterator & rhs) :
    mSeperator(rhs.mSeperator),
        mStr(rhs.mStr),
        mStart(rhs.mStart),
        mEnd(rhs.mEnd)
    {
    }

    smStringTokenIterator & operator++()
    {
        findNext();
        return *this;
    }

    smStringTokenIterator operator++(int)
    {
        smStringTokenIterator temp(*this);
        ++(*this);
        return temp;
    }

    std::string operator*() const
    {
        return std::string(*mStr, mStart, mEnd - mStart);
    }

    bool operator==(const smStringTokenIterator & rhs) const
    {
        return (rhs.mStr == mStr && rhs.mStart == mStart && rhs.mEnd == mEnd);
    }

    bool operator!=(const smStringTokenIterator & rhs) const
    {
        return !(rhs == *this);
    }

    /** This is an advanced version of std::string::find_first_of
    *   It can escape characters when searching for a special character.
    */
    static size_t findFirstNotEscapedOf( const std::string& str, char seperator, size_t start, char escapeChar = '@' )
    {
        bool escaped = true ;
        size_t startIdx = start ;
        size_t idx ;

        while ( escaped )
        {
            idx = str.find_first_of( seperator, startIdx ) ;

            if ( idx == std::string::npos )
                return std::string::npos ;

            if ( idx == 0 )
                escaped = false ;
            else
            {
                // Check if the character before is an escape char
                if ( str[idx-1] == escapeChar )
                    escaped = true ;
                else
                    escaped = false ;
            }

            startIdx = idx + 1 ;
        }

        return idx ;
    }
   

    /**
    *	Splits the string with the given character, the original use was unintuitive
    *	e.g.
    *		a, b,t , f, g,13, 0
    *   is also able to handle
    *		a,' ','',
    *   this gives "a", " " and ""
    */
    static std::vector<std::string> split( std::string str, char seperator, bool trimTokenWhitespace = true )
    {
        std::vector<std::string> list ;

		// Return here if string is empty.
		if ( str.empty() )
			return list ;

        str.insert( 0, (const char*) &seperator ) ;
        str.push_back( seperator ) ;

        std::string::size_type idxStart = findFirstNotEscapedOf( str, seperator, 0 ) ;
        std::string::size_type idxEnd = findFirstNotEscapedOf( str, seperator, idxStart + 1 ) ;

        do
        {
            std::string token = str.substr( idxStart + 1, idxEnd - (idxStart+1) ) ;

            //
            // values with ' around, need special care and allows ,'', or ,' ', kind of uses
            //
            if ( token[0] == '\'' && token[token.length()-1] == '\'' )
            {
                if ( token.length() != 2 )
                    // ' ' or '   ' or '   sdsf   '
                    list.push_back( token.substr( 1, token.length() - 2 ) ) ;
                else
                    // '' case
                    list.push_back( "" ) ;
            }
            else
            {
                if ( trimTokenWhitespace && token.size()>0) while ( token[0] == ' ' ) token.erase(0, 1) ;
                if ( trimTokenWhitespace && token.size()>0) while ( token[ token.size() - 1 ] == ' ' ) token.erase(token.size() - 1, 1) ;

                // Remove the white space from the list
                std::string cleanToken ;
                std::string::iterator i = token.begin() ;
                for ( ; i != token.end() ; i++ )
                    if ( *i != '@' )
                        cleanToken += (*i) ;

				if ( cleanToken != "" )
                    list.push_back( cleanToken ) ;
            }

            // Find the next seperator too, if not found return the last char idx.
            idxStart = findFirstNotEscapedOf( str, seperator, idxStart + 1 ) ;
            idxEnd	 = findFirstNotEscapedOf( str, seperator, idxStart + 1 ) ;
        }
        while( idxEnd != std::string::npos ) ;

        return list ;
    }

private:

    void findNext(void)
    {
        mStart = mStr->find_first_not_of(mSeperator, mEnd);
        if(mStart == std::string::npos)
        {
            mStart = mEnd = 0;
            mStr = 0;
            return;
        }

        mEnd = mStr->find_first_of(mSeperator, mStart);
    }

    const char*				mSeperator;
    const std::string*		mStr;
    std::string::size_type	mStart;
    std::string::size_type	mEnd;
};

#endif // SMSTRINGTOKENITERATOR_AVAILABLE