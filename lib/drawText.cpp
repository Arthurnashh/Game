#include "wpch.hpp"
#include "drawText.hpp"
#include "paramFileExt.hpp"
#include <El/ParamArchive/paramArchive.hpp>
#include <El/Common/perfProf.hpp>
#include <El/XML/xml.hpp>
#include "mbcs.hpp"
#include "engine.hpp"
#include "textbank.hpp"
#include "UI/uiViewport.hpp"

#define ISSPACE(c) ((c) >= 0 && (c) <= 32)

static NodeTypes DrawTextNodeTypes;
static AttributeTypes DrawTextAttributeTypes;

// Structured texts

#define HALIGN_ENUM(type,prefix,XX) \
	XX(type, prefix, Left) \
	XX(type, prefix, Center) \
	XX(type, prefix, Right)
//XX(type, prefix, Justify)
 
// call to enum factory using this macro:
#ifndef DECL_ENUM_HALIGN
#define DECL_ENUM_HALIGN
DECL_ENUM(HAlign)
#endif
DECLARE_DEFINE_ENUM(HAlign, H, HALIGN_ENUM)

#define VALIGN_ENUM(type,prefix,XX) \
	XX(type, prefix, Top) \
	XX(type, prefix, Middle) \
	XX(type, prefix, Bottom)
//XX(type, prefix, BaseLine)
 
// call to enum factory using this macro:
#ifndef DECL_ENUM_VALIGN
#define DECL_ENUM_VALIGN
DECL_ENUM(VAlign)
#endif
DECLARE_DEFINE_ENUM(VAlign, V, VALIGN_ENUM)

//! Convert text to PackedColor
PackedColor ParseColor(RString text)
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

// Align attribute
class AttrAlign : public IAttribute
{
protected:
	HAlign _value;

public:
	void Load(RString value);
	HAlign GetValue() const {return _value;}
  void SetValue(HAlign value) {_value = value;}

	LSError Serialize(ParamArchive &ar);
	bool IsEqualTo(const IAttribute *with) const;
};

void AttrAlign::Load(RString value)
{
	if (stricmp(value, "right") == 0) _value = HRight;
	else if (stricmp(value, "center") == 0) _value = HCenter;
	else
	{
		DoAssert(stricmp(value, "left") == 0)
		_value = HLeft;
	}
}

LSError AttrAlign::Serialize(ParamArchive &ar)
{
	CHECK(ar.SerializeEnum("value", _value, 1, HLeft));
	return LSOK;
}

bool AttrAlign::IsEqualTo(const IAttribute *with) const
{
	const AttrAlign *w = static_cast<const AttrAlign *>(with);
	return GetValue() == w->GetValue();
}

IAttribute *CreateAttrAlign() {return new AttrAlign();}

// VAlign attribute
class AttrVAlign : public IAttribute
{
protected:
	VAlign _value;

public:
	void Load(RString value);
	VAlign GetValue() const {return _value;}
	void SetValue(VAlign value) {_value = value;}

	LSError Serialize(ParamArchive &ar);
	bool IsEqualTo(const IAttribute *with) const;
};

void AttrVAlign::Load(RString value)
{
	if (stricmp(value, "bottom") == 0) _value = VBottom;
	else if (stricmp(value, "middle") == 0) _value = VMiddle;
	else if(stricmp(value, "top") == 0) _value = VTop;
  else
  {
    RptF("VAlign attribute invalid value \"%s\"",cc_cast(value));
		_value = VTop;
  }
}

LSError AttrVAlign::Serialize(ParamArchive &ar)
{
	CHECK(ar.SerializeEnum("value", _value, 1, VTop));
	return LSOK;
}

bool AttrVAlign::IsEqualTo(const IAttribute *with) const
{
	const AttrVAlign *w = static_cast<const AttrVAlign *>(with);
	return GetValue() == w->GetValue();
}

IAttribute *CreateAttrVAlign() {return new AttrVAlign();}


// Int attribute
class AttrInt : public IAttribute
{
public:
  int _value;

public:
  void Load(RString value);
  int GetValue() const {return _value;}
  void SetValue(int value) {_value = value;}

  LSError Serialize(ParamArchive &ar);
  bool IsEqualTo(const IAttribute *with) const;
};

void AttrInt::Load(RString value)
{
  _value = atoi(value);
}

LSError AttrInt::Serialize(ParamArchive &ar)
{
  CHECK(ar.Serialize("value", _value, 1));
  return LSOK;
}

bool AttrInt::IsEqualTo(const IAttribute *with) const
{
  const AttrInt *w = static_cast<const AttrInt *>(with);
  return GetValue() == w->GetValue();
}


IAttribute *CreateAttrInt() {return new AttrInt();}
int GetAttrInt(const IAttribute *attr) {return static_cast<const AttrInt *>(attr)->GetValue();}
void SetAttrInt(IAttribute *attr, int value) {static_cast<AttrInt *>(attr)->SetValue(value);}

// Color attribute
class AttrColor : public IAttribute
{
protected:
	PackedColor _value;

public:
	void Load(RString value);
	PackedColor GetValue() const {return _value;}
	void SetValue(PackedColor value) {_value = value;}

	LSError Serialize(ParamArchive &ar);
	bool IsEqualTo(const IAttribute *with) const;
};

void AttrColor::Load(RString value)
{
	_value = ParseColor(value);
}

LSError AttrColor::Serialize(ParamArchive &ar)
{
	if (ar.IsSaving())
	{
		DWORD value = _value;
		CHECK(ar.Serialize("value", (int &)value, 1));
	}
	else if (ar.GetPass() == ParamArchive::PassFirst)
	{
		DWORD value;
		CHECK(ar.Serialize("value", (int &)value, 1));
		_value = PackedColor(value);
	}
	return LSOK;
}

bool AttrColor::IsEqualTo(const IAttribute *with) const
{
	const AttrColor *w = static_cast<const AttrColor *>(with);
	return GetValue() == w->GetValue();
}

IAttribute *CreateAttrColor() {return new AttrColor();}

// Font attribute
class AttrFont : public IAttribute
{
protected:
	Ref<Font> _value;

public:
	void Load(RString value);
	Font *GetValue() const {return _value;}
  void SetValue(Font *value) {_value = value;}

	LSError Serialize(ParamArchive &ar);
	bool IsEqualTo(const IAttribute *with) const;
};

void AttrFont::Load(RString value)
{
	_value = GEngine->LoadFont(GetFontID(value));
}

