#include <Es/essencepch.hpp>
#include <El/elementpch.hpp>

#include <Es/Strings/rString.hpp>
#include "svnAutoUpdateClient.h"

#include <conio.h>

int main(int argc, const char *argv[])
{
  SVNAutoUpdaterClient svnAUC;
  //if ( !svnAUC.Init("..\\SvnAutoUpdater\\Distribution\\svnAutoUpdater.exe", "https://of.bistudio.com/svn/dist/Linda", "Linda", "/8wb1CE7k_J$") )
  if ( !svnAUC.Init("..\\SvnAutoUpdater\\Distribution\\svnAutoUpdater.exe", "https://of.bistudio.com/svn/dist/arma2OA", "arma2OA", "Arma2OA-24915472") )
    return EXIT_FAILURE;

  // MAIN LOOP
  do {
    //@{ PipeManager maintanance
    // receive new data
    if ( !svnAUC.Update() ) break;
    // process all commands
    //@}
    Sleep(1000);
  } while (true);

shutDown:
  // press any key...
  printf("Press any key to continue...\n");
  _getch();
  return EXIT_SUCCESS;
}
