/*!
\file
Mission editor functions and classes
*/

#ifdef _MSC_VER
#pragma once
#endif

//#ifndef _MISSION_EDITOR_HPP
//#define _MISSION_EDITOR_HPP

#if _ENABLE_EDITOR2

class ControlsContainer;
class Display;

Display *CreateMissionEditor(ControlsContainer *parent, bool multiplayer = false);

#if _VBS2
Display *CreateMissionEditorRealTime(ControlsContainer *parent);
Display *CreateMissionEditorRealTime(ControlsContainer *parent, RString clsName, RString initSQF, RString importSQF);
#endif

#endif // _ENABLE_EDITOR2

//#endif

