#include "commonDefs.hpp"
#include "resincl.hpp"
#include "rscCommon.hpp"

//Todo:
//Rename to rscEditor.
//Add back normal island select screen.

#define LINE_HEIGHT 0.05
#define LINE_Y __EVAL(line_y)
#define LINE_W(VALUE) __EVAL(VALUE * line_w)
#define LINE_X(VALUE) __EVAL(line_x + (VALUE * line_w))
#define NEXT_LINE __EXEC(line_y = line_y + LINE_HEIGHT)

#define DISPLAY(IDD, X, Y, W, H) \
	idd = IDD;\
	__EXEC(display_x = X);\
	__EXEC(display_y = Y);\
	__EXEC(display_w = W);\
	__EXEC(display_h = H);\
	__EXEC(line_x = X + 0.02);\
	__EXEC(line_w = W - 0.08);\
	__EXEC(line_y = Y + 0.1);\
	movingEnable = true;
	access = ReadAndWrite;

#define BACKGROUND(NAME) \
	class NAME: RscText\
	{\
		x = __EVAL(display_x);\
		y = __EVAL(display_y);\
		w = __EVAL(display_w);\
		h = __EVAL(display_h);\
		colorBackground[] = {0.3, 0.3, 0.3, 1};\
	};

#define TITLE(NAME, TEXT, IDC) \
	class NAME: RscText\
	{\
		idc = IDC;\
		style = ST_TITLE;\
		x = __EVAL(display_x);\
		y = __EVAL(display_y + 0.02);\
		w = __EVAL(display_w);\
		text = TEXT;\
	};

#define BEGIN_BOX \
	__EXEC(box_y = line_y);\
	__EXEC(line_x = display_x + 0.015);\
	__EXEC(line_w = display_w - 0.05);\
	__EXEC(line_y = box_y + 0.01);

#define END_BOX \
	__EXEC(line_y = line_y + 0.04);\
	__EXEC(line_x = display_x + 0.02);\
	__EXEC(line_w = display_w - 0.045);

#define	STATIC_TEXT(NAME, TEXT, IDC, XVAL, WVAL) \
	class NAME: RscText\
	{\
		idc = IDC;\
		x = LINE_X(XVAL);\
		y = LINE_Y;\
		w = LINE_W(WVAL);\
		text = TEXT;\
		sizeEx = Size_Text_Small;\
	};

#define	EDIT(NAME, IDC, XVAL, WVAL) \
	class NAME: RscEdit\
	{\
		idc = IDC;\
		x = LINE_X(XVAL);\
		y = LINE_Y + 0.01;\
		w = LINE_W(WVAL);\
		h = 0.03;\
		text = $STR_DISP_ERROR;\
		sizeEx = Size_Text_Small;\
	};

#define	EDIT_SCRIPT(NAME, IDC, XVAL, WVAL) \
	class NAME: RscEdit\
	{\
		idc = IDC;\
		x = LINE_X(XVAL);\
		y = LINE_Y + 0.01;\
		w = LINE_W(WVAL);\
		h = 0.03;\
		text = $STR_DISP_ERROR;\
		autocomplete="scripting";\
		sizeEx = Size_Text_Small;\
	};

#define	COMBO(NAME, IDC, XVAL, WVAL) \
	class NAME: RscCombo\
	{\
		idc = IDC;\
		x = LINE_X(XVAL);\
		y = LINE_Y + 0.01;\
		w = LINE_W(WVAL);\
		h = 0.03;\
		sizeEx = Size_Text_Small;\
	};

#define	SLIDER(NAME, IDC, XVAL, WVAL) \
	class NAME: RscSlider\
	{\
		idc = IDC;\
		x = LINE_X(XVAL);\
		y = LINE_Y + 0.01;\
		w = LINE_W(WVAL);\
		h = 0.03;\
	};

#define	BUTTON(NAME, TEXT, IDC, SOUND, VOLUME, XVAL, DEFAULT) \
	class NAME: RscButton\
	{\
		idc = IDC;\
		x = __EVAL(display_x + display_w - 0.153 - XVAL * 0.145);\
		y = __EVAL(display_y + display_h - 0.075);\
		w = 0.13;\
		h = 0.05;\
		text = TEXT;\
		default = DEFAULT;\
		sizeEx = Size_Text_Small;\
	};

