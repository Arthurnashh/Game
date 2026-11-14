#ifndef _RSC_COMMON_HPP
#define _RSC_COMMON_HPP

//Todo:
//Color and size defines should not be absolute.
//Change to use same class hierarchy as documentation.

//Common defines:

//Fonts

//Main sizes
#define Size_Main_Very_Small 0.018
#define Size_Main_Small 0.027
#define Size_Main_Normal 0.04

//Control sizes
#define Size_Text_Default Size_Main_Normal
#define Size_Text_Very_Small Size_Main_Very_Small
#define Size_Text_Small Size_Main_Small

//Base colors
#define Color_Black {0, 0, 0, 1}
#define Color_White {1, 1, 1, 1}
#define Color_WhiteDark {0.85, 0.85, 0.85, 1}
#define Color_Orange {1, 0.5, 0, 1}
#define Color_Gray {0.3, 0.3, 0.3, 1}
#define Color_GrayLight {0.6, 0.6, 0.6, 1}
#define Color_GrayDark {0.2, 0.2, 0.2, 1}

//Main colors
#define Color_Main_Active1 Color_GrayLight
#define Color_Main_Select1 Color_WhiteDark
#define Color_Main_Foreground1 Color_White
#define Color_Main_Background1 Color_Gray
#define Color_Main_Background2 Color_Gray

//Control colors
#define Color_Text_Default Color_Main_Foreground1
#define Color_Text_Orange Color_Main_Foreground1
#define Color_Background Color_Main_Background1
#define Color_Text_White Color_White

//Procedural colors
#define ProcTextWhite "#(argb,8,8,3)color(1,1,1,1)"
#define ProcTextBlack "#(argb,8,8,3)color(0,0,0,1)"
#define ProcTextGray "#(argb,8,8,3)color(0.3,0.3,0.3,1)"
#define ProcTextRed "#(argb,8,8,3)color(1,0,0,1)"
#define ProcTextGreen "#(argb,8,8,3)color(0,1,0,1)"
#define ProcTextBlue "#(argb,8,8,3)color(0,0,1,1)"
#define ProcTextOrange "#(argb,8,8,3)color(1,0.5,0,1)"
#define ProcTextTransparent	"#(argb,8,8,3)color(0,0,0,0)"
#define SCROLLBAR \
	class ScrollBar \
	{ \
		color[] = {1,1,1,0.6}; \
		colorActive[] = {1,1,1,1}; \
		colorDisabled[] = {1,1,1,0.3}; \
  		thumb = ProcTextWhite;\
  		arrowEmpty = ProcTextWhite;\
  		arrowFull = ProcTextWhite;\
  		border = ProcTextWhite;\
      shadow = 0;\
	};	


//Default text/font attributes for structured text controls.
class DefaultTextAttributes
{
	font = FontMAIN;
	color = "#ffffff";
	size = 1;
	align = "left";
	valign = "middle";
	shadow = "true";
	shadowOffset = 0.1;
	shadowColor = "#000000";
	underline = "false";
};

//Default sounds for controls.
class DefaultSounds
{
	soundOK[] = {"", 0.1, 1};
	soundCancel[] = {"", 0.1, 1};
	soundChangeFocus[] = {"", 0.1, 1};
	soundFail[] = {"", 0.1, 1};
};

//Standard static text.
class RscText
{
	access = ReadAndWrite;
	type = CT_STATIC;
	idc = -1;
	style = ST_LEFT;
	w = 0.1;
	h = 0.05;
	font = FontMAIN;
	sizeEx = Size_Text_Default;
	colorBackground[] = {0, 0, 0, 0};
	colorText[] = Color_Text_Default;
	text = "";
  fixedWidth = false;
  shadow = 0;
};

//Small static text.
class RscTextSmall: RscText
{
	h = 0.03;
	sizeEx = Size_Text_Small;
};

//Standard static text title.
class RscTitle: RscText
{
	style = ST_CENTER;
	x = 0.15;
	y = 0.06;
	w = 0.7;
  shadow = 0;
};

