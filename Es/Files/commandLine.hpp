#ifdef _MSC_VER
#  pragma once
#endif

#ifndef _ES_COMMAND_LINE_HPP
#define _ES_COMMAND_LINE_HPP

/*!
\file
command line parsing using iterators and ForEach
*/

#include <Es/Containers/forEach.hpp>
//! command line parsing using iterators and ForEach

class CommandLine
{
  const char *_cmdLine;

  
  public:
  //! iterators points on the character following the end of previous argument
  typedef const char *Iterator;
  //! stores begin and eng of single argument
  struct SingleArgument
  {
    const char *beg;
    const char *end;
    SingleArgument(const char *b, const char *e){beg = b,end = e;}
  };
  //! ItemType defined for easier Traits implementation
  typedef SingleArgument ItemType;

  //! construct object
  CommandLine(const char *cmdLine)
  :_cmdLine(cmdLine)
  {
  }

  //! skip one argument - usefull when skipping exe file name
  void SkipArgument()
  {
    _cmdLine = Advance(_cmdLine);
  }

  //! get iterator to the first argument
  Iterator Start() const {return _cmdLine;}
  //! check if we can continue
  bool NotEnd(Iterator i) const {return *i!=0;}
  //! get iterator to the next agument
  Iterator Advance(Iterator i) const
  {
    // advance to next command line argument
    while (isspace(*i)) i++; // skip any leading white spaces
		if( *i=='"' )
    {
			i++;
			// string argument
			while( *i && *i!='"' ) i++;
      if (*i=='"') i++;
    }
    else
    {
      while (!isspace(*i) && *i) i++; // skip any leading white spaces
    }
    return i;
  }
  //! get argument pointed by the itetrator
  SingleArgument Get(Iterator i) const
  {
    Iterator end = Advance(i);
    while (isspace(*i)) i++; // skip and leading spaces
    // ignore quotes if necessary
    if (*i=='"')
    {
      ++i;
      if (end[-1]=='"') --end;
    }
    return SingleArgument(i,end);
  }
};

//! traits for ForEach - see ContainerTraits
struct CommandLineForEachTraits
{
  typedef CommandLine Container;
	typedef CommandLine::Iterator Iterator;
	typedef CommandLine::ItemType ItemType;

	static Iterator Start(const Container &c) {return c.Start();}
	static void Advance(const Container &c, Iterator &it) {it = c.Advance(it);}
	static bool NotEnd(const Container &c, Iterator it) {return c.NotEnd(it);}

	static ItemType Select(const Container &c, Iterator it)
	{
		return c.Get(it);
	}
};

inline bool SingleArgumentCtx( const CommandLine::SingleArgument arg, void (*singleArg)(const char *beg, const char *end) )
{
  // function is passed as context
  singleArg(arg.beg,arg.end);
  return false;
}

inline void ScanCommandLine(const CommandLine &cmd, void (*singleArg)(const char *beg, const char *end))
{
  ForEach<CommandLineForEachTraits>(cmd,SingleArgumentCtx,singleArg);
}

#endif
