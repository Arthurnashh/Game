#include "El/elementpch.hpp"

//#include "global.hpp"
#include "cutPactext.hpp"
#include "cutTextbank.hpp"
#include "cutVertex.hpp"
#include <Es/Types/pointers.hpp>
#include <Es/Containers/staticArray.hpp>
#include <Es/Files/filenames.hpp>
#include "El/QStream/QBStream.hpp"

typedef word Pixel;

inline Pixel Conv888To555( int rgb )
{
	int r=(rgb>>16)&0xff,g=(rgb>>8)&0xff,b=(rgb>>0)&0xff;
	r>>=3,g>>=3,b>>=3;
	return Pixel( (r<<10)|(g<<5)|(b<<0) );
}

inline Pixel Conv888To565( int rgb )
{
	int r=(rgb>>16)&0xff,g=(rgb>>8)&0xff,b=(rgb>>0)&0xff;
	r>>=3,g>>=2,b>>=3;
	return Pixel( (r<<11)|(g<<5)|(b<<0) );
}

struct DXTBlock64
{
	// see "Compressed Texture Formats" in DX Docs
	word c0,c1; // color data
	word tex0,tex1; // texel data
};

#ifdef _MSC_VER
  #define TAGG 'TAGG'
  #define AVGC 'AVGC'
  #define FLAG 'FLAG'
  #define OFFS 'OFFS'
#else
  #define TAGG ((int)'G'+((int)'G'<<8)+((int)'A'<<16)+((int)'T'<<24))
  #define AVGC ((int)'C'+((int)'G'<<8)+((int)'V'<<16)+((int)'A'<<24))
  #define FLAG ((int)'G'+((int)'A'<<8)+((int)'L'<<16)+((int)'F'<<24))
  #define OFFS ((int)'S'+((int)'F'<<8)+((int)'F'<<16)+((int)'O'<<24))
#endif

// load texture map from PAC file

int PacPalette::Skip( QIStream &in )
{
	// each texture file starts with palette definition
	// return<0 - error
	// return==0 - texture loaded successfully
	int error=-1;
	
	for(;;)
	{
		// skip any taggs
		DWORD magic=fgetil(in);
		if( magic!=TAGG )
                {in.seekg(-4,QIOS::cur);break;}
		// some tagg - load it
		DWORD tagg=fgetil(in);
		(void)tagg;
		int size=fgetil(in);
		Assert( size>=0 );
		in.seekg(size,QIOS::cur);
		if( in.eof() || in.fail() ) goto Error;
	}

	enum {psize=256};

	// get palette  size
	long C;
	C=fgetiw(in);
	if( in.eof() || in.fail() ) goto Error;

	if( C<0 || C>psize ) {error=1;goto Error;} // mip-map list terminator

	in.seekg(3*C,QIOS::cur);
	if( in.eof() || in.fail() ) goto Error;
		
	error=0;
		
	Error:
	return error;
}

PacPalette::PacPalette()
:_nColors(0),_palette(NULL), //_palPixel(NULL),
_transparentColor(-1),_averageColor(HBlack),_averageColor32(0xff000000),
_isTransparent(false),_isAlpha(false)
{
}

PacPalette::~PacPalette()
{
	if( _palette ) delete[] _palette;
	//if( _palPixel ) delete[] _palPixel;
}

enum PicFlags
{
	PicFlagAlpha=1,
	PicFlagTransparent=2,
};

int PacPalette::Load( QIStream &in, int *startOffsets, int maxStartOffsets )
{
	int error=-1;
	// each texture file starts with palette definition
	// return<0 - error
	// return==0 - texture loaded successfully

	_averageColor32=PackedColor(0); // default - transparent
	for(;;)
	{
		// skip/load any taggs
		DWORD magic=fgetil(in);
		if( magic!=TAGG )
		{in.seekg(-4,QIOS::cur);break;}
		// some tagg - load it
		DWORD tagg=fgetil(in);
		int size=fgetil(in);
		if( in.eof() || in.fail() ) goto Error;
		Assert( size>=0 );
		switch( tagg )
		{
			case AVGC:
				Assert( size==sizeof(_averageColor32) );
				_averageColor32=PackedColor(fgetil(in));
			break;
			case FLAG:
			{
				int flags = fgetil(in);
				if (flags&PicFlagAlpha) _isAlpha = true;
				if (flags&PicFlagTransparent) _isTransparent = true;
			}
			break;
			case OFFS:
			{
				int nOffs = size/sizeof(int);
				for (int i=0; i<nOffs; i++)
				{
					int data = fgetil(in);
					if (i<maxStartOffsets) startOffsets[i]=data;
				}
			}	
			break;
			default:
				in.seekg(size,QIOS::cur);
			break;
		}
	}
	_averageColor=Color((long)_averageColor32);
	// load PAC file
	enum {psize=256};
	int i;

	// get palette  size
	long C;
	C=fgetiw(in);

	if( C<0 || C>psize ) {error=1;goto Error;} // mip-map list terminator

	_nColors=C;
	//_heap=heap;
	
	if( !_palette && _nColors>0) _palette=new DWORD[_nColors]; // packed 24-bit color (palette)
	//if( !_palPixel && _nColors>0 ) _palPixel=new Pixel[_nColors]; // packed 24-bit color (palette)

	_transparentColor=-1;
	if( _palette || _nColors==0 )
	{		
		for( i=0; i<C; i++ )
		{
			_palette[i]=fgeti24(in);
			//_palPixel[i]=Conv888To555(_palette[i]);
		}

		if( in.eof() || in.fail() ) goto Error;
		
		// check if texture contains transparent color
		for( i=0; i<_nColors; i++ )
		{
			if( _palette[i]==TRANSPARENT1_RGB ) _transparentColor=i;
			if( _palette[i]==TRANSPARENT2_RGB ) _transparentColor=i;
		}
		
		error=0;
		
		if( _transparentColor>=0 )
		{
			// convert transparent color to average texture color
			DWORD avg=_averageColor32;
			avg&=0xffffff; // transparent
			_palette[_transparentColor]=avg;
			_isTransparent = true;
		}
	}


	if (_averageColor32>0)
	{
		// some value loaded - it should be correct
		if ((_averageColor32&0xff000000)==0xff000000)
		{
			// it is not transparent, but is assumed to be
			_isTransparent = false;
		}
		else
		{
			// it should be alpha or transparent
			if (!_isAlpha) _isTransparent = true;
		}
	}
	/*
	// check transparency by averageColor?
	if (_isTransparent)
	{
		LogF("  T Average color A %.3f",_averageColor.A());
	}
	else
	{
		LogF("  O Average color A %.3f",_averageColor.A());
	}
	*/

	Error:
	if( error<0 )
	{
		//if( _palPixel ) delete[] _palPixel,_palPixel=NULL;
		if( _palette ) delete[] _palette,_palette=NULL;
		_nColors=0;
	}
	return error;
}