LSError AttrFont::Serialize(ParamArchive &ar)
{
	if (ar.IsSaving())
	{
		RString name = _value ? _value->Name() : RString();
		int langID = _value ? _value->GetLangID() : 0;
		CHECK(ar.Serialize("name", name, 1, RString()));
		CHECK(ar.Serialize("langID", langID, 1, 0));
	}
	else if (ar.GetPass() == ParamArchive::PassFirst)
	{
		RString name;
		int langID;
		CHECK(ar.Serialize("name", name, 1, RString()));
		CHECK(ar.Serialize("langID", langID, 1, 0));
		if (name.GetLength() == 0) _value = NULL;
		else _value = GEngine->LoadFont(FontID(name, langID));
	}
	return LSOK;
}

bool AttrFont::IsEqualTo(const IAttribute *with) const
{
	const AttrFont *w = static_cast<const AttrFont *>(with);
	
	Font *font1 = GetValue();
	Font *font2 = w->GetValue();

	if (font1)
	{
		if (font2) return stricmp(font1->Name(), font2->Name()) == 0 && font1->GetLangID() == font2->GetLangID();
		else return false;
	}
	else return font2 == NULL;
}

IAttribute *CreateAttrFont() {return new AttrFont();}

// Image attribute
class AttrImage : public IAttribute
{
protected:
	Ref<Texture> _value;

public:
	void Load(RString value);
	~AttrImage(){}
	Texture *GetValue() const {return _value;}
	void SetValue(Texture *image) {_value = image;}

	LSError Serialize(ParamArchive &ar);
	bool IsEqualTo(const IAttribute *with) const;
};

RString FindPicture(RString name);

void AttrImage::Load(RString value)
{
	RString name = FindPicture(value);
	name.Lower();
	_value = GlobLoadTexture(name);
	if (_value) _value->SetUsageType(Texture::TexUI); // no limits
}

LSError AttrImage::Serialize(ParamArchive &ar)
{
	if (ar.IsSaving())
	{
    RString value = _value ? RString(_value->GetName()) : RString();
    CHECK(ar.Serialize("value", value, 1, RString()));
	}
	else if (ar.GetPass() == ParamArchive::PassFirst)
	{
		RString value;
		CHECK(ar.Serialize("value", value, 1, RString()));
		value.Lower();
		if (value.GetLength() == 0) _value = NULL;
		else _value = GlobLoadTexture(value);
		if (_value) _value->SetUsageType(Texture::TexUI); // no limits
	}
	return LSOK;
}

bool AttrImage::IsEqualTo(const IAttribute *with) const
{
	const AttrImage *w = static_cast<const AttrImage *>(with);
	
	Texture *image1 = GetValue();
	Texture *image2 = w->GetValue();

	if (image1)
	{
		if (image2) return stricmp(image1->GetName(), image2->GetName()) == 0;
		else return false;
	}
	else return image2 == NULL;
}

IAttribute *CreateAttrImage() {return new AttrImage();}

static AttributeList DrawTextDefaultAttributes;

void InitDefaultDrawTextAttributes(ParamEntryVal cls)
{
	SetAttributes(DrawTextDefaultAttributes, cls, DrawTextAttributeTypes);
}

#define ATTRIBUTES_LIST(XX) \
	XX("size", attrSize, CreateAttrFloat) \
	XX("align", attrAlign, CreateAttrAlign) \
	XX("valign", attrVAlign, CreateAttrVAlign) \
	XX("color", attrColor, CreateAttrColor) \
	XX("font", attrFont, CreateAttrFont) \
	XX("image", attrImage, CreateAttrImage) \
	XX("shadow", attrShadow, CreateAttrInt) \
	XX("shadowOffset", attrShadowOffset, CreateAttrFloat) \
	XX("shadowColor", attrShadowColor, CreateAttrColor) \
  XX("underline", attrUnderline, CreateAttrBool)
	// name, variable, create function

#define DEFINE_ATTR_VARIABLE(name, variable, func) static int variable;
#define REGISTER_ATTR(name, variable, func) \
	variable = DrawTextAttributeTypes.attributes.AddValue(name); \
	DrawTextAttributeTypes.createFunctions.Access(variable); \
	DrawTextAttributeTypes.createFunctions[variable] = func;
	
ATTRIBUTES_LIST(DEFINE_ATTR_VARIABLE);

//! Base class for structured text section
class IText : public INode
{
public:
	// overload this functions
	virtual bool DrawItem(UIViewport *vp, Rect2DAbs &rect, RString text, int from, int to, float height, Rect2DAbs &clip, const AttributeList &attributes, float alpha, bool vpNoClip) = 0;
	virtual float GetItemHeight(RString text, float height, const AttributeList &attributes) = 0;
};

enum ProcessTextContextType
{
	CTProcessTextBuffer,
	CTPlainText,
  CTReadyToDraw
};

struct ProcessTextContext
{
	virtual ProcessTextContextType GetContextType() const = 0;
};

struct PlainTextContext : public ProcessTextContext
{
  PlainTextContext(ITextProcessParams *func) : _func(func) {}
	ProcessTextContextType GetContextType() const {return CTPlainText;}

  /// the result text
	RString text;
  /// parameters replacement function
  ITextProcessParams *_func;
};

RString GetTextPlain(INode *root, ITextProcessParams *func)
{
	AttributeManager manager(DrawTextDefaultAttributes);

	PlainTextContext context(func);
	root->PerformProcess(manager, &context);

	return context.text;
}

bool CompareText(INode *root1, INode *root2)
{
	return root1->IsEqualTo(root2);
}

struct ProcessTextItem
{
  /// the source node
  IText *node;

  // text area
  RString text;
	int from;
	int to;

	// item width
	float w;
	// no drawing, delete on line end
	bool space;
	// end of item is end of word
	bool wordBreak;

	// attributes
	AttributeList attributes;
};
TypeIsMovable(ProcessTextItem)

struct ProcessTextBuffer;

//! Text buffer for single row
typedef bool ProcessItemFunc(ProcessTextItem &item, Rect2DAbs &rect, Rect2DAbs &clip, float height, void *context);
typedef bool ProcessLineFunc(ProcessTextBuffer &buffer, float hLine, void *context);

struct ProcessTextBuffer : public ProcessTextContext
{
	ProcessTextContextType GetContextType() const {return CTProcessTextBuffer;}

