#include "commonDefs.hpp"
#include "resincl.hpp"
#include "rscCommon.hpp"

class RscDisplayESRBOnline: RscStandardDisplay
{
	idd = IDD_ESRB;
	
	class Controls
	{
	    class Title: RscTitle
	    {
	      text = $STR_DISP_ESRB_TITLE;
	    };
	    
	    class Text: RscStructuredText
	    {
	      x = 0.025; y = 0.12;
	      w = 0.3;	h = 0.3;
	      text = $STR_DISP_ESRB_ONLINE;
	    };
	
		class B_Cancel: RscButton
		{
			idc = IDC_CANCEL;
			text = $STR_DISP_CANCEL;
			x = 0.55; y = 0.90;
			w = 0.25;
		};
		class B_OK: B_Cancel
		{
			idc = IDC_OK;
			text = $STR_DISP_OK;
			x = 0.75;
			default = 1;
		};
	};	
};

//Main multiplayer screen / server browser.
class RscDisplayMultiplayer: RscStandardDisplay
{
	idd = IDD_MULTIPLAYER;

	sortUp = ProcTextWhite;
	sortDown = ProcTextOrange;
	
	showPassworded = ProcTextGreen;
	hidePassworded = ProcTextBlue;
	
	showFull = ProcTextGray;
	hideFull = ProcTextRed;

  showBattlEye = ProcTextRed;
  showNoBattlEye = ProcTextOrange;
  hideBattlEye = ProcTextGray;

  showExpansions = ProcTextGray;
  hideExpansions = ProcTextRed;

  colorPingUnknown[] = {1, 1, 1, 1};
  colorPingGood[] = {0, 1, 0, 1};
  colorPingPoor[] = {1, 1, 0, 1};
  colorPingBad[] = {1, 0, 0, 1};

  colorVersionGood[] = {1, 1, 1, 1};
  colorVersionBad[] = {1, 0, 0, 1};

	class controls
	{
		class Title: RscTitle 
		{
			idc = IDC_MULTI_TITLE;
			y = 0.01;
		};

		//Sort using the server name column.
		class IconServer: RscPicture
		{
			idc = IDC_MULTI_SERVER_ICON;
			x = 0.0; y = 0.061;
			w = 0.02; h = 0.02;
			text = ProcTextWhite;
		};

		class ColumnServer: RscActiveText
		{
			idc = IDC_MULTI_SERVER_COLUMN;
			text = $STR_MP_SERVER;
			style = ST_LEFT;
			sizeEx = Size_Main_Small;			
			x = 0.0; y = 0.07;
			w = 0.167;
		};

		//Active server name filter.
		class FilterServer: RscText
		{
			idc = IDC_MULTI_SERVER_FILTER;
			style = ST_CENTER;
			x = 0.18; y = 0.125;
			w = 0.2;
		};

		class ColumnType: ColumnServer
		{
			idc = IDC_MULTI_GAMETYPE_COLUMN;
			//Add new to stringtables
			text = $STR_DISP_ARCSENS_TYPE;
			x = 0.167;
		};
		
		class IconType: IconServer
		{
			idc = IDC_MULTI_GAMETYPE_ICON;
			x = 0.91;
		};
		
		class ColumnMission: ColumnServer
		{
			idc = IDC_MULTI_MISSION_COLUMN;
			text = $STR_SECTION_MISSION;
			x = 0.333;
			w = 0.167;
		};

		//Sort using the mission name column.
		class IconMission: IconServer
		{
			idc = IDC_MULTI_MISSION_ICON;
			x = 0.333;
		};

		//Active mission name filter.
		class FilterMission: FilterServer
		{
			idc = IDC_MULTI_MISSION_FILTER;
			x = 0.167;
			w = 0.3;
		};

		//Sort using the server state column.
		class IconState: IconServer
		{
			idc = IDC_MULTI_STATE_ICON;
			x = 0.5;
		};

		class ColumnState: ColumnServer
		{
			idc = IDC_MULTI_STATE_COLUMN;
			text = $STR_MENU_STATUS;
			x = 0.5;
			w = 0.08;
		};

		//Sort using the server friends column.
		class IconFriends: IconServer
		{
			idc = IDC_MULTI_FRIENDS_ICON;
			x = 0.58;
		};

		class ColumnFriends: ColumnServer
		{
			idc = IDC_MULTI_FRIENDS_COLUMN;
			text = $STR_DISP_MULTI_TIMEOFDAY;
			x = 0.58;
			w = 0.04;
		};

		//Sort using the time of day
		class IconTimeOfDay: IconServer
		{
			idc = IDC_MULTI_TIMEOFDAY_ICON;
			x = 0.623;
		};

		class ColumnTimeOfDay: ColumnServer
		{
			idc = IDC_MULTI_TIMEOFDAY_COLUMN;
			text = "Time";
			x = 0.623;
			w = 0.04;
		};

		//Sort using the hive
		class IconHive: IconServer
		{
			idc = IDC_MULTI_HIVE_ICON;
			x = 0.633;
		};

		class ColumnHive: ColumnServer
		{
			idc = IDC_MULTI_HIVE_COLUMN;
			text = "Time";
			x = 0.633;
			w = 0.04;
		};

		//Sort using the mode
		class IconMode: IconServer
		{
			idc = IDC_MULTI_MODE_ICON;
			x = 0.643;
		};

		class ColumnMode: ColumnServer
		{
			idc = IDC_MULTI_MODE_COLUMN;
			text = "Time";
			x = 0.643;
			w = 0.04;
		};

		//Sort using the player column.
		class IconPlayers: IconServer
		{
			idc = IDC_MULTI_PLAYERS_ICON;
			x = 0.667;
		};