inline int ConvColorTo888( ColorVal color )
{
	int r=toIntFloor(color.R()*255);
	int g=toIntFloor(color.G()*255);
	int b=toIntFloor(color.B()*255);
	return (r<<16)|(g<<8)|(b<<0);
}

#define PAA_4444 0x4444
#define PAA_8080 0x8080
#define PAA_1555 0x1555
#define PAA_DXT1 0xff01
#define PAA_DXT2 0xff02
#define PAA_DXT3 0xff03
#define PAA_DXT4 0xff04
#define PAA_DXT5 0xff05

PacFormat PacFormatFromDesc( int desc, bool &alpha )
{
	PacFormat format = PacFormatN;
	alpha = false;
	if( desc==PAA_8080 ) format=PacAI88, alpha=true;
	else if( desc==PAA_4444 ) format=PacARGB4444, alpha=true;
	else if( desc==PAA_1555 ) format=PacARGB1555;
	else if( desc==PAA_DXT1 ) format=PacDXT1;
	else if( desc==PAA_DXT2 ) format=PacDXT2;
	else if( desc==PAA_DXT3 ) format=PacDXT3;
	else if( desc==PAA_DXT4 ) format=PacDXT4;
	else if( desc==PAA_DXT5 ) format=PacDXT5;
	return format;
}


PacLevelMem::PacLevelMem()
:
//_memData(NULL),
_w(0),_h(0),
_pitch(0),_sFormat(PacP8),_dFormat(PacARGB1555),
_start(-1) // invalid offset
{
}

// general LZW decompression routine

#define N 4096

#define N		 4096 // textbuffer length
#define F		  18 // max. match len
#define THRESHOLD 2 // min. match len


static int DecodeLZW
(
	QIStream &in, char *dst, long lensb, int byteW, int pitch,
	Pixel *resPal=NULL, const PacPalette *pal=NULL
)
{
	if( lensb<=0 ) return 0;

	char text_buf[N + F - 1];
	int i,j,r,c,csum=0,csr;
	int flags;
	int lineCnt=byteW;
	int lineAlign=pitch-byteW;
	int pSize=( resPal ? sizeof(Pixel) : sizeof(char) );
	//Pixel lastVV=0;
	//for( i=0; i<N-F; i++ ) text_buf[i] = ' ';
	memset(text_buf,' ',N-F);
	r=N-F; flags=0;
	while( lensb>0 )
	{
		if( ((flags>>= 1)&256)==0 )
		{
			c=in.get();
			flags=c|0xff00;
		}
		if( in.fail() || in.eof() )
		{
			Fail("LZW: stream read failed");
			return -1;
		}
		if( flags&1 )
		{
			c=in.get();
			if( in.fail() || in.eof() )
			{
				Fail("LZW: stream read failed");
				return -1;
			}
			csum+=(char)c;
			// save pixel
			if( !resPal ) *dst=c;
			else
			{
				//if( c==pal->_transparentColor ) *(word *)dst=lastVV&0x7fff;
				//else lastVV=
				*(word *)dst=resPal[c];
			}
			dst+=pSize;
			lensb--;
			lineCnt-=pSize;
			if( lineCnt==0 ) dst+=lineAlign,lineCnt=byteW;
			// continue decompression
			text_buf[r]=(char)c;
			r++;r&=(N-1);
		}
		else
		{
			i=in.get();
			j=in.get();
			if( in.fail() || in.eof() )
			{
				Fail("LZW: stream read failed");
				return -1;
			}
			i|=(j&0xf0)<<4; j&=0x0f; j+=THRESHOLD;
			for( i=r-i,j+=i; i<=j; i++ )
			{
				c=(byte)text_buf[i&(N-1)];
				csum+=(char)c;
				// save pixel
				if( !resPal ) *dst=c;
				else
				{
					//if( c==pal->_transparentColor ) *(word *)dst=lastVV&0x7fff;
					//else lastVV=
					*(word *)dst=resPal[c];
				}
				dst+=pSize;
				lensb--;
				lineCnt-=pSize;
				if( lineCnt==0 ) dst+=lineAlign,lineCnt=byteW;
				// continue decompression
				text_buf[r]=(char)c;
				r++;r&=(N-1);
			}
		}
	}
	in.read((char *)&csr,sizeof(csr));
	if( in.fail() || in.eof() )
	{
		Fail("LZW: end of stream");
		return -1;
	}
	if( csr!=csum )
	{
		Fail("Checksum");
		return -1;
	}
	return 0;
}


/*
bool PacLevelMem::Check( int max ) const
{
	bool error=false;
	char *mem=(char *)_memData;
	int size=_w*_h;
	while( --size>=0 )
	{
		int c=(unsigned char)*mem++;
		if( c>=max ) error=true,mem[-1]=max-1;
	}
	return !error;
}
*/

#define MAGIC_W_LZW 1234
#define MAGIC_H_LZW 8765


int PacLevelMem::LoadPacP8( QIStream &in, void *mem, const PacPalette *pal ) const
{
	// return<0 - error
	// return>0 - no more mip-map levels available
	// return==0 - texture loaded successfully

	// load PAC file
	// get image size
	{
		long w=fgetiw(in);
		long h=fgetiw(in);
		if( w==0 && h==0 ) {return 1;} // mip-map list terminator
		
		if( w==MAGIC_W_LZW && h==MAGIC_H_LZW )
		{
			w=fgetiw(in);
			h=fgetiw(in);
			
			long dSize=fgeti24(in);
			(void)dSize;
			
			//if( w!=h ) goto Error; // square textures only
			if( w>4096 || w<2 )
			{
				Fail("Size out of range");
				return -1;
			}
			
			Assert( _w==w );
			Assert( _h==h );

			if( DecodeLZW(in,(char *)mem,_w*_h,_w,_pitch)<0 )
			{
				Fail("LZW Decode error.");
				return -1;
			}
		}
		else
		{
			//Fail("Old PAC Format.");
			// get compressed data size
			long dSize=fgeti24(in);
			(void)dSize;
			
			long W=w*h;
		
			//if( w!=h ) goto Error; // square textures only
			if( w>4096 || w<2 )
			{
				Fail("Size out of range");
				return -1;
			}
			
			Assert( _w==w );
			Assert( _h==h );

			int lineCnt=_w;
			int lineAlign=_pitch-_w;
			
			// resulting surface is pallete - simple copy
			byte *dst=(byte *)mem;
			while( W>0 )
			{
				int c=in.get();
				if( c&0x80 )
				{
					int v=in.get();
					c&=0x7f;
					c++;
					W-=c;
					if( lineAlign==0 )
					{
						while( --c>=0 ) *dst++=(byte)v;
					}
					else
					{
						while( --c>=0 )
						{
							*dst++=(byte)v;
							if( --lineCnt==0 ) dst+=lineAlign,lineCnt=w;
						}
					}
				}
				else
				{
					c++;
					W-=c;
					while( --c>=0 )
					{
						int v=in.get();
						*dst++=(byte)v;
						if( --lineCnt==0 ) dst+=lineAlign,lineCnt=w;
					}
				}
			}
			//Assert( Check(*pal) );
			//_heap->Lock();
		}
		if( in.fail() || in.eof() )
		{
			Fail("Stream Fail error.");
			return -1;
		}
		
		//_heap->Unlock();
		
		return 0;
	}
	
	//Error:
	//return error;
}

