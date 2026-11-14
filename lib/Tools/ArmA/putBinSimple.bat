@echo off
REM Exe distribution & archiving -  by Vilem
REM

echo PutBinSimple.bat: %1 %2 %3 %4 %5 %6

REM %1 is for name of computer
REM Parameter %2 is SVN revision number
REM Outputpath must be in %3
REM  when parameter %3 is set we are probably putting different version of .exe (demo/test etc.) and dont want it in mapfiles

if [%1]==[] (set identity=%COMPUTERNAME%-%USERNAME%) else (set identity=%1)
if [%2]==[] (set /a revision=0) else (set /a revision=%2)
if [%3]==[] (set distributionDir=X:\fp) else (set distributionDir=%3)
if [%4]==[] (set outputDiroutputExe=o:\ArmA\ArmA2Int) ELSE (set outputDiroutputExe=%4)
if [%5]==[] (set outputExe=ArmA2Int) ELSE (set outputExe=%5)
if [%6]==[] (set outputDir=o:\arma\) ELSE (set outputDir=%6)
if [%7]==[] (set codeSource=w:\c\Poseidon\Cfg\) ELSE (set codeSource=%7)

REM check regular build (identity set to REGULAR, use archiving directory with RegularBuild suffix)
if [%identity%]==[REGULAR] (set revisionDir=u:\mapfiles\%outputExe%RegularBuild\%revision%) ELSE (set revisionDir=u:\mapfiles\%outputExe%\%revision%)
if [%identity%]==[REGULAR] (set identity=%COMPUTERNAME%-%USERNAME%-REGULAR_BUILD) else (set identity=%1)

if [%revision%]==[0] echo ERROR: Bad revision number! (PutBinSimple.bat - 2nd parameter)

set mapdir=u:\mapfiles\%outputExe%\


echo PutBinSimple:  Identification: %identity% 2nd: %2 distributionDir: %distributionDir% outputDiroutputExe: %4 outputExe: %5 outputDir: %6

md %distributionDir%
md %distributionDir%\Dll

REM Copy .dll and other files (protection, joystick etc.)
copy "%outputDir%ijl15.dll"     "%distributionDir%"
REM copy "%outputDir%ifc22.dll"     "%distributionDir%"

copy "%outputDir%Dll\*"     "%distributionDir%\Dll"
copy "%outputDir%lang.ini"     "%distributionDir%"
copy "%outputDir%paul.dll"     "%distributionDir%"
copy "%outputDir%logo-paul.bmp"     "%distributionDir%"
copy "%outputDir%unicows.dll"     "%distributionDir%"	

:start
md %distributionDir%
md %distributionDir%\bin
echo putBinSimple.bat distributionDir is: %distributionDir%

rem CFG and SHDC files copying is most likely not required
copy "%codeSource%bin\*.hpp"      "%distributionDir%\bin"
copy "%codeSource%*.hpp"          "%distributionDir%\bin"
copy "%codeSource%bin\*.h"        "%distributionDir%\bin"
copy "%codeSource%..\lib\dikCodes.h"        "%distributionDir%\bin"
copy "%codeSource%bin\config.cpp" "%distributionDir%\bin"

REM Stringtables .csv depricated
REM copy "%codeSource%bin\*.csv" "%distributionDir%\bin"
copy "%outputDir%bin\*.shdc"  "%distributionDir%\bin"
copy "%outputDiroutputExe%.exe"     "%distributionDir%"
copy "%outputDiroutputExe%.exe.cfg" "%distributionDir%"
copy "%outputDiroutputExe%.map"     "%distributionDir%"
copy "%outputDiroutputExe%.pdb"     "%distributionDir%"

REM <<<Hack-start. Use unprotected exe instead of (autotested) protected ArmA2Int.exe TODO: delete after all team has securom keys
REM set outputDiroutputExeName=ArmA2Int
REM 
REM echo HACK- delete protected .exe: "%distributionDir%\%outputDiroutputExeName%.exe"
REM del /q /f "%distributionDir%\%outputDiroutputExeName%.exe" 2>nul
REM echo HACK- copy "%outputDiroutputExe%Unprotected.exe" to "%distributionDir%"
REM copy "%outputDiroutputExe%Unprotected.exe" "%distributionDir%"
REM 
REM echo HACK- rename "%distributionDir%\%outputDiroutputExeName%Unprotected.exe" to "%outputDiroutputExeName%.exe"
REM ren "%distributionDir%\%outputDiroutputExeName%Unprotected.exe" "%outputDiroutputExeName%.exe" 
REM 
REM Hack-end.>>>

echo Deployed from %identity% > %distributionDir%\ArmA2Int_deploy.txt
echo Deployed at %DATE% %TIME%>> %distributionDir%\ArmA2Int_deploy.txt



echo Archiving map files...
REM --------------------------------------------------
REM - Store to directory named after revision number -
REM --------------------------------------------------
:revisionDirStoring

mkdir "%revisionDir%"
  del "%revisionDir%" /f /q 
mkdir "%revisionDir%\bin"
  del "%revisionDir%\bin" /f /q 
mkdir "%revisionDir%\dta"
  del "%revisionDir%\dta" /f /q 
mkdir "%revisionDir%\dll"
  del "%revisionDir%\dll" /f /q 
  

copy "%outputDir%changeLog.txt" "%revisionDir%\"

REM xcopy "%outputDir%bin\*.*" "%revisionDir%\bin" /q
xcopy  /Y /R /D /I /K "%outputDir%bin.pbo*" %revisionDir%\dta

copy "%outputDiroutputExe%.pdb" "%revisionDir%\%outputExe%.pdb"
copy "%outputDiroutputExe%Unprotected.exe" "%revisionDir%\%outputExe%Unprotected.exe"
copy "%outputDiroutputExe%.exe" "%revisionDir%\%outputExe%.exe"
copy "%outputDiroutputExe%.map" "%revisionDir%\%outputExe%.map"

copy "%outputDir%dll\*" "%revisionDir%\dll\"


REM Creating README.txt
echo Deployed from (identity): %identity% > %revisionDir%\readme.txt
echo Deployed at: %DATE% %TIME%>> %revisionDir%\readme.txt
echo ---->> %revisionDir%\readme.txt                                                                                                                        
echo SVN revision: %revision%>> %revisionDir%\readme.txt
echo distributionDir: %distributionDir%>> %revisionDir%\readme.txt
echo outputExe: %outputExe%>> %revisionDir%\readme.txt
echo outputDir: %outputDir%>> %revisionDir%\readme.txt
echo revisionDir: %revisionDir%>> %revisionDir%\readme.txt
echo - >> %revisionDir%\readme.txt
echo all params: %0 %1 %2 %3 %4 %5 %6 %7 %8 %9>> %revisionDir%\readme.txt

del "%revisionDir%.7z" /f /q
b:\7zip\7za.exe a %revisionDir%.7z "%outputDiroutputExe%.map" "%outputDiroutputExe%.exe" "%outputDiroutputExe%Unprotected.exe" "%outputDiroutputExe%.pdb" "%outputDir%changeLog.txt" "%revisionDir%\dll" "%revisionDir%\dta" "%revisionDir%\readme.txt"

REM remove directory all files are stored in 7Z
RD /S /Q %revisionDir%







:end