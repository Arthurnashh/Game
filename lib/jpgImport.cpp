/*!
\patch 1.02 Date 7/6/2001 by Ondra.
- Added: JPG file reading.
*/

#include "wpch.hpp"
#include "jpgImport.hpp"
#include <Es/Common/win.h>

#if defined(_WIN32) 
#include <ijl.h>
#include <Es/Containers/staticArray.hpp>
#include "El/QStream/qbStream.hpp"
#include <El/FileServer/fileServer.hpp>
#include <El/Credits/credits.hpp>

#include <El/XML/inet.hpp>

/*!
\patch 1.24 Date 9/24/2001 by Ondra.
- Improved: progressive JPG support.
*/


typedef IJLERR IJL_STDCALL ijlInitFT ( JPEG_CORE_PROPERTIES* jcprops );
typedef	IJLERR IJL_STDCALL ijlReadFT ( JPEG_CORE_PROPERTIES* jcprops, IJLIOTYPE iotype );
typedef IJLERR IJL_STDCALL ijlFreeFT ( JPEG_CORE_PROPERTIES* jcprops );

#define MIN_MIP_SIZE 4

class JPGLibrary
{
	HMODULE _libHandle;
  /// floating-point control word used in this library
  /**
  IJL seems to be manipulating x87 control word. This presents a problems for the rest of the application, including DX.
  We solve this by using separate control word when executing IJL.
  */
  unsigned int _libCTRL;
	ijlInitFT *_init;
	ijlReadFT *_read;
	ijlFreeFT *_free;
	int _refCount;

	void Load();
	void Unload();

	public:
	JPGLibrary()
	{
		_init = NULL;
		_read = NULL;
		_free = NULL;
		_libHandle = NULL;
		_refCount = 0;
	}
	~JPGLibrary()
	{
		Unload();
	}

	void AddRef()
	{
		if (_refCount++==0) Load();
	}
	void Release()
	{
		if (--_refCount==0) Unload();
	}

	IJLERR Init(JPEG_CORE_PROPERTIES* jcprops)
	{
		AddRef();
		if (!_init) return IJL_PROG_NOT_SUPPORTED;
    // backup the original CTRL
    unsigned int oldCTRL = _control87(0, 0);
    // use the CTRL library wants
    _control87(_libCTRL, 0xffffffff);
		IJLERR ret = _init(jcprops);
    // restore the original CTRL
    _control87(oldCTRL, 0xffffffff);
    return ret;
	}
	IJLERR Read( JPEG_CORE_PROPERTIES* jcprops, IJLIOTYPE iotype )
	{
		if (!_read) return IJL_PROG_NOT_SUPPORTED;
    // backup the original CTRL
    unsigned int oldCTRL = _control87(0, 0);
    // use the CTRL library wants
    _control87(_libCTRL, 0xffffffff);
		IJLERR ret = _read(jcprops,iotype);
    // restore the original CTRL
    _control87(oldCTRL, 0xffffffff);
    return ret;
	}
	IJLERR Free( JPEG_CORE_PROPERTIES* jcprops )
	{
		if (!_free) return IJL_PROG_NOT_SUPPORTED;
    // backup the original CTRL
    unsigned int oldCTRL = _control87(0, 0);
    // use the CTRL library wants
    _control87(_libCTRL, 0xffffffff);
		IJLERR ret = _free(jcprops);
    // restore the original CTRL
    _control87(oldCTRL, 0xffffffff);
		Release();
		return ret;
	}
};

void JPGLibrary::Load()
{
  unsigned int oldCTRL = _control87(0, 0);
  // Use PATH search
	_libHandle = LoadLibrary("ijl15.dll");
  // store the CTRL library wants
  _libCTRL = _control87(0, 0);
  // restore the original CTRL
  _control87(oldCTRL, 0xffffffff);

  
  if (!_libHandle)
	{
		LogF("No jpg import library found");
		return;
	}

	_init = (ijlInitFT *)GetProcAddress(_libHandle,"ijlInit");
	_read = (ijlReadFT *)GetProcAddress(_libHandle,"ijlRead");
	_free = (ijlFreeFT *)GetProcAddress(_libHandle,"ijlFree");

	if (!_init || !_read || !_free)
	{
		LogF("Required JPG functions not found.");
		Unload();
		return;
	}
	LogF("JPGLibrary loaded");
}

void JPGLibrary::Unload()
{
	if (_libHandle)
	{
		LogF("JPGLibrary unloaded");
		FreeLibrary(_libHandle);
		_libHandle = NULL;
		_init = (ijlInitFT *)NULL;
		_read = (ijlReadFT *)NULL;
		_free = (ijlFreeFT *)NULL;
	}
}

static JPGLibrary GJPGLibrary;