#define	LEFT_BUTTON(NAME, TEXT, IDC, SOUND, VOLUME, XVAL, DEFAULT) \
	class NAME: RscButton\
	{\
		idc = IDC;\
		x = __EVAL(display_x + 0.02 + XVAL * 0.15);\
		y = __EVAL(display_y + display_h - 0.075);\
		w = 0.13;\
		h = 0.05;\
		text = TEXT;\
		default = DEFAULT;\
		sizeEx = Size_Text_Small;\
	};

#define BUTTON_OK(XVAL) BUTTON(ButtonOK, $STR_DISP_OK, IDC_OK, "", 0.2, XVAL, 0)
#define DEFAULT_BUTTON_OK(XVAL) BUTTON(ButtonOK, $STR_DISP_OK, IDC_OK, "", 0.2, XVAL, 1)
#define BUTTON_CANCEL(XVAL) BUTTON(ButtonCancel, $STR_DISP_CANCEL, IDC_CANCEL, "", 0.2, XVAL, 0)
#define DEFAULT_BUTTON_CANCEL(XVAL) BUTTON(ButtonCancel, $STR_DISP_CANCEL, IDC_CANCEL, "", 0.2, XVAL, 1)

//Island selection screen. Shown when accessing the mission editor.
class RscDisplaySelectIsland: RscStandardDisplay
{
	idd = IDD_SELECT_ISLAND;

	class controls
	{
		class Title: RscTitle {text = $STR_DISP_SELECT_ISLAND;};

		//The available islands.
		class Islands: RscListBox
		{
			idc = IDC_SELECT_ISLAND;
			x = 0.025; y = 0.2;
			w = 0.95; h = 0.6;
			rows = 8;
		};
		
		//Cancel back to the main menu.
		class B_Cancel: RscActiveText
		{
			idc = IDC_CANCEL;
			x = 0.59; y = 0.9;
			w = 0.25;
			text = $STR_DISP_CANCEL;
		};

		//Confirm the selected island and start the new editor.
		class B_OK: B_Cancel
		{
			//idc = IDC_OK; This would launch the old editor.
			idc = IDC_SELECT_ISLAND_EDITOR;
			x = 0.84;
			w = 0.15;
			text = $STR_DISP_OK;
			default = 1;
		};
	};
};

//ToDo: Verify this is working.
//Advanced island selection screen.
class RscDisplayCustomArcade: RscStandardDisplay
{
	idd = IDD_CUSTOM_ARCADE;

	class controls
	{
		class Title: RscTitle {text = $STR_DISP_MAIN_DESIGN;};

		//Tree interface for available islands and missions.
		class Game: RscTree
		{
			idc = IDC_CUST_GAME;
			x = 0.24; y = 0.22;
			w = 0.52;	h = 0.56;
			style = TR_AUTOCOLLAPSE;
		};

		//Edit the selected mission.
		class B_Edit: RscButton
		{
			idc = IDC_CUST_EDIT;
			x = 0.22; y = 0.9;
	    w = 0.16; h = 0.05;
			text = $STR_DISP_ERROR;
		};

		//Delete the selected mission.
		class B_Delete: B_Edit
		{
			idc = IDC_CUST_DELETE;
			x = 0.36;
			text = $STR_DISP_DELETE;
		};

		//Play the selected mission.
		class B_Play: B_Edit
		{
			idc = IDC_CUST_PLAY;
			x = 0.52;
			text = $STR_DISP_CUST_PLAY;
			default = 1;
		};

		//Cancel back to the main menu.
		class B_Cancel: B_Edit
		{
			idc = IDC_CANCEL;
			x = 0.66;
			text = $STR_DISP_CANCEL;
		};
	};
};

//X-box style mission wizard screen.
class RscDisplayXWizardTemplate: RscStandardDisplay
{
	idd = IDD_XWIZARD_TEMPLATE;