	// common context
	Rect2DAbs rect;
	float height;
	ProcessItemFunc *itemFunc;
	ProcessLineFunc *lineFunc;
	void *context;
  /// parameters replacement function
  ITextProcessParams *func;
	
	float lineY;

	float wLeft;
	float wCenter;
	float wRight;
	AutoArray<ProcessTextItem> textsLeft;
	AutoArray<ProcessTextItem> textsCenter;
	AutoArray<ProcessTextItem> textsRight;

	//! Return true if new line is outside rect
	bool NewLine();
	bool ProcessItems(float hMax);

	void RemoveSpaces(HAlign avoid);
	void Add(HAlign align, ProcessTextItem &item);

	void GetBuffer(HAlign align, AutoArray<ProcessTextItem> *&buffer, float *&wBuffer);

	float MaxHeight();
	void WordBreakLast(HAlign align);

	void Flush();
};

bool ProcessTextBuffer::NewLine()
{
	float hMax = MaxHeight();

	// process current line
	if (itemFunc && ProcessItems(hMax)) return true;
	if (lineFunc && lineFunc(*this, hMax, context)) return true;

	// advance to next line
	lineY += hMax;

	wLeft = 0;
	wCenter = 0;
	wRight = 0;
	textsLeft.Resize(0);
	textsCenter.Resize(0);
	textsRight.Resize(0);

	return false;
}

void ProcessTextBuffer::Flush()
{
	if (textsLeft.Size() > 0 || textsCenter.Size() > 0 || textsRight.Size() > 0)
	{
		float hMax = MaxHeight();
		if (itemFunc && ProcessItems(hMax)) return;
		if (lineFunc && lineFunc(*this, hMax, context)) return;
	}
}

void ProcessTextBuffer::RemoveSpaces(HAlign avoid)
{
	if (avoid != HLeft) for (int i=textsLeft.Size()-1; i>=0; i--)
	{
		ProcessTextItem &item = textsLeft[i];
		if (!item.space) break;
		wLeft -= item.w;
		textsLeft.Delete(i);
	}

	if (avoid != HCenter) for (int i=textsCenter.Size()-1; i>=0; i--)
	{
		ProcessTextItem &item = textsCenter[i];
		if (!item.space) break;
		wCenter -= item.w;
		textsCenter.Delete(i);
	}

	if (avoid != HRight) for (int i=textsRight.Size()-1; i>=0; i--)
	{
		ProcessTextItem &item = textsRight[i];
		if (!item.space) break;
		wRight -= item.w;
		textsRight.Delete(i);
	}
}

void ProcessTextBuffer::Add(HAlign align, ProcessTextItem &item)
{
	switch (align)
	{
	case HLeft:
		textsLeft.Add(item);
		wLeft += item.w;
		break;
	case HCenter:
		textsCenter.Add(item);
		wCenter += item.w;
		break;
	case HRight:
		textsRight.Add(item);
		wRight += item.w;
		break;
	}
}

void ProcessTextBuffer::GetBuffer(HAlign align, AutoArray<ProcessTextItem> *&buffer, float *&wBuffer)
{
	switch (align)
	{
	case HLeft:
		buffer = &textsLeft;
		wBuffer = &wLeft;
		break;
	case HCenter:
		buffer = &textsCenter;
		wBuffer = &wCenter;
		break;
	case HRight:
		buffer = &textsRight;
		wBuffer = &wRight;
		break;
	}
}

float ProcessTextBuffer::MaxHeight()
{
	float hMax = 0;

	for (int i=0; i<textsLeft.Size(); i++)
  {
    ProcessTextItem &item = textsLeft[i];
		saturateMax(hMax, item.node->GetItemHeight(item.text, height, item.attributes));
  }
	for (int i=0; i<textsCenter.Size(); i++)
  {
    ProcessTextItem &item = textsCenter[i];
    saturateMax(hMax, item.node->GetItemHeight(item.text, height, item.attributes));
  }
	for (int i=0; i<textsRight.Size(); i++)
  {
    ProcessTextItem &item = textsRight[i];
    saturateMax(hMax, item.node->GetItemHeight(item.text, height, item.attributes));
  }

	return hMax == 0 ? height : hMax;
}

void ProcessTextBuffer::WordBreakLast(HAlign align)
{
	AutoArray<ProcessTextItem> *buffer = NULL;
	switch (align)
	{
	case HLeft:
		buffer = &textsLeft;
		break;
	case HCenter:
		buffer = &textsCenter;
		break;
	case HRight:
		buffer = &textsRight;
		break;
	}
	if (buffer)
	{
		int n = buffer->Size();
		if (n > 0) buffer->Set(n - 1).wordBreak = true;
	}
}

#define BORDER_PIXELS 1

bool ProcessTextBuffer::ProcessItems(float hMax)
{
	Rect2DAbs r;
	r.y = lineY;
	r.h = hMax;

	// left aligned text
	r.x = rect.x + BORDER_PIXELS;
	for (int i=0; i<textsLeft.Size(); i++)
	{
		ProcessTextItem &item = textsLeft[i];
    if (!item.space)
		{
			r.w = item.w;
			if (itemFunc(item, r, rect, height, context)) return true;
		}
		r.x += item.w;
	}
	
	// center aligned text
	r.x = rect.x + 0.5 * (rect.w + wLeft - wCenter - wRight);
	// r.x = rect.x + wLeft + 0.5 * (rect.w - wLeft - wRight) - 0.5 * wCenter;
	for (int i=0; i<textsCenter.Size(); i++)
	{
		ProcessTextItem &item = textsCenter[i];
		if (!item.space)
		{
			r.w = item.w;
			if (itemFunc(item, r, rect, height, context)) return true;
		}
		r.x += item.w;
	}
	
	// right aligned text
	r.x = rect.x + rect.w - wRight - BORDER_PIXELS;
	for (int i=0; i<textsRight.Size(); i++)
	{
		ProcessTextItem &item = textsRight[i];
		if (!item.space)
		{
			r.w = item.w;
			if (itemFunc(item, r, rect, height, context)) return true;
		}
		r.x += item.w;
	}

	return false;
}

// DrawText implementation

struct DrawItemContext
{
  float alpha;
  UIViewport *vp;
  bool vpNoClip;
};

bool DrawItemCallback(ProcessTextItem &item, Rect2DAbs &rect, Rect2DAbs &clip, float height, void *context)
{
	DrawItemContext *ctx = reinterpret_cast<DrawItemContext *>(context);
	return item.node->DrawItem(ctx->vp, rect, item.text, item.from, item.to, height, clip, item.attributes, ctx->alpha, ctx->vpNoClip);
}

