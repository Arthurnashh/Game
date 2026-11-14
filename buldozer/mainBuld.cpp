// Poseidon - 3Dfx version wrapper
#include "..\lib\wpch.hpp"
#include "..\lib\global.hpp"
#include "..\lib\world.hpp"
#include "..\lib\landscape.hpp"

#include "..\lib\editor.hpp"

bool LandEditor=true;
extern RString ClientIP;

static Ref<EditCursor> cursor;

void InitWorld()
{
	cursor=new EditCursor(ClientIP);
	GWorld->InitEditor(GLandscape,cursor);
}

void DoneWorld()
{
	if (cursor)
	{
		if (GWorld) GWorld->DeleteVehicle(cursor);
		cursor = NULL;
	}
}
