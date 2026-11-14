#ifndef __FONT_HPP
#define __FONT_HPP

#include <Es/Strings/rString.hpp>
#include "types.hpp"

/// information about a single character
struct CharInfo
{
	/// texture name
	RefR<Texture> tex;
  /// position in the texture
	unsigned short xTex, yTex;
  /// dimensions of the used part of the texture
	unsigned char wTex, hTex;
  /// kerning width
  unsigned char kw;
};
TypeIsMovableZeroed(CharInfo)

/// single page (128) of characters
struct CharPage
{
  /// index of the the first valid character on the page
  int _startChar;
  /// info about characters
  AutoArray<CharInfo> _chars;
};
TypeIsMovable(CharPage)

/// single font with given height
class FontWithSize : public RefCount
{
protected:
	AutoArray<CharPage> _pages;

  /// array of textures for individual sets
  AutoArray< RefR<Texture> > _textures;

	int _maxHeight;
	int _maxWidth;
	/// font name, used also as a basis for a texture name
	RString _name;
  /// some features were introduced in newer file versions
  int _version;

public:	
	FontWithSize();
  ~FontWithSize();

	const char *Name() const {return _name;}
	void Load( const char *name );

  const CharInfo *GetInfo(int ch) const;

  float Height() const; // normalized height
	
  /// max. height in pixels
	int AbsHeight() const {return _maxHeight;}
  /// max. width in pixels
  int AbsWidth() const {return _maxWidth;}

  bool IsLoaded() const {return _maxHeight>0;}

  bool IsReady() const; // all textures are loaded

  /// version 0x100 introduced left/top anti aliasing border */
  bool HasAABorder() const {return _version>=0x100;}

protected:
  void DoDestruct();

  CharInfo &AddInfo(int ch);
};

class Font : public RefCount
{
protected:
  RefArray<FontWithSize> _fonts;

  RString _name;
  int _langID;
  float _spaceWidth;
  float _spacing;

public:	
  Font();
  ~Font();

  bool IsLoaded() const {return _fonts.Size() > 0;}

  RString Name() const {return _name;}

  int GetLangID() const {return _langID;}
  void SetLangID(int langID) {_langID = langID;}

  float GetSpaceWidth() const {return _spaceWidth;}
  float GetSpacing() const {return _spacing;}

  void Load(RString name);
  void Unload();
  
  const FontWithSize *Select(float size) const;
  const FontWithSize *Select1To1(float size) const;
};

#endif