bool ExitWhenOutside(ProcessTextBuffer &buffer, float hLine, void *context)
{
	return buffer.lineY + hLine >= buffer.rect.y + buffer.rect.h;
}

#define ROUND_EPS 0.001f

void DrawText(UIViewport *vp, INode *root, const Rect2DAbs &rect, float height, float alpha, ITextProcessParams *func, bool vpNoClip)
{
  SCOPE_GRF("struct. text",0);

  AttributeManager manager(DrawTextDefaultAttributes);

  DrawItemContext context;
  context.alpha = alpha;
  context.vp = vp;
  context.vpNoClip = vpNoClip;

	ProcessTextBuffer buffer;

  // avoid different spaces between elements when moving the text
  // fix: deterministic for integers
  buffer.rect.x = toIntCeil(rect.x - ROUND_EPS) - 0.5f;
  buffer.rect.y = toIntCeil(rect.y - ROUND_EPS) - 0.5f;
  // text width is always in whole pixels
  buffer.rect.w = toInt(rect.w);
  buffer.rect.h = rect.h;

  buffer.height = height;
	buffer.itemFunc = DrawItemCallback;
	buffer.lineFunc = ExitWhenOutside;
	buffer.context = &context;

  buffer.func = func;

	buffer.lineY = rect.y;
	buffer.wLeft = 0;
	buffer.wCenter = 0;
	buffer.wRight = 0;

	if (root->PerformProcess(manager, &buffer)) return;
	buffer.Flush();
}

/*!
\patch 5128 Date 2/9/2007 by Jirka
- Fixed: UI - structured text drawing - spaces between elements
*/

void DrawText(UIViewport *vp, INode *root, const Rect2DFloat &rect, float height, float alpha, ITextProcessParams *func, bool vpNoClip)
{
  SCOPE_GRF("struct. text",0);
	Rect2DAbs absRect;
	GEngine->Convert(absRect, rect);
	float absHeight = height * GEngine->Height2D();
	DrawText(vp, root, absRect, absHeight, alpha, func, vpNoClip);
}

// GetTextHeight implementation

bool SumHeight(ProcessTextBuffer &buffer, float hLine, void *context)
{
	float *h = reinterpret_cast<float *>(context);
	*h += hLine;
	return false;
}

float GetTextHeight(INode *root, const Rect2DAbs &rect, float height, ITextProcessParams *func)
{
	AttributeManager manager(DrawTextDefaultAttributes);

	float h = 0;

	ProcessTextBuffer buffer;
	buffer.rect = rect;
	buffer.height = height;
	buffer.itemFunc = NULL;
	buffer.lineFunc = SumHeight;
	buffer.context = &h;

	buffer.lineY = rect.y;
	buffer.wLeft = 0;
	buffer.wCenter = 0;
	buffer.wRight = 0;

  buffer.func = func;

	if (root->PerformProcess(manager, &buffer)) return h;
	buffer.Flush();

	return h;
}

float GetTextHeight(INode *root, const Rect2DFloat &rect, float height, ITextProcessParams *func)
{
	Rect2DAbs absRect;
	GEngine->Convert(absRect, rect);
	absRect.x = toIntCeil(absRect.x - ROUND_EPS) - 0.5f;
	absRect.y = toIntCeil(absRect.y - ROUND_EPS) - 0.5f;
	
	float absHeight = height * GEngine->Height2D();
	return GetTextHeight(root, absRect, absHeight, func) / GEngine->Height2D();
}

bool MaxWidth(ProcessTextBuffer &buffer, float hLine, void *context)
{
	float *w = reinterpret_cast<float *>(context);
	float wLine = buffer.wLeft + buffer.wCenter + buffer.wRight + 2 * BORDER_PIXELS;
	saturateMax(*w, wLine);
	return false;
}

float GetTextWidthAbs(INode *root, float height, ITextProcessParams *func)
{
	AttributeManager manager(DrawTextDefaultAttributes);

	float w = 0;

	ProcessTextBuffer buffer;
	buffer.rect = Rect2DClipAbs;
	buffer.height = height;
	buffer.itemFunc = NULL;
	buffer.lineFunc = MaxWidth;
	buffer.context = &w;

	buffer.lineY = buffer.rect.y;
	buffer.wLeft = 0;
	buffer.wCenter = 0;
	buffer.wRight = 0;

  buffer.func = func;

	if (root->PerformProcess(manager, &buffer)) return w;
	buffer.Flush();

	return w;
}

float GetTextWidthFloat(INode *root, float height, ITextProcessParams *func)
{
	float absHeight = height * GEngine->Height2D();
	return GetTextWidthAbs(root, absHeight, func) / GEngine->Width2D();
}

struct ReadyToDrawContext : public ProcessTextContext
{
  ProcessTextContextType GetContextType() const {return CTReadyToDraw;}
  bool _ready;
  float _height;
};

bool IsTextReadyToDraw(INode *root, float height)
{
  AttributeManager manager(DrawTextDefaultAttributes);

  ReadyToDrawContext ctx;
  ctx._ready = true;
  ctx._height = height;

  root->PerformProcess(manager, &ctx);
  return ctx._ready;
}

#define CHECK_ATTR(name) const IAttribute *at##name = manager.GetAttribute(attr##name); if (!at##name) return false;
#define ATTR_VALUE(name, type) static_cast<const type *>(at##name)->GetValue()

#define ITEM_ATTR_VALUE(name, type) static_cast<const type *>(attributes[attr##name].GetRef())->GetValue()

// ASCII text section

class TextASCII : public IText
{
protected:
	RString _text;

public:
	RString GetType() const {return "TextASCII";}
	LSError Serialize(ParamArchive &ar, const NodeTypes &types, const AttributeTypes &attrTypes);
	bool IsEqualTo(const INode *with) const;

	RString GetText() const {return _text;}
	void SetText(RString text) {_text = text;}

	bool Process(AttributeManager &manager, void *context);
	bool DrawItem(UIViewport *vp, Rect2DAbs &rect, RString text, int from, int to, float height, Rect2DAbs &clip, const AttributeList &attributes, float alpha, bool vpNoClip);
	float GetItemHeight(RString text, float height, const AttributeList &attributes);
};

LSError TextASCII::Serialize(ParamArchive &ar, const NodeTypes &types, const AttributeTypes &attrTypes)
{
	CHECK(IText::Serialize(ar, types, attrTypes))
	CHECK(ar.Serialize("text", _text, 1, RString()))
	return LSOK;
}