TextureSourceJPEG::TextureSourceJPEG()
{
	_propInit = false;
}

TextureSourceJPEG::~TextureSourceJPEG()
{
	FreeProp();
}

bool TextureSourceJPEG::InitProp()
{
	Assert (!_propInit);
	IJLERR err = GJPGLibrary.Init(&_prop);
	if (err!=IJL_OK) return false;
	_propInit = true;
	return true;
}

void TextureSourceJPEG::FreeProp()
{
	if (_propInit)
	{
		GJPGLibrary.Free(&_prop);
		_propInit = false;
	}
}

static bool IsPowerOfTwo(int x)
{
	while (x>0)
	{
		if (x&1) return x==1;
		x >>= 1;
	}
	return true;
}

void TextureSourceJPEG::SetName(const char *name)
{
	_name = name;
}

bool TextureSourceJPEG::Init(PacLevelMem *mips, int maxMips)
{
  _timestamp = 0;
  if (IsWebSource(_name))
  {
    size_t size = 0;
    AutoFree data = DownloadFile(_name, size, NULL);
    if (data == NULL) return false;

    InitProp();
    _data.Copy((const unsigned char *)(const void *)data, size);
  }
  else
  {
	QIFStreamB in;
	in.AutoOpen(_name);
	if (in.fail() || in.rest()==0) return false;

  // read all data
  _data.Realloc(in.rest());
  _data.Resize(in.rest());
  int rd = in.read(_data.Data(),_data.Size());
  if (rd!=_data.Size()) return false;
    _timestamp = QFBankQueryFunctions::TimeStamp(_name);

    InitProp();
  }
  _prop.JPGBytes = _data.Data();
	_prop.JPGSizeBytes = _data.Size();
	_prop.JPGFile = _name;
	
  IJLERR err = GJPGLibrary.Read( &_prop, IJL_JBUFF_READPARAMS );
	if (err!=IJL_OK)
	{
		LogF("Cannot read JPG file header from %s",(const char *)_name);
		FreeProp();
		return false;
	}

  // Set the JPG color space ... this will always be
  // somewhat of an educated guess at best because JPEG
  // is "color blind" (i.e., nothing in the bit stream
  // tells you what color space the data was encoded from).
  // However, in this example we assume that we are
  // reading JFIF files which means that 3 channel images
  // are in the YCbCr color space and 1 channel images are
  // in the Y color space.
  switch(_prop.JPGChannels)
  {
  case 1:
    _prop.JPGColor    = IJL_G;
    _prop.DIBChannels = 3;
    _prop.DIBColor    = IJL_RGB;
		//Fail("Unsupported");
    break;

  case 3:
    _prop.JPGColor    = IJL_YCBCR;
    _prop.DIBChannels = 3;
    _prop.DIBColor    = IJL_RGB;
    break;

  case 4:
    _prop.JPGColor    = IJL_YCBCRA_FPX;
    _prop.DIBChannels = 4;
    _prop.DIBColor    = IJL_RGBA_FPX;
		Fail("Unsupported");
    break;

  default:
    // This catches everything else, but no
    // color twist will be performed by the IJL.
		WarningMessage("Unknown JPG file format in %s",(const char *)_name);
		FreeProp();
		return false;
  }

  // verify power of 2 dimensions
  bool powerOfTwo = IsPowerOfTwo(_prop.JPGWidth) && IsPowerOfTwo(_prop.JPGHeight);

	int i;
	for (i=0; i<maxMips; i++)
	{
		PacLevelMem &mip=mips[i];
		if (i==0)
		{
			mip._w = _prop.JPGWidth;
			mip._h = _prop.JPGHeight;
		}
		else
		{
			// _w and _h size is known
			mip._w = mips[0]._w >> i;
			mip._h = mips[0]._h >> i;
		}

		int ret = mips[i].Init(PacARGB1555);

		if( ret<0 )
    {
      FreeProp();
      return false;
    }
		if( ret>0 ) break; // last mip-map read
		if (mip._w<=MIN_MIP_SIZE) break;
		if (mip._h<=MIN_MIP_SIZE) break;
    if (!powerOfTwo) break; // create a single mip-map for common sized image
	}

	return true;
}

int TextureSourceJPEG::GetMipmapCount() const
{
  bool powerOfTwo = IsPowerOfTwo(_prop.JPGWidth) && IsPowerOfTwo(_prop.JPGHeight);
  if (!powerOfTwo) return 1;
  
	/*
	IJL_JBUFF_READWHOLEIMAGE
  IJL_JBUFF_READONEHALF
  IJL_JBUFF_READONEQUARTER 
  IJL_JBUFF_READONEEIGHTH
	*/
	int maxSize = _prop.JPGWidth;
	saturateMax(maxSize,_prop.JPGHeight);

	int maxMipmaps = 0;
	while (maxSize>0) maxMipmaps++, maxSize >>= 1;

	// TODO: support more mipmaps than 4
	if (maxMipmaps>4) maxMipmaps = 4;
	return maxMipmaps;
}
PacFormat TextureSourceJPEG::GetFormat() const
{
	return PacARGB1555;
}