		class ColumnPlayers: ColumnServer
		{
			idc = IDC_MULTI_PLAYERS_COLUMN;
			text = $STR_MPTABLE_PLAYERS;
			x = 0.667;
			w = 0.12;
		};

		//Active player filter.
		class FilterPlayers: FilterServer
		{
			idc = IDC_MULTI_PLAYERS_FILTER;
			x = 0.665;
			w = 0.12;
		};

		//Sort using the ping column.
		class IconPing: IconServer
		{
			idc = IDC_MULTI_PING_ICON;
			x = 0.81;
		};

		class ColumnPing: ColumnServer
		{
			idc = IDC_MULTI_PING_COLUMN;
			text = $STR_DISP_MULTI_PING;
			x = 0.83;
			w = 0.08;
		};

		//Active ping filter.
		class FilterPing: FilterServer
		{
			idc = IDC_MULTI_PING_FILTER;
			x = 0.83;
			w = 0.08;
		};
		
		class FilterType: FilterServer
		{
			idc = IDC_MULTI_TYPE_FILTER;
			x = 0.93;
			w = 0.08;
		};
 

		//List of found servers and properties.
		class ValueSessions: RscListBox
		{
			idc = IDC_MULTI_SESSIONS;
			x = 0.0; y = 0.128;
			w = 1; h = 0.373;
			sizeEx = Size_Main_Small;
			rows = 9;
			password = ProcTextWhite;
			version = ProcTextWhite;
      addons = ProcTextRed;
      mods = ProcTextOrange;
      locked = ProcTextPurple;
			none = ProcTextWhite;
			star = ProcTextWhite;
			colorPingUnknown[] = {1, 1, 1, 1};
			colorPingGood[] = {0, 1, 0, 1};
			colorPingPoor[] = {1, 1, 0, 1};
			colorPingBad[] = {1, 0, 0, 1};
			columns[] =
			{
				0.125, // session name
				0.125, // hive
				0.125, // mode 
				0.125, // version
				0.125, // time
				0.125, // friends
				0.125, // players
				0.125  // ping
			};
		};

		//Connect to a remote server using an IP address.
		class B_Remote: RscActiveText
		{
			idc = IDC_MULTI_REMOTE;
			x = 0.167; y = 0.5;
			w = 0.3;
			style = ST_LEFT;
			sizeEx = Size_Main_Small;
			text = $STR_MULTI_REMOTE;
		};

		//Set a server password.
		class B_Password: RscActiveText
		{
			idc = IDC_MULTI_PASSWORD;
			x = 0.333; y = 0.5;
			w = 0.40;
			style = ST_LEFT;
			sizeEx = Size_Main_Small;
		};

		//Refresh the server list.
		class B_Refresh: RscActiveText
		{
			idc = IDC_MULTI_REFRESH;
			text = $STR_DISP_MULTI_REFRESH;
			x = 0.5; y = 0.5;
			w = 0.3;
			style = ST_LEFT;
			sizeEx = Size_Main_Small;
		};

		//Apply filters to the server list.
		class B_Filter: RscActiveText
		{
			idc = IDC_MULTI_FILTER;
			text = $STR_DISP_MULTI_FILTER;
			x = 0.667; y = 0.5;
			w = 0.3;
			style = ST_LEFT;
			sizeEx = Size_Main_Small;
		};

		//Change the used connection port.
		class B_Port: RscActiveText
		{
			idc = IDC_MULTI_PORT;
			text = $STR_DISP_PORT_TITLE;
			x = 0.833; y = 0.5;
			w = 0.3;
			style = ST_LEFT;
			sizeEx = Size_Main_Small;
		};

		//Server search progress bar.
		class Progress: RscProgress
		{
			idc = IDC_MULTI_PROGRESS;
			x = 0.0; y = 0.11;
			w = 1; h = 0.02;
		};

		class ServerDetailHost: RscText
		{
			idc = IDC_MP_HOST;
			x = 0.0;  y = 0.553;
			w = 0.24;
			sizeEx = Size_Main_Small;
		};

		class ServerDetailType: ServerDetailHost
		{
			idc = IDC_MP_TYPE;
			x = 0.25; 
		};	
		class ServerDetailMission: ServerDetailHost
		{
			idc = IDC_MP_MISSION;
			x = 0.5; 
		};		
		class ServerDetailState: ServerDetailHost
		{
			idc = IDC_MP_STATE;
			x = 0.75;
		};		
		class ServerDetailSlots: ServerDetailHost
		{
			idc = IDC_MP_SLOTS_PUBLIC;
			x = 0.0;  y = 0.603;
		};		
		class ServerDetailPing: ServerDetailHost
		{
			idc = IDC_MP_PING;
			x = 0.25;  y = 0.603;
		};		
		class ServerDetailLanguage: ServerDetailHost
		{
			idc = IDC_MP_LANGUAGE;
			x = 0.5; y = 0.603;
		};		
		class ServerDetailCountry: ServerDetailHost
		{
			idc = IDC_MP_COUNTRY;
			x = 0.75; y = 0.603;

		};
		class ServerDetailVersion: ServerDetailHost
		{
			idc = IDC_MP_VERSION;
			x = 0.0; y = 0.653;

		};		
		class ServerDetailVersionRequired: ServerDetailHost
		{
			idc = IDC_MP_VERSION_REQUIRED;
			x = 0.25; y = 0.653;

		};		
		class ServerDetailMods: ServerDetailHost
		{
			idc = IDC_MP_MODS;
			x = 0.5; y = 0.653;
		};		
		class ServerDetailIsland: ServerDetailHost
		{
			idc = IDC_MP_ISLAND;
			x = 0.75; y = 0.653;
		};		
		class ServerDetailDifficulty: ServerDetailHost
		{
			idc = IDC_MP_DIFFICULTY;
			x = 0.0; y = 0.703;
		};		
		class ServerDetailTimeLeft: ServerDetailHost
		{
			idc = IDC_MP_TIMELEFT;
			x = 0.25; y = 0.703;
		};		

