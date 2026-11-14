#ifndef _WIN32

#include "wpch.hpp"
#include "soundOAL.hpp"

static void OALDummyFail()
{
  Fail("OAL API function was called, which should not happen on this platform!");
}

///al.h
AL_API void AL_APIENTRY alGenSources( ALsizei n, ALuint* sources )
{
  OALDummyFail();
}

AL_API void AL_APIENTRY alDeleteSources( ALsizei n, const ALuint* sources )
{
  OALDummyFail();
}

AL_API void AL_APIENTRY alSourcef( ALuint sid, ALenum param, ALfloat value )
{
  OALDummyFail();
}

AL_API void AL_APIENTRY alSource3f( ALuint sid, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3 )
{
  OALDummyFail();
}

AL_API void AL_APIENTRY alSourcei( ALuint sid, ALenum param, ALint value )
{
  OALDummyFail();
}

AL_API void AL_APIENTRY alGetSourcei( ALuint sid,  ALenum param, ALint* value )
{
  OALDummyFail();
}

AL_API void AL_APIENTRY alSourcePlay( ALuint sid )
{
  OALDummyFail();
}

AL_API void AL_APIENTRY alSourceStop( ALuint sid )
{
  OALDummyFail();
}

AL_API void AL_APIENTRY alSourceRewind( ALuint sid )
{
  OALDummyFail();
}

AL_API void AL_APIENTRY alSourcePause( ALuint sid )
{
  OALDummyFail();
}

AL_API void AL_APIENTRY alSourceQueueBuffers( ALuint sid, ALsizei numEntries, const ALuint *bids )
{
  OALDummyFail();
}

AL_API void AL_APIENTRY alSourceUnqueueBuffers( ALuint sid, ALsizei numEntries, ALuint *bids )
{
  OALDummyFail();
}


AL_API void AL_APIENTRY alGenBuffers( ALsizei n, ALuint* buffers )
{
  OALDummyFail();
}

AL_API void AL_APIENTRY alDeleteBuffers( ALsizei n, const ALuint* buffers )
{
  OALDummyFail();
}

AL_API void AL_APIENTRY alBufferData( ALuint bid, ALenum format, const ALvoid* data, ALsizei size, ALsizei freq )
{
  OALDummyFail();
}

AL_API void AL_APIENTRY alDopplerFactor( ALfloat value )
{
  OALDummyFail();
}

AL_API void AL_APIENTRY alDopplerVelocity( ALfloat value )
{
  OALDummyFail();
}

AL_API void AL_APIENTRY alSpeedOfSound( ALfloat value )
{
  OALDummyFail();
}

AL_API void AL_APIENTRY alDistanceModel( ALenum distanceModel )
{
  OALDummyFail();
}

AL_API void* AL_APIENTRY alGetProcAddress( const ALchar* fname )
{
  OALDummyFail();
}

AL_API ALenum AL_APIENTRY alGetError( void )
{
  OALDummyFail();
}

AL_API void AL_APIENTRY alListenerfv( ALenum param, const ALfloat* values )
{
  OALDummyFail();
}

AL_API void AL_APIENTRY alListener3f( ALenum param, ALfloat value1, ALfloat value2, ALfloat value3 )
{
  OALDummyFail();
}

AL_API ALboolean AL_APIENTRY alIsExtensionPresent( const ALchar* extname )
{
  OALDummyFail();
}

///alc.h

ALC_API ALCcontext *    ALC_APIENTRY alcCreateContext( ALCdevice *device, const ALCint* attrlist )
{
  OALDummyFail();
}

ALC_API ALCboolean      ALC_APIENTRY alcMakeContextCurrent( ALCcontext *context )
{
  OALDummyFail();
}

ALC_API void            ALC_APIENTRY alcProcessContext( ALCcontext *context )
{
  OALDummyFail();
}

ALC_API void            ALC_APIENTRY alcSuspendContext( ALCcontext *context )
{
  OALDummyFail();
}

ALC_API void            ALC_APIENTRY alcDestroyContext( ALCcontext *context )
{
  OALDummyFail();
}

ALC_API ALCcontext *    ALC_APIENTRY alcGetCurrentContext( )
{
  OALDummyFail();
}

ALC_API ALCdevice*      ALC_APIENTRY alcGetContextsDevice( ALCcontext *context )
{
  OALDummyFail();
}

ALC_API ALCdevice *     ALC_APIENTRY alcOpenDevice( const ALCchar *devicename )
{
  OALDummyFail();
}

ALC_API ALCboolean      ALC_APIENTRY alcCloseDevice( ALCdevice *device )
{
  OALDummyFail();
}

ALC_API const ALCchar * ALC_APIENTRY alcGetString( ALCdevice *device, ALCenum param )
{
  OALDummyFail();
}

ALC_API void            ALC_APIENTRY alcGetIntegerv( ALCdevice *device, ALCenum param, ALCsizei size, ALCint *data )
{
  OALDummyFail();
}

ALC_API ALCboolean      ALC_APIENTRY alcIsExtensionPresent( ALCdevice *device, const ALCchar *extname )
{
  OALDummyFail();
}

ALC_API ALCenum         ALC_APIENTRY alcGetError( ALCdevice *device )
{
  OALDummyFail();
}

#endif