__forceinline int ConvertRGB8To1555(int r, int g, int b)
{
	return
	(
		((r<<(10-3))&(0x1f<<10))|
		((g<<(5-3))&(0x1f<<5))|
		((b>>3)&0x1f)|0x8000
	);
}

__forceinline int ConvertRGB8To565(int r, int g, int b)
{
	return
	(
		((r<<(11-3))&(0x1f<<11))|
		((g<<(5-2))&(0x3f<<5))|
		((b>>3)&0x1f)
	);
}

bool TextureSourceJPEG::GetMipmapData(void *mem, const PacLevelMem &mip, int level, int texSize) const
{
  _prop.JPGBytes = (unsigned char *)_data.Data();
	_prop.JPGSizeBytes = _data.Size();

	_prop.JPGFile = _name;

  //int imgSize = (_prop.DIBWidth * _prop.DIBChannels + _prop.DIBPadBytes) *
  //  _prop.DIBHeight;


	IJLIOTYPE iotype = IJL_JBUFF_READWHOLEIMAGE;
	int shift = 0;
	switch (level)
	{
		case 0: iotype = IJL_JBUFF_READWHOLEIMAGE, shift = 0; break;
		case 1: iotype = IJL_JBUFF_READONEHALF, shift = 1; break;
		case 2: iotype = IJL_JBUFF_READONEQUARTER, shift = 2; break;
		case 3: iotype = IJL_JBUFF_READONEEIGHTH, shift = 3; break;
		default:
			ErrF("Unsupported JPG mipmap (%d) in %s",level,(const char *)_name);
			return false;
	}

  _prop.DIBWidth    = _prop.JPGWidth>>shift;
  _prop.DIBHeight   = _prop.JPGHeight>>shift;
  _prop.DIBPadBytes = 0;
	DoAssert (_prop.DIBWidth==mip._w);
	DoAssert (_prop.DIBHeight==mip._h);
	AUTO_STATIC_ARRAY(unsigned char,temp,256*256*4);
	temp.Resize(_prop.DIBWidth*_prop.DIBHeight*_prop.DIBChannels);

	_prop.DIBBytes = temp.Data();
  
  IJLERR err = GJPGLibrary.Read( &_prop, iotype );
	if (err!= IJL_OK)
	{
		LogF("Cannot read JPG file data from %s",(const char *)_name);
		return false;
	}
	// convert image from 8, 888 or 8888 representation to 1555
	// TODO: other conversion, currently only 888 is supported
	// check destination format
	int srcPixels = _prop.DIBWidth*_prop.DIBHeight;
	unsigned short *dst = (unsigned short *)mem;
	const unsigned char *src = temp.Data();
	switch (mip._dFormat.GetEnumValue())
	{
		case PacARGB1555:
			while (--srcPixels>=0)
			{
				int r = *src++;
				int g = *src++;
				int b = *src++;
				*dst++ = ConvertRGB8To1555(r,g,b);
			}
			break;
		case PacRGB565:
			while (--srcPixels>=0)
			{
				int r = *src++;
				int g = *src++;
				int b = *src++;
				*dst++ = ConvertRGB8To565(r,g,b);
			}
			break;
	}


	return true;
}

bool TextureSourceJPEG::RequestMipmapData(
  const PacLevelMem *mips, int levelMin, int levelMax, FileRequestPriority prior
) const
{
  // with JPG texture all source data are currently always loaded in memory
  return true;
}


bool TextureSourceJPEGFactory::Check(const char *name)
{
  if (IsWebSource(name)) return true;
	//! pre-init data source
	return QFBankQueryFunctions::FileExists(name);
}
bool TextureSourceJPEGFactory::PreInit(
  const char *name, RequestableObject *obj, RequestableObject::RequestContext *ctx
)
{
	// TODO: request only headers
	//if (GFileServer) GFileServer->Request(2,name);
	return true;
}

ITextureSource *TextureSourceJPEGFactory::Create(const char *name, PacLevelMem *mips, int maxMips)
{
	TextureSourceJPEG *source = new TextureSourceJPEG;
	source->SetName(name);
	if (!source->Init(mips,maxMips))
	{
		delete source;
		return NULL;
	}
	return source;
}

static TextureSourceJPEGFactory STextureSourceJPEGFactory;

TextureSourceJPEGFactory *GTextureSourceJPEGFactory = &STextureSourceJPEGFactory;

REGISTER_CREDITS("Intel JPEG Library", "1998 Intel Corporation","")

#endif