		class ServerDetailPlatform: ServerDetailHost
		{
			idc = IDC_MP_PLATFORM;
			x = 0.5; y = 0.703;
		};			
		class ServerDetailPlayers: ServerDetailHost
		{
			idc = IDC_MP_PLAYERS_ROW;
			x = 0.75; y = 0.703;
		};	
		class ServerDetailPassword: RscPicture
		{
			idc = IDC_MP_PASSWORD;
			x = 0.1; y = 0.8;
			w = 0.05; h = 0.05;
		};
		class FilterFullServers: RscPicture
		{
			idc = IDC_MULTI_FULL_FILTER;
			x = 0.1; y = 0.8;
			w = 0.05; h = 0.05;
			text = ProcTextWhite;
		};
		class FilterPasswordedServers: RscPicture
		{
			idc = IDC_MULTI_PASSWORDED_FILTER;
			x = 0.1; y = 0.8;
			w = 0.05; h = 0.05;
			text = ProcTextWhite;
		};
    //Show only servers with BattlEye enabled / all servers
    class FilterBattlEyeServers: RscPicture
    {
      idc = IDC_MULTI_BATTLEYE_FILTER;
      x = 0.1; y = 0.8;
      w = 0.05; h = 0.05;
      text = ProcTextWhite;
    };
		//Cancel to the main menu.
		class B_Cancel: RscActiveText
		{
			idc = IDC_CANCEL;
			text = $STR_DISP_CANCEL;
			x = 0.55; y = 0.95;
			w = 0.15;
		};

		//Join the selected server.
		class B_Join: B_Cancel
		{
			idc = IDC_MULTI_JOIN;
			text = $STR_DISP_MULTI_JOIN;
			x = 0.7;
		};

		//Create a new server.
		class B_New: B_Cancel
		{
			idc = IDC_MULTI_NEW;
			text = $STR_DISP_MULTI_NEW;
			x = 0.85;
			default = 1;
		};
	};
};

class RscDisplayHostSettings : RscStandardDisplay
{
  idd = IDD_HOST_SETTINGS;

  class Controls
  {
    class Title: RscTitle
    {
      text = $STR_DISP_SERVER_TITLE;
    };

    // Session name
    class TextName : RscText
    {
      x = 0.025; y = 0.12;
      w = 0.3;
      text = $STR_DISP_GAME_NAME;
    };
    class ValueName : RscEdit
    {
      idc = IDC_HOST_NAME;
      x = 0.35; y = 0.12;
      w = 0.4;
    };

    // Password
    class TextPassword : RscText
    {
      x = 0.025; y = 0.22;
      w = 0.3;
      text = $STR_DISP_PASSWORD;
    };
    class ValuePassword : RscEdit
    {
      idc = IDC_HOST_PASSWORD;
      x = 0.35; y = 0.22;
      w = 0.4;
    };

    // Max. players
    class TextMaxPlayers : RscText
    {
      x = 0.025; y = 0.32;
      w = 0.3;
      text = $STR_DISP_FILTER_MAXPLAYERS;
    };
    class ValueMaxPlayers : RscEdit
    {
      idc = IDC_HOST_MAX_PLAYERS;
      x = 0.35; y = 0.32;
      w = 0.4;
    };

    // Private / public session (reporting to GameSpy)
    class TextPrivate : RscText
    {
      x = 0.025; y = 0.42;
      w = 0.3;
      text = $STR_DISP_FILTER_SERVER;
    };
    class ValuePrivate : RscListBox
    {
      idc = IDC_HOST_PRIVATE;
      x = 0.35; y = 0.42;
      w = 0.4;
    };

    // Port
    class TextPort : RscText
    {
      x = 0.025; y = 0.52;
      w = 0.3;
      text = $STR_DISP_IP_PORT;
    };
    class ValuePort : RscEdit
    {
      idc = IDC_HOST_PORT;
      x = 0.35; y = 0.52;
      w = 0.4;
    };

    // Buttons
    class B_Cancel : RscActiveText
    {
      idc = IDC_CANCEL;
      text = $STR_DISP_CANCEL;
      x = 0.55; y = 0.90;
      w = 0.25;
    };
    class B_OK : B_Cancel
    {
      idc = IDC_OK;
      text = $STR_DISP_OK;
      x = 0.75;
      default = 1;
    };
  };
};

//Mission and map selection on a server.
class RscDisplayRemoteMissions: RscStandardDisplay
{
	idd = IDD_SERVER;

	class controls
	{
		class Title: RscTitle {text = $STR_DISP_SERVER_TITLE;};

		//Select from available maps.
		class TextMaps: RscText
		{
			x = 0.025; y = 0.12;
			w = 0.3;
			text = $STR_DISP_SERVER_ISLAND;
		};
		class ValueMaps: RscListBox
		{
			idc = IDC_SERVER_ISLAND;
			x = 0.025; y = 0.17;
			w = 0.45; h = 0.6;
			rows = 5;
		};

		//Select fro available missions.
		class TextMissions: TextMaps
		{
			x = 0.525;
			text = $STR_DISP_SERVER_MISSION;
		};
		class ValueMissions: ValueMaps
		{
			idc = IDC_SERVER_MISSION;
			x = 0.525;
			rows = 10;
		};

		//Change the difficulty level.
		class B_Difficulty: RscListBox
		{
			idc = IDC_SERVER_DIFF;
			x = 0.05; y = 0.90;
			w = 0.25;
		};

