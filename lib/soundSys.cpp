#include "wpch.hpp"

#include "soundsys.hpp"

AbstractWave::AbstractWave( RString name )
:_name(name),_sticky(false), _autoUpdateEnable(false)
{
  debugMe = false;
	_onTerminate = NULL;
	_onPlay = NULL;
}

AbstractWave::~AbstractWave()
{
}

void AbstractWave::OnTerminateOnce()
{
	if (!_onTerminate) return;
	_onTerminate(this,_onTerminateContext);
	_onTerminate = NULL;
	_onTerminateContext.Free();
}

void AbstractWave::OnPlayOnce()
{
	if (!_onPlay) return;
	_onPlay(this,_onPlayContext);
	_onPlay = NULL;
	_onPlayContext.Free();
}

void AbstractWave::PreloadQueue()
{
}