bool TextASCII::IsEqualTo(const INode *with) const
{
	if (!IText::IsEqualTo(with)) return false;

	const TextASCII *w = static_cast<const TextASCII *>(with);
	return strcmp(_text, w->_text) == 0;
}

bool TextASCII::Process(AttributeManager &manager, void *context)
{
	Assert(context);
	ProcessTextContext *ctx = reinterpret_cast<ProcessTextContext *>(context);

	switch (ctx->GetContextType())
	{
		case CTProcessTextBuffer:
			{
        ProcessTextBuffer &buffer = *reinterpret_cast<ProcessTextBuffer *>(context);
        RString text = buffer.func ? _text.ParseFormat(*buffer.func) : _text;        

				int n = text.GetLength();
        // sometimes, there is loop, where only new line is added (to == from) - when there is not enough space not evan for one character
        // but it can also happen, that there can be never anything added (width too small) - so we need to allow at least one void loop
        int voidLoop = 0;
				if (n == 0) return false; // no text - no processing

				// check presence of attributes
				CHECK_ATTR(Color)
				CHECK_ATTR(Font)
				CHECK_ATTR(Size)
				CHECK_ATTR(Align)
				CHECK_ATTR(VAlign)
				CHECK_ATTR(Shadow)
				CHECK_ATTR(ShadowOffset)
				CHECK_ATTR(ShadowColor)
        CHECK_ATTR(Underline)
        
				Font *font = ATTR_VALUE(Font, AttrFont);
				float size = GetAttrFloat(atSize);
				HAlign hpos = ATTR_VALUE(Align, AttrAlign);

        float lineWidth = buffer.rect.w - 2 * BORDER_PIXELS;
        // handle a special case - no space to render at all
        if (lineWidth<=0) return true;

				float h = buffer.height * size;

        TextDrawAttr attr(h / GEngine->Height2D(), font);

				int from = 0;
				int to = 0;
				while (to < n)
				{
				  int oldTo = to;
					// spaces
					while (to < n && ISSPACE(text[to])) to++;
					if (to > from)
					{
						float w = GEngine->GetTextWidth(attr, text.Substring(from, to)) * GEngine->Width2D();
						if (w + buffer.wLeft + buffer.wCenter + buffer.wRight > lineWidth)
						{
							// new line
							buffer.RemoveSpaces((HAlign)-1);
							if (buffer.NewLine()) return true; // finish
						}
						else
						{
							ProcessTextItem item;
							item.node = this;
              item.text = text;
							item.from = from;
							item.to = to;
							item.w = w;
							item.space = true;
							item.wordBreak = true;
							item.attributes = manager.GetAttributeList();
							buffer.Add(hpos, item);
						}
						from = to;
					}

					// nonspaces
					bool newLine = false;
					while (to < n && !ISSPACE(text[to])) to++;
					if (to > from)
					{
						float w = GEngine->GetTextWidth(attr, text.Substring(from, to)) * GEngine->Width2D();
						if (w + buffer.wLeft + buffer.wCenter + buffer.wRight > lineWidth)
						{
							// new line
							buffer.RemoveSpaces(hpos);
							float wRest = lineWidth - buffer.wLeft - buffer.wCenter - buffer.wRight;
							if (w > wRest)
							{
								// check if buffer contain beginning of word
								float *wBuf = NULL;
								AutoArray<ProcessTextItem> *buf = NULL;
								buffer.GetBuffer(hpos, buf, wBuf);
								int length = buf->Size();
								int word = 0;
								for (int i=length-1; i>=0; i--)
									if (buf->Get(i).wordBreak) 
									{
										word = i + 1;
										break;
									}
								// word now contain index of first item, which is part of current word
								float wWord = w;
								for (int i=word; i<length; i++) wWord += buf->Get(i).w;
								if (wWord > lineWidth)
								{
									// word is wider than row, we can split anywhere
									to = from;
									w = 0;
                  // read the whole UTF-8 character
                  int charLen = 1;
                  char c = text[to];
                  if (c & 0x80)
                  {
                    DoAssert((c & 0xc0) == 0xc0); // the first character
                    while (((c = text[to + charLen]) & 0xc0) == 0x80)
                    {
                      charLen++;
                    };
                  }
									float wChar = GEngine->GetTextWidth(attr, text.Substring(to, to + charLen)) * GEngine->Width2D();
					        // avoid infinite loop - if there no space even for one letter, write it on any line
									while ((w + wChar <= wRest || wChar>lineWidth && w==0) && to + charLen < n)
									{
										to += charLen;
										w += wChar;

                    charLen = 1;
                    c = text[to];
                    if (c & 0x80)
                    {
                      DoAssert((c & 0xc0) == 0xc0); // the first character
                      while (((c = text[to + charLen]) & 0xc0) == 0x80)
                      {
                        charLen++;
                      };
                    }
										wChar = GEngine->GetTextWidth(attr, text.Substring(to, to + charLen)) * GEngine->Width2D();
									}
									// perform new line after add text
									newLine = true;
								}
								else
								{
									// move word to new line
									AUTO_STATIC_ARRAY(ProcessTextItem, cache, 16);
									for (int i=word; i<length; i++)
									{
										cache.Add(buf->Get(word));
										*wBuf -= buf->Get(word).w;
										buf->Delete(word);
									}
									for (int i=word-1; i>=0; i--)
									{
										if (!buf->Get(i).space) break;
										*wBuf -= buf->Get(i).w;
										buf->Delete(i);
									}
									if (buffer.NewLine()) return true; // finish
									for (int i=0; i<cache.Size(); i++)
									{
										buf->Add(cache[i]);
										*wBuf += cache[i].w;
									}
								}
							}
						}
						ProcessTextItem item;
            item.node = this;
						item.text = text;
						item.from = from;
						item.to = to;
						item.w = w;
						item.space = false;
						item.wordBreak = false;
						item.attributes = manager.GetAttributeList();
						buffer.Add(hpos, item);
						from = to;

						if (newLine)
						{
							// new line
							buffer.RemoveSpaces((HAlign)-1);
							if (buffer.NewLine()) return true; // finish
						}
					}
          if (to<=oldTo)
          {
            voidLoop++;
            if(voidLoop > 1)
            {
              // freeze hot-fix - no progress made  - we need to finish
              ErrF("TextASCII.Process infinite loop for %s at %d",cc_cast(text),to);
              return true;
            }
          }
          else voidLoop = 0;
				}
			}
			return false;
		case CTPlainText:
			{
				PlainTextContext *context = reinterpret_cast<PlainTextContext *>(ctx);
        context->text = context->text + (context->_func ? _text.ParseFormat(*context->_func) : _text);
			}
			return false;
    case CTReadyToDraw:
      {
        CHECK_ATTR(Font)
        CHECK_ATTR(Size)

        Font *font = ATTR_VALUE(Font, AttrFont);
        float size = GetAttrFloat(atSize);

        ReadyToDrawContext *ready = reinterpret_cast<ReadyToDrawContext *>(ctx);

        if (font)
        {
          float h = GEngine->Height2D();
          const FontWithSize *fontWithSize = font->Select(h * size * ready->_height);
          if (fontWithSize)
          {
            if (!fontWithSize->IsReady()) ready->_ready = false;
          }
          else
          {
            RptF("Font %s, size %.0f not found", cc_cast(font->Name()), h * size * ready->_height);
            ready->_ready = false;
          }
        }
        else
        {
          RptF("Missing font");
          ready->_ready = false;
        }
      }
      return false;
		default:
			Fail("Unknown context type");
			return false;
	}
}