		//Open the editor to edit the selected mission.
		class B_Editor: RscActiveText
		{
			idc = IDC_SERVER_EDITOR;
      x = 0.35; y = 0.90;
      w = 0.25;
			text = $STR_DISP_EDIT;
		};

    //Mission info: overview
    class Overview: RscHTML
    {
      idc = IDC_SERVER_MI_OVERVIEW;
      x = 0.025; y = 0.25;
      w = 0.45; h = 0.4;
    };

    //Mission info: game type
    class GameType: RscText
    {
      idc = IDC_SERVER_MI_GAMETYPE;
      x = 0.025; y = 0.75;
      w = 0.45; h = 0.1;
    };

    //Mission info: max players
    class MaxPlayers: RscText
    {
      idc = IDC_SERVER_MI_MAX_PLAYERS;
      x = 0.025; y = 0.85;
      w = 0.45; h = 0.1;
    };

		class B_Cancel: B_Editor
		{
			idc = IDC_CANCEL;
			text = $STR_DISP_CANCEL;
			x = 0.55;
		};

		class B_OK: B_Editor
		{
			idc = IDC_OK;
			text = $STR_DISP_OK;
			x = 0.75;
			default = 1;
		};
	};
};

//ToDo: finalize this display.
//Special display for people who already voted.
class RscDisplayRemoteMissionVoted: RscStandardDisplay
{
	idd = IDD_SERVER_VOTED;

	class controls
	{
		class TextWait: RscText
		{
			text = "Please wait...";
			x = 0.025; y = 0.4;
			w = 0.95;
		};

		class B_Cancel: RscActiveText
		{
			idc = IDC_CANCEL;
			text = $STR_DISP_CANCEL;
			x = 0.05; y = 0.90;
			w = 0.25;
		};
	};
};

//Create a new server screen.
class RscDisplayServer: RscDisplayRemoteMissions
{
	idd = IDD_SERVER;

  //temporary crazy colours, to see, if implementation works;
  colorEditor[] = {1, 0.2, 0.2, 1};
  colorWizard[] = {0.2, 1, 0.2, 1};
  colorMission[] = {0.2, 0.6, 1, 1};
  colorMissionEditor[]  = {1, 0.2, 1, 1};
  colorMissionWizard[] = {0.2, 1, 1, 1};			

  class controls
	{
		class Title: RscTitle {text = $STR_DISP_SERVER_TITLE;};

		class TextIslands: RscText
		{
			x = 0.025; y = 0.2;
			w = 0.3;
			text = $STR_DISP_SERVER_ISLAND;
		};

		//List of the available maps.
		class ValueIslands: RscListBox
		{
			idc = IDC_SERVER_ISLAND;
			x = 0.025; y = 0.25;
			w = 0.45; h = 0.6;
		};

		class TextMissions: RscText
		{
			x = 0.525; y = 0.2;
			w = 0.3;
			text = $STR_DISP_SERVER_MISSION;
		};

		//List of the available missions.
		class ValueMissions: RscListBox
		{
			idc = IDC_SERVER_MISSION;
			x = 0.525; y = 0.25;
			w = 0.45; h = 0.6;
		};

		//Change the difficulty.
		class B_Difficulty: RscListBox
		{
			idc = IDC_SERVER_DIFF;
			x = 0.05; y = 0.05;
			w = 0.2;
			//text = $STR_DISP_ERROR;
		};

		class TitlePrivateSlots: RscStructuredText
		{
			idc = IDC_SERVER_PRIVATE_SLOTS_TITLE;
			x = 0.2;
			y = 0.665;
			w = 0.5;
			h = 0.05;
			text = $STR_DISP_XBOX_PRIVATE_SLOTS;
			size = SizeNormal;
			class Attributes
			{
				font = FontMAIN;
				shadow = false;
				color = "#000000";
				align = "left";
			};
		};
		class ValuePrivateSlots: TitlePrivateSlots
		{
			idc = IDC_SERVER_PRIVATE_SLOTS_VALUE;
			x = 0.68;//0.643;
		};
		class SliderPrivateSlots: RscSlider
		{
			idc = IDC_SERVER_PRIVATE_SLOTS;
			style = SL_HORZ + SL_TEXTURES;
			x = 0.25;
			y = 0.7;
			w = 0.5;
			h = 0.05;
			class Title
			{
				idc = IDC_SERVER_PRIVATE_SLOTS_TITLE;
				colorBase[] = Color_White ;
				colorActive[] = Color_Orange;
			};
			class Value
			{
				idc = IDC_SERVER_PRIVATE_SLOTS_VALUE;
				colorBase[] = Color_White ;
				colorActive[] = Color_Orange;
			};
		};

		class TitleMaxSlots: RscStructuredText
		{
			idc = IDC_SERVER_MAXIMUM_SLOTS_TITLE;
			x = 0.2;
			y = 0.75;
			w = 0.5;
			h = 0.05;
			text = $STR_DISP_XBOX_MULTI_MAX_SLOTS_DED;
			size = SizeNormal;
			class Attributes
			{
				font = FontMAIN;
				shadow = false;
				color = "#000000";
				align = "left";
			};
		};
		class ValueMaxSlots: TitleMaxSlots
		{
			idc = IDC_SERVER_MAXIMUM_SLOTS_VALUE;
			x = 0.68;//0.643;
		};
		class SliderMaxSlots: RscSlider
		{
			idc = IDC_SERVER_MAXIMUM_SLOTS;
			style = SL_HORZ + SL_TEXTURES;
			x = 0.25;
			y = 0.8;
			w = 0.5;
			h = 0.05;
			class Title
			{
				idc = IDC_SERVER_MAXIMUM_SLOTS_TITLE;
				colorBase[] = Color_White;
				colorActive[] = Color_Orange;
			};
			class Value
			{
				idc = IDC_SERVER_MAXIMUM_SLOTS_VALUE;
				colorBase[] = Color_White;
				colorActive[] = Color_Orange;
			};
		};