//Standard progress bar.
class RscProgress
{
	access = ReadAndWrite;
	type = CT_PROGRESS;
	style = 0;
	colorFrame[] = Color_Text_Default;
	colorBar[] = Color_Text_Default;
	texture = ProcTextWhite;
	w = 1.2;
	h = 0.03;
  shadow = 0;
};
class RscProgressNotFreeze
{
  idc = -1;
  type = CT_ANIMATED_TEXTURE;
  style = 0;
  shadow = 0;
  x = 0;
  y = 0;
  w = 0.5;
  h = 0.1;
  texture = ProcTextTransparent;
};
class RscPicture
{
	access = ReadAndWrite;
	type = CT_STATIC;
	idc = -1;
	style = ST_PICTURE;
	colorBackground[] = {0, 0, 0, 0};
	colorText[] = {1, 1, 1, 1};
	font = FontMAIN;
	sizeEx = 0;
	lineSpacing = 0;
	text = "";
  fixedWidth = false;
  shadow = 0;
};

//Standard HTML control.
class RscHTML
{
	access = ReadAndWrite;
	type = CT_HTML;
	idc = -1;
	style = 0;
	filename = "";
	colorBackground[] = {0, 0, 0, 0};
	colorText[] = Color_Text_Default;
	colorBold[] = {0, 0, 0.2, 1};
	colorLink[] = Color_Orange;
	colorLinkActive[] = Color_Orange;
	colorPicture[] = {1, 1, 1, 1};
  colorPictureLink[] = {1, 1, 1, 1};
	colorPictureSelected[] = {1, 1, 1, 1};
	colorPictureBorder[] = {0, 0, 0, 0};
	tooltipColorText[] = {0, 0, 0, 1};
	tooltipColorBox[] = {0, 0, 0, 0.5};
	tooltipColorShade[] = {1, 1, 0.7, 1};
  shadow = 0;

  prevPage = ProcTextWhite;
  nextPage = ProcTextWhite;

	class H1
	{
		font = FontMAIN;
		fontBold = FontMAIN;
		sizeEx = Size_Text_Default;
	};

	class H2
	{
		font = FontMAIN;
		fontBold = FontMAIN;
		sizeEx = Size_Text_Default;
	};

	class H3
	{
		font = FontMAIN;
		fontBold = FontMAIN;
		sizeEx = Size_Text_Default;
	};

	class H4
	{
		font = FontMAIN;
		fontBold = FontMAIN;
		sizeEx = Size_Text_Default;
	};

	class H5
	{
		font = FontMAIN;
		fontBold = FontMAIN;
		sizeEx = Size_Text_Default;
	};

	class H6
	{
		font = FontMAIN;
		fontBold = FontMAIN;
		sizeEx = Size_Text_Default;
	};

	class P
	{
		font = FontMAIN;
		fontBold = FontMAIN;
		sizeEx = Size_Text_Default;
	};
};

//Standard button.
class RscButton
{
  // common control items
	access = ReadAndWrite;
	type = CT_BUTTON;
	style = ST_LEFT;
  x = 0;
  y = 0;
	w = 0.3;
	h = 0.1;

  // text properties
  text = "";
	font = FontMAIN;
	sizeEx = Size_Text_Default;
	colorText[] = Color_Black;
  colorDisabled[] = Color_Gray;

  // background properties
  colorBackground[] = Color_GrayLight;
  colorBackgroundDisabled[] = Color_GrayLight;
  colorBackgroundActive[] = Color_Orange;
  offsetX = 0.004; // distance of background from shadow
  offsetY = 0.004;
  offsetPressedX = 0.002; // distance of background from shadow when button is pressed
  offsetPressedY = 0.002;
  colorFocused[] = Color_Black; // color of the rectangle around background when focused

  // shadow properties
  colorShadow[] = Color_Black;
  shadow = 0;

  // border properties
  colorBorder[] = Color_Black;
  borderSize = 0.008; // when negative, the border is on the right side of background

  // sounds
  soundEnter[] = {"", 0.1, 1};
  soundPush[] = {"", 0.1, 1};
	soundClick[] = {"", 0.1, 1};
	soundEscape[] = {"", 0.1, 1};
};

class RscShortcutButton
{
  type = CT_SHORTCUTBUTTON;
  style = 0;	
  x = 0.1;
  y = 0.1;
  w = 0.3;
  h = 0.05;
  shadow = 0;