int PacLevelMem::LoadPacARGB1555( QIStream &in, void *mem, const PacPalette *pal ) const
{
	// picture is saved in Palette 8 format
	// load as raw ARGB 1555 data
	// return<0 - error
	// return>0 - no more mip-map levels available
	// return==0 - texture loaded successfully

	// load PAC file
	
	// resulting surface is 16b-rgb
	// prepare a palette for conversion
	//Temp<word> resPal(pal->_nColors);
	word resPal[256];
	Assert( pal->_nColors<=256 );
	int i;
	for( i=0; i<pal->_nColors; i++ )
	{
		word rgb=Conv888To555(pal->_palette[i]);
		if( i!=pal->_transparentColor ) rgb|=0x8000;
		//rgb&=0x1f;
		resPal[i]=rgb;
	}
	
	// get image size
	{
		long w=fgetiw(in);
		long h=fgetiw(in);
		if( w==0 && h==0 ) {return 1;} // mip-map list terminator

		if( w==MAGIC_W_LZW && h==MAGIC_H_LZW )
		{
			w=fgetiw(in);
			h=fgetiw(in);

			long dSize=fgeti24(in);
			(void)dSize;

			if( w>4096 || w<2 )
			{
				Fail("Size out of range");
				return -1;
			}
			
			Assert( _w==w );
			Assert( _h==h );

			// skip actual image data
			//in.seekg(dSize,QIOS::cur);
			//memset(mem,0xff,_pitch*_h);
			if( DecodeLZW(in,(char *)mem,_w*_h,_w*2,_pitch,resPal,pal)<0 )
			{
				Fail("LZW Decode error");
				return -1;
			}
		}
		else
		{
			// get compressed data size
			//Fail("Old PAC Format.");
			long dSize=fgeti24(in);
			(void)dSize;
			
			long W=w*h;
			
			//if( w!=h ) goto Error; // square textures only
			if( w>4096 || w<2 )
			{
				Fail("Size out of range");
				return -1;
			}
			
			//_w=w,_h=h;
			Assert( _w==w );
			Assert( _h==h );

			int lineCnt=w;
			int lineAlign=_pitch-w*2;
			
			word *B=(word *)mem;
			//int lastVV=0; // start with black
			while( W>0 )
			{
				int c=in.get();
				if( c&0x80 )
				{
					int v=in.get();
					c&=0x7f;
					c++;
					W-=c;
					word vv;
					//if( v==pal->_transparentColor ) vv=lastVV&0x7fff;
					//else lastVV=
					vv=resPal[v];
					if( lineAlign==0 )
					{
						while( --c>=0 ) *B++=vv;
					}
					else
					{
						while( --c>=0 )
						{
							*B++=vv;
							if( --lineCnt==0 ) B=(word *)((byte *)B+lineAlign),lineCnt=w;
						}
					}
				}
				else
				{
					c++;
					W-=c;
					while( --c>=0 )
					{
						int v=in.get();
						word vv;
						//if( v==pal->_transparentColor ) vv=lastVV&0x7fff;
						//else lastVV=
						vv=resPal[v];
						*B++=vv;
						if( --lineCnt==0 ) B=(word *)((byte *)B+lineAlign),lineCnt=w;
					}
				}
			}
		}
		
		if( in.fail() || in.eof() )
		{
			Fail("Texture load failed.");
			return -1;
		}
		
		return 0;
	}
	
	//Error:
	//return error;
}


int PacLevelMem::LoadPacRGB565( QIStream &in, void *mem, const PacPalette *pal ) const
{
	// picture is saved in Palette 8 format
	// load as raw RGB 565 data
	// return<0 - error
	// return>0 - no more mip-map levels available
	// return==0 - texture loaded successfully

	// load PAC file
	
	// resulting surface is 16b-rgb
	// prepare a palette for conversion
	//Temp<word> resPal(pal->_nColors);
	word resPal[256];
	Assert( pal->_nColors<=256 );
	Assert( pal->_transparentColor<0 );
	int i;
	for( i=0; i<pal->_nColors; i++ )
	{
		word rgb=Conv888To565(pal->_palette[i]);
		//rgb&=0x1f;
		resPal[i]=rgb;
	}
	
	// get image size
	{
		long w=fgetiw(in);
		long h=fgetiw(in);
		if( w==0 && h==0 ) {return 1;} // mip-map list terminator

		if( w==MAGIC_W_LZW && h==MAGIC_H_LZW )
		{
			w=fgetiw(in);
			h=fgetiw(in);

			long dSize=fgeti24(in);
			(void)dSize;

			if( w>4096 || w<2 )
			{
				Fail("Size out of range");
				return -1;
			}
			
			Assert( _w==w );
			Assert( _h==h );

			// skip actual image data
			//in.seekg(dSize,QIOS::cur);
			//memset(mem,0xff,_pitch*_h);
			if( DecodeLZW(in,(char *)mem,_w*_h,_w*2,_pitch,resPal,pal)<0 )
			{
				Fail("LZW Decode error");
				return -1;
			}
		}
		else
		{
			// get compressed data size
			//Fail("Old PAC Format.");
			long dSize=fgeti24(in);
			(void)dSize;
			
			long W=w*h;
			
			//if( w!=h ) goto Error; // square textures only
			if( w>4096 || w<2 )
			{
				Fail("Size out of range");
				return -1;
			}
			
			//_w=w,_h=h;
			Assert( _w==w );
			Assert( _h==h );

			int lineCnt=w;
			int lineAlign=_pitch-w*2;
			
			word *B=(word *)mem;
			//int lastVV=0; // start with black
			while( W>0 )
			{
				int c=in.get();
				if( c&0x80 )
				{
					int v=in.get();
					c&=0x7f;
					c++;
					W-=c;
					word vv;
					//if( v==pal->_transparentColor ) vv=lastVV&0x7fff;
					//else lastVV=
					vv=resPal[v];
					if( lineAlign==0 )
					{
						while( --c>=0 ) *B++=vv;
					}
					else
					{
						while( --c>=0 )
						{
							*B++=vv;
							if( --lineCnt==0 ) B=(word *)((byte *)B+lineAlign),lineCnt=w;
						}
					}
				}
				else
				{
					c++;
					W-=c;
					while( --c>=0 )
					{
						int v=in.get();
						word vv;
						//if( v==pal->_transparentColor ) vv=lastVV&0x7fff;
						//else lastVV=
						vv=resPal[v];
						*B++=vv;
						if( --lineCnt==0 ) B=(word *)((byte *)B+lineAlign),lineCnt=w;
					}
				}
			}
		}
		
		if( in.fail() || in.eof() )
		{
			Fail("Texture load failed.");
			return -1;
		}
		
		return 0;
	}
	
	//Error:
	//return error;
}


