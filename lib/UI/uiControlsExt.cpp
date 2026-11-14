// Implementation of control hierarchy (HTML, Tree)

#include "../wpch.hpp"
#include "uiControls.hpp"
#include "../keyInput.hpp"
#include "../engine.hpp"
#include "../world.hpp"
#include "../vkCodes.h"
#include <ctype.h>
#include <El/Evaluator/expressImpl.hpp>
#include <El/XML/inet.hpp>

#include "resincl.hpp"
#include "../txtPreload.hpp"

#include <El/QStream/qbStream.hpp>

#include <Es/Algorithms/qsort.hpp>

#include "../mbcs.hpp"

#include "../progress.hpp"

#include "../dikCodes.h"

#include "uiViewport.hpp"

#include "missionDirs.hpp"

#if _VBS2 // categorize game vehicle classes seperately from VBS2 classes
#include "../stringtableExt.hpp"
#else
#include <El/Stringtable/stringtable.hpp>
#endif

/*!
\file
Implementation file for extended controls (HTML control and Tree control).
*/

RString FindPicture(RString name);

// for tree
#if _VBS2
  #define SCROLL_SPEED    1000.0
  #define SCROLL_MIN      2.0
  #define SCROLL_MAX      1000.0
  #define SCROLL_Z_SPEED  100.0
  #define SCROLL_Z_MAX    25.0
#else  
  #define SCROLL_SPEED    100.0
  #define SCROLL_MIN      2.0
  #define SCROLL_MAX      10.0
  #define SCROLL_Z_SPEED  SCROLL_SPEED
  #define SCROLL_Z_MAX    SCROLL_MAX
#endif

#define ISSPACE(c) ((c) >= 0 && (c) <= 32)

#define DrawBottom(i, color) vp->DrawLine(Line2DPixel(xx + i, yy + hh - 1 - i, xx + ww - i, yy + hh - 1 - i),  color, color);
#define DrawRight(i, color) vp->DrawLine(Line2DPixel(xx + ww - 1 - i, yy + hh - 1 - i, xx + ww - 1 - i, yy + 0 + i), color, color);
#define DrawLeft(i, color) vp->DrawLine(Line2DPixel(xx + i, yy + hh - 1 - i, xx + i, yy + i), color, color);
#define DrawTop(i, color) vp->DrawLine(Line2DPixel(xx + i, yy + i, xx + ww - 1 - i, yy + i), color, color);

inline PackedColor ModAlpha( PackedColor color, float alpha )
{
  int a=toIntFloor(alpha*color.A8());
  saturate(a,0,255);
  return PackedColorRGB(color,a);
}

const float textBorder = 0.005;

///////////////////////////////////////////////////////////////////////////////
// class CHTMLContainer

Texture *HTMLField::GetTexture() const
{
  if (condition.GetLength() > 0)
  {
    GameState *gstate = GWorld->GetGameState();
    bool result = gstate->EvaluateBool(condition, GameState::EvalContext::_default, GWorld->GetMissionNamespace()); // mission namespace
    return result ? texture1 : texture2;
  }
  else return texture1;
}

CHTMLContainer::CHTMLContainer(ParamEntryPar cls)
{
  _bgColor = GetPackedColor(cls >> "colorBackground");
  _textColor = GetPackedColor(cls >> "colorText");
  _boldColor = GetPackedColor(cls >> "colorBold");
  _linkColor = GetPackedColor(cls >> "colorLink");
  _activeLinkColor = GetPackedColor(cls >> "colorLinkActive");

  _pictureColor = GetPackedColor(cls >> "colorPicture");
  _pictureColorLink = GetPackedColor(cls >> "colorPictureLink");
  _pictureColorSelected = GetPackedColor(cls >> "colorPictureSelected");
  _pictureColorBorder = GetPackedColor(cls >> "colorPictureBorder");

  _fontH1 = GLOB_ENGINE->LoadFont(GetFontID(cls >> "H1" >> "font"));
  _fontH1Bold = GLOB_ENGINE->LoadFont(GetFontID(cls >> "H1" >> "fontBold"));
  _sizeH1 = cls >> "H1" >> "sizeEx";
  _fontH2 = GLOB_ENGINE->LoadFont(GetFontID(cls >> "H2" >> "font"));
  _fontH2Bold = GLOB_ENGINE->LoadFont(GetFontID(cls >> "H2" >> "fontBold"));
  _sizeH2 = cls >> "H2" >> "sizeEx";
  _fontH3 = GLOB_ENGINE->LoadFont(GetFontID(cls >> "H3" >> "font"));
  _fontH3Bold = GLOB_ENGINE->LoadFont(GetFontID(cls >> "H3" >> "fontBold"));
  _sizeH3 = cls >> "H3" >> "sizeEx";
  _fontH4 = GLOB_ENGINE->LoadFont(GetFontID(cls >> "H4" >> "font"));
  _fontH4Bold = GLOB_ENGINE->LoadFont(GetFontID(cls >> "H4" >> "fontBold"));
  _sizeH4 = cls >> "H4" >> "sizeEx";
  _fontH5 = GLOB_ENGINE->LoadFont(GetFontID(cls >> "H5" >> "font"));
  _fontH5Bold = GLOB_ENGINE->LoadFont(GetFontID(cls >> "H5" >> "fontBold"));
  _sizeH5 = cls >> "H5" >> "sizeEx";
  _fontH6 = GLOB_ENGINE->LoadFont(GetFontID(cls >> "H6" >> "font"));
  _fontH6Bold = GLOB_ENGINE->LoadFont(GetFontID(cls >> "H6" >> "fontBold"));
  _sizeH6 = cls >> "H6" >> "sizeEx";
  _fontP = GLOB_ENGINE->LoadFont(GetFontID(cls >> "P" >> "font"));
  _fontPBold = GLOB_ENGINE->LoadFont(GetFontID(cls >> "P" >> "fontBold"));
  _sizeP = cls >> "P" >> "sizeEx";

  _cycleLinks = true;
  ConstParamEntryPtr entry = cls.FindEntry("cycleLinks");
  if (entry) _cycleLinks = *entry;

  _columns = 1;
  entry = cls.FindEntry("columns");
  if (entry)
  {
    _columns = *entry;
    if (_columns < 1) _columns = 1;
  }
  _columnsSpace = 0.0f;
  entry = cls.FindEntry("columnsSpace");
  if (entry)
  {
    _columnsSpace = *entry;
    saturateMax(_columnsSpace, 0.0f);
  }

  _cycleAllLinks = true;
  entry = cls.FindEntry("cycleAllLinks");
  if (entry) _cycleAllLinks = *entry;

  _prevPage = cls >> "prevPage";
  _nextPage = cls >> "nextPage";

  Init();
  _filename = cls >> "filename";
/*
  RString filename = cls>>"filename";
  if (filename.GetLength() > 0)
    Load(filename);
*/
}

int HTMLContent::FindSection(const char *name)
{
  for (int s=0; s<_sections.Size(); s++)
  {
    HTMLSection &to = _sections[s];
    for (int n=0; n<to.names.Size(); n++)
    {
      if (stricmp(name, to.names[n]) == 0)
        return s;
    }
  }
  return -1;
}

void CHTMLContainer::SwitchSectionRaw(int index)
{
  if (_columns > 1) // the _currentSection need to be multiply of _columns
    index = (index / _columns) * _columns;
  if (index == _currentSection) return;

  _currentSection = index;
  _activeFieldKeyboard = FirstActiveField();
  float mouseX = 0.5 + GInput.cursorX * 0.5;
  float mouseY = 0.5 + GInput.cursorY * 0.5;
  _activeFieldMouse = FindField(mouseX, mouseY);
}

void CHTMLContainer::SwitchSection(const char *name)
{
  int s = FindSection(name);
  if (s >= 0) SwitchSectionRaw(s);
}

int CHTMLContainer::ActiveBookmark() const
{
  if (_sections.Size() <= 0) return -1;
  const HTMLSection &section = _sections[_currentSection];
  int n = section.names.Size();
  int m = _bookmarks.Size();
  for (int i=0; i<n; i++)
  {
    RString name = section.names[i];
    for (int j=0; j<m; j++)
    {
      if (stricmp(name, _bookmarks[j]) == 0)
        return j;
    }
  }
  return -1;
}

RString CHTMLContainer::ActiveBookmarkName() const
{
  if (_sections.Size() <= 0) return RString();
  const HTMLSection &section = _sections[_currentSection];
  int n = section.names.Size();
  int m = _bookmarks.Size();
  for (int i=0; i<n; i++)
  {
    RString name = section.names[i];
    for (int j=0; j<m; j++)
    {
      if (stricmp(name, _bookmarks[j]) == 0)
        return name;
    }
  }
  return RString();
}

const HTMLField *CHTMLContainer::GetActiveFieldMouse() const
{
  if (!_activeFieldMouse.IsValid()) return NULL;
  if (_activeFieldMouse._section >= _sections.Size()) return NULL;
  if (_activeFieldMouse._field >= _sections[_activeFieldMouse._section].fields.Size())
  {
    // fix
    float mouseX = 0.5 + GInput.cursorX * 0.5;
    float mouseY = 0.5 + GInput.cursorY * 0.5;
    CHTMLContainer *fix = unconst_cast(this);
    fix->_activeFieldMouse = FindField(mouseX, mouseY);
    if (!_activeFieldMouse.IsValid()) return NULL;
  }
  return &_sections[_activeFieldMouse._section].fields[_activeFieldMouse._field];
}
  
const HTMLField *CHTMLContainer::GetActiveFieldKeyboard() const
{
  if (!_activeFieldKeyboard.IsValid()) return NULL;
  if (_activeFieldKeyboard._section >= _sections.Size()) return NULL;
  if (_activeFieldKeyboard._field >= _sections[_activeFieldKeyboard._section].fields.Size())
  {
    // fix
    CHTMLContainer *fix = unconst_cast(this);
    fix->_activeFieldKeyboard = FirstActiveField();
    if (!_activeFieldKeyboard.IsValid()) return NULL;
  }
  return &_sections[_activeFieldKeyboard._section].fields[_activeFieldKeyboard._field];
}
 
void HTMLContent::Init()
{
  _sections.Clear();
}

void CHTMLContainer::Init()
{
  base::Init();
  _currentSection = 0;
  _activeFieldMouse = FieldIndex();
  _activeFieldKeyboard = FieldIndex();
}

void HTMLContent::InitSection(int section)
{
  _sections[section].fields.Clear();
  _sections[section].rows.Clear();
  _sections[section].names.Clear();
}

int HTMLContent::AddSection()
{
  return _sections.Add();
}

void HTMLContent::AddBreak(int section, bool bottom, float lines)
{
  if (section < 0 || section >= _sections.Size()) return;
  
  HTMLSection &sec = _sections[section];
  int i = sec.fields.Add();
  HTMLField &fld = sec.fields[i];

//  fld.first = 0;
  fld.nextline = true;
  fld.exclude = false;
  fld.text = "";
  fld.format = HFP;
  fld.href = "";
  fld.bottom = bottom;
  fld.indent = _indent;
  fld.tableWidthAbs = 0;
  fld.tableWidthRel = 0;
  fld.heightAbs = 0;
  fld.heightLines = lines;
}

void HTMLContent::AddText(int section, RString text, HTMLFormat format, HTMLAlign align, bool bottom, bool bold, RString href, float tableWidthAbs, float tableWidthRel,bool colored, PackedColor color, float fontSize, RString fontName)
{  
  if (section < 0 || section >= _sections.Size()) return;

  HTMLSection &sec = _sections[section];
  int i = sec.fields.Add();
  HTMLField &fld = sec.fields[i];

  //  fld.first = 0;
  fld.nextline = false;
  fld.exclude = false;
  fld.text = text;
  fld.format = format;
  fld.align = align;
  fld.bold = bold;
  fld.href = href;
  fld.bottom = bottom;
  fld.indent = _indent;
  fld.tableWidthAbs = tableWidthAbs;
  fld.tableWidthRel = tableWidthRel;
  fld.heightAbs = 0;
  fld.heightLines = 0;
  //font properties
  fld.fontSize = fontSize;  //font size (size/408)
  fld.colored = colored;    //true if specific color should be used  
  fld.color = color;        //font specific color
  //empty and NULL test is present to avoid Font message "font not found" if no name is set
  if(!fontName.IsEmpty()) fld.font = GLOB_ENGINE->LoadFont(GetFontID(fontName));
  //so later i have to compare only against NULL
  if(fld.font != NULL && !fld.font->IsLoaded()) fld.font = NULL;
}

Ref<Texture> HTMLContent::LoadImage(RString path)
{
  bool caseSensitive = true;
  if (IsWebSource(path))
  {
    // keep case for web sources
  }
  else if (IsWebSource(_filename))
  {
    // interpret path as a relative link
    const char *last = strrchr(_filename, '/');
    if (last == NULL) return NULL;
    path = _filename.Substring(0, last - cc_cast(_filename) + 1) + path;
    // keep case for web sources
  }
  else
  {
    if (path[0] == '\\')
    {
      path = path.Substring(1, INT_MAX);
    }
    else
    {
      // find in current directory
      char buffer[256];
      strcpy(buffer, _filename);
      char *ext = strrchr(buffer, '\\');
      if (ext) *(++ext) = 0;
      strcat(buffer, path);
      if (QFBankQueryFunctions::FileExists(buffer))
        path = buffer;
      else
        path = FindPicture(path);
    }
    caseSensitive = false;
  }

  if (path.GetLength() == 0) return NULL;

  I_AM_ALIVE();
  if (GProgress) GProgress->Refresh();
  if (!caseSensitive) path.Lower();
  Ref<Texture> texture = GlobLoadTexture(path, caseSensitive);
  I_AM_ALIVE();
  if (GProgress) GProgress->Refresh();
  
  return texture;
}

HTMLField *HTMLContent::AddImage(int section, RString image, HTMLAlign align, bool bottom, float w, float h, RString href, RString text, float tableWidthAbs, float tableWidthRel)
{
  if (section < 0 || section >= _sections.Size()) return NULL;
  
  HTMLSection &sec = _sections[section];
  int i = sec.fields.Add();
  HTMLField &fld = sec.fields[i];

  fld.format = HFImg;
  fld.align = align;
  fld.nextline = false;
  fld.exclude = false;
  fld.text = text;
  fld.href = href;
  fld.bottom = bottom;
  fld.indent = _indent;
  fld.tableWidthAbs = tableWidthAbs;
  fld.tableWidthRel = tableWidthRel;
  
  const char *q = strchr(image, '?');
  if (q)
  {
    fld.condition = image.Substring(0, q - image);
    RString image1 = q + 1;
    const char *p = strchr(image1, ':');
    if (p)
    {
      RString image2 = p + 1;
      image1 = image1.Substring(0, p - image1);

      fld.texture2 = LoadImage(image2);
      }
    fld.texture1 = LoadImage(image1);
    if (!fld.texture1) fld.texture1 = fld.texture2;
  }
  else
  { 
    fld.texture1 = LoadImage(image);
    fld.texture2 = fld.texture1;
  }
  if (fld.texture1)
  {
    fld.texture1->SetUsageType(Texture::TexUI); // no limits
    if (w < 0)
    {
      MipInfo mip = GLOB_ENGINE->TextBank()->UseMipmap(fld.texture1, 0, 0);
      if (h < 0)
      {
        w = fld.texture1->AWidth();
        h = fld.texture1->AHeight();
      }
      else
      {
        w = fld.texture1->AWidth() * h / fld.texture1->AHeight();
      }
    }
    else if (h < 0)
    {
      h = fld.texture1->AHeight() * w / fld.texture1->AWidth();
    }

    fld.width = w * (1.0 / 640.0);
    fld.heightAbs = h * (1.0 / 480.0);
  }
  else
  {
    fld.width = (w > 0 ? w : h) * (1.0 / 640.0);
    fld.heightAbs = (h > 0 ? h : w) * (1.0 / 480.0);
    saturateMax(fld.width, 0);
    saturateMax(fld.heightAbs, 0);
  }
  fld.heightLines = 0;
  return &fld;
}

void HTMLContent::AddName(int section, RString name)
{
  if (section < 0 || section >= _sections.Size()) return;
  
  HTMLSection &sec = _sections[section];
  sec.names.Add(name);
}

void CHTMLContainer::AddBookmark(RString link)
{
  _bookmarks.Add(link);
}

static RString ReadTag(QIStream &in)
{
  char buf[256];
  int len = 0;
  int c = in.get();
  while (c!=EOF && (isalnum(c) || c == '/'))
  {
    if (len < sizeof(buf)-1) buf[len++] = c;
    c = in.get();
  }
  if (c!=EOF) in.unget();
  buf[len] = 0;
  return buf;
}

static void SkipTag(QIStream &in)
{
  int c = in.get();
  while (c!=EOF && c != '>')
  {
    c = in.get();
  }
}

