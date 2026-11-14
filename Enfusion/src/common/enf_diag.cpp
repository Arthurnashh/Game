//#include "precomp.h"
#include "common/enf_diag.h"
#include "enf_main.h"

namespace enf
{
#ifdef ENF_ENABLE_DIAG
int			Diag::Values[];
Diag::Mode	Diag::Modes[];

//-------------------------------------------------------------------------------
void Diag::SetupValues(Diag::Mode& mode)
{
	const char* values = mode.ValueNames;
	const char* beg = values;

	if(values == NULL || *values == 0)
	{
		mode.Min = 0;
		mode.Max = 0;
		return;
	}

	int p = 0;
	while(*values)
	{
		if(*values == ',')
		{
			p++;
			beg = values + 1;
			values = beg;
			continue;
		}
		values++;
	}

	mode.Min = 0;
	mode.Max = p;
}

//-------------------------------------------------------------------------------
void Diag::Unregister(uint index)
{
	enf_assert(index < MAX_DIAG_VALUES);

	Modes[index].Name = NULL;
	Modes[index].ParentName = NULL;
	Modes[index].ValueNames = NULL;
	Modes[index].KeyControl = NULL;
	Modes[index].KeyCode = 0;
	Modes[index].Parent = 0;
	Modes[index].Min = 0;
	Modes[index].Max = 0;
	Modes[index].Pressed = false;
}

//-------------------------------------------------------------------------------
void Diag::Register(uint index, const char* keycontrol, const char* name, const char* parent, const char* valuenames, uint color)
{
	enf_assert(index < MAX_DIAG_VALUES);
	enf_assert(name != NULL);

	Modes[index].KeyControl = keycontrol;

	int qualifier = 0;
	int keycode = 0;

	while(keycontrol && *keycontrol)
	{
		if(*keycontrol == '+')
		{
			if(qualifier == 0)
			{
				keycode = 0x6b;
			}
			keycontrol++;
		}
		else if(CStr("lctrl").ncmp(keycontrol, 5) == 0)
		{
			qualifier |= 1;
			keycontrol += 5;
		}
		else if(CStr("rctrl").ncmp(keycontrol, 5) == 0)
		{
			qualifier |= 2;
			keycontrol += 5;
		}
		else if(CStr("lshift").ncmp(keycontrol, 6) == 0)
		{
			qualifier |= 4;
			keycontrol += 6;
		}
		else if(CStr("rshift").ncmp(keycontrol, 6) == 0)
		{
			qualifier |= 8;
			keycontrol += 6;
		}
		else if(CStr("shift").ncmp(keycontrol, 5) == 0)
		{
			qualifier |= 8|4;
			keycontrol += 5;
		}
		else if(CStr("lalt").ncmp(keycontrol, 4) == 0)
		{
			qualifier |= 16;
			keycontrol += 4;
		}
		else if(CStr("ralt").ncmp(keycontrol, 4) == 0)
		{
			qualifier |= 32;
			keycontrol += 4;
		}
		else if(CStr("num").ncmp(keycontrol, 3) == 0 && *(keycontrol+3) >= '0' && *(keycontrol+3) <= '9')
		{
			keycode = *(keycontrol+3) - '0' + SysKey::KP_n;
			keycontrol += 4;
		}
		else if(*keycontrol == 'f' && *(keycontrol+1) >= '1' && *(keycontrol+1) <= '9')
		{
			keycode = *(keycontrol+1) - '1' + SysKey::Fn;
			keycontrol += 2;
		}
		else if(*keycontrol >= 'a' && *keycontrol <= 'z')
		{
			keycode = *keycontrol - 'a' + 'A';
			keycontrol++;
		}
		else if(*keycontrol >= '0' && *keycontrol <= '9')
		{
			keycode = *keycontrol;
			keycontrol++;
		}
		else
			keycontrol++;
	}

	Modes[index].Color = color;
	Modes[index].KeyCode = keycode;
	Modes[index].KeyQualifier = qualifier;
	Modes[index].Name = name;
	Modes[index].ParentName = parent;
	Modes[index].ValueNames = valuenames;
	SetupValues(Modes[index]);
	Values[index] = 0;

	for(uint n = 0; n < MAX_DIAG_VALUES; n++)
	{
		if(n == index) continue;
		
		if(parent && *parent != 0)
		{
			if(CStr(Modes[n].Name) == parent)
				Modes[index].Parent = n;
		}
		if(Modes[n].ParentName != NULL)
		{
			if(CStr(Modes[n].ParentName) == name)
				Modes[n].Parent = index;
		}
	}
}
#endif
}