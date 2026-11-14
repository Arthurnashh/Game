namespace simul
{
//! The namespace for the sky library.

/*! The namespace corresponding to the Sky module. It contains the Sky, SkyNode and
SkyInterface classes, as well as simul::sky::TextureGenerator.

Static library: Simul/lib/(PLATFORM)/(COMPILER)/(ConfigurationName)/SimulSky_(RUNTIME).lib

Dynamic library: Simul/exe/(PLATFORM)/(COMPILER)/(ConfigurationName)/SimulSky_(RUNTIME).dll

The Sky library depends on the Base, Math, and Meta libraries.

Simul Sky is a library that calculates the colour gradations of the sky, including the
position of the sun, the colour of sunlight and variations in brightness with azimuth
and elevation.

Simul Sky is a lightweight library which is independent of your rendering solution -
it returns values that can be used to generate textures or gradients, or sent to shaders.
For example shaders which show how to render skies and sunlight using these values,
see \ref SkyRenderer.

Simul Sky integrates with the Simul Clouds library, which provides realistic volume-
rendering of various types of cloud.

To use Simul Sky, create a simul::sky::SkyNode object, and access it either directly or
through simul::sky::SkyInterface.
*/
	namespace sky
	{
	}
}