int PacLevelMem::LoadPaaBin16( QIStream &in, void *mem, const PacPalette *pal ) const
{
	// return<0 - error
	// return>0 - no more mip-map levels available
	// return==0 - texture loaded successfully

	Assert( pal->_nColors==0 );

	// load PAA file
	// get image size
	long w=fgetiw(in);
	long h=fgetiw(in);
	if( w==0 && h==0 ) {return 1;} // mip-map list terminator

	// get compressed data size
	long dSize=fgeti24(in);
	(void)dSize;
	
	//long W=w*h;
	
	if( w>4096 || w<2 )
	{
		Fail("Size out of range");
		return -1;
	}
	
	Assert( _w==w );
	Assert( _h==h );
	
	if (_dFormat!=PacARGB8888)
	{
		// picture is saved in 4444 LZW format
		if( DecodeLZW(in,(char *)mem,_w*_h*2,_w*2,_pitch)<0 )
		{
			Fail("LZW Decode error");
			return -1;
		}

		if (_sFormat==PacAI88 && _dFormat==PacARGB4444)
		{
			int n=_w*_h;
			word *data=reinterpret_cast<word *>(mem);
			Assert( data );
			while( --n>=0 )
			{
				word s=*data;
				word a=(s&0xf000);
				word i=(s&0x00f0);
				word d=a|(i<<4)|i|(i>>4);
				*data++=d;
			}		
		}
		else
		{
			DoAssert( _sFormat==_dFormat );
		}
		//memset(mem,~0,_h*_pitch);
	}
	else
	{
		Assert( _sFormat==PacAI88 );
		// conversion from 88 to 8888
		// allocate temporary 88 surface
		AUTO_STATIC_ARRAY(char,temp,256*256*2);
		temp.Resize(_w*_h*2);
		if( DecodeLZW(in,temp.Data(),_w*_h*2,_w*2,_w*2)<0 )
		{
			Fail("LZW Decode error");
			return -1;
		}
		// convert from temp to mem
		int n=_w*_h;
		DWORD *data=reinterpret_cast<DWORD *>(mem);
		const word *sdata=reinterpret_cast<const word *>(temp.Data());
		Assert( data );
		while( --n>=0 )
		{
			DWORD s = *sdata++;
			//DWORD a = (s&0xff00)<<16;
			DWORD i = (s&0xff);
			DWORD d = (s<<16)|(i<<8)|i;
			*data++=d;
		}	

	}
	
	return 0;
}


// note: int. division very slow - precalc. table instead

//#define DIV_3(x) ( ((x)+1)/3 )
#define DIV_3(x) ( (x)/3 )

static char Div3[128];

static struct Div3Init
{
	Div3Init()
	{
		for (int i=64; i<128; i++)
		{
			Div3[i]=(i-64)/3;
		}
		for (int i=0; i<64; i++)
		{
			Div3[i]=-(-(i-64)/3);
		}
	}
} dummy;

inline int Convert565To1555( int x )
{
	return 0x8000|(x&0x1f)|((x>>1)&0x7fe0);
}

void PacLevelMem::DecompressDXT1( void *dst, const void *src, int w, int h )
{
	const DXTBlock64 *s = (const DXTBlock64 *)src;
	word *line = (word *)dst;
	for (int y=0; y<h; y+=4)
	{
		word *base = line;
		for (int x=0; x<w; x+=4, base+=4)
		{
			word color[4];
			// decompress current block
			int c0 = s->c0;
			int c1 = s->c1;
			if (c0>c1)
			{
				// 4 color block
				int rc0 = (c0>>11)&0x1f;
				int gc0 = (c0>> 5)&0x3f;
				int bc0 = (c0    )&0x1f;
				int rc1 = (c1>>11)&0x1f;
				int gc1 = (c1>> 5)&0x3f;
				int bc1 = (c1    )&0x1f;
				// c0-c1 is positive
				int rd3 = Div3[rc1-rc0+64];
				int gd3 = Div3[gc1-gc0+64];
				int bd3 = Div3[bc1-bc0+64];
				//color[2] = c0 + (c1-c0)/3; // (2 * c0 + c1) / 3;
				//color[3] = c1 - (c1-c0)/3; // (c0 + 2 * c1) / 3;
				// convert 565 to 1555
				// verify c2, c3 components in range
				int rc2 = rc0+rd3;
				int gc2 = gc0+gd3;
				int bc2 = bc0+bd3;

				int rc3 = rc1-rd3;
				int gc3 = gc1-gd3;
				int bc3 = bc1-bd3;
				int c2 = (rc2<<11)|(gc2<<5)|bc2;
				int c3 = (rc3<<11)|(gc3<<5)|bc3;
				color[0]= Convert565To1555(c0);
				color[1]= Convert565To1555(c1);
				color[2]= Convert565To1555(c2);
				color[3]= Convert565To1555(c3);
			}
			else if (c0==c1)
			{
				// mono-color transparent block
				word c01555 = Convert565To1555(c0);
				color[0]= c01555;
				color[1]= c01555;
				color[2]= c01555;
				color[3] = c01555&0x7fff;
			}
			else
			{
				int rc0 = (c0>>11)&0x1f;
				int gc0 = (c0>> 5)&0x3f;
				int bc0 = (c0    )&0x1f;
				int rc1 = (c1>>11)&0x1f;
				int gc1 = (c1>> 5)&0x3f;
				int bc1 = (c1    )&0x1f;

				int rd2 = (rc1-rc0)>>1;
				int gd2 = (gc1-gc0)>>1;
				int bd2 = (bc1-bc0)>>1;
				
				int c2 = ((rc0+rd2)<<11)|((gc0+gd2)<<5)|(bc0+bd2);

				color[0]= Convert565To1555(c0);
				color[1]= Convert565To1555(c1);
				color[2]= Convert565To1555(c2);
				color[3] = color[2]&0x7fff;
				// 3 color + transparency block
			}

			// get dibits
			int w0 = s->tex0;
			int w1 = s->tex1;

			word *tbase = base;
			*tbase++ = color[(w0    )&3]; // 1:0 Texel[0][0] 
			*tbase++ = color[(w0>> 2)&3]; // 3:2 Texel[0][1] 
			*tbase++ = color[(w0>> 4)&3]; // 5:4 Texel[0][2] 
			*tbase   = color[(w0>> 6)&3]; // 7:6 Texel[0][3] 
			tbase += w-3;

			*tbase++ = color[(w0>> 8)&3]; // 9:8 Texel[1][0] 
			*tbase++ = color[(w0>>10)&3]; // 11:10 Texel[1][1] 
			*tbase++ = color[(w0>>12)&3]; // 13:12 Texel[1][2] 
			*tbase   = color[(w0>>14)&3]; // 15:14 Texel[1][3] 
			tbase += w-3;

			*tbase++ = color[(w1    )&3]; // 1:0 Texel[2][0] 
			*tbase++ = color[(w1>> 2)&3]; // 3:2 Texel[2][1] 
			*tbase++ = color[(w1>> 4)&3]; // 5:4 Texel[2][2] 
			*tbase   = color[(w1>> 6)&3]; // 7:6 Texel[2][3] 
			tbase += w-3;

			*tbase++ = color[(w1>> 8)&3]; // 9:8 Texel[3][0] 
			*tbase++ = color[(w1>>10)&3]; // 11:10 Texel[3][1] 
			*tbase++ = color[(w1>>12)&3]; // 13:12 Texel[3][2] 
			*tbase   = color[(w1>>14)&3]; // 15:14 Texel[3][3] 

			s++; // move to next block
		}
		line += 4*w; // skip all 4 decompressed lines
	}
}