/*!
\patch 5130 Date 2/22/2007 by Jirka
- Fixed: Drawing of subtitles (all structured texts) - the offset of shadow was not always fixed
*/

bool TextASCII::DrawItem(UIViewport *vp, Rect2DAbs &rect, RString text, int from, int to, float height, Rect2DAbs &clip, const AttributeList &attributes, float alpha, bool vpNoClip)
{
	if (text.GetLength() == 0) return false;

	PackedColor color = ITEM_ATTR_VALUE(Color, AttrColor);
	Font *font = ITEM_ATTR_VALUE(Font, AttrFont);
	float size = GetAttrFloat(attributes[attrSize]);
	VAlign vpos = ITEM_ATTR_VALUE(VAlign, AttrVAlign);
	int shadow = GetAttrInt(attributes[attrShadow]);
	float shadowOffset = GetAttrFloat(attributes[attrShadowOffset]);
	PackedColor shadowColor = ITEM_ATTR_VALUE(ShadowColor, AttrColor);
  bool underline = GetAttrBool(attributes[attrUnderline]);

	color.SetA8(toIntFloor(color.A8() * alpha));
	shadowColor.SetA8(toIntFloor(shadowColor.A8() * alpha));

  // Fix: round the position to keep the same offset for the shadow after rounded in Engine::DrawText
	float h = height * size;
	Point2DAbs pos;
	pos.x = toIntCeil(rect.x - ROUND_EPS) - 0.5f;
	switch (vpos)
	{
	case VTop:
		pos.y = toIntCeil(rect.y - ROUND_EPS) - 0.5f;
		break;
	case VMiddle:
		pos.y = toIntCeil(rect.y - ROUND_EPS + 0.5 * (rect.h - h)) - 0.5f;
		break;
	case VBottom:
		pos.y = toIntCeil(rect.y - ROUND_EPS + rect.h - h) - 0.5f;
		break;
	}

	// TODO: draw yourself
  TextDrawAttr attr(h / GEngine->Height2D(), font, PackedBlack, shadow);
	RString t = text.Substring(from, to);
	if (shadow == 1)
	{
		float offsetX = 0.75 * shadowOffset * h; saturateMax(offsetX, 1.0f);
		float offsetY = shadowOffset * h; saturateMax(offsetY, 1.0f);
    attr._color = shadowColor;
		vp->DrawText(Point2DAbs(pos.x + offsetX, pos.y + offsetY), clip, attr, t);
    if (underline)
      vp->DrawLine(Line2DAbs(pos.x + offsetX, pos.y + h + offsetY, pos.x + rect.w + offsetX, pos.y + h + offsetY), shadowColor, shadowColor, clip);
	}
  attr._color = color;
  attr._shadow = shadow;
  if (vpNoClip)
    vp->DrawTextNoClip(pos, clip, attr, t);
  else
    vp->DrawText(pos, clip, attr, t);
  if (underline)
  {
    if (vpNoClip)
      vp->DrawLineNoClip(Line2DAbs(pos.x, pos.y + h, pos.x + rect.w, pos.y + h), color, color);
    else
      vp->DrawLine(Line2DAbs(pos.x, pos.y + h, pos.x + rect.w, pos.y + h), color, color, clip);
  }

	return false; // continue
}

float TextASCII::GetItemHeight(RString text, float height, const AttributeList &attributes)
{
	if (text.GetLength() == 0) return 0;

	float size = GetAttrFloat(attributes[attrSize]);
	return height * size;
}

INode *CreateTextASCII(INode *parent, RString str)
{
	TextASCII *text = new TextASCII();
	if (parent)
	{
		if (!parent->Add(text))
		{
			delete text;
			return NULL;
		}
	}
	text->SetText(str);
	return text;
}

// image text section

class TextImage : public IText
{
public:
	RString GetType() const {return "TextImage";}
	LSError Serialize(ParamArchive &ar, const NodeTypes &types, const AttributeTypes &attrTypes);
	bool IsEqualTo(const INode *with) const;

	bool Process(AttributeManager &manager, void *context);
	bool DrawItem(UIViewport *vp, Rect2DAbs &rect, RString text, int from, int to, float height, Rect2DAbs &clip, const AttributeList &attributes, float alpha, bool vpNoClip);
	float GetItemHeight(RString text, float height, const AttributeList &attributes);

	LSError Serialize(ParamArchive &ar);
};

LSError TextImage::Serialize(ParamArchive &ar, const NodeTypes &types, const AttributeTypes &attrTypes)
{
	return IText::Serialize(ar, types, attrTypes);
}

bool TextImage::IsEqualTo(const INode *with) const
{
	return IText::IsEqualTo(with);
}