static void SkipSpaces(QIStream &in)
{
  int c = in.get();
  while (c!=EOF && ISSPACE(c))
  {
    c = in.get();
  }
  if (c!=EOF) in.unget();
}

static RString ReadPropertyName(QIStream &in)
{
  char buf[256];
  int len = 0;
  int c = in.get();
  while (c!=EOF && !ISSPACE(c) && c != '=')
  {
    if (len < sizeof(buf)-1) buf[len++] = c;
    c = in.get();
  }
  if (c!=EOF) in.unget();
  Assert(len<sizeof(buf));
  buf[len] = 0;
  return buf;
}

/*!
\patch 1.45 Date 2/18/2002 by Ondra
- Fixed: Briefing HTML error could cause application crash during mission startup
when malformed HTML was loaded in some custom missions.
*/

static RString ReadPropertyValue(QIStream &in)
{
  SkipSpaces(in);
  int c = in.get();
  if (c != '=')
  {
    if (c!=EOF) in.unget();
    return "";
  }
  
  SkipSpaces(in);
  c = in.get();
  if (c != '"')
  {
    if (c!=EOF) in.unget();
    return "";
  }

  char buf[256];
  int len = 0;
  c = in.get();
  while (c!=EOF && c != '"')
  {
    if (len < sizeof(buf)-1) buf[len++] = c;
    c = in.get();
  }
  Assert(len<sizeof(buf));
  buf[len] = 0;
  return buf;
}

static char ReadChar(QIStream &in)
{
  char buf[2048];
  int len = 0;
  int c = in.get();
  while (c!=EOF && c != ';')
  {
    if (len < sizeof(buf)-1) buf[len++] = c;
    c = in.get();
  }
  buf[len] = 0;
  if (buf[0] == '#') return atoi(buf + 1);
  else if (strcmp(buf, "amp") == 0) return '&';
  else if (strcmp(buf, "quot") == 0) return '\"';
  else if (strcmp(buf, "lt") == 0) return '<';
  else if (strcmp(buf, "gt") == 0) return '>';
//  else if (stricmp(buf, "nbsp") == 0) return ' ';
  else if (strcmp(buf,"nbsp") == 0) return (char)160;
  else if (strcmp(buf,"iexcl") == 0) return (char)161;
  else if (strcmp(buf,"cent") == 0) return (char)162;
  else if (strcmp(buf,"pound") == 0) return (char)163;
  else if (strcmp(buf,"curren") == 0) return (char)164;
  else if (strcmp(buf,"yen") == 0) return (char)165;
  else if (strcmp(buf,"brvbar") == 0) return (char)166;
  else if (strcmp(buf,"sect") == 0) return (char)167;
  else if (strcmp(buf,"uml") == 0) return (char)168;
  else if (strcmp(buf,"copy") == 0) return (char)169;
  else if (strcmp(buf,"ordf") == 0) return (char)170;
  else if (strcmp(buf,"laquo") == 0) return (char)171;
  else if (strcmp(buf,"not") == 0) return (char)172;
  else if (strcmp(buf,"shy") == 0) return (char)173;
  else if (strcmp(buf,"reg") == 0) return (char)174;
  else if (strcmp(buf,"macr") == 0) return (char)175;
  else if (strcmp(buf,"deg") == 0) return (char)176;
  else if (strcmp(buf,"plusmn") == 0) return (char)177;
  else if (strcmp(buf,"sup2") == 0) return (char)178;
  else if (strcmp(buf,"sup3") == 0) return (char)179;
  else if (strcmp(buf,"acute") == 0) return (char)180;
  else if (strcmp(buf,"micro") == 0) return (char)181;
  else if (strcmp(buf,"para") == 0) return (char)182;
  else if (strcmp(buf,"middot") == 0) return (char)183;
  else if (strcmp(buf,"cedil") == 0) return (char)184;
  else if (strcmp(buf,"sup1") == 0) return (char)185;
  else if (strcmp(buf,"ordm") == 0) return (char)186;
  else if (strcmp(buf,"raquo") == 0) return (char)187;
  else if (strcmp(buf,"frac14") == 0) return (char)188;
  else if (strcmp(buf,"frac12") == 0) return (char)189;
  else if (strcmp(buf,"frac34") == 0) return (char)190;
  else if (strcmp(buf,"iquest") == 0) return (char)191;
  else if (strcmp(buf,"Agrave") == 0) return (char)192;
  else if (strcmp(buf,"Aacute") == 0) return (char)193;
  else if (strcmp(buf,"Acirc") == 0) return (char)194;
  else if (strcmp(buf,"Atilde") == 0) return (char)195;
  else if (strcmp(buf,"Auml") == 0) return (char)196;
  else if (strcmp(buf,"Aring") == 0) return (char)197;
  else if (strcmp(buf,"AElig") == 0) return (char)198;
  else if (strcmp(buf,"Ccedil") == 0) return (char)199;
  else if (strcmp(buf,"Egrave") == 0) return (char)200;
  else if (strcmp(buf,"Eacute") == 0) return (char)201;
  else if (strcmp(buf,"Ecirc") == 0) return (char)202;
  else if (strcmp(buf,"Euml") == 0) return (char)203;
  else if (strcmp(buf,"Igrave") == 0) return (char)204;
  else if (strcmp(buf,"Iacute") == 0) return (char)205;
  else if (strcmp(buf,"Icirc") == 0) return (char)206;
  else if (strcmp(buf,"Iuml") == 0) return (char)207;
  else if (strcmp(buf,"ETH") == 0) return (char)208;
  else if (strcmp(buf,"Ntilde") == 0) return (char)209;
  else if (strcmp(buf,"Ograve") == 0) return (char)210;
  else if (strcmp(buf,"Oacute") == 0) return (char)211;
  else if (strcmp(buf,"Ocirc") == 0) return (char)212;
  else if (strcmp(buf,"Otilde") == 0) return (char)213;
  else if (strcmp(buf,"Ouml") == 0) return (char)214;
  else if (strcmp(buf,"times") == 0) return (char)215;
  else if (strcmp(buf,"Oslash") == 0) return (char)216;
  else if (strcmp(buf,"Ugrave") == 0) return (char)217;
  else if (strcmp(buf,"Uacute") == 0) return (char)218;
  else if (strcmp(buf,"Ucirc") == 0) return (char)219;
  else if (strcmp(buf,"Uuml") == 0) return (char)220;
  else if (strcmp(buf,"Yacute") == 0) return (char)221;
  else if (strcmp(buf,"THORN") == 0) return (char)222;
  else if (strcmp(buf,"szlig") == 0) return (char)223;
  else if (strcmp(buf,"agrave") == 0) return (char)224;
  else if (strcmp(buf,"aacute") == 0) return (char)225;
  else if (strcmp(buf,"acirc") == 0) return (char)226;
  else if (strcmp(buf,"atilde") == 0) return (char)227;
  else if (strcmp(buf,"auml") == 0) return (char)228;
  else if (strcmp(buf,"aring") == 0) return (char)229;
  else if (strcmp(buf,"aelig") == 0) return (char)230;
  else if (strcmp(buf,"ccedil") == 0) return (char)231;
  else if (strcmp(buf,"egrave") == 0) return (char)232;
  else if (strcmp(buf,"eacute") == 0) return (char)233;
  else if (strcmp(buf,"ecirc") == 0) return (char)234;
  else if (strcmp(buf,"euml") == 0) return (char)235;
  else if (strcmp(buf,"igrave") == 0) return (char)236;
  else if (strcmp(buf,"iacute") == 0) return (char)237;
  else if (strcmp(buf,"icirc") == 0) return (char)238;
  else if (strcmp(buf,"iuml") == 0) return (char)239;
  else if (strcmp(buf,"eth") == 0) return (char)240;
  else if (strcmp(buf,"ntilde") == 0) return (char)241;
  else if (strcmp(buf,"ograve") == 0) return (char)242;
  else if (strcmp(buf,"oacute") == 0) return (char)243;
  else if (strcmp(buf,"ocirc") == 0) return (char)244;
  else if (strcmp(buf,"otilde") == 0) return (char)245;
  else if (strcmp(buf,"ouml") == 0) return (char)246;
  else if (strcmp(buf,"divide") == 0) return (char)247;
  else if (strcmp(buf,"oslash") == 0) return (char)248;
  else if (strcmp(buf,"ugrave") == 0) return (char)249;
  else if (strcmp(buf,"uacute") == 0) return (char)250;
  else if (strcmp(buf,"ucirc") == 0) return (char)251;
  else if (strcmp(buf,"uuml") == 0) return (char)252;
  else if (strcmp(buf,"yacute") == 0) return (char)253;
  else if (strcmp(buf,"thorn") == 0) return (char)254;
  else if (strcmp(buf,"yuml") == 0) return (char)255;
  else return '?';
}

/*!
\patch 1.27 Date 10/17/2001 by Ondra
- Fixed: buffer overflow in HTML reading, could cause crash when starting mission.
*/

static RString ReadText(QIStream &in)
{
  char buf[4*1024];
  int len = 0;
  int c = in.get();
  while (c!=EOF && c != '<')
  {
    if (c == 0x0a)
    {
      // avoid spaces at line end
      while (len > 0 && buf[len - 1] == ' ') len--;
      // avoid CR LF at begin of the text
      if (len == 0) goto ReadTextContinue;
    }
    if (c != 0x0d) // avoid CR LF -> 2 spaces (CRLF or LF are handled well)
    {
      if (ISSPACE(c)) c = ' ';
      else if (c == '&') c = ReadChar(in);

      if (len < sizeof(buf)-1) buf[len++] = c;
    }
ReadTextContinue:
    c = in.get();
  }
  if (c!=EOF) in.unget();
  buf[len] = 0;
  return buf;
}

static RString ConvertURL(RString src)
{
  char buf[2048];
  const char *ptr = src;

  int j = 0;
  while (*ptr)
  {
    if (strnicmp(ptr, "%20", 3) == 0)
    {
      buf[j++] = ' ';
      ptr += 3;
    }
    else
    {
      buf[j++] = *(ptr++);
    }
  }
  buf[j] = 0;
  return buf;
}

enum HTMLContext
{
  HTMLNone,
  HTMLHTML,
  HTMLHead,
  HTMLBody,
  HTMLP,
  HTMLH1,
  HTMLH2,
  HTMLH3,
  HTMLH4,
  HTMLH5,
  HTMLH6,
  HTMLFont,
  HTMLA,
  HTMLB,
  HTMLAddr,
  HTMLTable,
  HTMLTR,
  HTMLTD,
};

struct HTMLStackItem
{
  HTMLContext context;
  HTMLAlign align;
  HTMLFormat format;
  bool bold;
  float widthAbs;
  float widthRel;
  //font properties
  float fontSize;       //font size
  PackedColor color;    //specific color
  bool colored;         //true if specific color should be used
  RString fontName;     //font name
  
  HTMLStackItem(){}
  HTMLStackItem(HTMLContext c, HTMLAlign a, HTMLFormat f, bool b = false)
  {
    context = c; align = a; format = f; bold = b; widthAbs = 0; widthRel = 0;
    colored = false; color.SetRGB(0xFFFFFF); fontSize = -1; fontName = "";
  }
};
TypeIsSimple(HTMLStackItem);

class HTMLStack : public AutoArray<HTMLStackItem>
{
public:
  void Push(HTMLStackItem item) {Add(item);}
  void Push(HTMLContext c, HTMLAlign a, HTMLFormat f) {Add(HTMLStackItem(c, a, f));}
  HTMLStackItem Pop()
  {
    int n = Size() - 1;
    HTMLStackItem item = Get(n);
    Delete(n);
    return item;
  }
//  HTMLFormat GetFormat();
};

/*
HTMLFormat GetFormat(HTMLContext ctx)
{
  switch (ctx)
  {
    case HTMLP:
    case HTMLAddr:
      return HFP;
    case HTMLH1:
      return HFH1;
    case HTMLH2:
      return HFH2;
    case HTMLH3:
      return HFH3;
    case HTMLH4:
      return HFH4;
    case HTMLH5:
      return HFH5;
    case HTMLH6:
      return HFH6;
    default:
      return HFError;
  }
}

HTMLFormat HTMLStack::GetFormat()
{
  int n = Size() - 1;
  for (int i=n; i>=0; i--)
  {
    HTMLFormat f = ::GetFormat(Get(i).context);
    if (f != HFError) return f;
  }
  return HFError;
}
*/

//! Convert text to PackedColor
PackedColor HTMLContent::ParseColorHexa(RString text)
{
  if (text[0] != '#')
  {
      RptF("Wrong color format %s", (const char *)text);
      return PackedWhite;
  }
  DWORD color = 0;
  int n = text.GetLength();
  for (int i=1; i<n; i++)
  {
      color <<= 4;
      char c = text[i];
      if (c >= '0' && c <= '9')
          color += c - '0';
      else if (c >= 'A' && c <= 'F')
          color += c - 'A' + 10;
      else if (c >= 'a' && c <= 'f')
          color += c - 'a' + 10;
      else
      {
          RptF("Wrong color format %s", (const char *)text);
          return PackedWhite;
      }
  }
  if (n == 7) return PackedColor(color | 0xff000000);
  else if (n == 9) return PackedColor(color);

  RptF("Wrong color format size %s", (const char *)text);
  return PackedWhite;
}

//!body property - text (color for whole page)
void ReadBodyProperties (QIStream &in, HTMLStackItem &item)
{
  SkipSpaces(in);
  char c = in.get();
  if (c!=EOF) in.unget();
  while (c != '>' && c!=EOF)
  {
      RString name = ReadPropertyName(in);
      if (name.GetLength() == 0)
      {
          LogF("SYNTAX ERROR");
          break;
      }
      RString value = ReadPropertyValue(in);
      if (stricmp(name, "text") == 0)
      {
          item.color = HTMLContent::ParseColorHexa(value);
          item.colored = true;
      }
      SkipSpaces(in);
      c = in.get();
      if (c!=EOF) in.unget();
  }
}

//!Read font properties - text align
void ReadParagraphProperties(QIStream &in, HTMLStackItem &item)
{
  SkipSpaces(in);
  char c = in.get();
  if (c!=EOF) in.unget();
  while (c != '>' && c!=EOF)
  {
    RString name = ReadPropertyName(in);
    if (name.GetLength() == 0)
    {
      LogF("SYNTAX ERROR");
      break;
    }
    RString value = ReadPropertyValue(in);
    if (stricmp(name, "align") == 0)
    {
      if (stricmp(value, "left") == 0)
        item.align = HALeft;
      else if (stricmp(value, "center") == 0)
        item.align = HACenter;
      else if (stricmp(value, "right") == 0)
        item.align = HARight;
    }
    SkipSpaces(in);
    c = in.get();
    if (c!=EOF) in.unget();
  }
}

//!Read font properties - color, size, and font type
void ReadFontProperties (QIStream &in, HTMLStackItem &item)
{
  SkipSpaces(in);
  char c = in.get();
  if (c!=EOF) in.unget();
  while (c != '>' && c!=EOF)
  {
    RString name = ReadPropertyName(in);
    if (name.GetLength() == 0)
    {
      LogF("SYNTAX ERROR");
      break;
    }
    RString value = ReadPropertyValue(in);
    if (stricmp(name, "size") == 0)
    {
      item.fontSize = atoi(value)/408.0f; //408 - see commonDefs.hpp
    }
    else if (stricmp(name, "color") == 0)
    {
      item.color = HTMLContent::ParseColorHexa(value);
      item.colored = true;
    }
    else if (stricmp(name, "face") == 0)
    {
      item.fontName = value;
    }
    SkipSpaces(in);
    c = in.get();
    if (c!=EOF) in.unget();
  }
}

RString ReadLocalizedProperties(QIStream &in, HTMLStackItem &item)
{
  SkipSpaces(in);
  char c = in.get();
  if (c!=EOF) in.unget();

  RString _localizedString;
  
  while (c != '>' && c!=EOF)
  {
    RString name = ReadPropertyName(in);
    if (name.GetLength() == 0)
    {
      LogF("SYNTAX ERROR");
      break;
    }
    RString value = ReadPropertyValue(in);
    if (stricmp(name, "id") == 0)
    {
      _localizedString =  LocalizeString(value);
    }

    SkipSpaces(in);
    c = in.get();
    if(c=='/')
    {
      char a = in.get();
      if(a=='>') c = a;
      in.unget();
    }

    if (c!=EOF) in.unget();
  }

  return _localizedString;
}

/*!
\patch 1.44 Date 2/8/2002 by Jirka
- Added: Support for simple tables in html files 
*/

