/*
Progammer's interface:
----------------------

1.  To raise an instance:
RegExp My_grep( regexp );

where:  regexp is a UNIX style regular expression
contained in a NULL terminated character
string.  The argument is not overwritten
so it is ok to pass constants.
examples:
RegExp My_grep( "hello" );   // case 1
RegExp My_grep( "c[A-Z]" );  // case 2
RegExp My_grep( "[0-9]+" );  // case 3

2.  To parse for regular expressions:
Match( string );

where:  string is a NULL terminated string containing
the object.  The method returns a 1 if the
expression is present, otherwise 0.  The
string is not disturbed or overwritten.
examples:
My_grep.Match( "hello out there" );
returns 1 for case 1, 0 for cases 2 and 3

My_grep.Match( "RegExp" );
returns 1 for case 2, 0 for cases 1 and 3

My_grep.Match( "1.234" );
returns 1 for case 3, 0 for cases 1 and 2

There are some other things that can be done, such as
obtaining the location of the match, but they are
private members at the moment.  Feel free to hack
away.

History
-------

There is not much to tell.  If found the kernel on a German
BBS.  It was written in the old "C" convention and contained
a single comment describing that it was a regular expression
parser and that was it.  From the programming styles, I guess
that at least two fervent hackers had wrapped it - in
addition to the original author (whoever that was), and at
least one of them was using THINK C.  

It was clear, however, that the original author did some
excellent work and was doing some advanced thinking
in finite-state machines.  What a shame that his
own comments are missing.  Thanks whoever you are!

My own contribution was to clarify an interface and to
to encapsulate the engines into a C++ context.  I also
coded out the mallocs and reallocs and callocs; removed 
"n" number of #ifdef's (#ifdef DEBUG, #ifdef REALLYDEBUG, 
#ifdef NOSTRINGS, and so on); out-stringized the strings;
did a little translation to get the whole thing into
English; and what not.

I tested the code with VC++ 1.0 and Borland 3.1 and
basically got to the place I wanted to be at.  It 
may or may not work for your particular project,
and it may or may not conform to the GREP you
are working with.  

There is an example at the bottom of this file.

---------->> GJV

Improvements
------------

23.9.2004
* support CharIterator enables using own character readers, such as streams or another kind of strings.        

* added special characters '_' and '\x'. See regexp description.

*

Regular Expression Quickguide
-----------------------------

Regular expression is character string. Main purport is testing and validating character string sequence. 
It has special form, that using powerful operators. 

Regular expression (RegExp) can contain one of following characters (or sequence of characters)

^         at the begin of RegExp means, that following rules must be applied only at beginig of string
          (if RegExp doesn't start with this character, RegExp can match everwhere in string)
          Example "^hallo" - will match when word 'hallo' is at beggining of tested string.

$         at the end of RegExp means end of string. It means, no other characters after previous rule are
          allowed
          Example "hallo$" - will match when word 'hallo' is at the end of tested string

.         any character
          Example: "h.llo" - will match 'hallo' or 'hello' or 'h7llo' etc.

*         Previous character can be repeated 0-infinite times
          Example: "ab*a" - will match 'aba', 'abbbbba' and also  'aa';

+         Previous character can be repeated 1-infinite times
          Example: "ab*a" - will match 'aba', 'abbbbba' but not  'aa';

?         Previous character can is optional
          Example: "ab?a" - will match 'aba and also 'aa';

[abc]     one of specified characters

[a-z]     one of characters in range.

[a-zA-Z]  one of characters in ranges.

[a-z12,;] one of characters in range a-z and other specified characters

[]\\-]    one of characters ], -, or \ 

[^0-9]    any characters except characters in range 0-9 (character ^ have exclusion meaning)

[\s\n\r\_] special characters that means
          \s - any white space 
          \n - new line character
          \r - carriage return character
          \_ - any white space excluding new line
          \c - any control character (ascii 1-31)
          \t - tabulator


          all these special characters can be combined with other characters in [ ... ]

(subexp)  Defines subexpression. anything between ( and ) is evaluated as one character 
          Example: (abc)+ - will match also abcabcabcabcabcabc

_         Any white space excluding new line '\n'

\         Escapes next characters. if there is need compare character, that is in set of special characters,
          use '\' to escape it. Use double '\' (\\) to compare \.
          Example "\++" - will match '+' or '++++++++++++++'
          This is also applied in [...] operator. But you cannot write [\n-\r], it is intepreted as '\n','n-\','r'


other     Any other characters are interpeted and compared as it is. RegExp is case sensitive. If you
          compare case insensitive, lower or upper both strings before testing



*/