bool TextImage::Process(AttributeManager &manager, void *context)
{
	Assert(context);
	ProcessTextContext *ctx = reinterpret_cast<ProcessTextContext *>(context);
  switch (ctx->GetContextType())
	{
		case CTProcessTextBuffer:
			{
				// check presence of attributes
				CHECK_ATTR(Image)
				CHECK_ATTR(Color)
				CHECK_ATTR(Size)
				CHECK_ATTR(Align)
				CHECK_ATTR(VAlign)
				CHECK_ATTR(Shadow)
				CHECK_ATTR(ShadowOffset)
				CHECK_ATTR(ShadowColor)

				Texture *image = ATTR_VALUE(Image, AttrImage);
				if (!image) return false;
				float size = GetAttrFloat(atSize);
				HAlign hpos = ATTR_VALUE(Align, AttrAlign);

				ProcessTextBuffer &buffer = *reinterpret_cast<ProcessTextBuffer *>(context);
        float lineWidth = buffer.rect.w - 2 * BORDER_PIXELS;

				float h = buffer.height * size;
				float w = image->AWidth() * h / image->AHeight();

				bool fit = w + buffer.wLeft + buffer.wCenter + buffer.wRight <= lineWidth;
				bool enforce = buffer.wLeft + buffer.wCenter + buffer.wRight == 0;

				if (!fit && !enforce)
				{
					// new line
					buffer.RemoveSpaces((HAlign)-1);
					if (buffer.NewLine()) return true; // finish
				}
				buffer.WordBreakLast(hpos);
				ProcessTextItem item;
				item.node = this;
				item.from = 0;
				item.to = 1;
				item.w = w;
				item.space = false;
				item.wordBreak = true;
				item.attributes = manager.GetAttributeList();

        fit = w + buffer.wLeft + buffer.wCenter + buffer.wRight <= lineWidth;
				if (!fit)
				{
					// new line
					buffer.RemoveSpaces((HAlign)-1);
					if (buffer.NewLine()) return true; // finish
        }

        buffer.Add(hpos, item);
			}
			return false;
		case CTPlainText:
			return false;
    case CTReadyToDraw:
      {
        ReadyToDrawContext *ready = reinterpret_cast<ReadyToDrawContext *>(ctx);

        CHECK_ATTR(Image)
        Texture *image = ATTR_VALUE(Image, AttrImage);
        if (image)
        {
          if (!image->HeadersReady())
          {
            //image->LoadHeaders();
            ready->_ready = false;
            return false;
          }
          MipInfo mip = GEngine->TextBank()->UseMipmap(image, 1, 0);
          if (mip._level != 0) ready->_ready = false;
        }
        return false;
      }
		default:
			Fail("Unknown context type");
			return false;
	}
}

bool TextImage::DrawItem(UIViewport *vp, Rect2DAbs &rect, RString text, int from, int to, float height, Rect2DAbs &clip, const AttributeList &attributes, float alpha, bool vpNoClip)
{
	Texture *image = ITEM_ATTR_VALUE(Image, AttrImage);
	if (!image) return false;
	PackedColor color = ITEM_ATTR_VALUE(Color, AttrColor);
	float size = GetAttrFloat(attributes[attrSize]);
	VAlign vpos = ITEM_ATTR_VALUE(VAlign, AttrVAlign);
	int shadow = GetAttrInt(attributes[attrShadow]);
	float shadowOffset = GetAttrFloat(attributes[attrShadowOffset]);
	PackedColor shadowColor = ITEM_ATTR_VALUE(ShadowColor, AttrColor);

	color.SetA8(toIntFloor(color.A8() * alpha));
	shadowColor.SetA8(toIntFloor(shadowColor.A8() * alpha));

	float h = height * size;
	switch (vpos)
	{
	case VTop:
		break;
	case VMiddle:
		rect.y += 0.5 * (rect.h - h);
		break;
	case VBottom:
		rect.y += rect.h - h;
		break;
	}
	rect.h = h;

  Draw2DParsExt pars;
  pars.mip = GEngine->TextBank()->UseMipmap(image, 0, 0);
  pars.Init();
  pars.spec |= (shadow == 2)?UISHADOW : 0; 

	if (shadow == 1)
	{
		const float offsetX = 0.75 * shadowOffset * h;
		const float offsetY = shadowOffset * h;

    pars.SetColor(shadowColor);
    if (vpNoClip)
      vp->Draw2DNoClip(pars, Rect2DAbs(rect.x + offsetX, rect.y + offsetY, rect.w, rect.h));
    else
		  vp->Draw2D(pars, Rect2DAbs(rect.x + offsetX, rect.y + offsetY, rect.w, rect.h), clip);
	}
  pars.SetColor(color);
  if (vpNoClip)
    vp->Draw2DNoClip(pars, rect);
  else
  	vp->Draw2D(pars, rect, clip);

	return false; // continue
}

float TextImage::GetItemHeight(RString text, float height, const AttributeList &attributes)
{
	Texture *image = ITEM_ATTR_VALUE(Image, AttrImage);
	if (!image) return 0;

	float size = GetAttrFloat(attributes[attrSize]);
	return height * size;
}

INode *CreateTextImage(INode *parent)
{
	TextImage *text = new TextImage();
	if (parent)
	{
		if (!parent->Add(text))
		{
			delete text;
			return NULL;
		}
	}
	return text;
}

// Hard line break

class TextBreak : public IText
{
public:
	RString GetType() const {return "TextBreak";}
	LSError Serialize(ParamArchive &ar, const NodeTypes &types, const AttributeTypes &attrTypes);
	bool IsEqualTo(const INode *with) const;

	bool Process(AttributeManager &manager, void *context);
	bool DrawItem(UIViewport *vp, Rect2DAbs &rect, RString text, int from, int to, float height, Rect2DAbs &clip, const AttributeList &attributes, float alpha, bool vpNoClip) {return false;}
	float GetItemHeight(RString text, float height, const AttributeList &attributes) {return 0;}

	LSError Serialize(ParamArchive &ar);
};

LSError TextBreak::Serialize(ParamArchive &ar, const NodeTypes &types, const AttributeTypes &attrTypes)
{
	return IText::Serialize(ar, types, attrTypes);
}

bool TextBreak::IsEqualTo(const INode *with) const
{
	return IText::IsEqualTo(with);
}

bool TextBreak::Process(AttributeManager &manager, void *context)
{
	Assert(context);
	ProcessTextContext *ctx = reinterpret_cast<ProcessTextContext *>(context);
	switch (ctx->GetContextType())
	{
		case CTProcessTextBuffer:
			{
				ProcessTextBuffer &buffer = *reinterpret_cast<ProcessTextBuffer *>(context);
				buffer.RemoveSpaces((HAlign)-1);
				return buffer.NewLine();
			}
		case CTPlainText:
			return false;
    case CTReadyToDraw:
      return false;
		default:
			Fail("Unknown context type");
			return false;
	}
}

