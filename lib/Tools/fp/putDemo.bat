@echo off
set product=ArmADemo
set dstDir=U:\mapfiles\ArmA

set sourceProduct=ArmADemo
set sourceDir=o:\FPDemo
set source=%sourceDir%\%sourceProduct%

o:
cd o:\FP
call o:\FPDemo\ArmADemo.exe -nosplash -window -GenerateShaders

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


mkdir %dstDir%\%build%
mkdir %dstDir%\%build%\SecuROM
copy "%source%.pdb" "%dstDir%\%build%\%sourceProduct%.pdb"
copy "%source%.exe" "%dstDir%\%build%\%product%.exe"
copy "%source%.map" "%dstDir%\%build%\%product%.map"

rem Prepare Morphicon key store
b:\Protect\CopyKeyStore c:\bin\SecuROM\Morphicon_v2.ks

del /Q "%sourceDir%\Protected\*.*"
java -jar c:\bin\SecuROM\signedOETKApplet-5.02-port8002.jar -s "%source%.exe" -d "%sourceDir%\Protected\%sourceProduct%.exe" -p "Armed Assault$Demo" -c "W:\c\Poseidon\lib\Protect\Demo.oet"

copy "%sourceDir%\Protected\%sourceProduct%.exe" "%dstDir%\%build%\SecuROM\%product%.exe"

echo Deployed from %COMPUTERNAME% > X:\fp\bin\%product%_deploy.txt
echo Deployed by %USERNAME% >> X:\fp\bin\%product%_deploy.txt
echo Deployed by %DATE% >> X:\fp\bin\%product%_deploy.txt
echo Build number %build% >> X:\fp\bin\%product%_deploy.txt

exit /b 0


:Fail

echo Build failed
exit /b %errorlevel%
