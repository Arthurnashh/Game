// font implementation common to all engines
// (C) 1997, Suma
#include "wpch.hpp"

#include "engine.hpp"
#include "scene.hpp"
#include "font.hpp"
#include "textbank.hpp"
#include <El/QStream/qbStream.hpp>
#include "global.hpp"
#include "scene.hpp"
#include "camera.hpp"
#include <stdarg.h>

#include "mbcs.hpp"
#include "paramFileExt.hpp"
#include "fileLocator.hpp"

FontWithSize::FontWithSize()
{
}

FontWithSize::~FontWithSize()
{
  DoDestruct();
}

void FontWithSize::DoDestruct()
{
  //int i;
  //for( i=0; i<NChar; i++ ) _charTextures[i]=NULL;
}

const CharInfo *FontWithSize::GetInfo(int ch) const
{
  if (ch < 0) return NULL;

  // select page
  int pageIndex = ch / 0x80;
  if (pageIndex >= _pages.Size()) return NULL;
  const CharPage &page = _pages[pageIndex];

  // select char info
  int charIndex = (ch & 0x7f) - page._startChar;
  if (charIndex < 0 || charIndex >= page._chars.Size()) return NULL;
  return &page._chars[charIndex];
}

CharInfo &FontWithSize::AddInfo(int ch)
{
  Assert(ch >= 0);

  // select / add page
  int pageIndex = ch / 0x80;
  _pages.Access(pageIndex);
  CharPage &page = _pages[pageIndex];

  // select / add char
  int charIndex = ch & 0x7f;
  if (page._chars.Size() == 0)
  {
    // the first character on this page
    page._startChar = charIndex;
    page._chars.Resize(1);
    return page._chars[0];
  }
  else if (charIndex < page._startChar)
  {
    // before the first character on this page
    page._chars.InsertMultiple(0, page._startChar - charIndex);
    page._startChar = charIndex;
    return page._chars[0];
  }
  else
  {
    charIndex -= page._startChar;
    page._chars.Access(charIndex);
    return page._chars[charIndex];
  }
}

/*!
\patch_internal 1.31 Date 11/21/2001 by Jirka
- Changed: number of characters in font is now variable
*/

/*!
\patch 5145 Date 3/23/2007 by Ondra
- Fixed: Improved text output kerning.
*/
void FontWithSize::Load(const char *name)
{
  //LogF("Load font request %s",(const char *)name);
  _name = name;
  _name.Lower();
  // load all char textures
  // first check new font
  RString fontName = Format("%s.fxy",(const char *)_name);
  QIFStreamB f;
  f.AutoOpen(fontName);
  _version = 0;
  if (!f.fail() && f.rest()>0)
  {
    int version = 0;
    char magic[5];
    f.read(&magic,sizeof(magic)-1);
    magic[4]=0;
    if (!strcmp(magic,"BIFo"))
    {
      version = f.getil();
      if (version>0x101)
      {
        ErrorMessage("%s: Too new font (version %x)",cc_cast(name),version);
      }
    }
    else
    {
      f.seekg(0,QIOS::beg);
    }
    _version = version;
    _maxHeight = 0;
    int maxW = 0;
    while (f.rest()>0)
    {
      int c = f.getiw(); // get character code
      c += 0x20; // fxy format backward compatibility
      CharInfo &info = AddInfo(c);

      int setNum = f.getiw(); // get set number
      int x = f.getiw();
      int y = f.getiw();
      int w = f.getiw();
      int h = f.getiw();
      int kw = w;
      if (_version>=0x101)
      {
        // kerning width stored separately since 0x101;
        kw = f.getiw();
      }

      // make sure we are able to store values in the CharInfo
      DoAssert(x <= USHRT_MAX);
      DoAssert(y <= USHRT_MAX);
      DoAssert(w <= UCHAR_MAX);
      DoAssert(h <= UCHAR_MAX);
      DoAssert(kw <= UCHAR_MAX);

      _textures.Access(setNum);
      
      if (_textures[setNum].IsNull())
      {
        // create a texture name
        BString<256> texName;
        sprintf(texName,"%s-%02d.paa",(const char *)_name,setNum);
        Ref<Texture> tex = GlobLoadTexture(RStringB(texName));
        _textures[setNum] = tex;
        if (tex) tex->SetUsageType(Texture::TexUI);
      }
      info.tex = _textures[setNum];

      info.xTex = x;
      info.yTex = y;
      info.wTex = w;
      info.hTex = h;
      info.kw = kw;
      //  big font textures
      saturateMax(maxW,w-1);
      saturateMax(_maxHeight,h);
    }
    _maxWidth = maxW;

    // update width of the space
    const CharInfo *baseChar = GetInfo('o');
    int w = baseChar ? baseChar->wTex : _maxWidth;
    int kw = baseChar ? baseChar->kw : _maxWidth;
    DoAssert(w <= SCHAR_MAX);
    CharInfo &info = AddInfo(' ');
    info.wTex = w;
    info.kw = kw;
  }
  else
  {
    RptF("Cannot load font %s",(const char *)name);
    _maxHeight = 0;
    _maxWidth = 0;
  }
  //LogF("Font %s: h=%d",name,_maxHeight);
}