		//Edit the mission in the editor.
		class B_Copy: RscActiveText
		{
			idc = IDC_SERVER_COPY;
      x = 0.15; y = 0.0;
      w = 0.2;
			text = $STR_CA_COPY;
		};
				//Edit the mission in the editor.
		class B_Delete: B_Copy
		{
			idc = IDC_SERVER_DELETE;
			x = 0.25;
			text = $STR_DISP_DELETE;
		};
		//Edit the mission in the editor.
		class B_Editor: B_Copy
		{
			idc = IDC_SERVER_EDITOR;
			x = 0.45;
			text = $STR_DISP_EDIT;
		};

		//Cancel to the main menu.
		class B_Cancel: B_Copy
		{
			idc = IDC_CANCEL;
			x = 0.65;
			text = $STR_DISP_CANCEL;
		};

		//Confirm the selected action. (Continue from save)
		class B_OK: B_Copy
		{
			idc = IDC_OK;
			x = 0.85;
			text = $STR_DISP_OK;
			default = 1;
		};

    //Restart mission button.
    class B_Start: B_Copy
    {
      idc = IDC_SERVER_LOAD;
      x = 0.85; y = 0.05;
    };
	};
};

//Client logging on screen.
class RscDisplayClient: RscStandardDisplay
{
	idd = IDD_CLIENT;

	class controls
	{
		class Background: RscText
		{
			idc = IDC_CLIENT_TEXT;
			style = ST_CENTER;
			x = 0; y = 0.08;
			w = 1;
			text = $STR_DISP_CLIENT_TEXT;
		};

    class Progress: RscProgress
    {
      idc = IDC_CLIENT_PROGRESS;
      x = 0; y = 0.91;
      w = 1; h = 0.08;
    };

		class PlayersTitle: RscText
		{
			x = 0.7; y = 0.7;
			w = 0.25;
			text = $STR_DISP_SRVSETUP_PLAYERS;
			style = ST_LEFT + ST_WITH_RECT;
		};

		//List of players in the server.
		class Players: RscListBox
		{
			idc = IDC_CLIENT_PLAYERS;
			x = 0.7; y = 0.75;
			w = 0.25; h = 0.125;
		};
	};
};

//Enter a server password screen.
class RscDisplayPassword: RscStandardDisplay
{
	idd = IDD_PASSWORD;

	class controls
	{
		class Title: RscTitle {text = $STR_DISP_PASSWORD_TITLE;};

		class TextPassword: RscText
		{
			x = 0.25; y = 0.45;
			w = 0.2;
			text = $STR_DISP_PASSWORD;
		};

		//Enter password field.
		class ValuePassword: RscEdit
		{
			idc = IDC_PASSWORD;
			x = 0.43; y = 0.45;
			w = 0.4;
		};

		//Cancel back to the main multiplayer screen.
		class B_Cancel: RscActiveText
		{
			idc = IDC_CANCEL;
			text = $STR_DISP_CANCEL;
			x = 0.55; y = 0.9;
			w = 0.25;
		};

		//Confirm the entered password.
		class B_OK: B_Cancel
		{
			idc = IDC_OK;
			text = $STR_DISP_OK;
			x = 0.75;
			default = 1;
		};
	};
};

//Change port screen.
class RscDisplayPort: RscStandardDisplay
{
	idd = IDD_PORT;

	class controls
	{
		class Title: RscTitle {text = $STR_DISP_PORT_TITLE;};

		class TextPort: RscText
		{
			x = 0.25; y = 0.45;
			w = 0.2;
			text = $STR_DISP_IP_PORT;
		};

		//Enter a port number field.
		class ValuePort: RscEdit
		{
			idc = IDC_PORT_PORT;
			x = 0.43; y = 0.45;
			w = 0.4;
		};

		//Cancel to the main multiplayer screen.
		class B_Cancel: RscActiveText
		{
			idc = IDC_CANCEL;
			x = 0.55; y = 0.9;
			w = 0.15;
			text = $STR_DISP_CANCEL;
		};

		//Confirm the entered port number.
		class B_OK: B_Cancel
		{
			idc = IDC_OK;
			x = 0.75;
			text = $STR_DISP_OK;
			default = 1;
		};
	};
};

//Connect to a remote server screen.
class RscDisplayIPAddress: RscStandardDisplay
{
	idd = IDD_IP_ADDRESS;

	class controls
	{
		class Title: RscTitle {text = $STR_DISP_IP_TITLE;};

	  class TextAddress: RscText
		{
			x = 0.25; y = 0.45;
			w = 0.2;
			text = $STR_DISP_IP_ADDRESS;
		};

		//Enter IP address field.
		class ValueAddress: RscEdit
		{
			idc = IDC_IP_ADDRESS;
			x = 0.43; y = 0.45;
			w = 0.4;
		};

		class TextPort: RscText
		{
			x = 0.25; y = 0.5;
			w = 0.2;
			text = $STR_DISP_IP_PORT;
		};

		//Enter port number field.
		class ValuePort: RscEdit
		{
			idc = IDC_IP_PORT;
			x = 0.43; y = 0.5;
			w = 0.4;
		};

		//Cancel back to the main multiplayer screen.
		class B_Cancel: RscActiveText
		{
			idc = IDC_CANCEL;
			x = 0.55; y = 0.9;
			w = 0.15;
			text = $STR_DISP_CANCEL;
		};

		//Confirm the entered IP address.
		class B_OK: B_Cancel
		{
			idc = IDC_OK;
			x = 0.75;
			text = $STR_DISP_OK;
			default = 1;
		};
	};
};

