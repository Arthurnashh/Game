// Poseidon - 3Dfx version wrapper
#include "wpch.hpp"

#if _BULDOZER
bool LandEditor = true;
#else
bool LandEditor = false;
#endif

#if _ENABLE_BULDOZER
#include "world.hpp"
#include "editor.hpp"

static Ref<EditCursor> GEditCursor;
extern RString ClientIP;
#endif

extern bool ObjViewer;

void InitWorld()
{
#if _ENABLE_BULDOZER
  if (LandEditor)
  {
    GEditCursor = new EditCursor(ClientIP);
    GWorld->InitEditor(GLandscape, GEditCursor);
    if (ObjViewer)
    {
      // cursor is unused when acting as an object viewer
      // it would be better to prevent creating EditCursor, but we are unable to predict when to create it and when not
      // we should be deleting the only Ref to the object
      Assert(GEditCursor->RefCounter()==1)
      GEditCursor = NULL;
    }
  }
#endif
}

void DoneWorld()
{
#if _ENABLE_BULDOZER
  if (GEditCursor)
  {
    if (GWorld) GWorld->DeleteVehicle(GEditCursor);
    GEditCursor = NULL;
  }
#endif
}