void ReadTableProperties(QIStream &in, HTMLStackItem &item)
{
  SkipSpaces(in);
  char c = in.get();
  if (c!=EOF) in.unget();
  while (c != '>' && c!=EOF)
  {
    RString name = ReadPropertyName(in);
    if (name.GetLength() == 0)
    {
      LogF("SYNTAX ERROR");
      break;
    }
    RString value = ReadPropertyValue(in);
    if (stricmp(name, "width") == 0)
    {
      float w = atoi(value);
      int len = value.GetLength();
      if (len > 0 && value[len - 1] == '%')
        item.widthRel = 0.01 * w;
      else
        item.widthAbs = w * (1.0 / 640.0);
    }
    else if (stricmp(name, "align") == 0)
    {
      if (stricmp(value, "left") == 0)
        item.align = HALeft;
      else if (stricmp(value, "center") == 0)
        item.align = HACenter;
      else if (stricmp(value, "right") == 0)
        item.align = HARight;
    }
    else if (stricmp(name, "size") == 0)
    {
      switch (atoi(value))
      {
      case 0:
        item.format = HFP; break;
      case 1:
        item.format = HFH1; break;
      case 2:
        item.format = HFH2; break;
      case 3:
        item.format = HFH3; break;
      case 4:
        item.format = HFH4; break;
      case 5:
        item.format = HFH5; break;
      case 6:
        item.format = HFH6; break;
      default:
        Fail("Size");
        break;
      }
    }
    SkipSpaces(in);
    c = in.get();
    if (c!=EOF) in.unget();
  }
}

static RString GetAlignment(HTMLAlign align)
{
  switch (align)
  {
    case HALeft:
      return "left";
    case HACenter:
      return "center";
    case HARight:
      return "right";
    default:
      return "unknown";
  }
}

// also in El/preprocC/preprocC.cpp - move into some common source file?

#include <Es/Common/win.h>
#if defined(_WIN32) 
#include <shellapi.h>
#endif

/// check if the file contains Unicode text
/// returns 0 for non-unicode
///         1 for 32 bit unicode
///         2 for UTF-16LE
///         3 for UTF-16BE
static int IsUnicode(QIStream &in)
{
  int c1 = in.get();
  if (in.eof())
  {
    in.seekg(0);
    return 0;
  }
  int c2 = in.get();
  if (in.eof())
  {
    in.seekg(0);
    return 0;
  }

  if (c1 == 0xff && c2 == 0xfe)
  {
    int c3 = in.get();
    if (in.eof())
    {
      in.seekg(2);
      return 2;
    }
    if (c3==0)
    {
      int c4 = in.get();
      if (in.eof())
      {
        in.seekg(2);
        return 2;
      }
      in.seekg(2);
      if (c4==0) return 1; // FF FE 00 00 ... UTF-32, little endian
    }
    in.seekg(2);
    return 2;
  }
  if (c1 == 0xfe && c2 == 0xff)
  {
    RptF("Big-endian unicode not supported");
    return 3;
  }
  if (c1 == 0xef && c2 == 0xbb)
  {
    int c3 = in.get();
    if (!in.eof() && c3 == 0xbf) return 0; // UTF-8 prefix
  }
  in.seekg(0);
  return 0;
}

/// Functor converting Unicode stream to UTF-8 stream
class FileWideCharToMultiByte
{
protected:
  QOStream &_out;
  int       _uniType;

public:
  FileWideCharToMultiByte(QOStream &out, int unicodeType) : _out(out), _uniType(unicodeType) {}

  void operator ()(char *buf, int size)
  {
#if _M_PPC
    // fix the endians
    for (int i=0; i<size; i+=2) {char c = buf[i]; buf[i] = buf[i + 1]; buf[i + 1] = c;}
#endif
    WCHAR *src = (WCHAR *)(buf);
    int srcLen = size / sizeof(WCHAR);
    // check the size of the needed buffer
#ifdef _WIN32
    int dstLen = WideCharToMultiByte(CP_UTF8, 0, src, srcLen, NULL, 0, NULL, NULL);
    // allocate the buffer, convert and write to the stream
    Buffer<char> dst(dstLen);
    WideCharToMultiByte(CP_UTF8, 0, src, srcLen, dst.Data(), dstLen, NULL, NULL);
    _out.write(dst.Data(), dstLen);
#else
    // There was a problem with UTF-16LE on linux (it has been recognized as unicode, but used as 32bit)
    if (_uniType==1)
    {
      int dstLen = WideCharToMultiByte(CP_UTF8, 0, src, srcLen, NULL, 0, NULL, NULL);
      // allocate the buffer, convert and write to the stream
      Buffer<char> dst(dstLen);
      WideCharToMultiByte(CP_UTF8, 0, src, srcLen, dst.Data(), dstLen, NULL, NULL);
      _out.write(dst.Data(), dstLen);
    }
    else // UTF-16LE
    {
      int dstLen = UTF16ToUTF8(buf, size, NULL, 0);
      // allocate the buffer, convert and write to the stream
      Buffer<char> dst(dstLen);
      UTF16ToUTF8(buf, size, dst.Data(), dstLen);
      _out.write(dst.Data(), dstLen);
    }
#endif
  }
};

#if _VBS3
void HTMLContent::Parse(const RString missionDesc,bool add)
{
  Load(NULL,add,missionDesc);
}
#endif

void HTMLContent::Load(const char *filename, bool add, const RString parseString)
{
  _loaded = false;
  if (!add) Init(); // clear current content

#if _VBS3
  AutoArray<char> buffer;  // buffer used to copy text
  QIStrStream inputStream; // input stream used to carry buffer around

  if(filename != NULL)
  {
#endif
    _filename = filename;

    if (IsWebSource(filename))
    {
      size_t size = 0;
      AutoFree data;
      data = DownloadFile(filename, size, NULL);
      if (data != NULL)
      {
        QIStrStream stream(data, size);
        Load(stream);
      }
      return;
    }
    else if (!QFBankQueryFunctions::FileExists(filename))
    {
      // try to find alternate location
      RString missionDir = GetMissionDirectory();
      RString fullname = missionDir + _filename;
      if (missionDir.GetLength()>0 && QFBankQueryFunctions::FileExists(fullname))
      {
        filename = _filename = fullname;
      }
      else
      {
        fullname = GetBaseDirectory() + _filename;
        if (QFBankQueryFunctions::FileExists(fullname))
        {
          filename = _filename = fullname;
        }
        else return;
      }
    }

    QIFStreamB direct;
    direct.AutoOpen(filename);

#ifdef _WIN32
    HandleUnicode handleUnicode(direct);

#if _VBS3
    // Make a copy of the missions briefing screen. This is
    // then passed to AAR. Then is replayed to the user again, when the AAR file is played
    // Note: the mission description is only sent when loading briefing all other times missionDesc get overwritten
    QIStream &inputData = handleUnicode(); 

    // copy the converted file type
    WriteAutoArrayChar writeBuffer(buffer);
    inputData.Process(writeBuffer);

    _missionDescContent = RString(buffer.Data(),buffer.Size());

    QIStrStream cpy(buffer.Data(),buffer.Size());
    inputStream.init(buffer.Data(),buffer.Size());
  }
  else
  {
    inputStream.init(parseString.Data(), parseString.GetLength());// parse the input stream
  }

  QIStream &in = static_cast<QIStream &>(inputStream);
#else
  QIStream &in = handleUnicode();
#endif
#else //LINUX
    QIStrStream converted;
    int unicode = IsUnicode(direct);
    if (unicode)
    {
      // convert file to UTF-8
      QOStrStream out;
      FileWideCharToMultiByte func(out, unicode);
      direct.Process(func);

      // use the output as an input stream
      converted.init(out);
    }

    QIStream &in = unicode ? static_cast<QIStream &>(converted) : static_cast<QIStream &>(direct);
#endif

  Load(in);
}

void HTMLContent::Load(QIStream &in)
{
  _loaded = true;
  AutoArray<char> buffer,bufferTmp;  // buffer used to copy text
 
  QIStrStream inputStream; // input stream used to carry buffer around

  int c;
  HTMLStackItem item(HTMLNone, HALeft, HFP);
  while (true)
  {
    I_AM_ALIVE();

    c = in.get();
    if (in.eof() || in.fail()) break;
    
    if (c == '<')
    {
      // tag
      RString tag = ReadTag(in);
      bufferTmp.Clear();
      if (stricmp(tag, "localized") == 0)
      {
        RString text = ReadLocalizedProperties(in, item);
        bufferTmp.Copy(text,  text.GetLength());
        buffer.Merge(bufferTmp);
        SkipTag(in);
      }
      else
      {
        buffer.Add('<');
        bufferTmp.Copy(tag,  tag.GetLength());
        buffer.Merge(bufferTmp);
      }
    }
    else buffer.Add(c);
  }

  inputStream.init(buffer.Data(),buffer.Size());
  LoadLocalized(inputStream);
}


void HTMLContent::LoadLocalized(QIStream &in)
{
  int section = -1;

#if _VBS3
  RString htmlText;
#endif

  HTMLStackItem item(HTMLNone, HALeft, HFP);
  HTMLStack stack;
  RString href = "";
  bool bottom = false;
  int c;
  while (true)
  {
    I_AM_ALIVE();
    if (GProgress) GProgress->Refresh();

    c = in.get();
    if (in.eof() || in.fail()) break;

    if (c == '<')
    {
      // tag
      RString tag = ReadTag(in);
#if _VBS3 // support for dynamic briefing
      if (tag.GetLength() > 0) htmlText = htmlText + "<" + tag + ">";
#endif
      switch (item.context)
      {
      case HTMLNone:
        if (stricmp(tag, "html") == 0)
        {
          stack.Push(item);
          item.context = HTMLHTML;
        }
        break;
      case HTMLHTML:
        if (stricmp(tag, "/html") == 0)
        {
          item = stack.Pop();
        }
        else if (stricmp(tag, "head") == 0)
        {
          stack.Push(item);
          item.context = HTMLHead;
        }
        else if (stricmp(tag, "body") == 0)
        {
          stack.Push(item);
          item.context = HTMLBody;
          if (section >= 0) FormatSection(section);
          section = AddSection();
          ReadBodyProperties(in, item);
          bottom = false;
        }
        break;
      case HTMLHead:
        if (stricmp(tag, "/head") == 0)
        {
          item = stack.Pop();
        }
        break;
      case HTMLBody:
        if (stricmp(tag, "/body") == 0)
        {
          item = stack.Pop();
        }
        else if (stricmp(tag, "p") == 0)
        {
          stack.Push(item);
          item.context = HTMLP;
          item.format = HFP;
          ReadParagraphProperties(in, item);
        } 
        else if (stricmp(tag, "address") == 0)
        {
          stack.Push(item);
          item.context = HTMLAddr;
          bottom = true;
          ReadParagraphProperties(in, item);
        } 
        else if (stricmp(tag, "h1") == 0)
        {
          stack.Push(item);
          item.context = HTMLH1;
          item.format = HFH1;
          ReadParagraphProperties(in, item);
        }
        else if (stricmp(tag, "h2") == 0)
        {
          stack.Push(item);
          item.context = HTMLH2;
          item.format = HFH2;
          ReadParagraphProperties(in, item);
        }
        else if (stricmp(tag, "h3") == 0)
        {
          stack.Push(item);
          item.context = HTMLH3;
          item.format = HFH3;
          ReadParagraphProperties(in, item);
        }
        else if (stricmp(tag, "h4") == 0)
        {
          stack.Push(item);
          item.context = HTMLH4;
          item.format = HFH4;
          ReadParagraphProperties(in, item);
        }
        else if (stricmp(tag, "h5") == 0)
        {
          stack.Push(item);
          item.context = HTMLH5;
          item.format = HFH5;
          ReadParagraphProperties(in, item);
        }
        else if (stricmp(tag, "h6") == 0)
        {
          stack.Push(item);
          item.context = HTMLH6;
          item.format = HFH6;
          ReadParagraphProperties(in, item);
        }
        else if (stricmp(tag, "font") == 0)
        {
          //tag font - load doesn't manage nested tags and i need to handle...
          //.. font tag from other places, therefore solution is written lower.
          goto ChangeFont;
        }
        else if (stricmp(tag, "table") == 0)
        {
          stack.Push(item);
          item.context = HTMLTable;
        }
        else if (stricmp(tag, "hr") == 0)
        {
          if (section >= 0) FormatSection(section);
#if _VBS3 // support for dynamic briefing
          if (section >= 0)
          {
            int pos = htmlText.Find("<a></a>"); 
            int end = htmlText.GetLength() - htmlText.ReverseFind('/') + 1;
            if (pos > -1) htmlText = htmlText.Substring(pos+7,htmlText.GetLength()-end);
            int len = htmlText.GetLength();
            if (htmlText[len-1] <= 32) htmlText = htmlText.Substring(0,len-1);
            _sections[section].html = htmlText;
            htmlText = RString();
          }
#endif
          section = AddSection();
          bottom = false;
        }
        else goto General;
        break;
      case HTMLP:
        if (stricmp(tag, "/p") == 0)
        {
          item = stack.Pop();
          AddBreak(section, bottom);
        } 
        else goto ChangeFont; //here was  "goto Paragraph", but i need to check other nested tags first
        break;
      case HTMLAddr:
        if (stricmp(tag, "/address") == 0)
        {
          item = stack.Pop();
          AddBreak(section, bottom);
        } 
        else goto ChangeFont;
        break;
      case HTMLH1:
        if (stricmp(tag, "/h1") == 0)
        {
          item = stack.Pop();
          AddBreak(section, bottom);
        }
        else goto ChangeFont;
        break;
      case HTMLH2:
        if (stricmp(tag, "/h2") == 0)
        {
          item = stack.Pop();
          AddBreak(section, bottom);
        }
        else goto ChangeFont;
        break;
      case HTMLH3:
        if (stricmp(tag, "/h3") == 0)
        {
          item = stack.Pop();
          AddBreak(section, bottom);
        }
        else goto ChangeFont;
        break;
      case HTMLH4:
        if (stricmp(tag, "/h4") == 0)
        {
          item = stack.Pop();
          AddBreak(section, bottom);
        }
        else goto ChangeFont;
        break;
      case HTMLH5:
        if (stricmp(tag, "/h5") == 0)
        {
          item = stack.Pop();
          AddBreak(section, bottom);
        }
        else goto ChangeFont;
        break;
      case HTMLH6:
        if (stricmp(tag, "/h6") == 0)
        {
          item = stack.Pop();
          AddBreak(section, bottom);
        }
        else goto ChangeFont;
        break;
      case HTMLFont:
        if (stricmp(tag, "/font") == 0)
        {//font ended, AddBreak is not required
          item = stack.Pop();
        }
        else if (stricmp(tag, "/p") == 0) AddBreak(section, bottom);  //some nested paragraph ended
        else goto Paragraph;
        break;
      case HTMLTable:
        if (stricmp(tag, "/table") == 0)
        {
          item = stack.Pop();
        }
        else if (stricmp(tag, "tr") == 0)
        {
          stack.Push(item);
          item.context = HTMLTR;
        }
        break;
      case HTMLTR:
        if (stricmp(tag, "/tr") == 0)
        {
          item = stack.Pop();
          AddBreak(section, bottom);
        }
        else if (stricmp(tag, "td") == 0)
        {
          stack.Push(item);
          item.context = HTMLTD;
          ReadTableProperties(in, item);
        }
        break;
      case HTMLTD:
        if (stricmp(tag, "/td") == 0)
        {
          item = stack.Pop();
        }
        else goto ChangeFont;
        break;
      case HTMLA:
        if (stricmp(tag, "/a") == 0)
        {
          item = stack.Pop();
          href = "";
        }
        else goto General;
      case HTMLB:
        if (stricmp(tag, "/b") == 0)
        {
          item = stack.Pop();
        }
        else goto General;
        break;
ChangeFont:
        if (stricmp(tag, "font") == 0)
        {//font tag has been found
          stack.Push(item);          
          item.context = HTMLFont;
          ReadFontProperties(in, item);
        }
Paragraph:
        if (stricmp(tag, "a") == 0)
        {
          stack.Push(item);
          item.context = HTMLA;
          SkipSpaces(in);
          c = in.get();
          if (c!=EOF) in.unget();
          while (c != '>' && c!=EOF)
          {
            RString name = ReadPropertyName(in);
            if (name.GetLength() == 0)
            {
              LogF("SYNTAX ERROR");
              break;
            }
            RString value = ReadPropertyValue(in);
            if (stricmp(name, "href") == 0)
            {
              href = ConvertURL(value);
            }
            else if (stricmp(name, "name") == 0)
            {
              AddName(section, value);
            }
            SkipSpaces(in);
            c = in.get();
            if (c!=EOF) in.unget();
          }
        }
        else if (stricmp(tag, "b") == 0)
        {
          stack.Push(item);
          item.bold = true;
          item.context = HTMLB;
        }
        else goto General;
        break;
General:
        if (stricmp(tag, "br") == 0)
        {
          AddBreak(section, bottom);
        }
        else if (stricmp(tag, "img") == 0)
        {
          RString image = "";
          float w = -1.0;
          float h = -1.0;

          SkipSpaces(in);
          c = in.get();
          if (c!=EOF) in.unget();
          while (c != '>' && c!=EOF)
          {
            RString name = ReadPropertyName(in);
            if (name.GetLength() == 0)
            {
              LogF("SYNTAX ERROR");
              break;
            }
            RString value = ReadPropertyValue(in);
            if (stricmp(name, "src") == 0)
            {
              image = value;
            }
            else if (stricmp(name, "width") == 0)
            {
              w = atoi(value);
            }
            else if (stricmp(name, "height") == 0)
            {
              h = atoi(value);
            }
            SkipSpaces(in);
            c = in.get();
            in.unget();
          }
          AddImage(section, image, item.align, bottom, w, h, href, RString(), item.widthAbs, item.widthRel);
        }
        else if (stricmp(tag, "localized") == 0)
        {
          RString text = ReadLocalizedProperties(in, item);
          AddText(section, text, item.format, item.align, bottom, item.bold, href, item.widthAbs, item.widthRel,item.colored,item.color, item.fontSize,item.fontName);
        }
        break;
      }
      SkipTag(in);
    }
    else
    {
      // text
      if (c!=EOF) in.unget();
      RString text = ReadText(in);
#if _VBS3 // support for dynamic briefing
      htmlText = htmlText + text;
#endif
      switch (item.context)
      {
      case HTMLNone:
      case HTMLHTML:
      case HTMLHead:
      case HTMLBody:
        // ignore text
        break;
      case HTMLP:
      case HTMLAddr:
      case HTMLH1:
      case HTMLH2:
      case HTMLH3:
      case HTMLH4:
      case HTMLH5:
      case HTMLH6:
      case HTMLFont:
      case HTMLTD:
      case HTMLA:
      case HTMLB:
        AddText(section, text, item.format, item.align, bottom, item.bold, href, item.widthAbs, item.widthRel,item.colored,item.color, item.fontSize,item.fontName);
        break;
      }
    }
  }
  if (section >= 0) FormatSection(section);

  if (item.context != HTMLNone)
  {
    RptF("Error in HTML file %s", cc_cast(_filename));
    RptF("Context %d",item.context);
  }
  //DoAssert(item.context == HTMLNone); // not an assert not a program logic error
  //DoAssert(stack.Size() == 0);

  _sections.Compact();
}


