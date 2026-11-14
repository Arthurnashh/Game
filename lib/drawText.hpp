#ifdef _MSC_VER
#pragma once
#endif

#ifndef _DRAW_TEXT_HPP
#define _DRAW_TEXT_HPP

#include <El/Enum/dynEnum.hpp>
#include <El/HierWalker/hierWalker.hpp>

struct Point2DFloat;
struct Point2DAbs;
struct Rect2DFloat;
struct Rect2DAbs;

class ParamEntryVal;
class UIViewport;

//! Initialization of structured text attribute types
void InitDefaultDrawTextAttributes(ParamEntryVal cls);

/// Interface for the functor replacing parameters in the structured text
class ITextProcessParams
{
public:
  /// reads the parameter identification starting on ptr and return the string which will replace it (see RStringCT::ParseFormat)
  virtual RString operator ()(const char *&ptr) = 0;
};

//! Draw structured text
void DrawText(UIViewport *vp, INode *root, const Rect2DFloat &rect, float height, float alpha = 1.0f, ITextProcessParams *func = NULL, bool vpNoClip = false);
//! Draw structured text
void DrawText(UIViewport *vp, INode *root, const Rect2DAbs &rect, float height, float alpha = 1.0f, ITextProcessParams *func = NULL, bool vpNoClip = false);

//! Get height of structured text
float GetTextHeight(INode *root, const Rect2DFloat &rect, float height, ITextProcessParams *func = NULL);
//! Get height of structured text
float GetTextHeight(INode *root, const Rect2DAbs &rect, float height, ITextProcessParams *func = NULL);

//! Get width of structured text
float GetTextWidthAbs(INode *root, float height, ITextProcessParams *func = NULL);
//! Get width of structured text
float GetTextWidthFloat(INode *root, float height, ITextProcessParams *func = NULL);

//! Return plain text extracted from structured text
RString GetTextPlain(INode *root, ITextProcessParams *func = NULL);

//! Check if two texts are equal
bool CompareText(INode *root1, INode *root2);

//! Serialization of structured text
LSError SerializeText(RString name, INode *&value, ParamArchive &ar);

//! Is structured text ready for drawing?
bool IsTextReadyToDraw(INode *root, float height);

//! Create ASCII text
INode *CreateTextASCII(INode *parent, RString str);

//! Create empty image
INode *CreateTextImage(INode *parent);

//! Create line break
INode *CreateTextBreak(INode *parent);

//! Create empty structured text (if given, create content by parsing of argument)
INode *CreateTextStructured(INode *parent, RString content = RString());

//! Create ASCII text with hard breaks (\n)
INode *CreateTextPlain(INode *parent, RString content);

//! Set attribute of text
void SetTextAttribute(INode *text, RString name, RString value);

class PackedColor;

//! Set color of text
void SetTextColor(INode *text, PackedColor color);

//! Set color of text
void SetTextShadowColor(INode *text, PackedColor color);

//! Set image of text
void SetTextImage(INode *text, Texture *image);

//! Set font of text
void SetTextFont(INode *text, Font *font);

//! Set font of text
void SetShadow(INode *text, int shadow);

//! Set color of text
void SetTextCentred(INode *text);


#endif