//Set server filters screen.
class RscDisplayFilter: RscStandardDisplay
{
	idd = IDD_FILTER;

	#define Display_Filter_Y 0.2

	class controls
	{
  	class Title: RscTitle {text = $STR_DISP_FILTER_TITLE;};

		class TextServer: RscText
		{
			x = 0.1; y = Display_Filter_Y;
			w = 0.275;
			text = $STR_DISP_FILTER_SERVER;
		};

		//Todo: re-enable.
		/*class TextMission: TextServer
		{
			y = Display_Filter_Y + 0.05;
			text = $STR_DISP_FILTER_MISSION;
		};*/

		class TextMaxPing: TextServer
		{
			y = Display_Filter_Y + 0.05;
			text = $STR_DISP_FILTER_MAXPING;
		};

		class TextMinPlayers: TextServer
		{
			y = Display_Filter_Y + 0.1;
			text = $STR_DISP_FILTER_MINPLAYERS;
		};

		class TextMaxPlayers: TextServer
		{
			y = Display_Filter_Y + 0.15;
			text = $STR_DISP_FILTER_MAXPLAYERS;
		};

		//Server name filter field.
		class ValueServer: RscEdit
		{
			idc = IDC_FILTER_SERVER;
			x = 0.4; y = Display_Filter_Y;
			w = 0.4;
		};

		//Todo: re-enable.
		//Mission name filter field.
		/*class ValueMission: ValueServer
		{
			idc = IDC_FILTER_MISSION;
			y = Display_Filter_Y + 0.05;
		};*/

		//Maximum ping filter field.
		class ValueMaxPing: ValueServer
		{
			idc = IDC_FILTER_MAXPING;
			y = Display_Filter_Y + 0.05;
		};

		//Minimum amount of players filter field.
		class ValueMinPlayers: ValueServer
		{
			idc = IDC_FILTER_MINPLAYERS;
			y = Display_Filter_Y + 0.1;
		};

		//Maximum amount of players filter field.
		class ValueMaxPlayers: ValueServer
		{
			idc = IDC_FILTER_MAXPLAYERS;
			y = Display_Filter_Y + 0.15;
		};

		//Show or hide full servers.
		class FullServers: RscListBox
		{
			idc = IDC_FILTER_FULL;
			x = 0.15; y = 0.5;
			w = 0.7;
		};

		//Show or hide passworded servers.
		class PasswordedServers: FullServers
		{
			idc = IDC_FILTER_PASSWORDED;
			y = 0.57;
		};

    //Show only servers with BattlEye enabled / all servers
    class BattlEyeServers: FullServers
    {
      idc = IDC_FILTER_BATTLEYE;
      y = 0.64;
    };

    class FilterType: RscListbox
		{
			idc = IDC_FILTER_TYPE;
			x = 0.15; y = 0.5;
			w = 0.7; h = 0.2;

		}; 
  	//Restore all filters to the default setting.
  	class B_Default: RscActiveText
  	{
  		idc = IDC_FILTER_DEFAULT;
	 		x = 0.1; y = 0.9;
	 		w = 0.15;
  		text = $STR_DISP_DEFAULT;
  	};

  	//Cancel back to the main menu.
  	class B_Cancel: B_Default
  	{
  		idc = IDC_CANCEL;
  		text = $STR_DISP_CANCEL;
  		x = 0.55;
  	};

  	//Confirm the filter settings.
  	class B_OK: B_Default
  	{
  		idc = IDC_OK;
  		x = 0.75;
  		text = $STR_DISP_OK;
  		default = 1;
  	};
  };
};

//Player information and statistics screen.
class RscDisplayMPPlayers: RscStandardDisplay
{
	idd = IDD_MP_PLAYERS;

	class controls
	{
		class Title: RscTitle {text = $STR_DISP_MP_PLAYERS_TITLE;};

		class TextMission: RscTextSmall
		{
			x = 0.025; y = 0.12;
			w = 0.12;
			text = $STR_DISP_SRVSETUP_NAME;
		};

		//Mission being played.
		class ValueMission: RscTextSmall
		{
			idc = IDC_MP_PL_MISSION;
			x = 0.145; y = 0.12;
			w = 0.23;
		};

		class TextIsland: RscTextSmall
		{
			x = 0.40; y = 0.12;
			w = 0.1;
			text = $STR_DISP_SRVSETUP_ISLAND;
		};

		//Map being played on.
		class ValueIsland: RscTextSmall
		{
			idc = IDC_MP_PL_ISLAND;
			x = 0.50; y = 0.12;
			w = 0.2;
		};

		class TextTime: RscTextSmall
		{
			x = 0.625; y = 0.12;
			w = 0.1;
			text = $STR_DISP_MP_PL_TIME;
		};

		//Current playing time.
		class ValueTime: RscTextSmall
		{
			idc = IDC_MP_PL_TIME;
			x = 0.725; y = 0.12;
			w = 0.10;
		};

		//Estimated remaining playing time.
		class ValueRest: RscTextSmall
		{
			idc = IDC_MP_PL_REST;
			x = 0.825; y = 0.12;
			w = 0.15;
		};

		class TextPlayers: RscTextSmall
		{
			x = 0.025; y = 0.15;
			w = 0.25;
			text = $STR_DISP_MP_PLAYERS;
		};

		//Available players on the server.
		class ValuePlayers: RscListBox
		{
			idc = IDC_MP_PLAYERS;
			x = 0.025; y = 0.2;
			w = 0.25; h = 0.65;
			rows = 18.5;
		};

		class TextPlayer: RscTextSmall
		{
			x = 0.3; y = 0.15;
			w = 0.15;
			text = $STR_DISP_MP_PL;
		};