  class HitZone
  {
    left = 0.0;
    top = 0.0;
    right = 1.0;
    bottom = 1.0;
  };   
  class ShortcutPos
  {
    left = 0.005;
    top = 0.005;
    w = 0.0225; // aspect 4 : 3
    h = 0.03;
  };   
  class TextPos
  {
    left = 0.02;
    top = 0.005;
    right = 0.005;
    bottom = 0.005;
  };   
  // background animated textures    
  animTextureNormal = ProcTextWhite;
  animTextureDisabled = ProcTextGray;
  animTextureOver = "#(argb,8,8,3)color(0.8,0.3,0,1)";
  animTextureFocused = "#(argb,8,8,3)color(1,0.5,0,1)";
  animTexturePressed = ProcTextRed;
  animTextureDefault = ProcTextGreen;

  period = 0.1;
  periodFocus = 0.4; // color animation period
  periodOver = 0.4; // color animation period

  shortcuts[] = {};
  textureNoShortcut = ProcTextTransparent;  // texture to show when no shortcut is accessible

  color[] = {0,0,0,0.6}; // text and icon color
  color2[] = {0,0,0,1}; // secondary text and icon color
  colorDisabled[] = {0,0,0,0.3}; // text and icon color

  colorBackground[] = {1,1,1,1}; // text and icon color
  colorBackground2[] = {1,1,1,0.5}; // secondary text and icon color

  text = "";
  size = 0.04; // text height
  class Attributes
  {
    font = FontMAIN;
    color = "#000000";
    align = "left";
    shadow = false;
  };
};

class RscButtonSmall: RscButton
{
	w = 0.12;
};

//Standard edit text field.
class RscEdit
{
	access = ReadAndWrite;
	type = CT_EDIT;
	style = ST_LEFT;
	x = 0; y = 0;
	h = 0.04; w = 0.2;
	colorBackground[] = {0, 0, 0, 0};
	colorText[] = Color_Text_Default;
	colorSelection[] = {1, 1, 1, 0.25};
	font = FontMAIN;
	sizeEx = Size_Text_Default;
	autocomplete = "";
	text = "";
	size = 0.2;
  shadow = 0;
};

//Standard combo box.
class RscCombo
{
	access = ReadAndWrite;
	type = CT_COMBO;
	style = 0;
	h = 0.05;
	wholeHeight = 0.25;
	colorSelect[] = Color_GrayLight;
	colorText[] = Color_Text_Default;
	colorBackground[] = Color_GrayDark;
  colorScrollbar[] = Color_Text_Default;
	font = FontMAIN;
	sizeEx = Size_Text_Default;
  soundSelect[] = {"", 0.1, 1};
  soundExpand[] = {"", 0.1, 1};
  soundCollapse[] = {"", 0.1, 1};
  maxHistoryDelay = 1.0;
  shadow = 0;
  SCROLLBAR
};

//Standard listbox.
class RscListBox
{
	access = ReadAndWrite;
	type = CT_LISTBOX;
	style = 0;
	w = 0.4;
	h = 0.4;
	font = FontMAIN;
	sizeEx = Size_Text_Default;
	rowHeight = 0;
	colorText[] = Color_Text_Default;
	colorScrollbar[] = Color_Text_Default;
	colorSelect[] = Color_Black;
	colorSelect2[] = Color_Orange;
 	colorSelectBackground[] = Color_GrayLight;
	colorSelectBackground2[] = Color_GrayDark;
	colorBackground[] = Color_Black;
  maxHistoryDelay = 1.0;
	soundSelect[] = {"", 0.1, 1};
	period = 1;
  autoScrollSpeed = -1;
  autoScrollDelay = 5;
  autoScrollRewind = false;
  arrowEmpty = ProcTextWhite;
  arrowFull = ProcTextWhite;
  shadow = 0;
	SCROLLBAR
};

class RscListNBox
{
  access = ReadAndWrite;
  type = CT_LISTNBOX;
  style = 0;
  w = 0.4;
  h = 0.4;
  font = FontMAIN;
  sizeEx = Size_Text_Default;
  rowHeight = 0;
  colorText[] = Color_Text_Default;
  colorScrollbar[] = Color_Text_Default;
  colorSelect[] = Color_Black;
  colorSelect2[] = Color_Orange;
  colorSelectBackground[] = Color_GrayLight;
  colorSelectBackground2[] = Color_GrayDark;
  colorBackground[] = Color_Black;
  maxHistoryDelay = 1.0;
  soundSelect[] = {"", 0.1, 1};
  period = 1;
  autoScrollSpeed = -1;
  autoScrollDelay = 5;
  autoScrollRewind = false;
  arrowEmpty = ProcTextWhite;
  arrowFull = ProcTextWhite;
  drawSideArrows = false;
  columns[] = {0.3,0.6,0.7};
  idcLeft = -1;
  idcRight = -1;
  shadow = 0;
  SCROLLBAR
};