void CHTMLContainer::RemoveSection(int s)
{
  _sections.Delete(s);
}

void CHTMLContainer::CopySection(int from, int to)
{
  if (from < 0 || from >= _sections.Size()) return;
  if (to < 0 || to >= _sections.Size()) return;

  HTMLSection &src = _sections[from];
  HTMLSection &dest = _sections[to];

  for (int i=0; i<src.fields.Size(); i++)
  {
    HTMLField &fld = src.fields[i]; 
    int index = dest.fields.Add(fld);
    dest.fields[index].indent += _indent;
  }
}

void CHTMLContainer::FormatSection(int s)
{
  float maxLineWidth = GetPageWidth();
  float minHeight = 0;

  HTMLSection &section = _sections[s];
  section.rows.Clear();

  int r = section.rows.Add();
  HTMLRow *row = &section.rows[r];
  row->firstField = 0;
  row->firstPos = 0;
  row->width = 0;
  row->align = HALeft;
  row->height = minHeight;
  
  HTMLFormat format = HFP;
  bool bold = false;
  Font *font = _fontP;
  float size = _sizeP;

  bool bottom = false;

  int n = section.fields.Size();
  for (int f=0; f<n; f++)
  {
    HTMLField &field = section.fields[f];
    float lineWidth = maxLineWidth - field.indent;
    if (field.bottom) bottom = true;
    if (field.nextline)
    {
      row->lastField = f + 1;
      row->lastPos = 0;
      row->bottom = bottom;
      saturateMax(row->height, field.heightAbs);
      saturateMax(row->height, field.heightLines * _sizeP);
      r = section.rows.Add();
      row = &section.rows[r];
      row->firstField = f + 1;
      row->firstPos = 0;
      row->width = 0;
      row->align = HALeft;
      row->height = minHeight;
    }
    else if (field.format == HFImg)
    {
      if (row->width > 0 && row->width + field.width > lineWidth)
      {
        row->lastField = f;
        row->lastPos = 0;
        row->bottom = bottom;
        r = section.rows.Add();
        row = &section.rows[r];
        row->firstField = f;
        row->firstPos = 0;
        row->width = 0;
        row->align = HALeft;
        row->height = minHeight;
      }
      float height = floatMax(field.heightAbs, field.heightLines * _sizeP);
      if (field.text.GetLength() > 0)
      {
        height += _sizeP;
      }
      saturateMax(row->height, height);
      row->width += field.width;
      row->align = field.align;
      if (field.exclude)
      {
        row->lastField = f + 1;
        row->lastPos = 0;
        row->bottom = bottom;
        row->height = 0;
        r = section.rows.Add();
        row = &section.rows[r];
        row->firstField = f + 1;
        row->firstPos = 0;
        row->width = 0;
        row->align = HALeft;
        row->height = minHeight;
      }
    }
    else
    {
     // if (field.format != format || field.bold != bold) - because of fotn settings, same format does not mean same size
      {
        format = field.format;
        bold = field.bold;
        switch (field.format)
        {
        case HFP:
          if (bold)
            font = _fontPBold;
          else
            font = _fontP;
          size = _sizeP;
          break;
        case HFH1:
          if (bold)
            font = _fontH1Bold;
          else
            font = _fontH1;
          size = _sizeH1;
          break;
        case HFH2:
          if (bold)
            font = _fontH2Bold;
          else
            font = _fontH2;
          size = _sizeH2;
          break;
        case HFH3:
          if (bold)
            font = _fontH3Bold;
          else
            font = _fontH3;
          size = _sizeH3;
          break;
        case HFH4:
          if (bold)
            font = _fontH4Bold;
          else
            font = _fontH4;
          size = _sizeH4;
          break;
        case HFH5:
          if (bold)
            font = _fontH5Bold;
          else
            font = _fontH5;
          size = _sizeH5;
          break;
        case HFH6:
          if (bold)
            font = _fontH6Bold;
          else
            font = _fontH6;
          size = _sizeH6;
          break;
        default:
          Fail("Format");
          break;
        }
        //add specific <font> properties
        if(field.font!=NULL)  font = field.font;
        if(field.fontSize!=-1) size = field.fontSize;
      }
      saturateMax(row->height, size);

      if (field.tableWidthAbs > 0)
        row->width += field.tableWidthAbs;
      else if (field.tableWidthRel > 0)
        row->width += field.tableWidthRel * maxLineWidth;
      else
      {
        float curW = row->width;
        float wordW = curW;

        int wordI = -1;
        int n = field.text.GetLength();
        for (int i=0; i<n; i++)
        {
          int iLast = i - 1; // the position before the current character
          char c = field.text[i];
          Assert(c != 0);
          if (ISSPACE(c))
          {
            wordI = i;
            wordW = curW;
          }
          // read the single character in UTF-8 encoding
          char temp[7];
          int j = 0;
          temp[j++] = c;
          if (c & 0x80)
          {
            DoAssert((c & 0xc0) == 0xc0); // the first character
            while (((c = field.text[i + 1]) & 0xc0) == 0x80)
            {
              i++;
              if (j < 5) temp[j++] = c;
              else
              {
                Fail("Character too long");
              }
            };
          }
          temp[j] = 0;
          float cW = GetTextWidth(size, font, temp);

          if (curW + cW > lineWidth)
          {
            if (wordW > 0)
            {
              row->width = wordW;
              i = wordI;
            }
            else
            {
              row->width = curW;
              //hladas fix - infinite loop when lineWidth < 0
              if(cW <= lineWidth) i = iLast;
            }
            row->align = field.align;
            row->lastField = f;
            row->lastPos = i + 1;
            row->bottom = bottom;
            r = section.rows.Add();
            row = &section.rows[r];
            row->firstField = f;
            row->firstPos = i + 1;
            row->width = 0;
            row->height = floatMax(minHeight, size);
            row->align = HALeft;

            curW = 0;
            wordW = 0;
            wordI = i + 1;
          }
          else
            curW += cW;
        }
        row->width = curW;
        row->align = field.align;
      }
    }
  }
  row->lastField = section.fields.Size();
  row->lastPos = 0;
  row->bottom = bottom;

  // delete obsolete rows
  for (r=section.rows.Size()-1; r>=0; r--)
  {
    if (section.rows[r].firstField >= n)
      section.rows.Delete(r);
  }

  SplitSection(s);
#if 0
  // display formated HTML code

  LogF("\nSection %d", s);
  for (int i=0; i<section.fields.Size(); i++)
  {
    HTMLField &field = section.fields[i];
    const char *format;
    switch (field.format)
    {
    case HFError: format = "ERROR"; break;
    case HFP: format = "P"; break;
    case HFH1: format = "H1"; break;
    case HFH2: format = "H2"; break;
    case HFH3: format = "H3"; break;
    case HFH4: format = "H4"; break;
    case HFH5: format = "H5"; break;
    case HFH6: format = "H6"; break;
    case HFImg: format = "Img"; break;
    default: format = "Unknown"; break;
    }
    if (field.nextline) format = "Next Line";
    LogF
    (
      "Field %d (%s): text=%s, width=%.2f, height=%.2f, %s %s, tableWidth = %.2f",
      i, format, (const char *)field.text, field.width, field.height,
      field.bottom ? "bottom" : "top", GetAlignment(field.align), field.tableWidth
    );
  }

  for (int i=0; i<section.rows.Size(); i++)
  {
    HTMLRow &row = section.rows[i];
    LogF
    (
      "Row %d: from [%d, %d] to [%d, %d], width=%.2f, height=%.2f, %s",
      i, row.firstField, row.firstPos,
      row.lastField, row.lastPos,
      row.width, row.height, row.bottom ? "bottom" : "top"
    );
  }
#endif
}

void CHTMLContainer::SplitSection(int s)
{
  bool footer = !_prevPage.IsEmpty() || !_nextPage.IsEmpty();

  HTMLSection source = _sections[s];

  // check if section doesn't fit on one page
  float pageHeight = GetPageHeight();
  int n = source.rows.Size();
  float totalHeight = 0;
  for (int i=0; i<n; i++)
  {
    HTMLRow &row = source.rows[i];
    totalHeight += row.height;
  }
    
  if (totalHeight > pageHeight)
  {
    RString name;
    if (source.names.Size() >= 1) name = source.names[0];
    else name = Format("__SECTION_%d", s);

    char buffer[256];

    RemoveSection(s);

    float rowHeight = _sizeP;
    float imgRowHeight = 1.5f * rowHeight;
    float imgHeight = 480.0f * imgRowHeight;
    if (footer) pageHeight -= 2.0f * rowHeight + imgRowHeight;

    float curHeight = 0;
    int page = 0;

    s = _sections.Add();
    HTMLSection *dest = &_sections[s];
    dest->names.Add(name);
    sprintf(buffer, "%s/%d", (const char *)name, page);
    dest->names.Add(buffer);

    int firstField = 0;
    for (int i=0; i<n; i++)
    {
      HTMLRow &row = source.rows[i];
      curHeight += row.height;
      // keep line with height == 0 with the next page
      float checkHeight = curHeight;
      if (row.height == 0 && i + 1 < n)
      {
        checkHeight += source.rows[i + 1].height;
      }
      // next page?
      if (checkHeight > pageHeight && i > 0) // FIX: do not insert new page before first row
      {
        // complete old page
        int lastField = source.rows[i - 1].lastField;
        for (int j=firstField; j<=lastField; j++)
        {
          if (j >= source.fields.Size()) break;
          dest->fields.Add(source.fields[j]);
        }
        // set the offset for the new page
        firstField = row.firstField;
        if (footer)
        {
          // add link to prev / next
          int f1 = dest->fields.Size();
          {
            // empty line
            AddBreak(s, true);
            int r = dest->rows.Add();
            HTMLRow &rowDest = dest->rows[r];
            rowDest.firstField = f1;
            rowDest.firstPos = 0;
            f1 = dest->fields.Size();
            rowDest.lastField = f1;
            rowDest.lastPos = 0;
            rowDest.height = rowHeight;
            rowDest.width = 0;
            rowDest.bottom = true;
          }
          {
            // empty line
            AddBreak(s, true);
            int r = dest->rows.Add();
            HTMLRow &rowDest = dest->rows[r];
            rowDest.firstField = f1;
            rowDest.firstPos = 0;
            f1 = dest->fields.Size();
            rowDest.lastField = f1;
            rowDest.lastPos = 0;
            rowDest.height = rowHeight;
            rowDest.width = 0;
            rowDest.bottom = true;
          }
          if (page - _columns >= 0)
          {
            // ref to previous page
            int prev = ((page - _columns) / _columns) * _columns; // round to multiply of _columns
            sprintf(buffer, "#%s/%d", (const char *)name, prev);
            AddImage(s, _prevPage, HARight, true, -1.0, imgHeight, buffer);
          }
          int next = ((page + _columns) / _columns) * _columns; // round to multiply of _columns
          sprintf(buffer, "#%s/%d", (const char *)name, next);
          AddImage(s, _nextPage, HARight, true, -1.0, imgHeight, buffer);
          AddBreak(s, true);
          int r = dest->rows.Add();
          HTMLRow &rowDest = dest->rows[r];
          rowDest.firstField = f1;
          rowDest.firstPos = 0;
          f1 = dest->fields.Size();
          rowDest.lastField = f1;
          rowDest.lastPos = 0;
          rowDest.height = imgRowHeight;
          rowDest.width = 0;
          rowDest.bottom = true;
          for (int f=rowDest.firstField; f<rowDest.lastField; f++)
            rowDest.width += dest->fields[f].width;
          rowDest.align = HARight;
        }

        // done with this section, compact it
        dest->fields.Compact();
        dest->names.Compact();
        dest->rows.Compact();

        // new section
        curHeight = row.height;
        page++;
        PatchActiveField(s);
        s = _sections.Add();
        dest = &_sections[s];
        dest->names.Add(name);
        sprintf(buffer, "%s/%d", (const char *)name, page);
        dest->names.Add(buffer);
      }
      int r = dest->rows.Add(row);
      dest->rows[r].firstField -= firstField;
      dest->rows[r].lastField -= firstField;
    }
    // complete last page
    if (n > 0)
    {
      int lastField = source.rows[n - 1].lastField;
      for (int j=firstField; j<=lastField; j++)
      {
        if (j >= source.fields.Size()) break;
        dest->fields.Add(source.fields[j]);
      }
    }
    if (footer)
    {
      // add link to prev
      if (page - _columns >= 0)
      {
        int f1 = dest->fields.Size();
        {
          // empty line
          AddBreak(s, true);
          int r = dest->rows.Add();
          HTMLRow &rowDest = dest->rows[r];
          rowDest.firstField = f1;
          rowDest.firstPos = 0;
          f1 = dest->fields.Size();
          rowDest.lastField = f1;
          rowDest.lastPos = 0;
          rowDest.height = rowHeight;
          rowDest.width = 0;
          rowDest.bottom = true;
        }
        {
          // empty line
          AddBreak(s, true);
          int r = dest->rows.Add();
          HTMLRow &rowDest = dest->rows[r];
          rowDest.firstField = f1;
          rowDest.firstPos = 0;
          f1 = dest->fields.Size();
          rowDest.lastField = f1;
          rowDest.lastPos = 0;
          rowDest.height = rowHeight;
          rowDest.width = 0;
          rowDest.bottom = true;
        }
        int prev = ((page - _columns) / _columns) * _columns; // round to multiply of _columns
        sprintf(buffer, "#%s/%d", (const char *)name, prev);
        AddImage(s, _prevPage, HARight, true, -1.0, imgHeight, buffer);
        AddBreak(s, true);
        int r = dest->rows.Add();
        HTMLRow &rowDest = dest->rows[r];
        rowDest.firstField = f1;
        rowDest.firstPos = 0;
        f1 = dest->fields.Size();
        rowDest.lastField = f1;
        rowDest.lastPos = 0;
        rowDest.height = imgRowHeight;
        rowDest.width = 0;
        rowDest.bottom = true;
        for (int f=rowDest.firstField; f<rowDest.lastField; f++)
          rowDest.width += dest->fields[f].width;
        rowDest.align = HARight;
      }
    }

    // done with this section, compact it
    dest->fields.Compact();
    dest->names.Compact();
    dest->rows.Compact();
  }

  // FIX: set the active fields always after formatting
  PatchActiveField(s);
}

void CHTMLContainer::PatchActiveField(int s)
{
  if (s >= _currentSection && s < _currentSection + _columns)
  {
    if (_activeFieldKeyboard._section < _currentSection || _activeFieldKeyboard._section >= _currentSection + _columns)
      _activeFieldKeyboard = FirstActiveField();
    else
    {
      const HTMLSection &section = _sections[s];
      if (_activeFieldKeyboard._field >= section.fields.Size()) 
        _activeFieldKeyboard = FirstActiveField();
    }
    float mouseX = 0.5 + GInput.cursorX * 0.5;
    float mouseY = 0.5 + GInput.cursorY * 0.5;
    _activeFieldMouse = FindField(mouseX, mouseY);
  }
}