#include <Es/Containers/staticArray.hpp>

int PacLevelMem::LoadPaaDXT( QIStream &in, void *mem, const PacPalette *pal ) const
{
	// return<0 - error
	// return>0 - no more mip-map levels available
	// return==0 - texture loaded successfully

	Assert( pal->_nColors==0 );

	// load PAA file
	// get image size
	long w=fgetiw(in);
	long h=fgetiw(in);
	if( w==0 && h==0 ) {return 1;} // mip-map list terminator

	// get compressed data size
	long dSize=fgeti24(in);
	(void)dSize;
	
	//long W=w*h;
	
	if( w>4096 || w<2 )
	{
		Fail("Size out of range");
		return -1;
	}
	
	Assert( _w==w );
	Assert( _h==h );
	
	// picture is saved in raw compressed data format

	if (_dFormat==_sFormat)
	{
		in.read(mem,dSize);
		if (in.fail())
		{
			Fail("Compressed Read error");
			return -1;
		}
	}
	else
	{
		// load to temporary buffer and decompress
		AUTO_STATIC_ARRAY(char,temp,256*256);
		temp.Resize(dSize);
		in.read(temp.Data(),dSize);
		// decompress from temp to mem
		DecompressDXT1(mem,temp.Data(),_w,_h);

	}

	return 0;
}


int PacLevelMem::LoadPac( QIStream &in, void *mem, const PacPalette *pal ) const
{
	int ret=-1;
	switch( (PacFormat)_sFormat )
	{
		case PacDXT1: case PacDXT2: case PacDXT3: case PacDXT4: case PacDXT5:
			ret=LoadPaaDXT(in,mem,pal);
		break;
		case PacP8:
			switch ((PacFormat)_dFormat)
			{
				case PacARGB1555:
					ret=LoadPacARGB1555(in,mem,pal);
					break;
				case PacRGB565:
					ret=LoadPacRGB565(in,mem,pal);
					break;
				default:
					ErrF("Bad destination format for P8 source %d",(int)(PacFormat)_sFormat);
					ret=-1;
				break;
			}
			break;
		default:
			ErrF("Bad source texture format %d",(int)(PacFormat)_sFormat);
			ret=-1;
		break;
	}
	return ret;
}
int PacLevelMem::LoadPaa( QIStream &in, void *mem, const PacPalette *pal ) const
{
	int ret=-1;
	switch( (PacFormat)_sFormat )
	{
		case PacARGB1555:
			//Fail("Breakpoint");
		case PacARGB4444:
		case PacAI88:
			ret=LoadPaaBin16(in,mem,pal);
		break;
		case PacDXT1: case PacDXT2: case PacDXT3: case PacDXT4: case PacDXT5:
			ret=LoadPaaDXT(in,mem,pal);
		break;
		default:
			Fail("Bad texture format.");
			ret=-1;
		break;
	}
	return ret;
}

void PacLevelMem::Interpolate
(
	void *data, void *withData,
	const PacLevelMem &with, float factor
)
{
	if (_dFormat!=with._dFormat)
	{
		Fail("Interpolated surface format does not match.");
		return;
	}
	//LogF("Interpolate %d with %d",_dFormat,with._dFormat);
	if( _dFormat==PacARGB1555 )
	{
		Assert( _w==with._w );
		Assert( _pitch==with._pitch );
		Assert( _h==with._h );
		//Assert( with._memData );
		//Assert( _memData );
		const word *sData=reinterpret_cast<const word *>(withData);
		word *dData=reinterpret_cast<word *>(data);
		int x,y;
		int coef=toIntFloor(factor*256);
		if( coef>255 ) coef=255;if( coef<0 ) coef=0;
		int dCoef=255-coef;
		for( y=0; y<_h; y++ ) for( x=0; x<_w; x++ )
		{
			int offset=y*(_pitch/2)+x;
			word s=sData[offset];
			word &d=dData[offset];
			word dd=d;
			int r=(((s>>10)&0x1f)*coef+((dd>>10)&0x1f)*dCoef+128)>>8;
			int g=(((s>>5)&0x1f)*coef+((dd>>5)&0x1f)*dCoef+128)>>8;
			int b=(((s>>0)&0x1f)*coef+((dd>>0)&0x1f)*dCoef+128)>>8;
			// assume alpha opaque
			d=0x8000|(r<<10)|(g<<5)|b;
		}
	}
	else if( _dFormat==PacRGB565 )
	{
		Assert( _w==with._w );
		Assert( _pitch==with._pitch );
		Assert( _h==with._h );
		//Assert( with._memData );
		//Assert( _memData );
		const word *sData=reinterpret_cast<const word *>(withData);
		word *dData=reinterpret_cast<word *>(data);
		int x,y;
		int coef=toIntFloor(factor*256);
		if( coef>255 ) coef=255;if( coef<0 ) coef=0;
		int dCoef=255-coef;
		for( y=0; y<_h; y++ ) for( x=0; x<_w; x++ )
		{
			int offset=y*(_pitch/2)+x;
			word s=sData[offset];
			word &d=dData[offset];
			word dd=d;
			int r=(((s>>11)&0x1f)*coef+((dd>>11)&0x1f)*dCoef+128)>>8;
			int g=(((s>>5)&0x3f)*coef+((dd>>5)&0x3f)*dCoef+128)>>8;
			int b=(((s>>0)&0x1f)*coef+((dd>>0)&0x1f)*dCoef+128)>>8;
			// assume alpha opaque
			d=(r<<11)|(g<<5)|b;
		}
	}
	else
	{
		Fail("Unsupported interpolation format.");
	}
}