class RscXListBox
{
	idc = -1;
	style = 0;	
  type = CT_XLISTBOX;
	x = 0.1;
	y = 0.1;
	w = 0.3;
	h = 0.05;
  shadow = 0;
       
    //barvy bitmap
    color[] = {1,1,1,0.6};
    colorActive[] = {1,1,1,1};
    colorDisabled[] = {1,1,1,0.3};

    //konfigurace textur, není to nutné jelikož xlistbox používá textury definovane v class CfgWrapperUI > Slider
    arrowEmpty = ProcTextWhite;
    arrowFull = ProcTextOrange;
    border = ProcTextBlack;

    //barvy textu
    colorSelect[] = {1,1,1,1};
    colorText[] = {1,1,1,0.8};
    font = FontMAIN;
    sizeEx = Size_Text_Default;
    soundSelect[] = {"", 0.1, 1};	
};

class RscTree
{
	access = ReadAndWrite;
	type = CT_TREE;
	style = 0;
	colorBackground[] = {0.35, 0.38, 0.36, 1};
	colorSelect[] = {1, 1, 1, 1};
	colorText[] = {1, 1, 1, 0.75};
	colorBorder[] = {1, 1, 1, 1};
	colorArrow[] = {1, 1, 1, 1};
	font = FontMAIN;
	sizeEx = Size_Text_Default;
  maxHistoryDelay = 1.0;
  shadow = 0;
};

//Standard slider
class RscSlider
{
	access = ReadAndWrite;
	type = CT_SLIDER;
	style = SL_HORZ;
	h = 0.028;
	w = 0.3;
	color[] = Color_Text_Default;
	colorActive[] = {1, 1, 1, 1};
  shadow = 0;
};

//Remove
class RscSliderH: RscSlider {};

class RscXSliderH
{
	type = CT_XSLIDER;
	style = SL_HORZ  + SL_TEXTURES;
	h = __EVAL(3 * _ySpacing);
	color[] = {1,1,1,0.6};
	colorActive[] = {1,1,1,1};
	colorDisable[] = {1,1,1,0.4};
	arrowEmpty = ProcTextWhite;
	arrowFull = ProcTextWhite;
	border = ProcTextWhite;
	thumb = ProcTextWhite;
  shadow = 0;
};

//Standard active text.
class RscActiveText
{
	access = ReadAndWrite;
	type = CT_ACTIVETEXT;
	style = ST_CENTER;
	h = 0.05;
	w = 0.15;
	font = FontMAIN;
	sizeEx = Size_Text_Default;
  color[] = Color_Text_Default;
	colorActive[] = {1, 0.5, 0, 1};
	soundEnter[] = {"", 0.1, 1};
	soundPush[] = {"", 0.1, 1};
	soundClick[] = {"", 0.1, 1};
	soundEscape[] = {"", 0.1, 1};
	text = "";
	default = 0;
  shadow = 0;
};

//Standard structured text.
class RscStructuredText
{
	access = ReadAndWrite;
	type = CT_STRUCTURED_TEXT;
	idc = -1;
	style = 0;
	h = 0.05;
	text = "";
	size = Size_Text_Default;
	colorText[] = Color_Text_Default;
  shadow = 0;

	class Attributes
	{
		font = FontMAIN;
		color = "#ffffff";
		align = "center";
		shadow = true;
	};
};

//Standard controls group.
class RscControlsGroup
{
  type = CT_CONTROLS_GROUP;
  idc = -1;
  style = 0;
  x = 0; y = 0;
  w = 1; h = 1;
  shadow = 0;

  class VScrollbar
  {
    color[] = Color_Text_Default;
    width = 0.021;
    autoScrollSpeed = -1;
    autoScrollDelay = 5;
    autoScrollRewind = false;
    shadow = 0;
  };

  class HScrollbar
  {
    color[] = Color_Text_Default;
    height = 0.028;
    shadow = 0;
  };
  SCROLLBAR
  class Controls {};
};