  west = ProcTextWhite;
  east = ProcTextWhite;
  guer = ProcTextWhite;
  civl = ProcTextWhite;

	class controls
	{
		class Title: RscTitle {text = $STR_SINGLE_NEW_MISSION_TITLE;};

		class TemplateText: RscText
		{
			x = 0.025; y = 0.16;
			w = 0.3;
			text = $STR_DISP_MPW_TEMPLATE;
		};

		//Available templates.
		class TemplateValue: RscListBox
		{
			idc = IDC_XWIZ_TEMPLATE;
			x = 0.025; y = 0.24;
			w = 0.45; h = 0.57;
		};

		//HTML overview page of the selected template.
		class Overview: RscHTML
		{
			idc = IDC_XWIZ_TEMPLATE_OVERVIEW;
			x = 0.525; y = 0.24;
			w = 0.45; h = 0.57;
		};

		//Cancel template creation and exit.
		class B_Cancel: RscActiveText
		{
			idc = IDC_CANCEL;
			x = 0.65; y = 0.9;
			w = 0.15;
			text = $STR_DISP_CANCEL;
		};

		//Confirm template settings.
		class B_OK: B_Cancel
		{
			idc = IDC_OK;
			x = 0.85;
			default = 1;
			text = $STR_DISP_OK;
		};
	};
};

//X-box style mission wizard configuration screen.
class RscDisplayXWizardIntel: RscStandardDisplay
{
	idd = IDD_XWIZARD_INTEL;

  textureClear = ProcTextWhite;
  textureCloudly = ProcTextWhite;
  textureOvercast = ProcTextWhite;
  textureRainy = ProcTextWhite;
  textureStormy = ProcTextWhite;

	class controls
	{
		class Title: RscTitle {text = $STR_XBOX_WIZARD_EDIT;};

		class SumaTemplateValue: RscText
		{
			idc = IDC_XWIZ_SUM_TEMPLATE;
			x = 0.03; y = 0.14;
			w = 0.3; h = 0.08;
		};

	 	class Unit: RscButton
	 	{
	 		idc = IDC_XWIZ_UNIT;
			x = 0.03; y = 0.2;
			w = 0.25; h = 0.04;
	 		text = $STR_DISP_SRVSETUP_UNITS;
	 	};

    class Params: Unit
    {
      idc = IDC_XWIZ_PARAM;
      y = 0.26;
      text = $STR_DISP_XBOX_EDITOR_WIZARD_PARAMS;
    };

		class Island: Unit
		{
			idc = IDC_XWIZ_ISLAND;
			y = 0.32; 
			text = $STR_DISP_XBOX_EDITOR_WIZARD_ISLAND;
		};

	 	class Name: Unit
		{
			idc = IDC_XWIZ_NAME;
			y = 0.38;
			text = $STR_DISP_XBOX_EDIT_PROFILE_NAME;
		};

		class Map: Unit
		{
			idc = IDC_XWIZ_MAP;
			y = 0.44;
			text = $STR_USRACT_MAP;
		};

		class Time: Unit
		{
			idc = IDC_XWIZ_TIME;
			y = 0.5;
			text = $STR_DISP_XBOX_EDITOR_WIZARD_TIME_TITLE;
		};

		class Weather: Unit
		{
			idc = IDC_XWIZ_WEATHER;
			y = 0.56;
			text = $STR_DISP_INTEL_WEATHER;
		};

		class NameText: RscText
		{
			x = 0.54; y = 0.255;
			w = 0.2; h = 0.04;
			text = $STR_DISP_GAME_NAME;
		};

		class NameValue: RscText
		{
			idc = IDC_XWIZ_SUM_NAME;
			x = 0.64; y = 0.25;
			w = 0.3; h = 0.06;
		};

		class IslandText: NameText
		{
			idc = -1;
			y = 0.339;
			text = $STR_DISP_XBOX_MULTI_ISLAND;
		};

		class IslandValue: NameValue
		{
			idc = IDC_XWIZ_SUM_ISLAND;
			y = 0.33;
			text = "";
		};