void PacLevelMem::SetDestFormat(PacFormat dFormat, int align)
{
	_dFormat = dFormat;
	// mipmap already initialized
	Assert (_w>0 && _h>0);

	// TODO: make function for pitch initialization
	switch( (PacFormat)_dFormat )
	{
		case PacP8:
			_pitch=((_w+align-1)&~(align-1));
		break;
		case PacARGB4444: case PacARGB1555: case PacAI88: case PacRGB565:
			_pitch=((_w*2+align-1)&~(align-1));
		break;
		case PacARGB8888:
			_pitch=((_w*4+align-1)&~(align-1));
		break;
		case PacDXT1: case PacDXT2:
			_pitch = _w/2; // TODO: better pitch calculation
		break;
		case PacDXT3: case PacDXT4:
			_pitch = _w; // TODO: better pitch calculation
		case PacDXT5:
		default:
			Fail("Texture format");
		break;
	}

}

/*!
\patch_internal 1.08 Date 7/26/2001 by Ondra
- Improved: better handling of data errors.
*/


int PacLevelMem::Init( QIStream &in, PacFormat sFormat)
{
	// return<0 - error
	// return>0 - no more mip-map levels available
	// return==0 - texture loaded successfully
	
	// if data is already loaded, skip it

	int error=-1;

	_sFormat = sFormat;
	
	// get size and palette  size

	if (_w>0 && _h>0)
	{
		// mipmap already initialized
		// no need to read and seek - offset based file
		return 0;
	}

	{	
		int startOffset = in.tellg(); // remember start offset	

		int w=fgetiw(in); // get texture dimensions
		int h=fgetiw(in);
		if( w==0 && h==0 )
		{
			// Fail("Terminator reached");
			error=1;
			goto Error;
		} // mip-map list terminator
		
		if (_start>=0)
		{
			DoAssert( _start==startOffset );
		}
		_start = startOffset;
		// some textures are saved using lzw
		if( w==MAGIC_W_LZW && h==MAGIC_H_LZW )
		{
			w=fgetiw(in);
			h=fgetiw(in);
		}

		//int W=w*h;
		
		// get compressed data size
		int dSize=fgeti24(in);

		//if( w!=h ) goto Error; // square textures only
		// FIX
		if( w>4096 || w<2 || h>4096 || h<2)
		{
			ErrF("Extreme texture size (%dx%d)",w,h);
			goto Error; // check extreme size
		}
		
		_w=w,_h=h;
		
		// skip actual image data
		in.seekg(dSize,QIOS::cur);
		//_skip = in.tellg()-startOffset; // calculate space used
		
		error=0;
		// no ramp - ramp is attached by bank 
	}
	Error:
	if( error<0 )
	{
		Fail("Texture init failed.");
		//_memData=NULL;
		_w=0;
		_h=0;
	}
	return error;
}

void PacLevelMem::SeekLevel( QIStream &in ) const
{
	if (_start>=0)
	{
		in.seekg(_start,QIOS::beg);
		return;
	}
	Fail("Start point now known");
}

PackedColor PacLevelMem::GetPixel(void *data, float u, float v) const
{
	int iU=toIntFloor(u*_w);
	int iV=toIntFloor(v*_h);
	if( iU<0 ) iU=0;
	if( iV<0 ) iV=0;
	if( iU>_w-1 ) iU=_w-1;
	if( iV>_h-1 ) iV=_h-1;
	return GetPixelInt(data,iU,iV);
}

inline PackedColor PackedColor565( int rgb )
{
	int r=(rgb>>11)&0x1f;
	int g=(rgb>>5)&0x3f;
	int b=(rgb>>0)&0x1f;
	const int scale5=255/31;
	const int scale6=255/63;
	return PackedColor(r*scale5,g*scale6,b*scale5,255);
}

inline int Convert5To8( int x )
{
	//return toIntFloor(x*(255.0/31)); // exact but not fast
	return (x*8424)>>10; // much faster
}

inline int Convert6To8( int x )
{
	//return toIntFloor(x*(255.0/31)); // exact but not fast
	return (x*8290)>>11; // much faster
}

inline int Convert565To8888( int x )
{
	int r = Convert5To8((x>>11)&0x1f);
	int g = Convert6To8((x>>5 )&0x3f);
	int b = Convert5To8((x    )&0x1f);
	return 0xff000000|(r<<16)|(g<<8)|b;
}