class RscBackgroundStripeTop: RscText
{
	access = ReadAndWrite;
	x = -10.0;
	y = -10.0;
	w = 21;
	h = 10.125;
	text = ;
	colorBackground[] = {0.1, 0.1, 0.1, 1};
};

class RscBackgroundStripeBottom: RscText
{
	access = ReadAndWrite;
	x = -10.0;
	y = 0.875;
	w = 21;
	h = 10.125;
	text = ;
	colorBackground[] = {0.1, 0.1, 0.1, 1};
};

class RscDisplayBackgroundStripes
{
	access = ReadAndWrite;
	class Background1: RscBackgroundStripeTop {};
	class Background2: RscBackgroundStripeBottom {};
};

class RscCinemaBorder: RscDisplayBackgroundStripes
{
	idd = -1;
	movingEnable = false;
	enableSimulation = true;
	class controlsBackground
	{
		class Background1: RscBackgroundStripeTop
		{
		  	colorBackground[] = {0, 0, 0, 1};
		};

		class Background2: RscBackgroundStripeBottom
		{
		  	colorBackground[] = {0, 0, 0, 1};
		};
	};
};

//Standard toolbox.
class RscToolbox
{
	access = ReadAndWrite;
	type = CT_TOOLBOX;
	style = ST_CENTER;
	colorText[] = Color_Text_Default;
	color[] = Color_Text_Default;
	colorTextSelect[] = Color_Text_Default;
	colorSelect[] = Color_Text_Default;
	colorTextDisable[] = Color_Text_Default;
	colorDisable[] = Color_Text_Default;
  coloSelectedBg[] = {0,0,0,255};
	font = FontMAIN;
	sizeEx = Size_Text_Small;
  shadow = 0;
};

class RscXKeyShadow
{
	type = CT_XKEYDESC;
	idc = -1;
	style = 0;
	h = 0.06;
	size = SizeNormal;
	shadow = 0;
	class Attributes
	{
		font = FontX;
		color = "#C0C1BF";
		align = "left";
	};
	class AttributesImage
	{
		color = "#ffffff";
	};
};

class RscXKey : RscXKeyShadow
{
  class Attributes
  {
		shadow = false;		
	};
};
//Standard map.
class RscMapControl
{
	access = ReadAndWrite;
	type = CT_MAP_MAIN;
	idc = IDC_MAP;
	style = ST_PICTURE;
	shadow = 0;
  // map background
	colorBackground[] = {1, 1, 1, 1};
  // area outside of map
  colorOutside[] = {1, 0, 0, 1};
	colorText[] = {0, 0, 0, 1};
	font = FontMAIN;
	sizeEx = Size_Text_Default;
	
	colorSea[] = {0.56, 0.8, 0.98, 0.5};
	colorForest[] = {0.6, 0.8, 0.2, 0.5};
	colorRocks[] = {0.5, 0.5, 0.5, 0.5};
	colorCountlines[] = {0.65, 0.45, 0.27, 0.5};
	colorMainCountlines[] = {0.65, 0.45, 0.27, 1};
	colorCountlinesWater[] = {0, 0.53, 1, 0.5};
	colorMainCountlinesWater[] = {0, 0.53, 1, 1};
	colorForestBorder[] = {0.4, 0.8, 0, 1};
	colorRocksBorder[] = {0.5, 0.5, 0.5, 1};
	colorPowerLines[] = {0, 0, 0, 1};
  colorRailWay[] = {0.8, 0.2, 0.3, 1};
	colorNames[] = {0, 0, 0, 1};
	colorLevels[] = {0, 0, 0, 1};

  //widthRailWay = 5.0;
	
	fontLabel = FontMAIN;
	sizeExLabel = Size_Text_Default;
	fontGrid = FontMAIN;
	sizeExGrid = Size_Text_Default;
	fontNames = FontMAIN;
	sizeExNames = Size_Text_Default;
	fontInfo = FontMAIN;
	sizeExInfo = Size_Text_Default;
	fontLevel = FontMAIN;
	sizeExLevel = Size_Text_Default;
	
	text = ProcTextWhite;
	
	stickX[] = {0.2, {"Gamma", 1, 1.5}};
	stickY[] = {0.2, {"Gamma", 1, 1.5}};
	
	// Map drawing quality coefficients:
	//  units - the width of the screen == 800
	//  limits - size of the landscape square on screen when objects are drawn or single square content is drawn
	