		class TimeText: IslandText
		{
			y = 0.417;
			text = $STR_DISP_INTEL_TIME;
		};

		class TimeValue: IslandValue
		{
			idc = IDC_XWIZ_SUM_TIME;
			y = 0.41;
			text = "";
		};

		class DateText: IslandText
		{
			y = 0.495;
			text = $STR_DISP_INTEL_DATE;
		};

		class DateValue: IslandValue
		{
			idc = IDC_XWIZ_SUM_DATE;
			y = 0.49;
			text = "";
		};

		class WeatherCurrentText: IslandText
		{
			y = 0.65;
			w = 0.4;
			text = $STR_DISP_XBOX_EDITOR_WIZARD_WEATHER_SUMA_CURRENT;
		};

		class WeatherCurrentValue: RscPicture
		{
			idc = IDC_XWIZ_SUM_WEATHER;
			x = 0.835; y = 0.64;
			w = 0.09; h = 0.09;
		};

		class WeatherForecastText: IslandText
		{
			y = 0.72;
			w = 0.4;
			text = $STR_DISP_XBOX_EDITOR_WIZARD_WEATHER_SUMA_FORECAST;
		};

		class WeatherForecastValue: WeatherCurrentValue
		{
			idc = IDC_XWIZ_SUM_WEATHER_FORECAST;
			y = 0.71;
			text = "";
		};

    // Preview
    class B_OK: RscActiveText
    {
      idc = IDC_OK;
      x = 0.50; y = 0.9;
      w = 0.15;
      text = $STR_DISP_ARCMAP_PREVIEW; 
    };
		//Cancel template creation and exit.
		class B_Cancel: RscActiveText
		{
			idc = IDC_CANCEL;
			x = 0.65; y = 0.9;
			w = 0.15;
			text = $STR_DISP_CANCEL;
		};
	};
};

class RscDisplayXWizardIntelName: RscStandardDisplay
{
	idd = IDD_XWIZARD_NAME;

	class controls
	{
		class Title: RscTitle {text = $STR_DISP_XBOX_EDITOR_WIZARD_NAME_TITLE;};

		class Name: RscEdit
		{
			idc = IDC_PROFILE_NAME_PREVIEW;
			x = 0.025; y = 0.18;
			w = 0.4;
		};

		class B_Cancel: RscActiveText
		{
			idc = IDC_CANCEL;
			x = 0.65; y = 0.9;
			w = 0.15;
			text = $STR_DISP_CANCEL;
		};

		class B_OK: B_Cancel
		{
			idc = IDC_OK;
			x = 0.85;
			default = 1;
			text = $STR_DISP_OK;
		};
	};
};

class RscDisplayXWizardIntelIsland: RscStandardDisplay
{
	idd = IDD_XWIZARD_ISLAND;

	class controls
	{
		class Title: RscTitle {text = $STR_DISP_XBOX_EDITOR_WIZARD_ISLAND_TITLE;};

		class IslandValue: RscListBox
		{
			idc = IDC_XWIZ_ISLAND_LIST;
			x = 0.025; y = 0.18;
			w = 0.4; h = 0.3;
		};

		class IslandMap: RscPicture
		{
			idc = IDC_XWIZ_ISLAND_MAP;
			x = 0.055; y = 0.45;
			w = 0.4; h = 0.4;
		};

		class IslandShots: RscPicture
		{
			idc = IDC_XWIZ_ISLAND_SHOTS;
			x = 0.6; y = 0.17;
			w = 0.3; h = 0.68;
		};

		class B_Cancel: RscActiveText
		{
			idc = IDC_CANCEL;
			x = 0.65; y = 0.9;
			w = 0.15;
			text = $STR_DISP_CANCEL;
		};

		class B_OK: B_Cancel
		{
			idc = IDC_OK;
			x = 0.85;
			default = 1;
			text = $STR_DISP_OK;
		};
	};
};

class RscDisplayXWizardIntelWeather: RscStandardDisplay
{
	idd = IDD_XWIZARD_WEATHER;

