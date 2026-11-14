#pragma once
#include <el\evaluator\express.hpp>


class GdPreprocCmd
{
public:
  static void RegisterToGameState(GameState *gState);
};


/*
PreprocCmd

_file="name" preprocFile {... control script ...}

_content=loadTextFile "filename"


Control script -

vstupem je _this a obsahuje jmeno include (v prvním levelu obsahuje "name")
vystupem je textovy soubor nacteny z disku nebo jinak vytvoreny 
lze pouzit loadTextFile.

Control script muze vratit NIL, pak je to nahlaseno jako chyba.
Pokud preprocUsing vrati NIL, pak preprocError vraci obsahuje kod chyby.
*/