CHTMLContainer::FieldIndex CHTMLContainer::FindField(float x, float y) const
{
  // find the section (column)
  int si = -1;
  float colX = 0;
  float colW = GetPageWidth();
  for (int s=_currentSection; s<_currentSection+_columns; s++)
  {
    if (s >= _sections.Size()) break;
    if (x < colX + colW)
    {
      // found
      si = s; break;
    }
    colX += colW;
    if (x < colX + _columnsSpace) break; // in the space between columns
    colX += _columnsSpace;
  }
  if (si < 0) return FieldIndex(); // not found
  const HTMLSection &section = _sections[si];
  // move x to the selected column
  x -= colX;

  int r = -1;
  float top = 0;
  bool bottom = false;
  for (int i=0; i<section.rows.Size(); i++)
  {
    if (!bottom && section.rows[i].bottom)
    {
      bottom = true;
      top = GetPageHeight();
      for (int rr=i; rr<section.rows.Size(); rr++)
        top -= section.rows[rr].height;
      if (top > y) return FieldIndex();
    }
    top += section.rows[i].height;
    if (top > y)
    {
      r = i;
      break;
    }
  }
  if (r < 0) return FieldIndex();

  const HTMLRow &row = section.rows[r];
  float left;
  switch (row.align)
  {
  case HARight:
    left = GetPageWidth() - row.width;
    break;
  case HACenter:
    left = 0.5 * (GetPageWidth() - row.width);
    break;
  default:
    Assert(row.align == HALeft);
    left = 0;
    break;
  }
  if (left > x) return FieldIndex();
  for (int f=row.firstField; f<=row.lastField; f++)
  {
    if (f >= section.fields.Size()) continue;
    const HTMLField &field = section.fields[f];
    int from = 0;
    if (f == row.firstField) from = row.firstPos;
    int to = INT_MAX;
    if (f == row.lastField) to = row.lastPos;
    left += GetFieldWidth(field, from, to);
    if (left + field.indent > x) return FieldIndex(si, f);
  }

  return FieldIndex();
}

float CHTMLContainer::GetFieldWidth(const HTMLField &field, int from, int to) const
{
  if (field.nextline) return 0;
  if (from >= to) return 0;
  if (field.tableWidthAbs > 0) return field.tableWidthAbs;
  if (field.tableWidthRel > 0) return field.tableWidthRel * GetPageWidth();
  if (field.format == HFImg) return field.width;

  Font *font = _fontP;
  float size = _sizeP;
  switch (field.format)
  {
  case HFP:
    if (field.bold)
      font = _fontPBold;
    else
      font = _fontP;
    size = _sizeP;
    break;
  case HFH1:
    if (field.bold)
      font = _fontH1Bold;
    else
      font = _fontH1;
    size = _sizeH1;
    break;
  case HFH2:
    if (field.bold)
      font = _fontH2Bold;
    else
      font = _fontH2;
    size = _sizeH2;
    break;
  case HFH3:
    if (field.bold)
      font = _fontH3Bold;
    else
      font = _fontH3;
    size = _sizeH3;
    break;
  case HFH4:
    if (field.bold)
      font = _fontH4Bold;
    else
      font = _fontH4;
    size = _sizeH4;
    break;
  case HFH5:
    if (field.bold)
      font = _fontH5Bold;
    else
      font = _fontH5;
    size = _sizeH5;
    break;
  case HFH6:
    if (field.bold)
      font = _fontH6Bold;
    else
      font = _fontH6;
    size = _sizeH6;
    break;
  default:
    Fail("Format");
    break;
  }
  //set possible <font> properties
  if(field.font!=NULL) font = field.font;
  if(field.fontSize!=-1) size = field.fontSize;

  RString text = field.text.Substring(from, to);
  return GetTextWidth(size, font, text);
}

bool CHTMLContainer::ActivateLink(ControlsContainer *parent, int idc, bool mouse)
{
  const FieldIndex &f = mouse ? _activeFieldMouse : _activeFieldKeyboard;
  if (!f.IsValid()) return false;
  if (f._section >= _sections.Size()) return false;

  HTMLSection &section = _sections[f._section];
  HTMLField &field = section.fields[f._field];

  const char *text = field.href;
  if (*text == 0) return false;

  if (*text == '#')
  {
    SwitchSection(text + 1);
  }
  else
  {
    GetControl()->OnEvent(CEHTMLLink, field.href);
    bool handled = false;
    if (parent) handled = parent->OnHTMLLink(idc, field.href);
#if defined(_WIN32) 
    if (!handled)
    {
      // nobody wants this link, process common reaction
      if (IsWebSource(field.href))
      {
        // open in external browser
        RString cmd = Format("url.dll,FileProtocolHandler %s", cc_cast(field.href));
        ShellExecute(NULL, "open", "rundll32.exe", cmd, NULL, SW_SHOWNORMAL);
      }
      else if (IsWebSource(_filename))
      {
        // try to handle it as a relative link
        const char *last = strrchr(_filename, '/');
        if (last != NULL)
        {
          RString link = _filename.Substring(0, last - cc_cast(_filename) + 1) + field.href;
          Load(link);
        }
      }
    }
#endif
  }
  return true;
}

CHTMLContainer::FieldIndex CHTMLContainer::FirstActiveField() const
{
  for (int s=_currentSection; s<_currentSection+_columns; s++)
  {
    if (s >= _sections.Size()) return FieldIndex();
    const HTMLSection &section = _sections[s];

    int n = section.rows.Size();
    if (n == 0) continue;

    int first = section.rows[0].firstField;
    int last = section.rows[n - 1].lastField;
    for (int i=first; i<last; i++)
    {
      if (section.fields[i].href.GetLength() > 0) return FieldIndex(s, i);
    }
  }
  return FieldIndex();
}

CHTMLContainer::FieldIndex CHTMLContainer::LastActiveField() const
{
  int start = std::min(_currentSection + _columns, _sections.Size()) - 1;
  for (int s=start; s>=_currentSection; s--)
  {
    const HTMLSection &section = _sections[s];

    int n = section.rows.Size();
    if (n == 0) continue;
    int first = section.rows[0].firstField;
    int last = section.rows[n - 1].lastField;

    for (int i=last-1; i>=first; i--)
    {
      if (section.fields[i].href.GetLength() > 0) return FieldIndex(s, i);
    }
  }
  return FieldIndex();
}

bool CHTMLContainer::ForEachField(const FieldIndex &from, const FieldIndex &to, const std::function<bool(const FieldIndex &index, const HTMLSection &section, const HTMLField &field)> &func) const
{
  FieldIndex index;
  for (index._section=from._section; index._section<=to._section; index._section++)
  {
    const HTMLSection &section = _sections[index._section];
    int start = index._section == from._section ? from._field : 0;
    int end = index._section == to._section ? to._field : section.fields.Size() - 1;
    for (index._field=start; index._field<=end; index._field++)
    {
      if (func(index, section, section.fields[index._field])) return true;
    }
  }
  return false;
}

bool CHTMLContainer::ForEachFieldReverse(const FieldIndex &from, const FieldIndex &to, const std::function<bool(const FieldIndex &index, const HTMLSection &section, const HTMLField &field)> &func) const
{
  FieldIndex index;
  for (index._section=from._section; index._section>=to._section; index._section--)
  {
    const HTMLSection &section = _sections[index._section];
    int start = index._section == from._section ? from._field : section.fields.Size() - 1;
    int end = index._section == to._section ? to._field : 0;
    for (index._field=start; index._field>=end; index._field--)
    {
      if (func(index, section, section.fields[index._field])) return true;
    }
  }
  return false;
}

bool CHTMLContainer::NextActiveField(FieldIndex &activeField) const
{
  int lastSection = std::min(_currentSection + _columns, _sections.Size()) - 1;

  if (!activeField.IsValid() || activeField._section < _currentSection || activeField._section > lastSection)
  {
    activeField = FirstActiveField();
    return activeField.IsValid();
  }

  FieldIndex first, last;
  if (_cycleAllLinks)
  {
    for (int s=_currentSection; s<=lastSection; s++)
    {
      const HTMLSection &section = _sections[s];
      int n = section.rows.Size();
      if (n > 0)
      {
        first = FieldIndex(s, section.rows[0].firstField);
        break;
      }
    }
    if (!first.IsValid()) return false;
    for (int s=lastSection; s>=_currentSection; s--)
    {
      const HTMLSection &section = _sections[s];
      int n = section.rows.Size();
      if (n > 0)
      {
        const HTMLRow &row = section.rows[n - 1];
        int lastField = row.lastPos == 0 ? row.lastField - 1 : row.lastField;
        last = FieldIndex(s, lastField);
        break;
      }
    }
    Assert(last.IsValid());
  }
  else
  {
    const HTMLSection &section = _sections[activeField._section];
    int n = section.rows.Size();
    // find the current row
    int r = -1;
    for (int i=n-1; i>=0; i--)
    {
      const HTMLRow &row = section.rows[i];
      int lastField = row.lastPos == 0 ? row.lastField - 1 : row.lastField;
      if (row.firstField <= activeField._field && lastField >= activeField._field)
      {
        r = i;
        break;
      }
    }
    if (r < 0) return false;

    // cycling possible through current row only
    first = FieldIndex(activeField._section, section.rows[r].firstField);
    last = FieldIndex(activeField._section, section.rows[r].lastField);
  }

  // now, find the best field in range first ... last
  if (ForEachField(FieldIndex(activeField._section, activeField._field + 1), last, [&activeField](const FieldIndex &index, const HTMLSection &section, const HTMLField &field) -> bool
  {
    if (!field.href.IsEmpty())
    {
      activeField = index;
      return true;
    }
    return false;
  })) return true;

  if (!_cycleLinks) return false;

  return ForEachField(first, FieldIndex(activeField._section, activeField._field - 1), [&activeField](const FieldIndex &index, const HTMLSection &section, const HTMLField &field) -> bool
  {
    if (!field.href.IsEmpty())
    {
      activeField = index;
      return true;
    }
    return false;
  });
}

bool CHTMLContainer::PrevActiveField(FieldIndex &activeField) const
{
  int lastSection = std::min(_currentSection + _columns, _sections.Size()) - 1;

  if (!activeField.IsValid() || activeField._section < _currentSection || activeField._section > lastSection)
  {
    activeField = LastActiveField();
    return activeField.IsValid();
  }

  FieldIndex first, last;
  if (_cycleAllLinks)
  {
    for (int s=_currentSection; s<=lastSection; s++)
    {
      const HTMLSection &section = _sections[s];
      int n = section.rows.Size();
      if (n > 0)
      {
        first = FieldIndex(s, section.rows[0].firstField);
        break;
      }
    }
    if (!first.IsValid()) return false;
    for (int s=lastSection; s>=_currentSection; s--)
    {
      const HTMLSection &section = _sections[s];
      int n = section.rows.Size();
      if (n > 0)
      {
        const HTMLRow &row = section.rows[n - 1];
        int lastField = row.lastPos == 0 ? row.lastField - 1 : row.lastField;
        last = FieldIndex(s, lastField);
        break;
      }
    }
    Assert(last.IsValid());
  }
  else
  {
    const HTMLSection &section = _sections[activeField._section];
    int n = section.rows.Size();
    // find the current row
    int r = -1;
    for (int i=n-1; i>=0; i--)
    {
      const HTMLRow &row = section.rows[i];
      int lastField = row.lastPos == 0 ? row.lastField - 1 : row.lastField;
      if (row.firstField <= activeField._field && lastField >= activeField._field)
      {
        r = i;
        break;
      }
    }
    if (r < 0) return false;

    // cycling possible through current row only
    first = FieldIndex(activeField._section, section.rows[r].firstField);
    last = FieldIndex(activeField._section, section.rows[r].lastField);
  }

  // now, find the best field in range first ... last
  if (ForEachFieldReverse(FieldIndex(activeField._section, activeField._field - 1), first, [&activeField](const FieldIndex &index, const HTMLSection &section, const HTMLField &field) -> bool
  {
    if (!field.href.IsEmpty())
    {
      activeField = index;
      return true;
    }
    return false;
  })) return true;

  if (!_cycleLinks) return false;

  return ForEachFieldReverse(last, FieldIndex(activeField._section, activeField._field + 1), [&activeField](const FieldIndex &index, const HTMLSection &section, const HTMLField &field) -> bool
  {
    if (!field.href.IsEmpty())
    {
      activeField = index;
      return true;
    }
    return false;
  });
}

int CHTMLContainer::BestActiveField(int sectionIndex, int rowIndex, float offset) const
{
  const HTMLSection &section = _sections[sectionIndex];
  const HTMLRow &row = section.rows[rowIndex];

  float error = FLT_MAX;
  int best = -1;
  float x = 0;
  switch (row.align)
  {
  case HARight:
    x = GetPageWidth() - row.width;
    break;
  case HACenter:
    x = 0.5 * (GetPageWidth() - row.width);
    break;
  default:
    Assert(row.align == HALeft);
    x = 0;
    break;
  }
  for (int f=row.firstField; f<=row.lastField; f++)
  {
    int from = f == row.firstField ? row.firstPos : 0;
    int to = f == row.lastField ? row.lastPos : INT_MAX;
    if (to == 0) break; // last field not included
    const HTMLField &field = section.fields[f];
    float w = GetFieldWidth(field, from, to);
    if (field.href.GetLength() > 0)
    {
      // float e = fabs(offset - (x + 0.5 * w));
      float e = 0;
      if (offset < x) e = x - offset;
      else if (offset > x + w) e = offset - x - w;
      if (e < error)
      {
        best = f;
        error = e;
      }
    }
    x += w;
  }
  return best;
}

float CHTMLContainer::GetFieldOffset(const HTMLSection &section, const HTMLRow &row, int fld) const
{
  float offset = 0;
  switch (row.align)
  {
  case HARight:
    offset = GetPageWidth() - row.width;
    break;
  case HACenter:
    offset = 0.5 * (GetPageWidth() - row.width);
    break;
  default:
    Assert(row.align == HALeft);
    offset = 0;
    break;
  }
  for (int f=row.firstField; f<=fld; f++)
  {
    const HTMLField &field = section.fields[f];
    int from = f == row.firstField ? row.firstPos : 0;
    int to = f == row.lastField ? row.lastPos : INT_MAX;
    float w = GetFieldWidth(field, from, to);
    if (f == fld)
      offset += 0.5 * w;
    else
      offset += w;
  }
  return offset;
}

bool CHTMLContainer::ForEachRow(int from, int to, const std::function<bool(int sectionIndex, int rowIndex, int totalIndex)> &func) const
{
  // find the section to start from
  int sectionIndex = -1;
  int sectionOffset = 0;
  for (int s=_currentSection; s<_currentSection+_columns; s++)
  {
    if (s >= _sections.Size()) break;
    const HTMLSection &section = _sections[s];
    if (from < sectionOffset + section.rows.Size())
    {
      // section found
      sectionIndex = s;
      break;
    }
    sectionOffset += section.rows.Size();
  }
  int totalIndex = from;
  if (sectionIndex < 0)
  {
    // from out of range, set it to the last row
    sectionIndex = std::min(_currentSection + _columns, _sections.Size()) - 1;
    if (sectionIndex < _currentSection) return false;
    totalIndex = sectionOffset - 1;
    // move offset to the last section
    const HTMLSection &section = _sections[sectionIndex];
    sectionOffset -= section.rows.Size();
  }

  // enumeration
  bool first = true;
  int start = totalIndex - sectionOffset;
  if (to < from)
  {
    // reverse order
    for (int s=sectionIndex; s>=_currentSection; s--)
    {
      const HTMLSection &section = _sections[s];
      if (first) first = false;
      else start = section.rows.Size() - 1;
      for (int r=start; r>=0; r--)
      {
        if (func(s, r, totalIndex)) return true;
        totalIndex--;
        if (totalIndex < to) return false;
      }
    }
    return false;
  }
  else
  {
    for (int s=sectionIndex; s<_currentSection+_columns; s++)
    {
      if (s >= _sections.Size()) break;
      const HTMLSection &section = _sections[s];
      if (first) first = false;
      else start = 0;
      for (int r=start; r<section.rows.Size(); r++)
      {
        if (func(s, r, totalIndex)) return true;
        totalIndex++;
        if (totalIndex > to) return false;
      }
    }
    return false;
  }
}