	class controls
	{
		class Title: RscTitle {text = $STR_DISP_INTEL_WEATHER;};

		class Bourka: RscPicture
		{
			text = ProcTextWhite;
			x = 0.12; y = 0.33;
			w = 0.1; h = 0.12;
		};

		class Jasno: RscPicture
		{
			text = ProcTextWhite;
			x = 0.78; y = 0.33;
			w = 0.1; h = 0.12;
		};

		class WeatherText: RscText
		{
			x = 0.1; y = 0.25;
			w = 0.34;
			text = $STR_DISP_XBOX_EDITOR_WIZARD_WEATHER;
		};

		class Current: RscText
		{
			idc = IDC_XWIZ_WEATHER_TITLE;
			style = ST_CENTER;
			x = 0.33; y = 0.27;
			w = 0.34;
			text = $STR_DISP_XBOX_EDITOR_WIZARD_WEATHER_CURRENT;
		};

		class WeatherValueTitle: RscText
		{
			idc = IDC_XWIZ_WEATHER_TITLE;
			x = 0.25; y = 0.32;
			w = 0.5;
		};

		class WeatherValue: RscSlider
		{
			idc = IDC_XWIZ_WEATHER_VALUE;
			x = 0.25; y = 0.32;
			w = 0.5;
		};

		class Forecast: Current
		{
			idc = IDC_XWIZ_WEATHER_FORECAST_TITLE;
			y = 0.37;
			text = $STR_DISP_INTEL_FORECAST;
		};

		class ForecastWeatherTitle: RscText
		{
			idc = IDC_XWIZ_WEATHER_FORECAST_TITLE;
			x = 0.25; y = 0.42;
			w = 0.5;
		};

		class ForecastWeatherValue: RscSlider
		{
			idc = IDC_XWIZ_WEATHER_FORECAST_VALUE;
			x = 0.25; y = 0.42;
			w = 0.5;
		};

		class FogText: WeatherText
		{
			y = 0.5;
			h = 0.09;
			text = $STR_DISP_XBOX_EDITOR_WIZARD_WEATHER_FOG;
		};

		class Current2: Current
		{
			idc = IDC_XWIZ_FOG_TITLE;
			y = 0.52;
		};

		class FogValueTitle: RscText
		{
			idc = IDC_XWIZ_FOG_TITLE;
			x = 0.25; y = 0.57;
			w = 0.5;
		};

		class FogValue: RscSlider
		{
			idc = IDC_XWIZ_FOG_VALUE;
			x = 0.25; y = 0.57;
			w = 0.5;
		};

		class Forecast2: Forecast
		{
			idc = IDC_XWIZ_FOG_FORECAST_TITLE;
			y = 0.62;
		};

		class ForecastFogValueTitle: RscText
		{
			idc = IDC_XWIZ_FOG_FORECAST_TITLE;
			x = 0.25; y = 0.67;
			w = 0.5;
		};

		class ForecastFogValue: RscSlider
		{
			idc = IDC_XWIZ_FOG_FORECAST_VALUE;
			x = 0.25; y = 0.67;
			w = 0.5;
		};

		class B_Cancel: RscActiveText
		{
			idc = IDC_CANCEL;
			x = 0.65; y = 0.9;
			w = 0.15;
			text = $STR_DISP_CANCEL;
		};

		class B_OK: B_Cancel
		{
			idc = IDC_OK;
			x = 0.85;
			default = 1;
			text = $STR_DISP_OK;
		};
	};
};

class RscDisplayXWizardIntelTime: RscStandardDisplay
{
	idd = IDD_XWIZARD_TIME;

	class controls
	{
		class Title: RscTitle {text = $STR_DISP_XBOX_EDITOR_WIZARD_TIME_TITLE;};

		class HourText: RscText
		{
			x = 0.25; y = 0.26;
			w = 0.2; h = 0.05;
			text = $STR_DISP_XBOX_EDITOR_WIZARD_TIME_HOUR;
		};

		class MinuteText: HourText
		{
			y = 0.36;
			text = $STR_DISP_XBOX_EDITOR_WIZARD_TIME_MINUTES;
		};