		//Selected player.
		class ValuePlayer: RscTextSmall
		{
			idc = IDC_MP_PL;
			x = 0.45; y = 0.15;
			w = 0.525;
		};

		class TextPlayerName: RscTextSmall
		{
			x = 0.325; y = 0.18;
			w = 0.125;
			text = $STR_DISP_MP_PL_NAME;
		};

		//Player's real name.
		class ValuePlayerName: RscTextSmall
		{
			idc = IDC_MP_PL_NAME;
			x = 0.45; y = 0.18;
			w = 0.525;
		};

		class TextPlayerMail: RscTextSmall
		{
			x = 0.325; y = 0.21;
			w = 0.125;
			text = $STR_DISP_MP_PL_MAIL;
		};

		//Player's e-mail address.
		class ValuePlayerMail: RscTextSmall
		{
			idc = IDC_MP_PL_MAIL;
			x = 0.45; y = 0.21;
			w = 0.525;
		};

		class TextPlayerICQ: RscTextSmall
		{
			x = 0.325; y = 0.24;
			w = 0.125;
			text = $STR_DISP_MP_PL_ICQ;
		};

		//Player's ICQ number.
		class ValuePlayerICQ: RscTextSmall
		{
			idc = IDC_MP_PL_ICQ;
			x = 0.45; y = 0.24;
			w = 0.525;
		};

		class TextPlayerRemark: RscTextSmall
		{
			x = 0.325; y = 0.27;
			w = 0.125;
			text = $STR_DISP_MP_PL_REMARK;
		};

		//Player's custom remark.
		class ValuePlayerRemark: RscTextSmall
		{
			idc = IDC_MP_PL_REMARK;
			x = 0.45; y = 0.27;
			w = 0.525; h = 0.2;
			style = ST_MULTI;
			lines = 5;
		};

		class PingTitle: RscTextSmall
		{
			x = 0.325; y = 0.42;
			w = 0.150;
			text = $STR_DISP_MP_PING;
		};

		//Player's minimum ping.
		class PingMinValue: RscTextSmall
		{
			idc = IDC_MP_PL_MINPING;
			x = 0.48; y = 0.42;
			w = 0.05;
		};

		//Player's average ping.
		class PingAvgValue: RscTextSmall
		{
			idc = IDC_MP_PL_AVGPING;
			x = 0.58; y = 0.42;
			w = 0.05;
		};

		//Player's maximum ping.
		class PingMaxValue: RscTextSmall
		{
			idc = IDC_MP_PL_MAXPING;
			x = 0.68; y = 0.42;
			w = 0.05;
		};

		class BandwidthTitle: RscTextSmall
		{
			x = 0.325; y = 0.45;
			w = 0.15;
			text = $STR_DISP_MP_BANDWIDTH;
		};

		//Player's minimum bandwidth.
		class BandwidthMinValue: RscTextSmall
		{
			idc = IDC_MP_PL_MINBAND;
			x = 0.48; y = 0.45;
			w = 0.05;
		};

		//Player's average bandwidth.
		class BandwidthAvgValue: RscTextSmall
		{
			idc = IDC_MP_PL_AVGBAND;
			x = 0.58; y = 0.45;
			w = 0.05;
		};

		//Player's maximum bandwidth.
		class BandwidthMaxValue: RscTextSmall
		{
			idc = IDC_MP_PL_MAXBAND;
			x = 0.68; y = 0.45;
			w = 0.05;
		};

		class DesyncTitle: RscTextSmall
		{
			x = 0.325; y = 0.48;
			w = 0.15;
			text = $STR_DISP_MP_DESYNC;
		};

		//Player's desynchronization.
		class DesyncValue: RscTextSmall
		{
			idc = IDC_MP_PL_DESYNC;
			x = 0.48; y = 0.48;
			w = 0.05;
		};

		//Kick the selected player from the server.
		class B_KickOff: RscActiveText
		{
			idc = IDC_MP_KICKOFF;
			text = $STR_DISP_MP_KICKOFF;
			x = 0.05; y = 0.9;
			w = 0.2;
		};

		//Kick and ban the selected player from the server.
		class B_Ban: B_KickOff
		{
			idc = IDC_MP_BAN;
			text = $STR_DISP_MP_BAN;
			x = 0.3;
		};

		class B_Mute: B_KickOff
		{
			idc = IDC_MP_MUTE;
			text = $STR_DIK_MUTE;
			x = 0.55; y = 0.9;
			w = 0.2;
		};
		//Cancel back to the game.
		class B_Cancel: B_KickOff
		{
			idc = IDC_CANCEL;
			x = 0.75;
			default = 1;
			text = $STR_DISP_CLOSE;
		};
	};
};

//Todo:
class RscMPSetupMessage
{
	access = ReadAndWrite;
	idd = -1;
	movingEnable = true;
	enableSimulation = false;
	enableDisplay = false;

	class Controls
	{
		class Background: RscText
		{
			idc = -1;
			style = "";
			x = 0.130;
			y = 0.38;
			w = 0.755;
			h = 0.33;
			text = "";
			sizeEx = Size_Text_Default;
			colorBackground[] = {0, 0, 0, 1};
			colorText[] = {0, 0, 0, 0};
		};

		class Text
		{
			access = ReadAndWrite;
			type = CT_STRUCTURED_TEXT;
			idc = IDC_MPSETUP_MSG_MESSAGE;
			style = 0;
			x = 0.12;
			y = 0.44;
			w = 0.66;
			h = 0.10;
			size = SizeNormal;
			text = $STR_DISP_ERROR;

			class Attributes
			{
				font = FontMAIN;
				color = #323232;
				align = center;
				shadow = false;
			};
		};