bool CHTMLContainer::NextRowActiveField(FieldIndex &activeField) const
{
  if (!activeField.IsValid()) return false;

  int lastSection = std::min(_currentSection + _columns, _sections.Size()) - 1;
  if (activeField._section < _currentSection || activeField._section > lastSection)
  {
    activeField = FirstActiveField();
    return activeField.IsValid();
  }

  // find current row
  const HTMLSection &section = _sections[activeField._section];
  int n = section.rows.Size();
  int r = -1;
  for (int i=0; i<n; i++)
  {
    const HTMLRow &row = section.rows[i];
    int lastField = row.lastPos == 0 ? row.lastField - 1 : row.lastField;
    if (row.firstField <= activeField._field && lastField >= activeField._field)
    {
      r = i;
      break;
    }
  }
  if (r < 0) return false;

  // find offset of field in current row
  const HTMLRow &row = section.rows[r];
  float offset = GetFieldOffset(section, row, activeField._field);

  // convert row index to total index
  int totalIndex = r;
  for (int s=_currentSection; s<activeField._section; s++) totalIndex += _sections[s].rows.Size();

  // check next rows
  if (ForEachRow(totalIndex + 1, INT_MAX, [this, offset, &activeField](int sectionIndex, int rowIndex, int totalIndex) -> bool
  {
    int f = BestActiveField(sectionIndex, rowIndex, offset);
    if (f >= 0)
    {
      activeField._section = sectionIndex; activeField._field = f; return true;
    }
    return false;
  })) return true;

  if (!_cycleLinks) return false;

  return ForEachRow(0, totalIndex - 1, [this, offset, &activeField](int sectionIndex, int rowIndex, int totalIndex) -> bool
  {
    int f = BestActiveField(sectionIndex, rowIndex, offset);
    if (f >= 0)
    {
      activeField._section = sectionIndex; activeField._field = f; return true;
    }
    return false;
  });
}

bool CHTMLContainer::PrevRowActiveField(FieldIndex &activeField) const
{
  if (!activeField.IsValid()) return false;

  int lastSection = std::min(_currentSection + _columns, _sections.Size()) - 1;
  if (activeField._section < _currentSection || activeField._section > lastSection)
  {
    activeField = LastActiveField();
    return activeField.IsValid();
  }

  // find current row
  const HTMLSection &section = _sections[activeField._section];
  int n = section.rows.Size();
  int r = -1;
  for (int i=0; i<n; i++)
  {
    const HTMLRow &row = section.rows[i];
    int lastField = row.lastPos == 0 ? row.lastField - 1 : row.lastField;
    if (row.firstField <= activeField._field && lastField >= activeField._field)
    {
      r = i;
      break;
    }
  }
  if (r < 0) return false;

  // find offset of field in current row
  const HTMLRow &row = section.rows[r];
  float offset = GetFieldOffset(section, row, activeField._field);

  // convert row index to total index
  int totalIndex = r;
  for (int s=_currentSection; s<activeField._section; s++) totalIndex += _sections[s].rows.Size();

  // check previous rows
  if (ForEachRow(totalIndex - 1, 0, [this, offset, &activeField](int sectionIndex, int rowIndex, int totalIndex) -> bool
  {
    int f = BestActiveField(sectionIndex, rowIndex, offset);
    if (f >= 0)
    {
      activeField._section = sectionIndex; activeField._field = f; return true;
    }
    return false;
  })) return true;

  if (!_cycleLinks) return false;

  return ForEachRow(INT_MAX, totalIndex + 1, [this, offset, &activeField](int sectionIndex, int rowIndex, int totalIndex) -> bool
  {
    int f = BestActiveField(sectionIndex, rowIndex, offset);
    if (f >= 0)
    {
      activeField._section = sectionIndex; activeField._field = f; return true;
    }
    return false;
  });
}

///////////////////////////////////////////////////////////////////////////////
// class CHTML

CHTML::CHTML(ControlsContainer *parent, int idc, ParamEntryPar cls)
  : Control(parent, CT_HTML, idc, cls),
    CHTMLContainer(cls)
{
  if (_filename.GetLength() > 0)
    Load(_filename);
}

bool CHTML::IsEnabled() const
{
  return _enabled;
}

void CHTML::OnLButtonDown(float x, float y)
{
  if (IsInside(x, y))
  {
    _activeFieldMouse = FindField(x, y);
    ActivateLink(_parent, IDC(), true);
  }
}

bool CHTML::OnKeyDown(int dikCode)
{
  switch (dikCode)
  {
  case DIK_UP:
  case INPUT_DEVICE_XINPUT + XBOX_Up:
  case INPUT_DEVICE_XINPUT + XBOX_LeftThumbYUp:
    {
      bool result = PrevRowActiveField(_activeFieldKeyboard);
      if (_parent && result) _parent->UpdateKeyHints();
      return result;
    }
  case DIK_LEFT:
  case INPUT_DEVICE_XINPUT + XBOX_Left:
  case INPUT_DEVICE_XINPUT + XBOX_LeftThumbXLeft:
    {
      bool result = PrevActiveField(_activeFieldKeyboard);
      if (_parent && result) _parent->UpdateKeyHints();
      return result;
    }
  case DIK_DOWN:
  case INPUT_DEVICE_XINPUT + XBOX_Down:
  case INPUT_DEVICE_XINPUT + XBOX_LeftThumbYDown:
    {
      bool result = NextRowActiveField(_activeFieldKeyboard);
      if (_parent && result) _parent->UpdateKeyHints();
      return result;
    }
  case DIK_RIGHT:
  case INPUT_DEVICE_XINPUT + XBOX_Right:
  case INPUT_DEVICE_XINPUT + XBOX_LeftThumbXRight:
    {
      bool result = NextActiveField(_activeFieldKeyboard);
      if (_parent && result) _parent->UpdateKeyHints();
      return result;
    }
  case DIK_SPACE:
  case INPUT_DEVICE_XINPUT + XBOX_A:
    return true;
  }
  return Control::OnKeyDown(dikCode);
}

bool CHTML::OnKeyUp(int dikCode)
{
  switch (dikCode)
  {
  case DIK_SPACE:
  case INPUT_DEVICE_XINPUT + XBOX_A:
    return ActivateLink(_parent, IDC(), false);
  }
  return Control::OnKeyUp(dikCode);
}

void CHTML::OnMouseMove(float x, float y, bool active)
{
  if (active)
    _activeFieldMouse = FindField(x, y);
  else
    _activeFieldMouse = FieldIndex();
}

void CHTML::OnMouseHold(float x, float y, bool active)
{
  if (active)
    _activeFieldMouse = FindField(x, y);
  else
    _activeFieldMouse = FieldIndex();
}

bool CHTML::OnSetFocus(ControlFocusAction action, bool def)
{
  if (!Control::OnSetFocus(action, def)) return false;
  switch (action)
  {
  case CFAPrev:
    _activeFieldKeyboard = LastActiveField();
    break;
  case CFANext:
    _activeFieldKeyboard = FirstActiveField();
    break;
  }
  return true;
}

bool IsTextureReady(Texture *texture);
bool IsFontReady(Font *font, float size);

bool CHTML::IsReady() const
{
  bool ready = true;

  for (int s=_currentSection; s<_currentSection+_columns; s++)
  {
    if (s >= _sections.Size()) break;

    const HTMLSection &section = _sections[s];

    for (int f=0; f<section.fields.Size(); f++)
    {
      const HTMLField &field = section.fields[f];
      if (field.nextline) continue;
      if (field.format == HFImg)
      {
        if (field.text.GetLength() > 0)
        {
          if (!IsFontReady(_fontP, _scale * SCALED(_sizeP))) ready = false;
        }
        Texture *texture = field.GetTexture();
        if (texture)
        {
          if (!IsTextureReady(texture)) ready = false;
        }
      }
      else
      {
        Font *font = _fontP;
        float size = _scale * _sizeP;
        switch (field.format)
        {
        case HFP:
          if (field.bold)
            font = _fontPBold;
          else
            font = _fontP;
          size = _scale * _sizeP;
          break;
        case HFH1:
          if (field.bold)
            font = _fontH1Bold;
          else
            font = _fontH1;
          size = _scale * _sizeH1;
          break;
        case HFH2:
          if (field.bold)
            font = _fontH2Bold;
          else
            font = _fontH2;
          size = _scale * _sizeH2;
          break;
        case HFH3:
          if (field.bold)
            font = _fontH3Bold;
          else
            font = _fontH3;
          size = _scale * _sizeH3;
          break;
        case HFH4:
          if (field.bold)
            font = _fontH4Bold;
          else
            font = _fontH4;
          size = _scale * _sizeH4;
          break;
        case HFH5:
          if (field.bold)
            font = _fontH5Bold;
          else
            font = _fontH5;
          size = _scale * _sizeH5;
          break;
        case HFH6:
          if (field.bold)
            font = _fontH6Bold;
          else
            font = _fontH6;
          size = _scale * _sizeH6;
          break;
        default:
          Fail("Format");
          break;
        }
        //set possible <font> properties
        if(field.font!=NULL) font = field.font;
        if(field.fontSize!=-1) size = _scale * field.fontSize;

        if (!IsFontReady(font, SCALED(size))) ready = false;
      }
    }
  }
  return ready;
}

#define DRAW_TABLE_BORDERS  0

void CHTML::OnDraw(UIViewport *vp, float alpha)
{
  const int w = GLOB_ENGINE->Width2D();
  const int h = GLOB_ENGINE->Height2D();

  PackedColor pictureColor = ModAlpha(_pictureColor, alpha);;
  PackedColor pictureColorLink = ModAlpha(_pictureColorLink, alpha);;
  PackedColor pictureColorSelected = ModAlpha(_pictureColorSelected, alpha);;
  PackedColor pictureColorBorder = ModAlpha(_pictureColorBorder, alpha);;
  PackedColor bgColor = ModAlpha(_bgColor, alpha);

  {
    float xx = CX(_x);
    float yy = CY(_y);
    float ww = CW(SCALED(_w));
    float hh = CH(SCALED(_h));

    Draw2DParsExt pars;
    pars.mip = GEngine->TextBank()->UseMipmap(NULL, 0, 0);
    pars.SetColor(bgColor);
    pars.Init();
    vp->Draw2D(pars, Rect2DPixel(xx, yy, ww, hh));
  }

  float colX = _x;
  float colW = _w / _columns - (_columns - 1) * _columnsSpace;

  for (int s=_currentSection; s<_currentSection+_columns; s++)
  {
    if (s >= _sections.Size()) return;
    HTMLSection &section = _sections[s];

    float top = _y;
    bool bottom = false;

    float maxPageWidth = GetPageWidth();

    bool focused = IsFocused();

    for (int r=0; r<section.rows.Size(); r++)
    {
      HTMLRow &row = section.rows[r];
      if (!bottom && row.bottom)
      {
        bottom = true;
        top = _y + SCALED(_h);
        for (int rr=r; rr<section.rows.Size(); rr++)
          top -= _scale * SCALED(section.rows[rr].height);
      }
      float left;
      switch (row.align)
      {
      case HARight:
        left = colX + SCALED(colW) - _scale * (SCALED(row.width + textBorder));
        break;
      case HACenter:
        left = colX + 0.5 * (SCALED(colW - _scale * row.width));
        break;
      default:
        Assert(row.align == HALeft);
        left = colX + _scale * SCALED(textBorder);
        break;
      }
      for (int f=row.firstField; f<=row.lastField; f++)
      {
        if (f >= section.fields.Size()) continue;
        HTMLField &field = section.fields[f];
        if (field.nextline) continue;
        int from = 0;
        if (f == row.firstField) from = row.firstPos;
        int to = INT_MAX;
        if (f == row.lastField) to = row.lastPos;
        if (from >= to) continue;

        float fieldWidth = 0;
        if (field.tableWidthAbs > 0) fieldWidth =  SCALED(field.tableWidthAbs);
        else if (field.tableWidthRel > 0) fieldWidth = field.tableWidthRel * SCALED(maxPageWidth);

        float fieldHeight = SCALED(floatMax(field.heightAbs, field.heightLines * _sizeP));

        if (field.format == HFImg)
        {
          float l = left;
          if (fieldWidth > 0)
          {
            switch (field.align)
            {
            case HARight:
              l += _scale * (fieldWidth - SCALED(field.width));
              break;
            case HACenter:
              l += 0.5 * _scale * (fieldWidth - SCALED(field.width));
              break;
            }
          }
          if (field.text.GetLength() > 0)
          {
            float t;
            if (row.height > 0)
              t = top + _scale * (SCALED(row.height - _sizeP) - fieldHeight);
            else
              t = top - _scale * SCALED(_sizeP);
            float textWidth = GetTextWidth(SCALED(_sizeP), _fontP, field.text);
            float lText = l + 0.5 * _scale * (SCALED(field.width + field.indent) - textWidth);
            PackedColor color;
            if (field.href.GetLength() == 0)
              color = _textColor;
            else if (focused && _activeFieldKeyboard._section == s && _activeFieldKeyboard._field == f)
              color = _activeLinkColor;
            else
              color = _linkColor;

            TextDrawAttr attr(_scale * SCALED(_sizeP), _fontP, color, _shadow);
            vp->DrawText(Point2DFloat(lText, t), Rect2DFloat(colX, _y, SCALED(colW), SCALED(_h)), attr, field.text);
          
            if (_activeFieldMouse._section == s && _activeFieldMouse._field == f && field.href.GetLength() > 0)
            {
              float b = t + _scale * SCALED(_sizeP);
              vp->DrawLine
              (
                Line2DFloat(lText, b, lText + _scale * textWidth, b),
                color, color, Rect2DFloat(colX, _y, SCALED(colW), SCALED(_h))
              );
            }
          }
          Texture *texture = field.GetTexture();
          if (texture)
          {
            float t;
            if (row.height > 0)
              t = top + _scale * (SCALED(row.height) - fieldHeight);
            else
              t = top;
            Draw2DParsExt pars;
            pars.mip = GEngine->TextBank()->UseMipmap(texture, 0, 0);
            pars.spec = NoZBuf|IsAlpha|ClampU|ClampV|IsAlphaFog;
            PackedColor color;
            bool selected = false;
            if (field.href.GetLength() == 0)
              color = pictureColor;
            else if (focused && _activeFieldKeyboard._section == s && _activeFieldKeyboard._field == f)
            {
              color = pictureColorSelected;
              selected = true;
            }
            else
              color = pictureColorLink;
            pars.SetColor(color);
            pars.SetU(0,1);
            pars.SetV(0,1);
            Rect2DPixel rect((l + _scale * SCALED(field.indent)) * w, t * h, 
              _scale * SCALED(field.width) * w, _scale * fieldHeight * h);
            Rect2DPixel clip(colX * w, _y * h, SCALED(colW) * w, SCALED(_h) * h);
            vp->Draw2D(pars, rect, clip);
            if (selected)
            {
              AUTO_STATIC_ARRAY(Line2DPixelInfo, lines, 4)
              Line2DPixelInfo line;
              line.c0 = pictureColorBorder;
              line.c1 = pictureColorBorder;
              float left = CX(l + _scale * SCALED(field.indent));
              float right = CX(l + _scale * SCALED(field.indent + field.width));
              float top = CY(t);
              float bottom = CY(t + _scale * SCALED(fieldHeight));
              line.beg = Point2DPixel(left, top);
              line.end = Point2DPixel(right, top);
              lines.Add(line);
              line.beg = Point2DPixel(right, top);
              line.end = Point2DPixel(right, bottom);
              lines.Add(line);
              line.beg = Point2DPixel(right, bottom);
              line.end = Point2DPixel(left, bottom);
              lines.Add(line);
              line.beg = Point2DPixel(left, bottom);
              line.end = Point2DPixel(left, top);
              lines.Add(line);
              vp->DrawLines(lines.Data(), lines.Size(), clip);
            }
          }
          if (fieldWidth > 0)
          {
  #if DRAW_TABLE_BORDERS
            vp->DrawLine
            (
              CX(left), CY(top),
              CX(left + _scale * fieldWidth), CY(top),
              PackedBlack, PackedBlack
            );
            vp->DrawLine
            (
              CX(left + _scale * fieldWidth), CY(top),
              CX(left + _scale * fieldWidth), CY(top + _scale * SCALED(row.height)),
              PackedBlack, PackedBlack
            );
            vp->DrawLine
            (
              CX(left + _scale * fieldWidth), CY(top + _scale * SCALED(row.height)),
              CX(left), CY(top + _scale * SCALED(row.height)),
              PackedBlack, PackedBlack
            );
            vp->DrawLine
            (
              CX(left), CY(top + _scale * SCALED(row.height)),
              CX(left), CY(top),
              PackedBlack, PackedBlack
            );
  #endif
            left += _scale * fieldWidth;
          }
          else
            left += _scale * field.width;
        }
        else
        {
          Font *font = _fontP;
          float size = _scale * _sizeP;
          switch (field.format)
          {
          case HFP:
            if (field.bold)
              font = _fontPBold;
            else
              font = _fontP;
            size = _scale * _sizeP;
            break;
          case HFH1:
            if (field.bold)
              font = _fontH1Bold;
            else
              font = _fontH1;
            size = _scale * _sizeH1;
            break;
          case HFH2:
            if (field.bold)
              font = _fontH2Bold;
            else
              font = _fontH2;
            size = _scale * _sizeH2;
            break;
          case HFH3:
            if (field.bold)
              font = _fontH3Bold;
            else
              font = _fontH3;
            size = _scale * _sizeH3;
            break;
          case HFH4:
            if (field.bold)
              font = _fontH4Bold;
            else
              font = _fontH4;
            size = _scale * _sizeH4;
            break;
          case HFH5:
            if (field.bold)
              font = _fontH5Bold;
            else
              font = _fontH5;
            size = _scale * _sizeH5;
            break;
          case HFH6:
            if (field.bold)
              font = _fontH6Bold;
            else
              font = _fontH6;
            size = _scale * _sizeH6;
            break;
          default:
            Fail("Format");
            break;
          }     
          PackedColor color;


          if(field.font!=NULL) font = field.font;         //check specific <font> type
          if(field.fontSize!=-1) size =_scale * field.fontSize;   //check specific <font> size
           size = SCALED(size);      
          if(field.colored) color = field.color;          //check specific <font> color
          else if (field.href.GetLength() == 0)
          {
            if (field.bold)
              color = _boldColor;
            else
              color = _textColor;
          }
          else if (focused && _activeFieldKeyboard._section == s && _activeFieldKeyboard._field == f)
            color = _activeLinkColor;
          else
            color = _linkColor;

          RString text = field.text.Substring(from, to);
          float l = left;
          float textWidth = GetTextWidth(size, font, text);
          if (fieldWidth > 0)
          {
            switch (field.align)
            {
            case HARight:
              l += _scale * (fieldWidth - textWidth);
              break;
            case HACenter:
              l += 0.5 * _scale * (fieldWidth - textWidth);
              break;
            }
          }
          float t = top + _scale * SCALED(row.height) - size;
          float lText = l + _scale * SCALED(field.indent);

          TextDrawAttr attr(size, font, color, _shadow);
          vp->DrawText(Point2DFloat(lText, t), Rect2DFloat(colX, _y, SCALED(colW), SCALED(_h)), attr, text);
          if (_activeFieldMouse._section == s && _activeFieldMouse._field == f  && field.href.GetLength() > 0)
          {
            float b = t + size;
            vp->DrawLine
            (
              Line2DFloat(lText, b, lText + _scale * textWidth, b),
              color, color, Rect2DFloat(colX, _y, SCALED(colW), SCALED(_h))
            );
          }
          if (fieldWidth > 0)
          {
  #if DRAW_TABLE_BORDERS
            vp->DrawLine
            (
              CX(left), CY(top),
              CX(left + _scale * fieldWidth), CY(top),
              PackedBlack, PackedBlack
            );
            vp->DrawLine
            (
              CX(left + _scale * fieldWidth), CY(top),
              CX(left + _scale * fieldWidth), CY(top + _scale * SCALED(row.height)),
              PackedBlack, PackedBlack
            );
            vp->DrawLine
            (
              CX(left + _scale * fieldfieldWidth), CY(top + _scale * SCALED(row.height)),
              CX(left), CY(top + _scale * row.height),
              PackedBlack, PackedBlack
            );
            vp->DrawLine
            (
              CX(left), CY(top + _scale * SCALED(row.height)),
              CX(left), CY(top),
              PackedBlack, PackedBlack
            );
  #endif
            left += _scale * fieldWidth;
          }
          else
            left += GetTextWidth(size, font, text);
        }
      }
      top += _scale * SCALED(row.height);
    }

    colX += colW + _columnsSpace;
  }
}

