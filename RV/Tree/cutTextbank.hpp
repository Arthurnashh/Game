#ifndef _cutTextBank_hpp_
#define _cutTextBank_hpp_

#include <Es/Strings/rString.hpp>
#include <Es/Types/lLinks.hpp>
#include "cutPactext.hpp"
//#include "colors.hpp"

#define MAX_MIPMAPS 7
// that would be enough for 256*256 texture
// last mipmap will have dimensions 4x4

class Texture: public RemoveLLinks
{
	// abstract class
	friend class AnimatedTexture;
	friend class AbstractTextBank;

	private:
	AnimatedTexture *_inAnimation;
	float _roughness; // for physical simulation
	float _dustness; // for physical simulation
	//int _special; // some special flags
	RStringB _name;
	RStringB _soundEnv;
	bool _refCountLocked;

	protected:
	int _mipmapWanted,_mipmapNeeded;

	public:

	Texture();

	void PrepareMipmap( int wanted, int needed );
	void ResetMipmap();

	Texture *GetAnimation( int i ) const;
	int AnimationLength() const;
	bool IsAnimated() const {return _inAnimation!=NULL;}
	AnimatedTexture *GetAnimatedTexture() const {return _inAnimation;}

	//! load all headers so any data-accessing function can be used
	virtual void LoadHeaders() {}

	virtual void SetMaxSize( int maxSize ) =NULL;
	virtual int AMaxSize() const =NULL;

	virtual int AWidth( int level=0 ) const =NULL;
	virtual int AHeight( int level=0 ) const =NULL;
	virtual int ANMipmaps() const=NULL;
	virtual AbstractMipmapLevel &AMipmap( int level ) =NULL;
	virtual const AbstractMipmapLevel &AMipmap( int level  ) const =NULL;
	virtual void ASetNMipmaps( int n ) =NULL;
	virtual Color GetPixel( int level, float u, float v ) const = NULL;
	virtual bool IsTransparent() const = NULL;
	virtual Color GetColor() = NULL;
	virtual bool IsAlpha() const = NULL;

	// some APIs (Glide) require u,v conversion
	virtual float UToPhysical( float u ) const {return u;}
	virtual float VToPhysical( float v ) const {return v;}

	virtual float UToLogical( float u ) const {return u;}
	virtual float VToLogical( float v ) const {return v;}

	virtual void SetMipmapRange( int min, int max ){}

	virtual bool VerifyChecksum( const MipInfo &mip ) const = NULL; // verify consistency

	const char *Name() const {return _name;}
	const RStringB &GetName() const {return _name;}
	void SetName( RStringB name );

	float Roughness() const {return _roughness;} // for physical simulation
	void SetRoughness( float roughness ) {_roughness=roughness;}

	float Dustness() const {return _dustness;}
	void SetDustness( float dustness ) {_dustness=dustness;}

	RStringB GetSoundEnv() const {return _soundEnv;}
	
	virtual void SetMultitexturing( int type ) {}
	
	virtual ~Texture();

	protected:
	void Lock();
	void Unlock();

	public:
	NoCopy(Texture)
};

// each texture bank knows type of its textures
// overloads function "Load" with different return type (if neccessary")
// casts parameter of function UseMipmap to know type - this is potentially unsafe

struct SurfaceInfo
{
	RStringB _name;
	float _roughness;
	float _dustness;
	RStringB _soundEnv;
	bool operator == ( const SurfaceInfo &with ) const
	{
		return _name==with._name;
	}
};

TypeIsMovable(SurfaceInfo);

class QFBank;

class AbstractTextBank
{
	friend class Texture;

	FindArray<SurfaceInfo> _surfaces;
	RefArray<AnimatedTexture> _animatedTextures;

	public:
		
	AnimatedTexture *LoadAnimated( RStringB name );
	void DeleteAnimated( AnimatedTexture *texture );
	void DeleteAllAnimated();

	virtual Ref<Texture> Load( RStringB text )=NULL;
	virtual Ref<Texture> LoadInterpolated( RStringB n1, RStringB n2, float factor )=NULL;
	virtual void AddGamma( float g ) {}

	virtual int NTextures() const = NULL;
	virtual Texture *GetTexture( int i ) const = NULL;

	void PrepareMipmap
	(
		Texture *texture, int level, int levelTop
	)
	{
		// hint - if we will use mipmap, we will want this one
		texture->PrepareMipmap(level,levelTop);
	}
	virtual MipInfo UseMipmap
	(
		Texture *texture, int level, int levelTop
	) = NULL; // request - we want this some mipmap
	
	virtual void Compact() = NULL;
	virtual void Preload() = NULL;
	virtual void FlushTextures() = NULL; // flush temporary data
	virtual void FlushBank(QFBank *bank) = NULL;
	
	virtual void StartFrame();
	virtual void FinishFrame();
	virtual bool NeedUVConversion() const {return false;}

	virtual bool VerifyChecksums() {return true;}

	static int AnimatedName( const char *name, char *prefix, char *postfix );
	static int AnimatedNumber( const char *name );

	// state toggle - no nesting neccessary

	protected:
	int FindSurface( const char *name ) const; 	// pattern matching ('?')
	const SurfaceInfo &GetSurface( const char *name ) const;



	public:
	AbstractTextBank();
	virtual ~AbstractTextBank();

	void LockAllTextures(); // disable automatic release
	void UnlockAllTextures(); // enable automatic release


	NoCopy(AbstractTextBank)
};


Texture *GlobPreloadTexture( RStringB name );
Ref<Texture> GlobLoadTexture( RStringB name );
Ref<Texture> GlobLoadTextureInterpolated(  RStringB n1, RStringB n2, float factor );
AnimatedTexture *GlobLoadTextureAnimated( RStringB name );

class AnimatedTexture: public RefArray<Texture>,public RefCount
{
	AbstractTextBank *_bank;

	public:
	AnimatedTexture( AbstractTextBank *bank );
	~AnimatedTexture();

	void Remove( Texture *text );
	const char *Name() const; // name of first texture
	RStringB GetName() const; // name of first texture

	private:
	void operator = ( const AnimatedTexture &src );
	AnimatedTexture( const AnimatedTexture &src );
};

extern LLink<Texture> DefaultTexture;

#endif