		class Progress
		{
			access = ReadAndWrite;
			type = CT_PROGRESS;
			idc = IDC_MPSETUP_MSG_PROGRESS;
			style = 0;
			x = 0.28;
			y = 0.5;
			w = 0.4;
			h = 0.05;
			colorFrame[] = {0.2, 0.2, 0.2, 1};
			colorBar[] = {0.2, 0.2, 0.2, 1};
		};
		
		class ProgressIcon: RscPicture
		{
          idc = IDC_MPSETUP_MSG_TIME;
          x = 0.28;
          y = 0.6;
          w = 0.05;
          h = 0.05;
          text = ProcTextWhite;
        };
	};
};
class RscDisplayDedicatedServerSettings: RscStandardDisplay
{
	idd = IDD_DEDICATED_SERVER_SETTINGS;
	movingEnable = true;
	colorslider[] = Color_White;
 	colorsliderActive[] = Color_Orange;

	class controls
	{
		class Title: RscTitle
		{
			text = $STR_XBOX_DEDICATED_HOST;
		};

		class TitlePrivateSlots: RscStructuredText
		{
			idc = IDC_DED_SERVER_SLOTS_PRIVATE_TITLE;
			x = 0.2;
			y = 0.35;
			w = 0.5;
			h = 0.05;
			text = $STR_DISP_XBOX_PRIVATE_SLOTS;
			size = SizeNormal;
			class Attributes
			{
				font = FontMAIN;
				shadow = false;
				color = "#000000";
				align = "left";
			};
		};
		class ValuePrivateSlots: TitlePrivateSlots
		{
			idc = IDC_DED_SERVER_SLOTS_PRIVATE_VALUE;
			x = 0.68;//0.643;
		};

		class SliderPrivateSlots: RscSlider
		{
			idc = IDC_DED_SERVER_SLOTS_PRIVATE;
	    	style = SL_HORZ + SL_TEXTURES;
			x = 0.25;
			y = 0.4;
			w = 0.5;
			h = 0.05;
			class Title
			{
				idc = IDC_DED_SERVER_SLOTS_PRIVATE_TITLE;
				colorBase[] = Color_White ;
				colorActive[] = Color_Orange;
			};
			class Value
			{
				idc = IDC_DED_SERVER_SLOTS_PRIVATE_VALUE;
				colorBase[] = Color_White ;
				colorActive[] = Color_Orange;
			};
		};

		class TitleMaxSlots: RscStructuredText
		{
			idc = IDC_DED_SERVER_SLOTS_PUBLIC_TITLE;
			x = 0.2;
			y = 0.55;
			w = 0.5;
			h = 0.05;
			text = $STR_DISP_XBOX_MULTI_MAX_SLOTS_DED;
			size = SizeNormal;
			class Attributes
			{
				font = FontMAIN;
				shadow = false;
				color = "#000000";
				align = "left";
			};
		};
		class ValueMaxSlots: TitleMaxSlots
		{
			idc = IDC_DED_SERVER_SLOTS_PUBLIC_VALUE;
			x = 0.68;//0.643;
		};

		class SliderMaxSlots: RscSlider
		{
			idc = IDC_DED_SERVER_SLOTS_PUBLIC;
			style = SL_HORZ + SL_TEXTURES;
			x = 0.25;
			y = 0.6;
			w = 0.5;
			h = 0.05;
			class Title
			{
				idc = IDC_DED_SERVER_SLOTS_PUBLIC_TITLE;
				colorBase[] = Color_White;
				colorActive[] = Color_Orange;
			};
			class Value
			{
				idc = IDC_DED_SERVER_SLOTS_PUBLIC_VALUE;
				colorBase[] = Color_White;
				colorActive[] = Color_Orange;
			};
		};


		class HintB: RscXKey
		{
			key = KEY_XBOX_B;
			x = 0.4; y = 0.88;
			w = 0.3;
		};
		class HintStart: RscXKey
		{
			key = KEY_XBOX_Start;
			x = 0.4; y = 0.935;
			w = 0.492;
		};
	    class B_Cancel : RscActiveText
	    {
	      idc = IDC_CANCEL;
	      text = $STR_DISP_CANCEL;
	      x = 0.55; y = 0.90;
	      w = 0.25;
	    };
	    class B_OK : B_Cancel
	    {
	      idc = IDC_OK;
	      text = $STR_DISP_OK;
	      x = 0.75;
	      default = 1;
	    };		
	};
};

class RscDisplayDedicatedServer: RscStandardDisplay
{
	idd = IDD_DEDICATED_SERVER;

	class controls
	{
		class Title: RscTitle {text = $STR_XBOX_DEDICATED_HOST;};

		class Reporting: RscListBox
		{
      		idc = IDC_DS_REPORT;
			x = 0.06;
			y = 0.2;
			w = 0.877;
			h = 0.58;
			rowHeight = 0.04;
		};
	};
};

class RscPendingInvitation
{
  textureConnectionQualityPoor=ProcTextOrange;
  textureConnectionQualityBad=ProcTextRed;
  textureDesyncLow=ProcTextOrange;
  textureDesyncHigh=ProcTextRed;
	color[] = {1, 1, 1, 1};
	x = 0.94;
	y = 0.0;
	w = 0.06;
	h = 0.08;
	timeout = 10;
	blinkingPeriod = 2;
};

class RscPendingInvitationInGame
{
  textureConnectionQualityPoor=ProcTextOrange;
  textureConnectionQualityBad=ProcTextRed;
  textureDesyncLow=ProcTextOrange;
  textureDesyncHigh=ProcTextRed;
	color[] = {1, 1, 1, 1};
	x = 0.94;
	y = 0.0;
	w = 0.06;
	h = 0.08;
	timeout = 10;
	blinkingPeriod = 2;
};