CHTMLContainer::FieldIndex CHTML::FindField(float x, float y) const
{
  if (!IsInside(x, y)) return FieldIndex();

  return CHTMLContainer::FindField
  (
    (x - _x) / (SCALED(_scale)) - SCALED(textBorder),
    (y - _y) / (SCALED(_scale))
  );
}

float CHTML::GetPageWidth() const
{
  float width = (_w - 2 * textBorder) / _scale;
  return width / _columns - (_columns - 1) * _columnsSpace;
}

float CHTML::GetPageHeight() const
{
  return _h / _scale;
}

float CHTML::GetTextWidth(float size, Font *font, const char *text) const
{
  return GEngine->GetTextWidth(size, font, text);
}

void CHTML::SwitchSection(const char *name)
{
  CHTMLContainer::SwitchSection(name);
  if (_parent) _parent->UpdateKeyHints();
}

///////////////////////////////////////////////////////////////////////////////
// class CTree

CTreeItem::CTreeItem(int l, CTreeItem *p)
{
  level = l;
  parent = p;

  text = "";
  data = "";
  value = 0;
  texture = NULL;
  drawTexture = true;

  expanded = false;
}

CTreeItem *CTreeItem::AddChild()
{
  CTreeItem *item = new CTreeItem(level + 1, this);
  children.Add(item);
  return item;
}

CTreeItem *CTreeItem::InsertChild(int pos)
{
  saturate(pos, 0, children.Size());
  CTreeItem *item = new CTreeItem(level + 1, this);
  children.Insert(pos, item);
  return item;
}

int CompChildren(const Ref<CTreeItem> *pitem1, const Ref<CTreeItem> *pitem2)
{
  CTreeItem *item1 = *pitem1;
  CTreeItem *item2 = *pitem2;
#if _VBS2 // categorize game vehicle classes seperately from VBS2 classes
  bool hasGame1 = strstr(item1->text,LocalizeString("STR_EDITOR_GAME_CLASS")) != NULL;
  bool hasGame2 = strstr(item2->text,LocalizeString("STR_EDITOR_GAME_CLASS")) != NULL;
  if (hasGame1 && hasGame2)
    return stricmp(item1->text, item2->text);
  if (!hasGame1 && hasGame2)
    return -1;
  if (hasGame1 && !hasGame2)
    return +1;
#endif
  return stricmp(item1->text, item2->text);
}

int CompChildrenRev(const Ref<CTreeItem> *pitem1, const Ref<CTreeItem> *pitem2)
{
  CTreeItem *item1 = *pitem1;
  CTreeItem *item2 = *pitem2;
  return stricmp(item2->text, item1->text);
}

void CTreeItem::SortChildren(bool reversed)
{
  if (reversed)
    QSort(children.Data(), children.Size(), CompChildrenRev);
  else
    QSort(children.Data(), children.Size(), CompChildren);
}

void CTreeItem::ExpandTree(bool expand)
{
  expanded = expand;
  int n = children.Size();
  for (int i=0; i<n; i++)
    children[i]->ExpandTree(expand);
}

CTreeItem *CTreeItem::GetOBrother()
{
  if (!parent) return NULL;
  int n = parent->children.Size();
  for (int i=0; i<n-1; i++)
  {
    CTreeItem *item = parent->children[i];
    if (item == this)
      return parent->children[i + 1];
  }
  return NULL;
}

CTreeItem *CTreeItem::GetYBrother()
{
  if (!parent) return NULL;
  int n = parent->children.Size();
  CTreeItem *item = parent->children[0];
  if (item == this) return NULL;
  for (int i=1; i<n; i++)
  {
    item = parent->children[i];
    if (item == this)
      return parent->children[i - 1];
  }
  return NULL;
}

CTreeItem *CTreeItem::GetNextVisible()
{
  if (children.Size() > 0 && expanded)
    return children[0];

  CTreeItem *item = this;
  while (item->parent)
  {
    CTreeItem *brother = item->GetOBrother();
    if (brother) return brother;
    item = item->parent;
  }
  return NULL;
}

CTreeItem *CTreeItem::GetLastVisible()
{
  if (expanded)
  {
    int n = children.Size();
    if (n > 0) return children[n - 1]->GetLastVisible();
  }
  return this;
}

CTreeItem *CTreeItem::GetPrevVisible()
{
  if (!parent) return NULL;
  CTreeItem *brother = GetYBrother();
  if (brother) return brother->GetLastVisible();
  return parent;
}

int CTreeItem::NVisibleItems()
{
  int n = 0;
  CTreeItem *item = this;
  while (item)
  {
    n++;
    item = item->GetNextVisible();
  }
  return n;
}

CTree::CTree(ControlsContainer *parent, int idc, ParamEntryPar cls)
  : Control(parent, CT_TREE, idc, cls)
{
  _bgColor = GetPackedColor(cls >> "colorBackground");
  _ftColor = GetPackedColor(cls >> "colorText");
  _selColor = GetPackedColor(cls >> "colorSelect");
  _borderColor = GetPackedColor(cls >> "colorBorder");
  _arrowColor = GetPackedColor(cls >> "colorArrow");
  _font = GLOB_ENGINE->LoadFont(GetFontID(cls>>"font"));
  _size = cls >> "sizeEx";

  CTreeItem *item = new CTreeItem(0, NULL);
  _root = item;
  if ((_style & TR_SHOWROOT) == 0) _root->expanded = true;
  _firstVisible = FindFirstVisible();
  _selected = _firstVisible;

  _scrollUp = false;
  _scrollDown = false;
  _scrollTime = Glob.uiTime;
  _offset = 0;

  //_charBuffer contains what has user written. if _lastCharTime>_maxHistoryDelay, buffer is cleared
  _charBuffer = "";
  _maxHistoryDelay = cls >> "maxHistoryDelay";
  _lastCharTime = Glob.uiTime.toFloat();
}

void CTree::SetSelected(CTreeItem *item)
{
  if ((_style & TR_SHOWROOT) == 0 && item == _root) return;

  _selected = item;
  OnEvent(CETreeSelChanged);
  if (_parent) _parent->OnTreeSelChanged(this);
  
  EnsureVisible(_selected);
}

void CTree::Expand(CTreeItem *item, bool expand)
{
  item->expanded = expand;
  if (expand)
    OnEvent(CETreeExpanded);
  else
    OnEvent(CETreeCollapsed);

  if (expand && item == _selected && (_style & TR_AUTOCOLLAPSE))
  {
    CTreeItem *except = _selected;
    CTreeItem *parent = _selected->parent;
    while (parent)
    {
      int n = parent->children.Size();
      for (int i=0; i<n; i++)
      {
        if (parent->children[i] != except)
          parent->children[i]->ExpandTree(false);
      }

      except = parent;
      parent = except->parent;
    }
    EnsureVisible(_selected);
  }

  if (!expand && !(_style & TR_AUTOCOLLAPSE))
  {
    // collapse children
    item->ExpandTree(false);
  }

  if (_parent) _parent->OnTreeItemExpanded(this,item,expand);
}

void CTree::EnsureVisible(CTreeItem *item)
{
  while
  (
    _firstVisible && _firstVisible->parent &&
    !_firstVisible->parent->expanded
  )
    _firstVisible = _firstVisible->parent;
  
  CTreeItem *firstVisible = FindFirstVisible();
  if (!_firstVisible) _firstVisible = firstVisible;
  if (!_firstVisible) return;

  // special case
  if (item == _firstVisible)
  {
    _offset = 0;
    return;
  }

  int lineItem = -1;
  int lineFirst = -1;

  CTreeItem *p = firstVisible;
  int line = 0;
  while (p)
  {
    if (p == item)
    {
      lineItem = line;
      if (lineFirst >= 0) goto EVOK;
    }
    if (p == _firstVisible)
    {
      lineFirst = line;
      if (lineItem >= 0) goto EVOK;
    }
    p = p->GetNextVisible();
    line++;
  }
  return;

EVOK:
  if (lineItem < lineFirst)
  {
    _offset = 0;
    int n = lineFirst - lineItem;
    for (int i=0; i<n; i++)
    {
      CTreeItem *it = _firstVisible->GetPrevVisible();
      if (!it) break;
      _firstVisible = it;
    }
  }
  else
  {
    int n = lineItem - lineFirst;

    const float border = TEXT_BORDER;
    float textHeight = SCALED(_size);
    float h = SCALED(_h) + _offset * textHeight - 2 * border;
    if (_offset > 0 || _firstVisible != firstVisible)
      h -= textHeight;
    int nItems = _firstVisible->NVisibleItems();
    if (nItems * textHeight > h)
      h -= textHeight;
    if ((n + 1) * textHeight <= h) return;

    _offset = 0;    
    h = SCALED(_h) - 2 * border - textHeight;
    if (item->GetNextVisible()) h -= textHeight;
    int nn = toIntCeil(((n + 1) * textHeight - h) / textHeight);
    for (int i=0; i<nn; i++)
    {
      CTreeItem *it = _firstVisible->GetNextVisible();
      if (!it) break;
      _firstVisible = it;
    }
  }
}

void CTree::RemoveAll()
{
  _root->children.Clear();

  _root->text = "";
  _root->data = "";
  _root->value = 0;

  _root->expanded = (_style & TR_SHOWROOT) == 0;

  //CTreeItem *item = _root;
  _firstVisible = FindFirstVisible();
  _selected = _firstVisible;
}

void CTree::OnDraw(UIViewport *vp, float alpha)
{
  const int w = GLOB_ENGINE->Width2D();
  const int h = GLOB_ENGINE->Height2D();

  float xx = CX(_x);
  float yy = CY(_y);
  float ww = CX(SCALED(_w) + _x) - xx;
  float hh = CY(SCALED(_h) + _y) - yy;

  PackedColor bgColor = ModAlpha(_bgColor, alpha);
  PackedColor ftColor = ModAlpha(_ftColor, alpha);
  PackedColor selColor = ModAlpha(_selColor, alpha);
  PackedColor borderColor = ModAlpha(_borderColor, alpha);
  PackedColor arrowColor = ModAlpha(_arrowColor, alpha);

  // draw list
  Draw2DParsExt pars;
  pars.mip = GEngine->TextBank()->UseMipmap(NULL, 0, 0);
  pars.SetColor(bgColor);
  pars.Init();
  vp->Draw2D(pars, Rect2DPixel(xx, yy, ww, hh));
  DrawLeft(0, borderColor);
  DrawTop(0, borderColor);
  DrawBottom(0, borderColor);
  DrawRight(0, borderColor);

  CTreeItem *firstVisible = FindFirstVisible();
  if (!_firstVisible) _firstVisible = firstVisible;
  if (!_firstVisible) return;
  CTreeItem *item = _firstVisible;

  const float border = SCALED(TEXT_BORDER);
  
  float top = _y;
  float textHeight = SCALED(_size);
  if (_offset > 0 || item != firstVisible)
  {
    float y0 = CY(_y + textHeight);
    vp->DrawLine(Line2DPixel(xx, y0, xx + ww, y0), borderColor, borderColor);

    float x0 = CX(_x + 0.5 * SCALED(_w));
    float x1 = CX(_x + 0.5 * SCALED(_w) - 0.3 * textHeight);
    float x2 = CX(_x + 0.5 * SCALED(_w) + 0.3 * textHeight);
    float y1 = CY(_y + border);
    float y2 = CY(_y + textHeight - border);
    vp->DrawLine(Line2DPixel(x0, y1, x1, y2), arrowColor, arrowColor);
    vp->DrawLine(Line2DPixel(x1, y2, x2, y2), arrowColor, arrowColor);
    vp->DrawLine(Line2DPixel(x2, y2, x0, y1), arrowColor, arrowColor);
    
    top += textHeight;
  }
  top += border;
  float topClip = top;
  top -= _offset * textHeight;
  
  float bottomClip = _y + SCALED(_h) - border;
  int nItems = item->NVisibleItems();
  if (top + nItems * textHeight > bottomClip)
  {
    float y0 = CY(_y + SCALED(_h) - textHeight);
    vp->DrawLine(Line2DPixel(xx, y0, xx + ww, y0), borderColor, borderColor);

    float x0 = CX(_x + 0.5 * SCALED(_w));
    float x1 = CX(_x + 0.5 * SCALED(_w) - 0.3 * textHeight);
    float x2 = CX(_x + 0.5 * SCALED(_w) + 0.3 * textHeight);
    float y1 = CY(_y + SCALED(_h) - border);
    float y2 = CY(_y + SCALED(_h) - textHeight + border);
    vp->DrawLine(Line2DPixel(x0, y1, x1, y2), arrowColor, arrowColor);
    vp->DrawLine(Line2DPixel(x1, y2, x2, y2), arrowColor, arrowColor);
    vp->DrawLine(Line2DPixel(x2, y2, x0, y1), arrowColor, arrowColor);

    bottomClip -= textHeight;
  }
  
  while (item && top < bottomClip)
  {
    DrawItem(vp, alpha, item, top, topClip, bottomClip, item == firstVisible);
    top += textHeight;
    item = item->GetNextVisible();
  }
}