	//@{ coefficients which determine rendering density / threshold
	ptsPerSquareSea = 6;   // seas
	ptsPerSquareTxt = 8;   // textures
	ptsPerSquareCLn = 8;   // count-lines
	ptsPerSquareCost = 8;  // cost
	//@}
	
	//@{ coefficients which determine when rendering of given type is done
	ptsPerSquareFor = 4.0f;   // forests
	ptsPerSquareForEdge = 10.0f;   // forest edges
	ptsPerSquareRoad = 2;  // roads
	ptsPerSquareObj = 10;    // other objects
	//@}

  showCountourInterval = "true";

  // _scale is % of map you can see;  satellite map alpha is
  // if map (_scale < alphaFadeStartScale) -> alpha = 1.0 * maxSatelliteAlpha;
  // else map (_scale > alphaFadeEndScale) -> alpha = 0.0;
  // else -> alpha = ((alphaFadeEndScale - _scale) / (alphaFadeEndScale - alphaFadeStartScale)) * maxSatelliteAlpha;  
  maxSatelliteAlpha = 0.66;
  alphaFadeStartScale = 0.05;
  alphaFadeEndScale = 0.15;

	colorFriendly[] = {0, 0.5, 0, 1};
	colorEnemy[] = {0.5, 0, 0, 1};
	colorNeutral[] = {0.5, 0.5, 0.5, 1};
	colorCivilian[] = {0, 0, 1, 1};
	colorUnknown[] = {0.5, 0.5, 0, 1};

	iconPlayer = ProcTextWhite;
	iconCheckpoint = ProcTextWhite;
	iconCamera = ProcTextWhite;
	iconSelect = ProcTextWhite;
	iconSensor = ProcTextWhite;

	sizeLeader = 24;
	size = 20;
  sizePlayer = 16;

	colorMe[] = {Red, 1};
	colorPlayable[] = {0.7, 0, 0.7, 1};
	colorSelect[] = {Green, 0.5};
	colorCamera[] = {Blue, 1};
	colorSensor[] = {Blue, 1};

	colorDragging[] = {White, 1};

	colorInactive[] = {1,1,1, 0.6};

  colorTracks[] = {0.35, 0.2, 0.1, 0.8};
  colorRoads[] = {0.35, 0.2, 0.1, 1};
  colorMainRoads[] = {0, 0, 0, 1};
  colorTracksFill[] = {0, 0, 0, 0};
  colorRoadsFill[] = {1, 0.92, 0.74, 1};
  colorMainRoadsFill[] = {0.93, 0.11, 0.14, 0.8};
	colorGrid[] = {0.15, 0.15, 0.05, 0.9};
	colorGridMap[] = {0.25, 0.25, 0.1, 0.75};
	colorCheckpoints[] = {0.2, 0.2, 0.2, 1.0};
	colorMissions[] = {0.5, 0.5, 0.0, 0.5};
	colorActiveMission[] = {0.0, 0.0, 0.0, 1.0};

	colorPath[] = {0, 1, 1, 1.0};
  colorPathDone[] = {0, 1, 0, 0.8};
  texturePath = ProcTextWhite;
  texturePathDone = ProcTextWhite;
  sizePath = 10;
  sizePathDone = 2;

	colorInfoMove[] = {1.0, 1.0, 1.0, 1.0};

	colorGroups[] = {0.0, 0.5, 0.5, 0.5};
	colorActiveGroup[] = {0.0, 1.0, 1.0, 1.0};

	colorSync[] = {0.0, 0.0, 1.0, 1.0};

	colorDetectorSync[] = {0,1,0,1};

	colorLabelBackground[] = {0.0, 0.0, 0.0, 1.0};

	cursorLineWidth = 3;

  class Legend
  {
    x = 0.7;
    y = 0.85;
    w = 0.25;
    h = 0.1;

    font = FontMAIN;
    sizeEx = Size_Text_Default;

    colorBackground[] = {1, 1, 1, 1};
    color[] = {0, 0, 0, 1};
  };

	class ActiveMarker
	{
		color[] = {0.3, 0.1, 0.9, 1};
		size = 50;
	};

  class Task
  {
    icon = ProcTextGreen;
    iconCreated = ProcTextWhite;
    iconCanceled = ProcTextBlue;
    iconDone = ProcTextBlack;
    iconFailed = ProcTextRed;

