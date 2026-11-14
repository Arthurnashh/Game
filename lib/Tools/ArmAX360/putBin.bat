@echo off

echo Generating X360 shaders...
o:
cd o:\ArmA

del /Q o:\ArmA\bin\*.*
call o:\ArmA\X360ShaderGen.exe -nosplash -window
if errorlevel 1 goto fail

md X:\fpX360\bin 2>nul
md X:\fpX360\dta 2>nul
copy "w:\c\Poseidon\Cfg\bin\*.hpp" "X:\fpX360\bin"
copy "w:\c\Poseidon\Cfg\*.hpp" "X:\fpX360\bin"
copy "w:\c\Poseidon\Cfg\bin\*.h" "X:\fpX360\bin"
copy "w:\c\Poseidon\Cfg\bin\config.cpp" "X:\fpX360\bin"
copy "w:\c\Poseidon\Cfg\bin\*.csv" "X:\fpX360\bin"
copy "o:\ArmA\bin\*.shdc" "X:\fpX360\bin"

rem Create the PBO
echo +Create the PBO...
b:\filebank\filebank -exclude b:\FileBank\exclude.lst -property prefix=bin X:\fpX360\bin
b:\security\dsSignFile.exe c:\bis\security\bi.biprivatekey X:\fpX360\bin.pbo
b:\security\dsSignFile.exe c:\bis\security\ArmA2Demo.biprivatekey X:\fpX360\bin.pbo
move X:\fpX360\bin.pbo X:\fpX360\dta
move X:\fpX360\bin.pbo.*.bisign X:\fpX360\dta

REM The following will copy XArmA_Release.[xex,xdb,pdb,map,exe]
xcopy /D /Y /I O:\ArmA\XArmA_Release.??? X:\fpX360

REM The following will copy XArmA_Retail.[xex,xdb,pdb,map,exe]
xcopy /D /Y /I O:\ArmA\XArmA_Retail.??? X:\fpX360
xcopy /D /Y /I "O:\ArmA\XArmA_Release PGInstrument.???" X:\fpX360\PGO
xcopy /D /Y /I "O:\ArmA\XArmA_Retail PGInstrument.???" X:\fpX360\PGO


xcopy /D /Y /I /E O:\ArmA\Keys\*.* X:\fpX360\Keys

xcopy /D /Y /I /E "C:\Program Files\Microsoft Xbox 360 SDK\redist\xbox\$SystemUpdate\*.*" X:\ca\$SystemUpdate

echo Sending pack request...
if [%1]==[] (set targetComputer=%COMPUTERNAME%) else (set targetComputer=%1)
echo ["pack","p:\ofp2\bin","%targetComputer%",false] >> x:\packserver\packbot\requests.lst
goto :end

:fail
echo X360 shader generation FAILED! (sending notify via kecal to: %1)
c:\bis\kecal\kecal.exe %1 (Build by %user%) X360 shader generation failed! (in w:\C\Poseidon\cfg\lib\Tools\ArmAX360\putBin.bat)

:end