PackedColor PacLevelMem::GetPixelInt(void *data, int u, int v) const
{
	switch( (PacFormat)_dFormat )
	{
		case PacDXT1:
		{
			// calc. 64-b block address
			int x = u>>2, y = v>>2;
			int bPitch = _w>>2;
			DXTBlock64 *s = ((DXTBlock64 *)data)+y*bPitch+x;
			// pitch of one 64b blocks line (2 actual lines) size
			// get color 0 and 1

			int xb = u&3, yb = v&3;

			DWORD color[4];
			int c0 = s->c0;
			int c1 = s->c1;
			if (c0>c1)
			{
				// 4 color block
				int rc0 = (c0>>11)&0x1f;
				int gc0 = (c0>> 5)&0x3f;
				int bc0 = (c0    )&0x1f;
				int rc1 = (c1>>11)&0x1f;
				int gc1 = (c1>> 5)&0x3f;
				int bc1 = (c1    )&0x1f;
				// c0-c1 is positive
				int rd3 = Div3[rc1-rc0+64];
				int gd3 = Div3[gc1-gc0+64];
				int bd3 = Div3[bc1-bc0+64];
				//color[2] = c0 + (c1-c0)/3; // (2 * c0 + c1) / 3;
				//color[3] = c1 - (c1-c0)/3; // (c0 + 2 * c1) / 3;
				// convert 565 to 1555
				// verify c2, c3 components in range
				int rc2 = rc0+rd3;
				int gc2 = gc0+gd3;
				int bc2 = bc0+bd3;

				int rc3 = rc1-rd3;
				int gc3 = gc1-gd3;
				int bc3 = bc1-bd3;
				int c2 = (rc2<<11)|(gc2<<5)|bc2;
				int c3 = (rc3<<11)|(gc3<<5)|bc3;

				color[0]= Convert565To8888(c0);
				color[1]= Convert565To8888(c1);
				color[2]= Convert565To8888(c2);
				color[3]= Convert565To8888(c3);
			}
			else if (c0==c1)
			{
				// mono-color transparent block
				DWORD c08888 = Convert565To8888(c0);
				color[0]= c08888;
				color[1]= c08888;
				color[2]= c08888;
				color[3] = c08888&0x00ffffff;
			}
			else
			{
				int rc0 = (c0>>11)&0x1f;
				int gc0 = (c0>> 5)&0x3f;
				int bc0 = (c0    )&0x1f;
				int rc1 = (c1>>11)&0x1f;
				int gc1 = (c1>> 5)&0x3f;
				int bc1 = (c1    )&0x1f;

				int rd2 = (rc1-rc0)>>1;
				int gd2 = (gc1-gc0)>>1;
				int bd2 = (bc1-bc0)>>1;
				
				int c2 = ((rc0+rd2)<<11)|((gc0+gd2)<<5)|(bc0+bd2);

				color[0]= Convert565To8888(c0);
				color[1]= Convert565To8888(c1);
				color[2]= Convert565To8888(c2);
				color[3] = color[2]&0x00ffffff;
				// 3 color + transparency block
			}

			int w0 = s->tex0;
			int w1 = s->tex1;

			int b = (yb<<2)|xb;
			switch(b)
			{
				case 0: return (PackedColor)color[(w0    )&3]; // 1:0 Texel[0][0] 
				case 1: return (PackedColor)color[(w0>> 2)&3]; // 3:2 Texel[0][1] 
				case 2: return (PackedColor)color[(w0>> 4)&3]; // 5:4 Texel[0][2] 
				case 3: return (PackedColor)color[(w0>> 6)&3]; // 7:6 Texel[0][3] 

				case 4: return (PackedColor)color[(w0>> 8)&3]; // 9:8 Texel[1][0] 
				case 5: return (PackedColor)color[(w0>>10)&3]; // 11:10 Texel[1][1] 
				case 6: return (PackedColor)color[(w0>>12)&3]; // 13:12 Texel[1][2] 
				case 7: return (PackedColor)color[(w0>>14)&3]; // 15:14 Texel[1][3] 

				case 8: return (PackedColor)color[(w1    )&3]; // 1:0 Texel[2][0] 
				case 9: return (PackedColor)color[(w1>> 2)&3]; // 3:2 Texel[2][1] 
				case 10: return (PackedColor)color[(w1>> 4)&3]; // 5:4 Texel[2][2] 
				case 11: return (PackedColor)color[(w1>> 6)&3]; // 7:6 Texel[2][3] 

				case 12: return (PackedColor)color[(w1>> 8)&3]; // 9:8 Texel[3][0] 
				case 13: return (PackedColor)color[(w1>>10)&3]; // 11:10 Texel[3][1] 
				case 14: return (PackedColor)color[(w1>>12)&3]; // 13:12 Texel[3][2] 
				case 15: return (PackedColor)color[(w1>>14)&3]; // 15:14 Texel[3][3] 
			}
		}	
		case PacP8:
		{
			Fail("Unsupported format palette 8");
			return PackedColor(0);
		}
		case PacRGB565:
		{
			int rgb=((Pixel *)data)[v*_w+u];
			return PackedColor565(rgb);
		}
		//break;
		case PacARGB1555:
		{
			int rgb=((Pixel *)data)[v*_w+u];
			int r=(rgb>>10)&0x1f;
			int g=(rgb>>5)&0x1f;
			int b=(rgb>>0)&0x1f;
			int a=((rgb>>15)&1)*255;
			const int scale5=255/31;
			return PackedColor(r*scale5,g*scale5,b*scale5,a);
		}
		case PacARGB4444:
		{
			int rgb=((Pixel *)data)[v*_w+u];
			int a=(rgb>>12)&0xf;
			int r=(rgb>>8)&0xf;
			int g=(rgb>>4)&0xf;
			int b=(rgb>>0)&0xf;
			const int scale4=255/15;
			return PackedColor(r*scale4,g*scale4,b*scale4,a*scale4);
		}
		case PacAI88:
		{
			int rgb=((Pixel *)data)[v*_w+u];
			int i=(rgb>>0)&0xff;
			int a=(rgb>>8)&0xff;
			return PackedColor(i,i,i,a);
		}
		default:
		{
			Fail("Bad texture format.");
			return PackedColor(0,0,0,0);
		}
	}
}

/*
int PacLevel::LoadPac( QIStream &in, MemoryHeap *heap, const PacPalette *pal )
{
	// allocate memory and load
	void *mem=NULL;
	MemoryItem *dataBuf=NULL;
	if( !_data )
	{
		_heap=heap;
		dataBuf=_heap->Alloc(Size());
		if( !dataBuf )
		{
			Log("PacLevel::Load out of memory");
			return -1;
		}
		mem=dataBuf->Memory();
	}
	int ret=PacLevelMem::LoadPac(in,mem,pal);
	if( ret )
	{
		if( dataBuf ) _heap->Free(dataBuf);
	}
	else _data=dataBuf;
	return ret;
}
int PacLevel::LoadPaa( QIStream &in, MemoryHeap *heap, const PacPalette *pal )
{
	// allocate memory and load
	void *mem=NULL;
	MemoryItem *dataBuf=NULL;
	if( !_data )
	{
		_heap=heap;
		dataBuf=_heap->Alloc(Size());
		if( !dataBuf )
		{
			Log("PacLevel::Load out of memory");
			return -1;
		}
		mem=dataBuf->Memory();
	}
	int ret=PacLevelMem::LoadPaa(in,mem,pal);
	if( ret )
	{
		if( dataBuf ) _heap->Free(dataBuf);
	}
	else _data=dataBuf;
	return ret;
}

void PacLevel::Clear()
{
	if( _data ) _heap->Free(_data),_data=NULL;
	//_memData=NULL;
}
*/


// general memory management

SystemHeap::SystemHeap()
:_alocated(NULL),_size(0)
{
}

void SystemHeap::Release()
{
	//Assert( Check() );
	MemoryHeap::Release();
	if( _alocated ) delete[] _alocated,_alocated=NULL;
	_size=0;
}
void SystemHeap::Init( int size )
{
	// we must remember alocated memory because we need to free it
	// and it may be aligned by MemoryHeap::Init
	_alocated=new byte[size];
	_size=size;
	if( _alocated )
	{
		MemoryHeap::Init(_alocated,_size,8);
		Assert( (char *)Memory()+Size()<=(char *)_alocated+_size );
	}
}

DEFINE_FAST_ALLOCATOR(MemoryItem)

