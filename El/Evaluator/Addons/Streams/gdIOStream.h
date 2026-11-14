#pragma once
#include <el\evaluator\express.hpp>
#include "archivestream.h"
#include "ctWeakRef.h"

using namespace std;

#define TYPES_IOSTREAM(XX, Category) \
  XX("IOStream",EvalType_IOStream,CreateIOStream ,"@IOStream","IOStream","Handles various input/output streams, such as file, network, memory and console streams and also enables simple text file parsing",Category)\

TYPES_IOSTREAM(DECLARE_TYPE, "IOStream")

typedef wkRefCount<ArchiveStream> ArchiveStreamRef;

class GdIOStream: public RefCount {

    friend class GdIOStreamV;

    Ref<ArchiveStreamRef> _streamIn;
    Ref<ArchiveStreamRef> _streamOut;  
    RString _debugText;
    ctWeakRef<ArchiveStream> _inBuff;  //input pre-buffer, to buffer token while parsing. ArchiveStreamMemory is used
    int _posmax; //maximum position read

public:
    void SetStreams(
      Ref<ArchiveStreamRef> streamIn,
    Ref<ArchiveStreamRef> streamOut)
    {
      _streamIn=streamIn;_streamOut=streamOut;
    }

    bool CreateConsole();                             //newStream ["?/console",0]
    bool CreateErrorConsole();                         //newStream ["?/console",1]
    bool CreateFileIO(int mode, const char *filename);  //newStream ["filename",mode]
    bool CreateMemoryStream(const char *inbuffer);    //newStream ["?/memory",input_buffer]
    bool CreateTCPIPStream(const char *server, int timeout);     //file ["?/network/server:port",<contimeout>]
    bool CreateTCPIPStream(int port, int timeout);  //newStream ["?/network/*:port",<contimeout>]
    bool CreatePipeStream(const char *filename);
    
    bool TestRegExp(const char *pattern, SRef<char> &result); //tests input regular expression. Character $ is automatically inserted before pattern
    void ReadInput(int characters, char *output); //fills buffer output with readed data, if output is null, skips data. Buffer must be large enought. Use ReadInputCountChars to get buffer length
    int ReadInputCountChars(int characters); //returns buffer size
    void ResetInputCmpEnd() 
    {_posmax=0;
    }
    void CorrectPosMax(int diff) 
    {_posmax-=diff;if (_posmax<0) _posmax=0;
    }

    char operator[] (int pos);  //for stream_char_interface - reading characters from buffer
    char operator[] (int pos) const 
    {
      return const_cast<GdIOStream *>(this)->operator[](pos);
    }
    Ref<ArchiveStreamRef> GetInputStream() 
    {return _streamIn;
    }
    Ref<ArchiveStreamRef> GetOutputStream() 
    {return _streamOut;
    }

    USE_FAST_ALLOCATOR;
};

class GdIOStreamV : public GameData
{
    Ref<GdIOStream> _stream;

  public:
    GdIOStreamV(void) {_stream = new GdIOStream();}
    GdIOStreamV(GdIOStream *stream):_stream(stream) {}
    
    void SetStreams(
      Ref<ArchiveStreamRef> streamIn,
    Ref<ArchiveStreamRef> streamOut)
    {
      _stream->SetStreams(streamIn,streamOut);
    }

    GdIOStream *GetStream() {return _stream;}
    
    const GameType &GetType() const 
    {return EvalType_IOStream;
    }
    RString GetText() const;
    
    virtual GameBoolType GetBool() const 
    {return false;
    }
    virtual GameScalarType GetScalar() const 
    {return 0;
    }
    virtual GameStringType GetString() const 
    {return "";
    }
    /*  virtual const GameArrayType &GetArray() const;*/
    
    bool IsEqualTo(const GameData *data) const;
    const char *GetTypeName() const 
    {return "IOStream";
    }
    GameData *Clone() const 
    {return new GdIOStreamV (*this);
    }
    Ref<ArchiveStreamRef> GetInputStream() 
    {return _stream->_streamIn;
    }
    Ref<ArchiveStreamRef> GetOutputStream() 
    {return _stream->_streamOut;
    }
    
    USE_FAST_ALLOCATOR;
    
    static void RegisterToGameState(GameState *gState);
};