		class DayText: HourText
		{
			y = 0.46;
			text = $STR_DISP_XBOX_EDITOR_WIZARD_TIME_DAY;
		};

		class MonthText: HourText
		{
			y = 0.56;
			text = $STR_DISP_XBOX_EDITOR_WIZARD_TIME_MONTH;
		};

		class YearText: HourText
		{
			y = 0.66;
			text = $STR_DISP_XBOX_EDITOR_WIZARD_TIME_YEAR;
		};

		class Hour: RscCombo
		{
			idc = IDC_XWIZ_TIME_HOUR;
			x = 0.25; y = 0.3;
			w = 0.3; h = 0.06;
		};

		class Minute: Hour
		{
			idc = IDC_XWIZ_TIME_MINUTE;
			y = 0.4;
		};

		class Day: Hour
		{
			idc = IDC_XWIZ_TIME_DAY;
			y = 0.5;
		};

		class Month: Hour
		{
			idc = IDC_XWIZ_TIME_MONTH;
			y = 0.6;
		};

		class Year: Hour
		{
			idc = IDC_XWIZ_TIME_YEAR;
			y = 0.7;
      from = 1980;
      to = 2020;
		};

		class B_Cancel: RscActiveText
		{
			idc = IDC_CANCEL;
			x = 0.65; y = 0.9;
			w = 0.15;
			text = $STR_DISP_CANCEL;
		};

		class B_OK: B_Cancel
		{
			idc = IDC_OK;
			x = 0.85;
			default = 1;
			text = $STR_DISP_OK;
		};
	};
};

class RscDisplayXWizardUnit: RscStandardDisplay
{
	idd = IDD_XWIZARD_UNIT;

  west = ProcTextWhite;
  east = ProcTextWhite;
  guer = ProcTextWhite;
  civl = ProcTextWhite;

	class controls
	{
		class Title: RscTitle {text = $STR_DISP_XBOX_EDITOR_WIZARD_UNIT_CHOOSE;};

		class UnitText: RscText
		{
			x = 0.038; y = 0.14;
			w = 0.4; h = 0.06;
			text = $STR_DISP_ARCUNIT_VEHICLE;
			style = ST_CENTER;
		};

		class UnitValue: RscListBox
		{
			idc = IDC_XWIZ_UNITS;
			x = 0.05; y = 0.23;
			w = 0.87; h = 0.55;
		};

		class B_Cancel: RscActiveText
		{
			idc = IDC_CANCEL;
			x = 0.65; y = 0.9;
			w = 0.15;
			text = $STR_DISP_CANCEL;
		};

		class B_OK: B_Cancel
		{
			idc = IDC_OK;
			x = 0.85;
			default = 1;
			text = $STR_DISP_OK;
		};
	};
};

class RscDisplayXWizardUnitSelect: RscStandardDisplay
{
	idd = IDD_XWIZARD_UNIT_SELECT;

	class controls
	{
		class Title: RscTitle
		{
	    idc = IDC_XWIZ_UNIT_SELECT_TITLE;
			text = $STR_DISP_XBOX_EDITOR_WIZARD_UNIT_SELECT;
		};

		class UnitText: RscText
		{
			x = 0.038; y = 0.14;
			w = 0.4; h = 0.06;
			text = $STR_DISP_ARCUNIT_VEHICLE;
			style = ST_CENTER;
		};

		class UnitValue: RscListBox
		{
			idc = IDC_XWIZ_UNIT_SELECT;
			x = 0.05; y = 0.23;
			w = 0.87; h = 0.55;
		};

		class B_Cancel: RscActiveText
		{
			idc = IDC_CANCEL;
			x = 0.65; y = 0.9;
			w = 0.15;
			text = $STR_DISP_CANCEL;
		};

		class B_OK: B_Cancel
		{
			idc = IDC_OK;
			x = 0.85;
			default = 1;
			text = $STR_DISP_OK;
		};
	};
};

class RscDisplayXWizardUnitSelectCustom: RscStandardDisplay
{
	idd = IDD_XWIZARD_UNIT_SELECT_CUSTOM;

