@echo off
set product=ArmA

set sourceProduct=ArmA
set sourceDir=o:\FPSuperRelease
set source=%sourceDir%\%sourceProduct%

set sourceProductSR=ArmA
set sourceDirSR=o:\ArmASecuROM
set sourceSR=%sourceDirSR%\%sourceProductSR%

set ftpUpload=c:\bin\curl\curl.exe -u robot:emiL- -T
set ftpTarget=ftp://bistudio.adt.cz

o:
cd o:\FP
call %source%.exe -nosplash -window -GenerateShaders

copy "o:\FP\bin\*.hpp" "X:\fp\bin"
copy "o:\FP\bin\*.h" "X:\fp\bin"
copy "o:\FP\bin\config.cpp" "X:\fp\bin"
copy "o:\FP\bin\*.csv" "X:\fp\bin"
copy "o:\FP\bin\*.shdc" "X:\fp\bin"
copy "%source%.exe" "X:\fp\%product%.exe"
copy "%source%.map" "X:\fp\%product%.map"

rem Do not copy HLSL source files (delete the header from X as well)
rem copy "o:\FP\bin\*.hlsl" "X:\fp\bin"
del "X:\fp\bin\psvs.h"

set ssdir=\\new_server\vss\Pgm

rem Checkout the build number file
w:
cd w:\c\Poseidon\lib\Version
"C:\Program Files\Microsoft Visual Studio\Common\VSS\win32\ss.exe" checkOut $/Poseidon/lib/Version/buildNo.h -i-
if errorlevel 1 goto Fail

rem Retrieve and increment build version
b:\buildNo\buildNo buildNo.h
set build=%errorlevel%

rem Checkin the changed build number file
"C:\Program Files\Microsoft Visual Studio\Common\VSS\win32\ss.exe" checkIn $/Poseidon/lib/Version/buildNo.h -i-

echo Deploying ArmA %build%

rem Distribution of unprotected files
mkdir u:\mapfiles\%product%\%build%
mkdir u:\mapfiles\%product%\%build%\SecuROMUnprotected
copy "%source%.pdb" "u:\mapfiles\%product%\%build%\%sourceProduct%.pdb"
copy "%source%.exe" "u:\mapfiles\%product%\%build%\%product%.exe"
copy "%source%.map" "u:\mapfiles\%product%\%build%\%product%.map"
copy "%sourceDir%\ArmA_Server.exe" "u:\mapfiles\%product%\%build%\"
copy "%sourceDir%\ArmA_Server.map" "u:\mapfiles\%product%\%build%\"
copy "%sourceDir%\ArmA_Server.pdb" "u:\mapfiles\%product%\%build%\"
copy "%sourceSR%.pdb" "u:\mapfiles\%product%\%build%\SecuROMUnprotected\%sourceProductSR%.pdb"
copy "%sourceSR%.exe" "u:\mapfiles\%product%\%build%\SecuROMUnprotected\%product%.exe"
copy "%sourceSR%.map" "u:\mapfiles\%product%\%build%\SecuROMUnprotected\%product%.map"

set /p changeFrom= <U:\mapfiles\ArmA\changesFrom.txt
echo Scanning changes from %changeFrom%
rem ChangeLog from the sources
b:\buildNo\patchList.exe -sort -noheaders -from=%changeFrom% w:\c\Poseidon >u:\mapfiles\%product%\%build%\changeLog.txt
rem ChangeLog from the elements and essence libraries
b:\buildNo\patchList.exe -sort -noheaders -from=%changeFrom% w:\c\El >>u:\mapfiles\%product%\%build%\changeLog.txt
b:\buildNo\patchList.exe -sort -noheaders -from=%changeFrom% w:\c\Es >>u:\mapfiles\%product%\%build%\changeLog.txt
rem ChangeLog from the configs
b:\buildNo\patchList.exe -sort -noheaders -from=%changeFrom% o:\Fp\bin >>u:\mapfiles\%product%\%build%\changeLog.txt

rem Protection
c:\bis\dev\run {w:\c\Poseidon\lib\Tools\fp\protectSecuROM.bat %sourceDirSR% %sourceProductSR% %sourceDir% ArmA_Server %product% %build%} {w:\c\Poseidon\lib\Tools\fp\protectStarForce.bat %sourceDir% %product% %build%} 

rem Deploy protected files to the FTP
%ftpUpload% o:\fp\bin.pbo %ftpTarget%/Bin/
%ftpUpload% o:\fp\bin.pbo.bi.bisign %ftpTarget%/Bin/
%ftpUpload% %sourceDir%\StarForce\%sourceProduct%.exe %ftpTarget%/StarForce/
%ftpUpload% %sourceDir%\StarForce\protect.dll %ftpTarget%/StarForce/
%ftpUpload% %sourceDir%\StarForce\protect.drv %ftpTarget%/StarForce/
%ftpUpload% %sourceDir%\StarForce\protect.exe %ftpTarget%/StarForce/
%ftpUpload% %sourceDir%\StarForceMetaboli\%sourceProduct%.exe %ftpTarget%/StarForceMetaboli/
%ftpUpload% %sourceDir%\StarForceMetaboli\protect.dll %ftpTarget%/StarForceMetaboli/
%ftpUpload% %sourceDir%\StarForceMetaboli\protect.exe %ftpTarget%/StarForceMetaboli/
%ftpUpload% %sourceDir%\ProActive\%sourceProduct%.exe %ftpTarget%/ProActive/
%ftpUpload% %sourceDir%\ProActive\protect.dll %ftpTarget%/ProActive/
%ftpUpload% %sourceDir%\ProActive\protect.exe %ftpTarget%/ProActive/
%ftpUpload% %sourceDir%\Protected\ArmA_Server.exe %ftpTarget%/SecuROM/
%ftpUpload% %sourceDirSR%\Protected\%sourceProductSR%.exe %ftpTarget%/SecuROM/
%ftpUpload% %sourceDirSR%\ProtectedUS\%sourceProductSR%.exe %ftpTarget%/SecuROMAtari/
rem %ftpUpload% test.txt %ftpTarget%/SoftWrap/

rem Deploy info
echo Deployed from %COMPUTERNAME% > X:\fp\bin\%product%_deploy.txt
echo Deployed by %USERNAME% >> X:\fp\bin\%product%_deploy.txt
echo Deployed by %DATE% >> X:\fp\bin\%product%_deploy.txt
echo Build number %build% >> X:\fp\bin\%product%_deploy.txt

exit /b 0

:Fail

echo Distribution failed
exit /b %errorlevel%