void CTree::DrawItem
(
  UIViewport *vp, float alpha, CTreeItem *item,
  float top, float topClip, float bottomClip, bool first
)
{
  const int w = GLOB_ENGINE->Width2D();
  const int h = GLOB_ENGINE->Height2D();
  const float border = TEXT_BORDER;

  Rect2DPixel clip(_x * w, topClip * h, SCALED(_w) * w, (bottomClip - topClip) * h);

  float textHeight = SCALED(_size);
  float textWidth = 0.75 * textHeight;
  float left = _x + border + textWidth * item->level;

  PackedColor color = ModAlpha(_ftColor, alpha);
  float lineleft = left - textWidth;
  CTreeItem *cur = item;
  if (cur->parent)
  {
    if (cur->children.Size() == 0)
    {
      float x1 = CX(lineleft + 0.5 * textWidth);
      float x2 = CX(lineleft + textWidth + 0.5 * border);
      float y2 = CY(top + 0.5 * textHeight);
      if (!first)
      {
        float y1 = CY(top);
        vp->DrawLine
        (
          Line2DPixel(x1, y1, x1, y2), color, color, clip
        );
      }
      vp->DrawLine
      (
        Line2DPixel(x1, y2, x2, y2), color, color, clip
      );
      if (cur->GetOBrother())
      {
        float y3 = CY(top + textHeight);
        vp->DrawLine
        (
          Line2DPixel(x1, y2, x1, y3), color, color, clip
        );
      }
    }
    else
    {
      float x1 = CX(lineleft + 0.15 * textWidth);
      float x2 = CX(lineleft + 0.30 * textWidth);
      float x3 = CX(lineleft + 0.50 * textWidth);
      float x4 = CX(lineleft + 0.70 * textWidth);
      float x5 = CX(lineleft + 0.85 * textWidth);
      float x6 = CX(lineleft + 1.00 * textWidth + 0.5 * border);
      float y1 = CY(top + 0.15 * textHeight);
      float y3 = CY(top + 0.50 * textHeight);
      float y5 = CY(top + 0.85 * textHeight);
      vp->DrawLine(Line2DPixel(x1, y1, x1, y5), color, color, clip);
      vp->DrawLine(Line2DPixel(x1, y5, x5 + 1, y5), color, color, clip);
      vp->DrawLine(Line2DPixel(x5, y5, x5, y1), color, color, clip);
      vp->DrawLine(Line2DPixel(x5, y1, x1, y1), color, color, clip);
      vp->DrawLine(Line2DPixel(x2, y3, x4 + 1, y3), color, color, clip);
      if (!cur->expanded)
      {
        float y2 = CY(top + 0.3 * textHeight);
        float y4 = CY(top + 0.7 * textHeight);
        vp->DrawLine(Line2DPixel(x3, y2, x3, y4 + 1), color, color, clip);
      }
      if (!first)
      {
        float y0 = CY(top);
        vp->DrawLine(Line2DPixel(x3, y0, x3, y1), color, color, clip);
      }
      vp->DrawLine(Line2DPixel(x5, y3, x6, y3), color, color, clip);
      if (cur->GetOBrother())
      {
        float y6 = CY(top + textHeight);
        vp->DrawLine(Line2DPixel(x3, y5, x3, y6), color, color, clip);
      }
    }
    cur = cur->parent;
    lineleft -= textWidth;
    while (cur->parent)
    {
      if (cur->GetOBrother())
      {
        float x1 = CX(lineleft + 0.5 * textWidth);
        float y1 = CY(top);
        float y2 = CY(top + textHeight);
        vp->DrawLine
        (
          Line2DPixel(x1, y1, x1, y2), color, color, clip
        );
      }
      cur = cur->parent;
      lineleft -= textWidth;
    }
  }

  TextDrawAttr attr(SCALED(_size), _font, color,_shadow);

  float width = vp->GetTextWidth(attr, item->text) + 2 * border;
  float height = floatMin(textHeight, bottomClip - topClip);

  float xx = CX(left);
  float yy = CY(top);
  float ww = CW(width);  
  float hh = CH(height);
  
  // selected box should go around texture
  if (item->texture && item->drawTexture) ww += hh;

  if (item == _selected)
  {
    color = ModAlpha(_selColor, alpha);
    vp->DrawLine
    (
      Line2DPixel(xx, yy, xx + ww, yy), color, color, clip
    );
    vp->DrawLine
    (
      Line2DPixel(xx + ww, yy, xx + ww, yy + hh + 1), color, color, clip
    );
    vp->DrawLine
    (
      Line2DPixel(xx + ww, yy + hh, xx, yy + hh), color, color, clip
    );
    vp->DrawLine
    (
      Line2DPixel(xx, yy + hh, xx, yy), color, color, clip
    );
  }

  if (item->texture && item->drawTexture)
  {
    if (item->texture->AHeight() > 0)
    {
      Draw2DParsExt pars;
      pars.mip = GEngine->TextBank()->UseMipmap(item->texture, 0, 0);
      pars.SetColor(PackedWhite);
      pars.Init();

      Rect2DPixel rect(xx, yy, hh, hh);
      vp->Draw2D
      (
        pars,
        rect
      );
      left += height;
    }
  }

  vp->DrawText
  (
    Point2DFloat(left + border , top), Rect2DFloat(_x, topClip, SCALED(_w), bottomClip - topClip),
    attr, item->text
  );
}

bool CTree::OnKeyDown(int dikCode)
{
  CTreeItem *firstVisible = FindFirstVisible();
  if (!_firstVisible) _firstVisible = firstVisible;
  if (!_firstVisible) return false;
  if (!_selected) _selected = _firstVisible;

  switch (dikCode)
  {
  case DIK_UP:
  case INPUT_DEVICE_XINPUT + XBOX_Up:
  case INPUT_DEVICE_XINPUT + XBOX_LeftThumbYUp:
    if (_selected != firstVisible)
    {
      CTreeItem *item = _selected->GetPrevVisible();
      Assert(item);
      SetSelected(item);
    }
    return true;
  case DIK_DOWN:
  case INPUT_DEVICE_XINPUT + XBOX_Down:
  case INPUT_DEVICE_XINPUT + XBOX_LeftThumbYDown:
    {
      CTreeItem *item = _selected->GetNextVisible();
      if (item) SetSelected(item);
    }
    return true;
  case DIK_PRIOR:
    {
      CTreeItem *item = _selected;
      for (int i=0; i<10; i++)
      {
        if (item == firstVisible) break;
        CTreeItem *it = item->GetPrevVisible();
        Assert(it);
        item = it;
      }
      SetSelected(item);
    }
    return true;
  case DIK_NEXT:
    {
      CTreeItem *item = _selected;
      for (int i=0; i<10; i++)
      {
        CTreeItem *it = item->GetNextVisible();
        if (!it) break;
        item = it;
      }
      SetSelected(item);
    }
    return true;
  case DIK_LEFT:
  case INPUT_DEVICE_XINPUT + XBOX_Left:
  case INPUT_DEVICE_XINPUT + XBOX_LeftThumbXLeft:
   if (_selected->expanded)
      Expand(_selected, false);
    else
    {
      CTreeItem *item = _selected->parent;
      if (_style & TR_SHOWROOT)
      {
        if (item) SetSelected(item);
      }
      else
      {
        if (item && item != _root) SetSelected(item);
      }
    }
    return true;
  case DIK_RIGHT:
  case INPUT_DEVICE_XINPUT + XBOX_Right:
  case INPUT_DEVICE_XINPUT + XBOX_LeftThumbXRight:
    if (!_selected->expanded)
      Expand(_selected, true);
    else
    {
      if (_selected->children.Size() > 0)
        SetSelected(_selected->children[0]);
    }
    return true;
  default:
    return false;
  }
}

bool CTree::OnChar(unsigned nChar, unsigned nRepCnt, unsigned nFlags)
{
  CTreeItem *firstVisible = FindFirstVisible();
  if (!_firstVisible) _firstVisible = firstVisible;
  if (!_firstVisible) return false;
  if (!_selected) _selected = _firstVisible;

  switch (nChar)
  {
  case '+':
    Expand(_selected, true);
    return true;
  case '-':
    Expand(_selected, false);
    return true;
  default:
    return SelectStartString(nChar);
  }
}

bool CTree::SelectStartString(unsigned nChar)
{
  //check if new string should be started
  if(Glob.uiTime.toFloat() - _lastCharTime > _maxHistoryDelay) _charBuffer = "";
  _lastCharTime = Glob.uiTime.toFloat();
  //create new string
  char c[2]; 
  c[0] = nChar;   c[1] = '\0';
  //add new char to current string
  _charBuffer=_charBuffer + c;
  //is not case sensitive
  _charBuffer.Lower();

  //first try currently selected item (to avoid some unnecessary selecting)
  RString item = _selected->text;
  
  //is not case sensitive
  item.Lower(); 
  if(strncmp(item,_charBuffer,_charBuffer.GetLength())==0)
  {//current one is OK, so we don't have to change anything
    return true;
  }
  CTreeItem *treeItem = _firstVisible;

  while(treeItem)
  {//find first match between all items
    item = treeItem->text;
    item.Lower(); 

    if(strncmp(item,_charBuffer,_charBuffer.GetLength())==0)
    {//select first match and go away
      SetSelected(treeItem);
      return true;
    }
    treeItem = treeItem->GetNextVisible();
  }
  return false;
}

void CTree::OnLButtonDown(float x, float y)
{
  CTreeItem *item = FindItem(x, y);
  if (!item) return;

  const float border = SCALED(TEXT_BORDER);
  float textHeight = SCALED(_size);
  float textWidth = 0.75 * textHeight;
  float left = _x + border + textWidth * item->level;
  
  if (x < left - textWidth) return;
  SetSelected(item);
  if (x < left) Expand(item, !item->expanded);
  if (x >= left)
  {
    OnEvent(CETreeLButtonDown);
    if (_parent) _parent->OnTreeLButtonDown(IDC(), item);
  }
}

void CTree::OnLButtonDblClick(float x, float y)
{
  CTreeItem *item = FindItem(x, y);
  if (!item) return;
/*    
  const float border = 0.005;
  float textHeight = _size;
  float textWidth = 0.75 * textHeight;
  float left = _x + border + textWidth * item->level;

  if (x < left) return;
  Expand(item, !item->expanded);
*/
  // don't allow double-click if not over text
  const float border = SCALED(TEXT_BORDER);
  float textWidth = 0.75 * SCALED(_size);
  float left = _x + border + textWidth * item->level;
  if (x < left) return;

  OnEvent(CETreeDblClick);
  if (_parent) _parent->OnTreeDblClick(IDC(), GetSelected());
}

void CTree::OnMouseMove(float x, float y, bool active)
{
  // if the cursor has left the area, do not perform a part of the processing
  if (active)
  {
    CTreeItem *item = FindItem(x, y);
    OnEvent(CETreeMouseMove);
    if (_parent) _parent->OnTreeMouseMove(IDC(), item);
  }

  // required for OnMouseExit
  IControl::OnMouseMove(x, y, active);

  if (!GInput.mouseL) return;

  OnMouseHold(x, y, active);
}

void CTree::OnMouseExit(float x, float y)
{  
  OnEvent(CETreeMouseExit);
  if (_parent) _parent->OnTreeMouseExit(IDC(), _selected);
}

void CTree::OnMouseHold(float x, float y, bool active)
{
  CTreeItem *item = FindItem(x, y);
  OnEvent(CETreeMouseHold);
  if (_parent) _parent->OnTreeMouseHold(IDC(), item);

  if (!GInput.mouseL) return;

  CTreeItem *firstVisible = FindFirstVisible();
  if (!_firstVisible) _firstVisible = firstVisible;
  if (!_firstVisible) return;

  const float border = SCALED(TEXT_BORDER);

  bool canScrollUp = false;
  bool canScrollDown = false;

  float top = _y;
  float textHeight = SCALED(_size);
  if (_offset > 0 || _firstVisible != firstVisible)
  {
    canScrollUp = true;
    top += textHeight;
  }
  top += border - _offset * textHeight;
  float bottomClip = _y + SCALED(_h) - border;
  int nItems = _firstVisible->NVisibleItems();
  if (top + nItems * textHeight > bottomClip)
  {
    canScrollDown = true;
  }

  float dy;
  if 
  (
    canScrollUp && 
    (dy = _y + textHeight - y) > 0
  )
  {
    _scrollDown = false;
    // scroll up
    if (_scrollUp)
    {
      float scroll = SCROLL_SPEED * dy;
      saturate(scroll, SCROLL_MIN, SCROLL_MAX);
      _offset -= scroll * (Glob.uiTime - _scrollTime);
      CTreeItem *item = _firstVisible;
      while (_offset < 0)
      {
        if (item == firstVisible)
        {
          _offset = 0;
          break;
        }
        item = item->GetPrevVisible();
        _offset++;
      }
      if (item == firstVisible) _offset = 0;
      _firstVisible = item;
      _scrollTime = Glob.uiTime;
    }
    else
    {
      _scrollUp = true;
      _scrollTime = Glob.uiTime;
    }
  }
  else if
  (
    canScrollDown &&
    (dy = y - _y - SCALED(_h) + textHeight) > 0
  )
  {
    _scrollUp = false;
    // scroll down
    if (_scrollDown)
    {
      float scroll = SCROLL_SPEED * dy;
      saturate(scroll, SCROLL_MIN, SCROLL_MAX);
      _offset += scroll * (Glob.uiTime - _scrollTime);
      CTreeItem *item = _firstVisible;
      while (_offset > 1.0)
      {
        if (item->GetNextVisible() == NULL)
        {
          _offset = 0;
          break;
        }
        item = item->GetNextVisible();
        _offset--;
      }
      if (item == firstVisible && _offset > 0)
      {
        item = item->GetNextVisible();
      }
      _firstVisible = item;
      _scrollTime = Glob.uiTime;
    }
    else
    {
      _scrollDown = true;
      _scrollTime = Glob.uiTime;
    }
  }
  else
  {
    _scrollUp = _scrollDown = false;
  }
}

void CTree::OnMouseZChanged(float dz)
{
  if (dz == 0) return;
  
  CTreeItem *firstVisible = FindFirstVisible();
  if (!_firstVisible) _firstVisible = firstVisible;
  if (!_firstVisible) return;

  const float border = SCALED(TEXT_BORDER);

  bool canScrollUp = false;
  bool canScrollDown = false;

  float top = _y;
  float textHeight = SCALED(_size);
  if (_offset > 0 || _firstVisible != firstVisible)
  {
    canScrollUp = true;
    top += textHeight;
  }
  top += border - _offset * textHeight;
  float bottomClip = _y + SCALED(_h) - border;
  int nItems = _firstVisible->NVisibleItems();
  if (top + nItems * textHeight > bottomClip)
  {
    canScrollDown = true;
  }

  if (canScrollUp && dz > 0)
  {
    // scroll up
    float scroll = -SCROLL_Z_SPEED * dz;
    saturate(scroll, -SCROLL_Z_MAX, -SCROLL_MIN);
    _offset += 0.1 * scroll;
    CTreeItem *item = _firstVisible;
    while (_offset < 0)
    {
      if (item == firstVisible)
      {
        _offset = 0;
        break;
      }
      item = item->GetPrevVisible();                                        
      _offset++;
    }
    if (item == firstVisible) _offset = 0;
    _firstVisible = item;
  }
  else if (canScrollDown && dz < 0)
  {
    // scroll down
    float scroll = -SCROLL_Z_SPEED * dz;
    saturate(scroll, SCROLL_MIN, SCROLL_Z_MAX);
    _offset += 0.1 * scroll;
    CTreeItem *item = _firstVisible;
    while (_offset > 1.0)
    {
      if (item->GetNextVisible() == NULL)
      {
        _offset = 0;
        break;
      }
      item = item->GetNextVisible();
      _offset--;
    }
    if (item == firstVisible && _offset > 0)
    {
      item = item->GetNextVisible();
    }
    _firstVisible = item;
  }
}

CTreeItem *CTree::FindItem(float x, float y)
{
  Assert(IsInside(x, y));
  CTreeItem *firstVisible = FindFirstVisible();
  if (!_firstVisible) _firstVisible = firstVisible;
  if (!_firstVisible) return NULL;
  CTreeItem *item = _firstVisible;

  const float border = SCALED(TEXT_BORDER);
  
  float top = _y;
  float textHeight = SCALED(_size);
  if (_offset > 0 || item != firstVisible)
  {
    top += textHeight;
    if (y < top) return NULL;
  }
  top += border - _offset * textHeight;
  
  float bottomClip = _y + SCALED(_h) - border;
  int nItems = item->NVisibleItems();
  if (top + nItems * textHeight > bottomClip)
  {
    bottomClip -= textHeight;
    if (y > bottomClip) return NULL;
  }
  
  while (item && top < bottomClip)
  {
    top += textHeight;
    if (y < top) return item;
    item = item->GetNextVisible();
  }
  return NULL;
}

CTreeItem *CTree::FindFirstVisible()
{
  if (_style & TR_SHOWROOT)
  {
    return _root;
  }
  else
  {
    return _root->GetNextVisible();
  }
}