	class controls
	{
		class Title: RscTitle
		{
      idc = IDC_XWIZ_UNIT_SELECT_CUSTOM_TITLE;
			text = $STR_DISP_XBOX_EDITOR_WIZARD_UNIT_SELECT;
		};

		class TitleClass: RscText
		{
			x = 0.05; y = 0.25;
			w = 0.45; h = 0.05;
			text = $STR_DISP_ARCUNIT_CLASS;
		};

		class ButtonClass: RscCombo
		{
			idc = IDC_XWIZ_UNIT_CLASS;
			x = 0.05; y = 0.3;
			w = 0.45; h = 0.07;
		};

		class TitleType: RscText
		{
			x = 0.05; y = 0.4;
			w = 0.45; h = 0.05;
			text = $STR_DISP_ARCGRP_TYPE;
		};

		class ButtonType: Buttonclass
		{
			idc = IDC_XWIZ_UNIT_TYPE;
			y = 0.45;
		};

		class TitleCount: RscText
		{
			x = 0.05; y = 0.55;
			w = 0.4; h = 0.05;
			text = $STR_DISP_XBOX_EDITOR_WIZARD_UNIT_COUNT;
		};

		class ButtonCount: Buttonclass
		{
			idc = IDC_XWIZ_UNIT_COUNT;
			y = 0.6;
		};

		class B_Cancel: RscActiveText
		{
			idc = IDC_CANCEL;
			x = 0.65; y = 0.9;
			w = 0.15;
			text = $STR_DISP_CANCEL;
		};

		class B_OK: B_Cancel
		{
			idc = IDC_OK;
			x = 0.85;
			default = 1;
			text = $STR_DISP_OK;
		};
	};
};

class RscDisplayXWizardMap: RscStandardDisplay
{
	idd = IDD_XWIZARD_MAP;

	class controlsBackground
	{
		class Map: RscMapControl
		{
			moveOnEdges = true;
			x = -0.088; y = -0.088;
			w = 1.176; h = 1.176;

			class Command
			{
				icon = ProcTextWhite;
				color[] = {0, 0, 0, 1};
				size = 12;
        importance = 1; // not used
        coefMin = 1; // not used
        coefMax = 1; // not used
			};

			class ActiveMarker
			{
				color[] = {0.3, 0.1, 0.9, 1};
				size = 50;
			};

			scaleMin = 0.03;
			scaleMax = 0.32;
			scaleDefault = 0.096;
		};
	};

	class controls
	{
		class Points: RscText
		{
			idc = IDC_XWIZ_MAP_POINTS;
			x = 0; y = 0;
			w = 1; h = 0.06;
			text = $STR_MISSION_WIZARD_POINTS;
		};

		class Briefing: RscHTML
		{
			idc = IDC_BRIEFING;
			x = 0.12; y = 0.07;
			w = 0.8; h = 0.7;
		};
    // Preview
    class B_OK: RscActiveText
    {
      idc = IDC_OK;
      x = 0.50; y = 0.9;
      w = 0.15;
      text = $STR_DISP_ARCMAP_PREVIEW;
    };
	};
};

class RscDisplayXWizardMapInsertUnit: RscStandardDisplay
{
	idd = IDD_XWIZARD_UNIT_SELECT_CUSTOM;

	class controls
	{
		class Title: RscTitle {text = $STR_DISP_XBOX_EDITOR_WIZARD_UNIT_SELECT;};

		class TitleSide: RscText
		{
			x = 0.05; y = 0.23;
			w = 0.45; h = 0.05;
			text = $STR_DISP_ARCUNIT_SIDE;
		};

		class ComboSide: RscCombo
		{
			idc = IDC_XWIZ_UNIT_SIDE;
			x = 0.05; y = 0.28;
			w = 0.45; h = 0.07;
		};

		class TitleClass: RscText
		{
			x = 0.05; y = 0.35;
			w = 0.45; h = 0.05;
			text = $STR_DISP_ARCUNIT_CLASS;
		};

		class Comboclass: ComboSide
		{
			idc = IDC_XWIZ_UNIT_CLASS;
			y = 0.40;
		};