float FontWithSize::Height() const
{
  // normalized height (fraction of screen height)
  return _maxHeight*(1.0/600);
}

bool FontWithSize::IsReady() const
{
  bool ready = true;
  for (int i=0; i<_textures.Size(); i++)
  {
    Ref<Texture> tex = _textures[i].GetRef();
    if (!tex) continue;
    tex->SetUsageType(Texture::TexUI);
    if (!tex->HeadersReady())
    {
      ready = false;
// LogF("Texture %s - headers are not ready", (const char *)tex->GetName());
      continue;
    }
    //
    MipInfo mip = GEngine->TextBank()->UseMipmap(tex, 1, 0, TexPriorityUIHigh);
    if (mip._level != 0)
    {
      ready = false;
// LogF("Texture %s - level %d", (const char *)tex->GetName(), mip._level);
    }
  }
  return ready;
}

Font::Font()
{
  _langID = English;
  _spaceWidth = 1.0f;
  _spacing = 0;
}

Font::~Font()
{
  GLOB_ENGINE->FontDestroyed(this);
}

void Font::Load(RString name)
{
  _name = name;

  ParamEntryVal cls = Pars >> "CfgFontFamilies" >> name;
  ParamEntryVal array = cls >> "fonts";
  int n = array.GetSize();
  _fonts.Realloc(n);
  _fonts.Resize(0);
  for (int i=0; i<n; i++)
  {
    Ref<FontWithSize> font = new FontWithSize();
    RString name = array[i];
    font->Load(GetDefaultName(name, ""));
    if (font->IsLoaded())
      _fonts.Add(font);
    else
      RptF("Fonts file %s not found", (const char *)name);
  }

  ConstParamEntryPtr entry = cls.FindEntry("spaceWidth");
  if (entry) _spaceWidth = *entry;
  entry = cls.FindEntry("spacing");
  if (entry) _spacing = *entry;
}

/** make sure all texture handles held by this font are released */

void Font::Unload()
{
  // we will leave the object as unloaded
  // all rendering functions will now perform nothing, as IsLoaded will return false
  _fonts.Clear();
}

const FontWithSize *Font::Select(float size) const
{
  float bestErr = FLT_MAX;
  const FontWithSize *best = NULL;
  for (int i=0; i<_fonts.Size(); i++)
  {
    float h = _fonts[i]->AbsHeight();
    float err = fabs(Square(size) - Square(h));
    if (bestErr > err) bestErr = err, best = _fonts[i];
  }

  return best;
}

const FontWithSize *Font::Select1To1(float size) const
{
  float bestErr = FLT_MAX;
  const FontWithSize *best = NULL;
  for (int i=0; i<_fonts.Size(); i++)
  {
    float h = _fonts[i]->AbsHeight();
    float err = size - h;
    // check if we have a chance to 1:1 mapping
    if (err<-1.3f) continue; // font too large - skip it
    if (err>size*0.2f) continue; // font way too small - skip it
    if (bestErr > fabs(err)) bestErr = fabs(err), best = _fonts[i];
  }

  return best;
}

Font *FontCache::Load(FontID id)
{
  char lowName[256];
  strcpy(lowName,id.name),strlwr(lowName);
  int i;
  for( i=0; i<_fonts.Size(); i++ )
  {
    Font *font=_fonts[i];
    if( font && !strcmp(font->Name(),lowName) )
    {
      DoAssert(font->GetLangID() == id.langID);
      return font;
    }
  }
  // add new font
  Font *font=new Font;
  Assert( font );
  font->Load(lowName);
  font->SetLangID(id.langID);
  _fonts.Add(font);
  return font;
}

Font *Engine::LoadFont(FontID id)
{
  // search for loaded fonts
  return _fonts.Load(id);
}


void FontCache::RemoveFont( Font *font )
{
  // TODO: dynamic font loading/unloading
}

void FontCache::Clear()
{
  // make sure all textures in all fonts are unloaded
  for (int i=0; i<_fonts.Size(); i++)
  {
    Font *font = _fonts[i];
    font->Unload();
  }
  
  _fonts.Clear();
  Assert( _fonts.Size()==0 );
}