    colorCreated[] = {1, 1, 1, 1};
    colorCanceled[] = {1, 1, 1, 1};
    colorDone[] = {1, 1, 1, 1};
    colorFailed[] = {1, 1, 1, 1};

    color[] = {1, 1, 1, 1};
    size = 18;
    importance = 1; // not used
    coefMin = 1; // not used
    coefMax = 1; // not used
  };
  class CustomMark
  {
    icon = ProcTextOrange;
    color[] = {1, 1, 1, 1};
    size = 18;
    importance = 1; // not used
    coefMin = 1; // not used
    coefMax = 1; // not used
  };
	class Tree
	{
		icon = ProcTextBlack;
		color[] = {0, 0, 0, 1};
		size = 16;
		importance = 0.9 * 16 * 0.05; // limit for map scale
		coefMin = 0.25;
		coefMax = 4;
	};

	class SmallTree
	{
		icon = ProcTextBlack;
		color[] = {0, 0, 0, 1};
		size = 16;
		importance = 0.6 * 16 * 0.05; // limit for map scale
		coefMin = 0.25;
		coefMax = 4;
	};

	class Bush
	{
		icon = ProcTextBlack;
		color[] = {0, 0, 0, 1};
		size = 16;
		importance = 0.2 * 16 * 0.05; // limit for map scale
		coefMin = 0.25;
		coefMax = 4;
	};

	class Church
	{
		icon = ProcTextBlack;
		color[] = {0, 0, 0, 1};
		size = 16;
		importance = 2 * 16 * 0.05; // limit for map scale
		coefMin = 0.25;
		coefMax = 4;
	};

	class Chapel
	{
		icon = ProcTextBlack;
		color[] = {0, 0, 0, 1};
		size = 16;
		importance = 1 * 16 * 0.05; // limit for map scale
		coefMin = 0.25;
		coefMax = 4;
	};

	class Cross
	{
		icon = ProcTextBlack;
		color[] = {0, 0, 0, 1};
		size = 16;
		importance = 0.7 * 16 * 0.05; // limit for map scale
		coefMin = 0.25;
		coefMax = 4;
	};

	class Rock
	{
		icon = ProcTextBlack;
		color[] = {0, 0, 0, 1};
		size = 16;
		importance = 1.5 * 16 * 0.05; // limit for map scale
		coefMin = 0.25;
		coefMax = 4;
	};

	class Bunker
	{
		icon = ProcTextBlack;
		color[] = {0, 0, 0, 1};
		size = 16;
		importance = 1.5 * 16 * 0.05; // limit for map scale
		coefMin = 0.25;
		coefMax = 4;
	};

	class Fortress
	{
		icon = ProcTextBlack;
		color[] = {0, 0, 0, 1};
		size = 16;
		importance = 2 * 16 * 0.05; // limit for map scale
		coefMin = 0.25;
		coefMax = 4;
	};

	class Fountain
	{
		icon = ProcTextBlack;
		color[] = {0, 0, 1, 1};
		size = 16;
		importance = 1 * 16 * 0.05; // limit for map scale
		coefMin = 0.25;
		coefMax = 4;
	};

	class ViewTower
	{
		icon = ProcTextBlack;
		color[] = {0, 0, 0, 1};
		size = 16;
		importance = 2.5 * 16 * 0.05; // limit for map scale
		coefMin = 0.25;
		coefMax = 4;
	};

	class Lighthouse
	{
		icon = ProcTextBlack;
		color[] = {0, 0, 0, 1};
		size = 16;
		importance = 3 * 16 * 0.05; // limit for map scale
		coefMin = 0.25;
		coefMax = 4;
	};

	class Quay
	{
		icon = ProcTextBlack;
		color[] = {0, 0, 0, 1};
		size = 16;
		importance = 2 * 16 * 0.05; // limit for map scale
		coefMin = 0.25;
		coefMax = 4;
	};

	class Fuelstation
	{
		icon = ProcTextBlack;
		color[] = {0, 0, 0, 1};
		size = 16;
		importance = 2 * 16 * 0.05; // limit for map scale
		coefMin = 0.25;
		coefMax = 4;
	};

	class Hospital
	{
		icon = ProcTextBlack;
		color[] = {1, 0, 0, 1};
		size = 16;
		importance = 2 * 16 * 0.05; // limit for map scale
		coefMin = 0.25;
		coefMax = 4;
	};