		class TitleType: RscText
		{
			x = 0.05; y = 0.47;
			w = 0.4; h = 0.05;
			text = $STR_DISP_ARCGRP_TYPE;
		};

		class ComboType: ComboSide
		{
			idc = IDC_XWIZ_UNIT_TYPE;
			y = 0.52;
		};

		class TitleCount: RscText
		{
			x = 0.05; y = 0.59;
			w = 0.4; h = 0.05;
			text = $STR_DISP_XBOX_EDITOR_WIZARD_UNIT_COUNT;
		};

		class ComboCount: ComboSide
		{
			idc = IDC_XWIZ_UNIT_COUNT;
			y = 0.64;
		};
	};
};

class RscDisplayXWizardMapInsertWaypoint: RscStandardDisplay
{
	idd = IDD_XWIZARD_WAYPOINT;

	class controls
	{
		class Title: RscTitle {text = $STR_DISP_XBOX_WAYPOINT_TITLE;};

		class TitleType: RscText
		{
			x = 0.1; y = 0.22;
			w = 0.4; h = 0.05;
			text = $STR_DISP_ARCWP_TYPE;
		};

		class ValueType: RscCombo
		{
			idc = IDC_XWIZ_WP_TYPE;
			x = 0.1; y = 0.27;
			w = 0.4; h = 0.07;
		};

		class TitleBehaviour: RscText
		{
			x = 0.1; y = 0.37;
			w = 0.4; h = 0.05;
			text = $STR_DISP_ARCWP_COMBAT;
		};

		class ValueBehaviour: ValueType
		{
			idc = IDC_XWIZ_WP_BEHAVIOUR;
			y = 0.42;
		};

		class TitleFormation: RscText
		{
			x = 0.1; y = 0.52;
			w = 0.4; h = 0.05;
			text = $STR_DISP_ARCWP_FORM;
		};

		class ValueFormation: ValueType
		{
			idc = IDC_XWIZ_WP_FORMATION;
			y = 0.57;
		};

		class TitleCombatMode: RscText
		{
			x = 0.1; y = 0.67;
			w = 0.4; h = 0.05;
			text = $STR_DISP_ARCWP_SEMAPHORE;
		};

		class ValueCombatMode: ValueType
		{
			idc = IDC_XWIZ_WP_COMBAT_MODE;
			y = 0.72;
		};
	};
};


class RscDisplayXWizardParams: RscStandardDisplay
{
  idd = IDD_XWIZARD_PARAMS;

  class controls
  {
    class Title: RscTitle 
    {
      idc = -1;
      text = "Params";
    };

    class LB_ParamsTitles: RscListBox
    {
      idc = IDC_XWIZ_PARAMS_TITLES;
      x = 0.15; y = 0.15;
      w = 0.5; h = 0.4;
    };

    class B_Edit: RscActiveText
    {
      idc = IDC_EDIT;
      x = 0.65; y = 0.9;
      w = 0.15;
      text = $STR_DISP_EDIT;
    };

    class B_OK: B_Edit
    {
      idc = IDC_OK;
      x = 0.85;
      text = $STR_DISP_OK;
      default = 1;
    };
  };
};


class RscDisplayXWizardParameter: RscStandardDisplay
{
  idd = IDD_XWIZARD_PARAMETER;

  class controls
  {
    class Title: RscTitle 
    {
      idc = IDD_XWIZARD_PARAMETER_TITLE;
      text = "";
    };

    class LB_ParamsTitles: RscListBox
    {
      idc = IDC_XWIZ_PARAMS_VALUES;
      x = 0.15; y = 0.15;
      w = 0.3; h = 0.4;
    };

    class B_OK: RscActiveText
    {
      idc = IDC_OK;
      x = 0.45; y = 0.9;
      w = 0.15;
      text = $STR_DISP_OK;
      default = 1;
    };

    class B_CANCEL: B_OK
    {
      idc = IDC_CANCEL;
      x = 0.85;
      default = 1;
      text = $STR_DISP_CANCEL;
    };
  };
};