INode *CreateTextBreak(INode *parent)
{
	TextBreak *text = new TextBreak();
	if (parent)
	{
		if (!parent->Add(text))
		{
			delete text;
			return NULL;
		}
	}
	return text;
}

void SetTextAttribute(INode *text, RString name, RString value)
{
	text->SetAttribute(name, value, DrawTextAttributeTypes);
}

void SetTextColor(INode *text, PackedColor color)
{
	IAttribute *attr = text->AccessAttribute("color", DrawTextAttributeTypes);
	if (!attr) return;
	static_cast<AttrColor *>(attr)->SetValue(color);
}

void SetTextShadowColor(INode *text, PackedColor color)
{
  IAttribute *attr = text->AccessAttribute("shadowColor", DrawTextAttributeTypes);
  if (!attr) return;
  static_cast<AttrColor *>(attr)->SetValue(color);
}

void SetShadow(INode *text, int shadow)
{
  IAttribute *attr = text->AccessAttribute("shadow", DrawTextAttributeTypes);
  if (!attr) return;
  static_cast<AttrInt *>(attr)->SetValue(shadow);
}

void SetTextImage(INode *text, Texture *image)
{
	IAttribute *attr = text->AccessAttribute("image", DrawTextAttributeTypes);
	if (!attr) return;
	static_cast<AttrImage *>(attr)->SetValue(image);
}

void SetTextFont(INode *text, Font *font)
{
  IAttribute *attr = text->AccessAttribute("font", DrawTextAttributeTypes);
  if (!attr) return;
  static_cast<AttrFont *>(attr)->SetValue(font);
}

//! Set color of text
void SetTextCentred(INode *text)
{
  IAttribute *attr = text->AccessAttribute("valign", DrawTextAttributeTypes);
  if (!attr) return;
  static_cast<AttrVAlign *>(attr)->SetValue(VMiddle);

  attr = text->AccessAttribute("align", DrawTextAttributeTypes);
  if (!attr) return;
  static_cast<AttrAlign *>(attr)->SetValue(HCenter);
}
// structured text section parsing

TypeIsSimple(INode *)

class TextParser : public SAXParser
{
protected:
	AutoArray<INode *> _stack;

public:
	TextParser(INode *root);
	void OnStartElement(RString name, XMLAttributes &attributes);
	void OnEndElement(RString name);
	void OnCharacters(RString chars);
};

TextParser::TextParser(INode *root)
{
	_trim = false;

	_stack.Add(root);
}

void TextParser::OnStartElement(RString name, XMLAttributes &attributes)
{
	int n = _stack.Size();
	if (n < 1) return;

	INode *parent = _stack[n - 1];
	if (!parent)
	{
		_stack.Add(NULL);
		return;
	}

	INode *text = NULL;
	if (strcmp(name, "img") == 0)
	{
		text = new TextImage;
	}
	else if (strcmp(name, "br") == 0)
	{
		text = new TextBreak;
		// do not set _current, no inner elements are enabled
		if (!parent->Add(text))
		{
			RptF("Cannot add line break");
			delete text;
		}
		_stack.Add(NULL);
		return;
	}
	else if (strcmp(name, "t") == 0)
	{
		text = new NodeStructured;
	}
	else
	{
		RptF("Wrong text element '%s'", (const char *)name);
		_stack.Add(NULL);
		return;
	}

	Assert(text);
	if (!parent->Add(text))
	{
		RptF("Cannot add text section '%s'", (const char *)name);
		delete text;
		_stack.Add(NULL);
		return;
	}

	for (int i=0; i<attributes.Size(); i++)
		SetTextAttribute(text, attributes[i].name, attributes[i].value);

	_stack.Add(text);
}

void TextParser::OnEndElement(RString name)
{
	int n = _stack.Size();
	if (n < 1) return;

	_stack.Delete(n - 1);
}

void TextParser::OnCharacters(RString chars)
{
	int n = _stack.Size();
	if (n < 1) return;

	INode *parent = _stack[n - 1];
	if (!parent) return;

	TextASCII *text = new TextASCII;
	if (!parent->Add(text))
	{
		RptF("Cannot add text '%s'", (const char *)chars);
		delete text;
		return;
	}
	text->SetText(chars);
}

INode *CreateTextStructured(INode *parent, RString content)
{
	NodeStructured *text = new NodeStructured();
	if (parent)
	{
		if (!parent->Add(text))
		{
			delete text;
			return NULL;
		}
	}
	if (content.GetLength() > 0)
	{
		TextParser parser(text);
		QIStrStream in(content, content.GetLength());
		parser.Parse(in);
	}
	return text;
}

INode *CreateTextPlain(INode *parent, RString content)
{
	NodeStructured *text = new NodeStructured();
	if (parent)
	{
		if (!parent->Add(text))
		{
			delete text;
			return NULL;
		}
	}
	if (content.GetLength() > 0)
	{
		const char *start = content;
		const char *ptr = strstr(start, "\\n");
		while (ptr)
		{
			CreateTextASCII(text, content.Substring(start-content, ptr-content));
			CreateTextBreak(text);
			start = ptr + 2;
			ptr = strstr(start, "\\n");
		}
		CreateTextASCII(text, content.Substring(start-content, INT_MAX));
	}
	return text;
}

LSError SerializeText(RString name, INode *&value, ParamArchive &ar)
{
	return SerializeNode(name, value, ar, DrawTextNodeTypes, DrawTextAttributeTypes);
}

INode *CreateNodeStructured() {return new NodeStructured();}
INode *CreateTextASCII() {return new TextASCII();}
INode *CreateTextImage() {return new TextImage();}
INode *CreateTextBreak() {return new TextBreak();}

// DrawText nodes and attributes registration
#define NODES_LIST(XX) \
	XX("NodeStructured", CreateNodeStructured) \
	XX("TextASCII", CreateTextASCII) \
	XX("TextImage", CreateTextImage) \
	XX("TextBreak", CreateTextBreak)
	// name, variable, create function

#define REGISTER_NODE(name, func) \
	{ \
		int index = DrawTextNodeTypes.nodes.AddValue(name); \
		DrawTextNodeTypes.createFunctions.Access(index); \
		DrawTextNodeTypes.createFunctions[index] = func; \
	}

#include <El/Modules/modules.hpp>

INIT_MODULE(DrawText, 4)
{
	// Register nodes
	NODES_LIST(REGISTER_NODE);

	// Register attributes
	ATTRIBUTES_LIST(REGISTER_ATTR);
	DrawTextAttributeTypes.attributes.Close();
};