#ifndef __REGEXP_CLASS_HEADER_
#define __REGEXP_CLASS_HEADER_

#pragma warning (push)
#pragma warning (disable:4018)
#pragma warning (disable:4309)



template <class T>
class CharIterator
{
  int _pos;
  const T *_ifc;
public:
  CharIterator()   {_pos=0;_ifc=0;}
  CharIterator(const T *ifc) {_pos=0;_ifc=ifc;}
  //  operator char() const {return (*_ifc)[_pos];}
  char operator *() const {return (*_ifc)[_pos];}
  CharIterator operator+(int offset) const {CharIterator res=*this;res._pos+=offset;return res;}
  CharIterator operator-(int offset) const {CharIterator res=*this;res._pos-=offset;return res;}
  CharIterator &operator+=(int offset) {_pos+=offset;return *this;}
  CharIterator &operator-=(int offset) {_pos-=offset;return *this;}
  CharIterator &operator++() {_pos+=1;return *this;}
  CharIterator operator++(int zero) {CharIterator res=*this;_pos+=1;return res;}
  CharIterator &operator--() {_pos-=1;return *this;}
  CharIterator operator--(int zero) {CharIterator res=*this;_pos-=1;return res;}
  int operator-(const CharIterator &other) const {return _pos-other._pos;}
  bool operator==(const CharIterator &other) const {return _pos==other._pos && _ifc==other._ifc;}
  bool operator!=(const CharIterator &other) const {return _pos!=other._pos  || _ifc!=other._ifc;}
  bool operator>(const CharIterator &other) const {return _pos>other._pos && _ifc==other._ifc;}
  bool operator<(const CharIterator &other) const {return _pos<other._pos && _ifc==other._ifc;}
  bool operator>=(const CharIterator &other) const {return _pos>=other._pos && _ifc==other._ifc;}
  bool operator<=(const CharIterator &other) const {return _pos<=other._pos && _ifc==other._ifc;}  
  bool operator==(int null) const {return null==0 && _ifc==0;}
  CharIterator &operator=(int null) {_ifc=0;_pos=0;return *this;}
  void SetInterface(T *ifc) {_ifc=ifc;}
};


#define __DEMO__
#ifdef __DEMO__
#include   <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#endif

//
//
//--------------------------------------------------------------
//-------------  Class declaration for RegExp  ------------------
//--------------------------------------------------------------
//  NOTE:  As a transferability (not portability, but transferability)
//         consideration, all methods are kept inline.
//
//
template <class T=const char *>
class RegExp 
{
  enum Flags
  {
  RegExpBitCanMatchNothing          =0x00,
  RegExpBitMoreThenNull             =0x01,
  RegExpBitOpensOnStarOrPlus        =0x04,
  RegExpBitStarOrPlusOk             =0x02
  };
  //  Character values
  enum CharacterValues
  {
  RegExpCharCloseBracker       =']',
  RegExpCharCloseParen         =')',
  RegExpCharDollar             ='$',
  RegExpCharBackSlash          ='\\',
  RegExpCharHoch               ='^',
  RegExpCharMinus              ='-',
  RegExpCharUnderscore         ='_',
  RegExpCharOpenBracket        ='[',
  RegExpCharOpenParen          ='(',
  RegExpCharOr                 ='|',
  RegExpCharPeriod             ='.',
  RegExpCharPlus               ='+',
  RegExpCharQuestion           ='?',
  RegExpCharStar               ='*',
  RegExpCharZero              ='\0'
  };

  //  Limitations
  enum Limitations
  {
  RegExpMaxStore             =2048,
  RegExpMaxSubExpressions     =20
  };
  // Opcodes used internally
  enum Opcodes
  {
  RegExpOpAny                =3,
  RegExpOpAnyBut             =5,
  RegExpOpAnyOf              =4,
  RegExpOpBack               =7,
  RegExpOpBol                =1,
  RegExpOpBranch             =6,
  RegExpOpClose             =30,
  RegExpOpEnd                =0,
  RegExpOpEol                =2,
  RegExpOpExact              =8,
  RegExpOpNMEmpty            =9,
  RegExpOpOpen              =20,
  RegExpOpPlus              =11,
  RegExpOpStar              =10,
  RegExpSignature         =0x9C
  };
  //  Pleasantly obscure functions
  static inline int RegExpAsUnsigned(const char *p)        { return ((int)*(unsigned char *)(p)); }
  static inline int RegExpChar2Opcode(const char *p)       { return (*(p)); }
  static inline char *RegExpControllingOp(char *p)         { return ((p)+3); }
  static inline int RegExpGetNext(const char *p)          { return (((*((p)+1)&0xFF)<<8)+(*((p)+2)&0xFF)); }
  static inline bool RegExpIsConpound(char c)              { return ((c)==RegExpCharStar||(c)==RegExpCharPlus||(c)==RegExpCharQuestion); }

private:

  T strchr(T findwhere,int what)
  {
    T c=findwhere;
    while (*c && *c!=what)
    {
      findwhere++;
      c=findwhere;
    }
    if (*c==what)  return c;
    else return T();
  }

  int strncmp(T a1,const char *a2, int len)
  {
    while (len-- && *a2 && *a1==*a2 )
    {
      a1++;a2++;
    }
    if (len<0) return 0;
    if (*a2==0) return 0;
    if (*a1>*a2) return 1;
    if (*a1<*a2) return -1;
    return 0;
  }

  int strncmp(const char *a2,T a1, int len)
  {
    return -strncmp(a1,a2,len);
  }

  int strlen(T a1)
  {
    int len=0;
    while (*a1) 
    {++a1;++len;
    }
    return len;
  }

  //
  //


public:
  // query this element to learn if your regular expression
  // was compiled ok.
  int _errorCode;
  // if the RegExp instance did not like something,
  // you can read all about it here.
  char _errorString[  255 ];
public:
  // The single constructor requires a UNIX regular
  // expression.  It is ok to pass a constant.
  RegExp(const char *exp ) 
  {
    _specialChars = "^$.[()|?+*\\_";
    _errorCode = 0;
    strcpy( _errorString, "" );
    if( exp == NULL ) 
    {
      RecordError( 0 );
      return;
    }
    _inputScanPointer = exp;
    _parenthesesCount = 1;
    _symbolStringSize = 0L;
    _codeEmitPointer = &_regExpShadow;
    EmitNextByte( (char)RegExpSignature );
    int flags;
    if( ParseParens( 0, &flags ) == NULL ) 
    {
      RecordError( 1 );
      return;
    }
    _inputScanPointer = exp;
    _parenthesesCount = 1;
    _codeEmitPointer = _symbolString;
    EmitNextByte( (char)RegExpSignature );
    if( ParseParens( 0, &flags ) == NULL ) 
    {
      RecordError( 1 );
      return;
    }
    _firstCharOfRegExp = RegExpCharZero;
    _isAnchored = 0;
    _requiredSubstring = NULL;
    _lengthOfRequiredSubstring = 0;
    char *scan = _symbolString + 1;
    if( RegExpChar2Opcode( GetNextPointer( scan ) ) == RegExpOpEnd ) 
    {
      scan = RegExpControllingOp( scan );
      if( RegExpChar2Opcode( scan ) == RegExpOpExact )
        _firstCharOfRegExp = *RegExpControllingOp( scan );
      else if( RegExpChar2Opcode( scan ) == RegExpOpBol )
        _isAnchored++;
      if( flags & RegExpBitOpensOnStarOrPlus ) 
      {
        char *longest = NULL;
        int len = 0;
        for( ; scan != NULL; scan = GetNextPointer( scan ) )
          if( RegExpChar2Opcode( scan ) == RegExpOpExact && ::strlen( RegExpControllingOp( scan ) ) >= len ) 
          {
            longest = RegExpControllingOp( scan );
            len = ::strlen( RegExpControllingOp( scan ) );
          }
          _requiredSubstring = longest;
          _lengthOfRequiredSubstring = len;
      }
    }
  }
  //  This method returns 1 if the regular expression
  //  was matched.  Otherwise it returns 0.
  int Match( T string ) 
  {
    T s;
    if( string == NULL ) 
    {
      RecordError( 0 );
      return( 0 );
    }
    if( RegExpAsUnsigned( _symbolString ) != RegExpSignature ) 
    {
      RecordError( 2 );
      return( 0 );
    }
    if( _requiredSubstring != NULL ) 
    {
      s = string;
      while( ( s = strchr( s, _requiredSubstring[ 0 ] ) ) != NULL ) 
      {
        if( strncmp( s, _requiredSubstring, _lengthOfRequiredSubstring ) == 0 )
          break;
        s++;
      }
      if( s == NULL )
        return( 0 );
    }
    _beginningOfInput = string;
    if( _isAnchored )
      return( EvaluateNext( string ) );
    s = string;
    if( _firstCharOfRegExp != RegExpCharZero )
      while( ( s = strchr( s, _firstCharOfRegExp ) ) != NULL ) 
      {
        if( EvaluateNext( s ) )
          return( 1 );
        s++;
      }
    else do 
    {
      if( EvaluateNext( s ) )
        return( 1 );
    } while( *s++ != RegExpCharZero );
    return( 0 );
  }
private:
  //----------------------------------------------------------
  //------------  THE PRIVATE INTERFACE BEGINS HERE  ---------
  //----------------------------------------------------------
  T _matchBeginsHere[ RegExpMaxSubExpressions ];
  T _matchEndsHere[ RegExpMaxSubExpressions ];
  char _firstCharOfRegExp;
  char _isAnchored;
  char *_requiredSubstring;
  int _lengthOfRequiredSubstring;
  char _symbolString[ RegExpMaxStore ];
  const char *_inputScanPointer;
  int _parenthesesCount;
  char _regExpShadow;
  char *_codeEmitPointer;
  long _symbolStringSize;
  T _inputStringPointer;
  T _beginningOfInput;
  T *_rembh;
  T *_remeh;
  char *_specialChars;
  //----------------------------------------------------------
  //----------------------------------------------------------
  void RecordError( int n ) 
  {
    //--------------------------------------------------------------
    //-------------  Error codes that can occur  -------------------
    //--------------------------------------------------------------
    //  NOTE:  These are the only string literals referenced within
    //         the class.  Modify to suit your language preference
    //         or desired level of obscurity.
    //
    char *RegExp_errors[] = 
    {
      "Invalid argument",
        "Insufficient expression",
        "Parse overload",
        "Mismatched () or []",
        "Extraneous characters",
        "Illegal use of * or +",
        "Illegal nesting",
        "Illegal [] range",
        "Illegal ?, +, or * usage",
        "Illegal backslashes",
        NULL
    };

    strcpy( _errorString, RegExp_errors[ n ] );
    _errorCode=n+1;
  }
  //----------------------------------------------------------
  char *ParseParens( int paren, int *flagp ) 
  {
    *flagp = RegExpBitMoreThenNull;
    char *ret = NULL;
    int parno = 0;
    if( paren ) 
    {
      if( _parenthesesCount >= RegExpMaxSubExpressions ) 
      {
        RecordError( 3 );
        return( NULL );
      }
      parno = _parenthesesCount;
      _parenthesesCount++;
      ret = EmitNode( RegExpOpOpen + parno );
    } else
      ret = NULL;
    int flags;
    char *br = OneSideOfOrOperator( &flags );
    if( br == NULL )
      return( NULL );
    if( ret != NULL )
      GoToEndOfChain( ret, br );
    else
      ret = br;
    if( !( flags & RegExpBitMoreThenNull ) )
      *flagp &= ~RegExpBitMoreThenNull;
    *flagp |= flags & RegExpBitOpensOnStarOrPlus;
    while( *_inputScanPointer == RegExpCharOr ) 
    {
      _inputScanPointer++;
      br = OneSideOfOrOperator( &flags );
      if( br == NULL )
        return( NULL );
      GoToEndOfChain( ret, br );
      if( !( flags&RegExpBitMoreThenNull ) )
        *flagp &= ~RegExpBitMoreThenNull;
      *flagp |= flags & RegExpBitOpensOnStarOrPlus;
    }
    char *ender = EmitNode( ( paren ) ? RegExpOpClose + parno : RegExpOpEnd );
    GoToEndOfChain( ret, ender );
    for( br = ret; br != NULL; br = GetNextPointer( br ) )
      GoToEndOfChainOnOperand( br, ender );
    if( paren && *_inputScanPointer++ != RegExpCharCloseParen ) 
    {
      RecordError( 3 );
      return( NULL );
    } else if( !paren && *_inputScanPointer != RegExpCharZero ) 
    {
      if( *_inputScanPointer == RegExpCharCloseParen ) 
      {
        RecordError( 3 );
        return( NULL );
      } else
        RecordError( 4 );
      return( NULL );
    }
    return( ret );
  }
  //----------------------------------------------------------
  char *OneSideOfOrOperator( int *flagp ) 
  {
    int flags = 0;
    char *ret = NULL;
    *flagp = RegExpBitCanMatchNothing;
    ret = EmitNode( RegExpOpBranch );
    char *chain = NULL;
    char *latest = NULL;
    while( *_inputScanPointer != RegExpCharZero && *_inputScanPointer != RegExpCharOr && *_inputScanPointer != RegExpCharCloseParen ) 
    {
      latest = TrailingWild( &flags );
      if( latest == NULL )
        return( NULL );
      *flagp |= flags & RegExpBitMoreThenNull;
      if( chain == NULL ) /* First piece. */
        *flagp |= flags & RegExpBitOpensOnStarOrPlus;
      else
        GoToEndOfChain( chain, latest );
      chain = latest;
    }
    if( chain == NULL ) /* Loop ran zero times. */
      (void) EmitNode( RegExpOpNMEmpty );
    return( ret );
  }
  //----------------------------------------------------------
  char *TrailingWild( int *flagp ) 
  {
    int flags = 0;
    char *ret = CompressOrdinaryCharacters( &flags );
    char *next = NULL;
    if( ret == NULL )
      return( NULL );
    char op = *_inputScanPointer;
    if( !RegExpIsConpound( op ) ) 
    {
      *flagp = flags;
      return( ret );
    }
    if( !( flags & RegExpBitMoreThenNull ) && op != RegExpCharQuestion ) 
    {
      RecordError( 5 );
      return( NULL );
    }
    *flagp =( op != RegExpCharPlus ) ?( RegExpBitCanMatchNothing | RegExpBitOpensOnStarOrPlus ) :
    ( RegExpBitCanMatchNothing | RegExpBitMoreThenNull );
    if( op == RegExpCharStar &&( flags & RegExpBitStarOrPlusOk ) )
      InsertOperator( RegExpOpStar, ret );
    else if( op == RegExpCharStar ) 
    {
      InsertOperator( RegExpOpBranch, ret );
      GoToEndOfChainOnOperand( ret, EmitNode( RegExpOpBack ) );
      GoToEndOfChainOnOperand( ret, ret );
      GoToEndOfChain( ret, EmitNode( RegExpOpBranch ) );
      GoToEndOfChain( ret, EmitNode( RegExpOpNMEmpty ) );
    } else if( op == RegExpCharPlus &&( flags&RegExpBitStarOrPlusOk ) )
      InsertOperator( RegExpOpPlus, ret );
    else if( op == RegExpCharPlus ) 
    {
      next = EmitNode( RegExpOpBranch );
      GoToEndOfChain( ret, next );
      GoToEndOfChain( EmitNode( RegExpOpBack ), ret );
      GoToEndOfChain( next, EmitNode( RegExpOpBranch ) );
      GoToEndOfChain( ret, EmitNode( RegExpOpNMEmpty ) );
    } else if( op == RegExpCharQuestion ) 
    {
      InsertOperator( RegExpOpBranch, ret );
      GoToEndOfChain( ret, EmitNode( RegExpOpBranch ) );
      next = EmitNode( RegExpOpNMEmpty );
      GoToEndOfChain( ret, next );
      GoToEndOfChainOnOperand( ret, next );
    }
    _inputScanPointer++;
    if( RegExpIsConpound( *_inputScanPointer ) ) 
    {
      RecordError( 6 );
      return( NULL );
    }
    return( ret );
  }
  //----------------------------------------------------------
  char *CompressOrdinaryCharacters( int *flagp ) 
  {
    char *ret = NULL;
    int flags = 0;
    *flagp = RegExpBitCanMatchNothing;
    switch( *_inputScanPointer++ ) 
    {
    case RegExpCharHoch:
      ret = EmitNode( RegExpOpBol );
      break;
    case RegExpCharDollar:
      ret = EmitNode( RegExpOpEol );
      break;
    case RegExpCharPeriod:
      ret = EmitNode( RegExpOpAny );
      *flagp |= RegExpBitMoreThenNull | RegExpBitStarOrPlusOk;
      break;
    case RegExpCharOpenBracket: 
      {
        int gjv_reg_class;
        int classend;
        if( *_inputScanPointer == RegExpCharHoch ) 
        {
          ret = EmitNode( RegExpOpAnyBut );
          _inputScanPointer++;
        } else
          ret = EmitNode( RegExpOpAnyOf );
        if( *_inputScanPointer == RegExpCharCloseBracker || *_inputScanPointer == RegExpCharMinus )
          EmitNextByte( *_inputScanPointer++ );
        while( *_inputScanPointer != RegExpCharZero && *_inputScanPointer != RegExpCharCloseBracker ) 
        {
          if (*_inputScanPointer == RegExpCharBackSlash && _inputScanPointer[1]!=RegExpCharCloseBracker && _inputScanPointer [1]!=RegExpCharZero )
          {
            _inputScanPointer++;
            switch (toupper(*_inputScanPointer))
            {
            case 'S': EmitNextByte ('\n');
              EmitNextByte ('\r');
              EmitNextByte ('\t');
              EmitNextByte ('\v');
              EmitNextByte (' ');
              EmitNextByte ('\a');
              EmitNextByte ('\b');
              EmitNextByte ('\f');
              break;
            case 'T':  EmitNextByte ('\t');break;
            case 'N':  EmitNextByte ('\n');break;
            case 'F':  EmitNextByte ('\f');break;
            case 'R':  EmitNextByte ('\r');break;
            case 'C':  
              {int i; for ( i=1;i<31;i++) EmitNextByte (i);
              }break;
            case '_':  EmitNextByte ('\t');
              EmitNextByte ('\v');
              EmitNextByte (' ');
              EmitNextByte ('\a');
              EmitNextByte ('\b');
              EmitNextByte ('\f');
              EmitNextByte ('\r');
              break;                
            case '\\':  EmitNextByte ('\\');break;                              
            default:    EmitNextByte (*_inputScanPointer);break;
            }                           
            _inputScanPointer++;
          }
          else  if( *_inputScanPointer == RegExpCharMinus ) 
          {
            _inputScanPointer++;
            if( *_inputScanPointer == RegExpCharCloseBracker || *_inputScanPointer == RegExpCharZero )
              EmitNextByte( RegExpCharMinus );
            else 
            {
              gjv_reg_class = RegExpAsUnsigned( _inputScanPointer - 2 ) + 1;
              classend = RegExpAsUnsigned( _inputScanPointer );
              if( gjv_reg_class > classend + 1 ) 
              {
                RecordError( 7 );
                return( NULL );
              }
              for( ; gjv_reg_class <= classend; gjv_reg_class++ )
                EmitNextByte( gjv_reg_class );
              _inputScanPointer++;
            }
          }
          else
            EmitNextByte( *_inputScanPointer++ );
        }
        EmitNextByte( RegExpCharZero );
        if( *_inputScanPointer != RegExpCharCloseBracker ) 
        {
          RecordError( 3 );
          return( NULL );
        }
        _inputScanPointer++;
        *flagp |= RegExpBitMoreThenNull | RegExpBitStarOrPlusOk; 
      }
      break;
    case RegExpCharUnderscore:
      ret=EmitNode( RegExpOpAnyOf );
      EmitNextByte ('\t');
      EmitNextByte ('\v');
      EmitNextByte (' ');
      EmitNextByte ('\a');
      EmitNextByte ('\b');
      EmitNextByte ('\f');
      EmitNextByte ('\r');
      EmitNextByte( RegExpCharZero );
      *flagp |= RegExpBitMoreThenNull | RegExpBitStarOrPlusOk;                     
      break;
    case RegExpCharOpenParen:
      ret = ParseParens( 1, &flags );
      if( ret == NULL )
        return( NULL );
      *flagp |= flags & ( RegExpBitMoreThenNull | RegExpBitOpensOnStarOrPlus );
      break;
    case RegExpCharZero:
    case RegExpCharOr:
    case RegExpCharCloseParen:
      RecordError( 2 );
      return( NULL );
    case RegExpCharQuestion:
    case RegExpCharPlus:
    case RegExpCharStar:
      RecordError( 8 );
      return( NULL );
    case RegExpCharBackSlash:
      if( *_inputScanPointer == RegExpCharZero ) 
      {
        RecordError( 9 );
        return( NULL );
      }
      ret = EmitNode( RegExpOpExact );
      EmitNextByte( *_inputScanPointer++ );
      EmitNextByte( RegExpCharZero );
      *flagp |= RegExpBitMoreThenNull | RegExpBitStarOrPlusOk;
      break;
    default: 
      {
        int len;
        char ender;
        _inputScanPointer--;
        len = strcspn( _inputScanPointer, _specialChars );
        if( len <= 0 ) 
        {
          RecordError( 2 );
          return( NULL );
        }
        ender = *( _inputScanPointer+len );
        if( len > 1 && RegExpIsConpound( ender ) )
          len--;
        *flagp |= RegExpBitMoreThenNull;
        if( len == 1 )
          *flagp |= RegExpBitStarOrPlusOk;
        ret = EmitNode( RegExpOpExact );
        while( len > 0 ) 
        {
          EmitNextByte( *_inputScanPointer++ );
          len--;
        }
        EmitNextByte( RegExpCharZero ); 
      }
      break;
    }
    return( ret );
  }
  //----------------------------------------------------------
  char *EmitNode( char op ) 
  {
    char *ret = _codeEmitPointer;
    if( ret == &_regExpShadow ) 
    {
      _symbolStringSize += 3;
      return( ret );
    }
    char *ptr = ret;
    *ptr++ = op;
    *ptr++ = RegExpCharZero;
    *ptr++ = RegExpCharZero;
    _codeEmitPointer = ptr;
    return( ret );
  }
  //----------------------------------------------------------
  void EmitNextByte( char b ) 
  {
    if( _codeEmitPointer != &_regExpShadow )
      *_codeEmitPointer++ = b;
    else
      _symbolStringSize++;
  }
  //----------------------------------------------------------
  void InsertOperator( char op, char *opnd ) 
  {
    if( _codeEmitPointer == &_regExpShadow ) 
    {
      _symbolStringSize += 3;
      return;
    }
    char *src = _codeEmitPointer;
    _codeEmitPointer += 3;
    char *dst = _codeEmitPointer;
    while( src > opnd )
      *--dst = *--src;
    char *place = opnd;
    *place++ = op;
    *place++ = RegExpCharZero;
    *place++ = RegExpCharZero;
  }
  //----------------------------------------------------------
  void GoToEndOfChain( char *p, char *val ) 
  {
    int offset = 0;
    if( p == &_regExpShadow )
      return;
    char *scan = p;
    for( ;; ) 
    {
      char *temp = GetNextPointer( scan );
      if( temp == NULL )
        break;
      scan = temp;
    }
    if( RegExpChar2Opcode( scan ) == RegExpOpBack )
      offset = scan - val;
    else
      offset = val - scan;
    *( scan + 1 ) = ( offset >> 8 ) & 0xFF;
    *( scan + 2 ) = offset & 0xFF;
  }
  //----------------------------------------------------------
  void GoToEndOfChainOnOperand( char *p, char *val ) 
  {
    if( p == NULL || p == &_regExpShadow || RegExpChar2Opcode( p ) != RegExpOpBranch )
      return;
    GoToEndOfChain( RegExpControllingOp( p ), val );
  }
  //----------------------------------------------------------
  int LookupEngine( char *prog ) 
  {
    char *scan = prog;
    char *next = NULL;
    while( scan != NULL ) 
    {
      next = GetNextPointer( scan );
      switch( RegExpChar2Opcode( scan ) ) 
      {
      case RegExpOpBol:
        if( _inputStringPointer != _beginningOfInput )
          return( 0 );
        break;
      case RegExpOpEol:
        if( *_inputStringPointer != RegExpCharZero )
          return( 0 );
        break;
      case RegExpOpAny:
        if( *_inputStringPointer == RegExpCharZero )
          return( 0 );
        _inputStringPointer++;
        break;
      case RegExpOpExact: 
        {
          int len;
          char *opnd;
          opnd = RegExpControllingOp( scan );
          if( *opnd != *_inputStringPointer )
            return( 0 );
          len = ::strlen( opnd );
          if( len > 1 && strncmp( opnd, _inputStringPointer, len ) != 0 )
            return( 0 );
          _inputStringPointer += len;
        }
        break;
      case RegExpOpAnyOf:
        if( *_inputStringPointer == RegExpCharZero || ::strchr( RegExpControllingOp( scan ), *_inputStringPointer ) == NULL )
          return( 0 );
        _inputStringPointer++;
        break;
      case RegExpOpAnyBut:
        if( *_inputStringPointer == RegExpCharZero || ::strchr( RegExpControllingOp( scan ), *_inputStringPointer ) != NULL )
          return( 0 );
        _inputStringPointer++;
        break;
      case RegExpOpNMEmpty:
        break;
      case RegExpOpBack:
        break;
      case RegExpOpOpen + 1:
      case RegExpOpOpen + 2:
      case RegExpOpOpen + 3:
      case RegExpOpOpen + 4:
      case RegExpOpOpen + 5:
      case RegExpOpOpen + 6:
      case RegExpOpOpen + 7:
      case RegExpOpOpen + 8:
      case RegExpOpOpen + 9: 
        {
          int no;
          T save;
          no = RegExpChar2Opcode( scan ) - RegExpOpOpen;
          save = _inputStringPointer;
          if( LookupEngine( next ) ) 
          {
            if( _rembh[ no ] == NULL )
              _rembh[ no ] = save;
            return( 1 );
          } else
            return( 0 );
        }
        break;
      case RegExpOpClose + 1:
      case RegExpOpClose + 2:
      case RegExpOpClose + 3:
      case RegExpOpClose + 4:
      case RegExpOpClose + 5:
      case RegExpOpClose + 6:
      case RegExpOpClose + 7:
      case RegExpOpClose + 8:
      case RegExpOpClose + 9: 
        {
          int no;
          T save;
          no = RegExpChar2Opcode( scan ) - RegExpOpClose;
          save = _inputStringPointer;
          if( LookupEngine( next ) ) 
          {
            if( _remeh[ no ] == NULL )
              _remeh[ no ] = save;
            return( 1 );
          } else
            return( 0 ); 
        }
        break;
      case RegExpOpBranch: 
        {
          T save;
          if( RegExpChar2Opcode( next ) != RegExpOpBranch )
            next = RegExpControllingOp( scan );
          else 
          {
            do 
            {
              save = _inputStringPointer;
              if( LookupEngine( RegExpControllingOp( scan ) ) )
                return( 1 );
              _inputStringPointer = save;
              scan = GetNextPointer( scan );
            } while( scan != NULL && RegExpChar2Opcode( scan ) == RegExpOpBranch );
            return( 0 );
          }
        }
        break;
      case RegExpOpStar:
      case RegExpOpPlus: 
        {
          char nextch;
          int no;
          T save;
          int min;
          nextch = RegExpCharZero;
          if( RegExpChar2Opcode( next ) == RegExpOpExact )
            nextch = *RegExpControllingOp( next );
          min = ( RegExpChar2Opcode( scan ) == RegExpOpStar ) ? 0 : 1;
          save = _inputStringPointer;
          no = WilldcardLookup( RegExpControllingOp( scan ) );
          while( no >= min ) 
          {
            if( nextch == RegExpCharZero || *_inputStringPointer == nextch )
              if( LookupEngine( next ) )
                return( 1 );
            no--;
            _inputStringPointer = save + no;
          }
          return( 0 );
        }
        break;
      case RegExpOpEnd:
        return( 1 );
      default:
        RecordError( 2 );
        return( 0 );
      }
      scan = next;
    }
    RecordError( 2 );
    return( 0 );
  }
  //----------------------------------------------------------
  int WilldcardLookup( char *p ) 
  {
    int count = 0;
    T scan = _inputStringPointer;
    char *opnd = RegExpControllingOp( p );
    switch( RegExpChar2Opcode( p ) ) 
    {
    case RegExpOpAny:
      count = strlen( scan );
      scan += count;
      break;
    case RegExpOpExact:
      while( *opnd == *scan ) 
      {
        count++;
        scan++;
      }
      break;
    case RegExpOpAnyOf:
      while( *scan != RegExpCharZero && ::strchr( opnd, *scan ) != NULL ) 
      {
        count++;
        scan++;
      }
      break;
    case RegExpOpAnyBut:
      while( *scan != RegExpCharZero && ::strchr( opnd, *scan ) == NULL ) 
      {
        count++;
        scan++;
      }
      break;
    default:
      RecordError( 2 );
      count = 0;
      return( NULL );
    }
    _inputStringPointer = scan;
    return( count );
  }
  //----------------------------------------------------------
  char *GetNextPointer( char *p )
  {
    if( p == &_regExpShadow )
      return( NULL );
    int offset = RegExpGetNext( p );
    if( offset == 0 )
      return( NULL );
    if( RegExpChar2Opcode( p ) == RegExpOpBack )
      return( p - offset );
    else
      return( p + offset );
  }
  //----------------------------------------------------------
  int EvaluateNext( T string ) 
  {
    _inputStringPointer = string;
    _rembh = _matchBeginsHere;
    _remeh = _matchEndsHere;
    T *sp = _matchBeginsHere;
    T *ep = _matchEndsHere;
    for( int i = RegExpMaxSubExpressions; i > 0; i-- ) 
    {
      *sp++ = NULL;
      *ep++ = NULL;
    }
    if( LookupEngine( _symbolString + 1 ) ) 
    {
      _matchBeginsHere[ 0 ] = string;
      _matchEndsHere[ 0 ] = _inputStringPointer;
      return( 1 );
    } else
      return( 0 );
  }
public:
  T GetBeginOfSubExpression(int level) const
  {return _matchBeginsHere[ level ];
  }
  T GetEndOfSubExpression(int level) const
  {return _matchEndsHere[ level ];
  }
};
//--------------------------------------------------------------
//---------------- END OF CLASS --------------------------------
//--------------------------------------------------------------
/*#ifdef __DEMO__
int main( )
{
char s[ 255 ];
//printf( "Hit F6 to quit\n" );
RegExp G( "c[A-Z]+[0-9]" );
RegExp W( "[0-9]+" );
while( fgets( s, 255, stdin ) ) {
int v = W.Match( s );
printf( "v = %d E = %s\n", v, W._errorString );
int a = G.Match( s );
printf( "a = %d E = %s\n", a, G._errorString );
}
return( 0 );
}
#endif

*/

#pragma warning (pop)
#endif