	class BusStop
	{
		icon = ProcTextBlack;
		color[] = {0, 0, 1, 1};
		size = 8;
		importance = 1 * 8 * 0.05; // limit for map scale
		coefMin = 0.25;
		coefMax = 4;
	};

  class Transmitter
  {
		icon = ProcTextBlack;
		color[] = {0, 0, 1, 1};
		size = 8;
		importance = 1 * 8 * 0.05; // limit for map scale
		coefMin = 0.25;
		coefMax = 4;
  };

  class Stack
  {
		icon = ProcTextBlack;
		color[] = {0, 0, 1, 1};
		size = 8;
		importance = 1 * 8 * 0.05; // limit for map scale
		coefMin = 0.25;
		coefMax = 4;
  };

  class Ruin
  {
		icon = ProcTextBlack;
		color[] = {0, 0, 1, 1};
		size = 8;
		importance = 1 * 8 * 0.05; // limit for map scale
		coefMin = 0.25;
		coefMax = 4;
  };

  class Tourism
  {
		icon = ProcTextBlack;
		color[] = {0, 0, 1, 1};
		size = 8;
		importance = 1 * 8 * 0.05; // limit for map scale
		coefMin = 0.25;
		coefMax = 4;
  };

  class Watertower
  {
		icon = ProcTextBlack;
		color[] = {0, 0, 1, 1};
		size = 8;
		importance = 1 * 8 * 0.05; // limit for map scale
		coefMin = 0.25;
		coefMax = 4;
  };
};

class RscObject
{
	access = ReadAndWrite;
	type = CT_OBJECT;
	scale = 1.0;
	direction[] = {0, 0, 1};
	up[] = {0, 1, 0};
  shadow = 0;
};

class RscStandardDisplay
{
	access = ReadAndWrite;
	movingEnable = false;
	enableSimulation = false;
	enableDisplay = false;
};
class RscLineBreak 
{
  idc = -1;
  type = CT_LINEBREAK;
  shadow = 0;
};

class RscCompass
{
  x = 0.5;
  y = 0.5;
  w = 0.5;
  h = 0.5;
  z = 1.0;
};
class RscWatch
{
  x = 0.0;
  y = 0.5;
  w = 0.5;
  h = 0.5;
  z = 1.0;
};
//In-game GPS minimap
class RscMiniMap
{
	access = ReadAndWrite;
	idd = IDD_MINIMAP;
	movingEnable = false;
	enableSimulation = true;
	enableDisplay = true;
	
	class controls 
	{
		class MiniMap: RscMapControl 
		{
			IDC = IDC_MINIMAP;
			x = 0.1;
			y = 0.1;
			w = 0.3;
			h = 0.3;
      showCountourInterval = "false";
		};
	};
};

//In-game GPS minimap small
class RscMiniMapSmall: RscMiniMap {};

class RscButtonImages
{
};

class RscObjectives
{
  access = ReadAndWrite;
  done = ProcTextWhite;
  failed = ProcTextWhite;
  active = ProcTextWhite;
  cancled = ProcTextWhite;
};

//Todo:
//BISML™ 1.0 Strict:

//Body tag. Defines the main page margins.
#define BISML_BODY(X, Y, W, H) \
	__EXEC(iBISML_X = X);\
	__EXEC(iBISML_Y = Y);\
	__EXEC(iBISML_W = W);\
	__EXEC(iBISML_H = H);\
	__EXEC(iBISML_CURX = iBISML_X);\
	__EXEC(iBISML_CURY = iBISML_Y);

//Row tag. Defines the end of a row.
#define BISML_NEXTROW \
	__EXEC(iBISML_CURY = iBISML_CURY + iBISML_NEXTROW_Y);\
	__EXEC(iBISML_CURX = iBISML_X);

//Cell tag. Defines a layout cell within a row. Replace with virtual cell definition.
#define BISML_CELL(NAME, W, H) \
	class NAME: RscText\
	{\
		x = __EVAL(iBISML_CURX);\
		y = __EVAL(iBISML_CURY);\
		w = W;\
		h = H;\
		colorBackground[] = {1, 0, 0, 1};\
		color[] = {1, 1, 1, 1};\
	};\
	__EXEC(iBISML_CURX = iBISML_CURX + W);\
	__EXEC(iBISML_NEXTROW_Y = H);

#endif