#if DO_FILLS
// for easier debugging performs fills on alloc and free
MemoryItem *MemoryHeap::Alloc( int size )
{
	MemoryItem *mem=Heap<byte *,int>::Alloc(size);
	if( mem )
	{
		int *buf=(int *)mem->Memory();
		for( int i=0; i<size/4; i++ ) buf[i]=NEW_FILL;
	}
	return mem;
}
void MemoryHeap::Free( MemoryItem *mem )
{
	Assert( mem );
	int *buf=(int *)mem->Memory();
	int size=mem->Size()/4;
	for( int i=0; i<size; i++ ) buf[i]=DEL_FILL;
	Heap<byte *,int>::Free(mem);
}

#endif

bool SystemHeap::Check() const
{
	return MemoryHeap::Check();
}

SystemHeap::~SystemHeap()
{
	Release();
}


class TextureSourcePac: public ITextureSource
{
	PacPalette _pal;
	int _mipmaps; //!< source file mipmap count
	PacFormat _format; //!< source file pixel format
	bool _isPaa;
	RStringB _name;

	public:
	TextureSourcePac();
	~TextureSourcePac();

	bool Init(const char *name, PacLevelMem *mips, int maxMips);

	int GetMipmapCount() const;
	PacFormat GetFormat() const;
	bool GetMipmapData(void *mem, const PacLevelMem &mip, int level) const;	

	PackedColor GetAverageColor() const {return _pal.AverageColor32();}

	bool IsAlpha() const {return _pal._isAlpha;}
	bool IsTransparent() const {return _pal._isTransparent;}
	void ForceAlpha() {_pal._isAlpha = true;}
};

//! all routines required to create pac/paa texture
class TextureSourcePacFactory: public ITextureSourceFactory
{
	public:
	bool Check(const char *name);
	void PreInit(const char *name);
	ITextureSource *Create(const char *name, PacLevelMem *mips, int maxMips);
};

#include "cutFileServer.hpp"

TextureSourcePac::TextureSourcePac()
{
	_mipmaps = 0;
	_isPaa = false;
}

TextureSourcePac::~TextureSourcePac()
{
}

static PacFormat BasicFormat( const char *name )
{
	char *ext=strrchr(name,'.');
	if( ext && !strcmpi(ext,".paa") ) return PacARGB4444;
	else return PacP8;
}

#define MIN_MIP_SIZE 4 // guarantee QWORD alignment (4 16-bit pixels are enough)

bool TextureSourcePac::Init(const char *name, PacLevelMem *mips, int maxMips)
{
	_name = name;
	QIFStream in;
	GFileServer->Open(in,name);
	if (in.fail()) return false;
	if (in.rest()==0) return false;

	// .paa should start with format marker
	int desc = fgetiw(in);
	bool alpha = false;
	PacFormat format=BasicFormat(name);

	if( format==PacARGB4444 ) _isPaa=true;
	else _isPaa=false;

	PacFormat nFormat = PacFormatFromDesc(desc,alpha);
	if( nFormat!=PacFormatN ) format=nFormat;
	else in.seekg(-2,QIOS::cur);


	_format = format;

	const int maxOffsets = 16;
	int offsets[maxOffsets];
	for (int i=0; i<maxOffsets; i++ ) offsets[i] = -1;

	if (_pal.Load(in,offsets,maxOffsets)) return false;
	// get number of mipmaps
	// initialize them
	

	int i;
	for (i=0; i<maxMips; i++)
	{
		if (offsets[0]>=0 && offsets[i]<0) break; // last known mipmap read

		PacLevelMem &mip=mips[i];

		mip.SetStart(offsets[i]);

		if (i>0 && offsets[i]>=0 )
		{
			// _w and _h size is known
			mip._w = mips[0]._w >> i;
			mip._h = mips[0]._h >> i;
		}

		int ret = mips[i].Init(in,format);

		if( ret<0 ) return false;
		if( ret>0 ) break; // last mip-map read
		if (mip._w<=MIN_MIP_SIZE) break;
		if (mip._h<=MIN_MIP_SIZE) break;
	}
	_mipmaps = i;

	return true;
}

int TextureSourcePac::GetMipmapCount() const
{
	return _mipmaps;
}

PacFormat TextureSourcePac::GetFormat() const
{
	return _format;
}

bool TextureSourcePac::GetMipmapData(void *mem, const PacLevelMem &mip, int level) const
{
	int ret;
	QIFStream in;
	GFileServer->Open(in,_name);

	mip.SeekLevel(in);
	if( _isPaa )
	{
		ret=mip.LoadPaa(in,mem,&_pal);
	}
	else
	{
		ret=mip.LoadPac(in,mem,&_pal);
	}

	return ret==0;
}


ITextureSource *CreateTextureSourcePac(const char *name, PacLevelMem *mips, int maxMips)
{
	TextureSourcePac *source = new TextureSourcePac;
	bool ret = source->Init(name,mips,maxMips);
	if (!ret)
	{
		delete source;
		return NULL;
	}
	return source;
}

#include "cutJpgImport.hpp"

ITextureSource *CreateTextureSourceJPEG(const char *name, PacLevelMem *mips, int maxMips)
{
	#if defined _WIN32 && !defined _XBOX
	TextureSourceJPEG *source = new TextureSourceJPEG;
	if (!source->Init(name,mips,maxMips))
	{
		delete source;
		return NULL;
	}
	return source;
	#else
	return NULL;
	#endif
}

bool TextureSourcePacFactory::Check(const char *name)
{
	//! pre-init data source
	return QIFStreamB::FileExist(name);
}
void TextureSourcePacFactory::PreInit(const char *name)
{
	// TODO: request only headers
	GFileServer->Request(name,2);
}

ITextureSource *TextureSourcePacFactory::Create(const char *name, PacLevelMem *mips, int maxMips)
{
	TextureSourcePac *source = new TextureSourcePac;
	if (!source->Init(name,mips,maxMips))
	{
		delete source;
		return NULL;
	}
	return source;
}

static TextureSourcePacFactory STextureSourcePacFactory;

TextureSourcePacFactory *GTextureSourcePacFactory = &STextureSourcePacFactory;

ITextureSourceFactory *SelectTextureSourceFactory(const char *name)
{
	if (!name) return NULL;
	const char *ext = GetFileExt(GetFilenameExt(name));
	if (!strcmpi(ext,".pac") || !strcmpi(ext,".paa"))
	{
		return GTextureSourcePacFactory;
	}
	#if defined _WIN32 && !defined _XBOX
	if (!strcmpi(ext,".jpg") || !strcmpi(ext,".jpeg"))
	{
		return GTextureSourceJPEGFactory;
	}
	#endif
	ErrF("Unrecognized texture type '%s': '%s'",ext,name);
	return NULL;
}
