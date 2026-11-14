REM
REM protectInternal.bat - applies SECUROM protection
REM  by Vilem
REM
REM Note: list of build configurations (and place this is called from) see:  https://dev.bistudio.com/svn/pgm/trunk/BuildServer/BIS/Build/!ServerBuildArmaIB.bat
REM


@echo off
set user=%1
c:\bis\kecal\kecal.exe %user% Protecting %2 (cmdline was %0 %1 %2 %3 %4 %5 %6)

rem Prepare Idea Games key store
b:\Protect\CopyKeyStore c:\bin\SecuROM\Idea_v2.ks

if /I [%2] == [] goto :Arma
if /I [%2] == [Default] goto :Arma
if /I [%2] == [DefaultVersion] goto :Arma
if /I [%2] == [FSMEditor] goto :FSM
if /I [%2] == [Newsreader] goto :News
if /I [%2] == [SuperRelease] goto :SuperRelease
if /I [%2] == [Retail] goto :Retail
if /I [%2] == [Demo] goto :Demo
if /I [%2] == [TestApp] goto :Demo
if /I [%2] == [Preview] goto :Demo
if /I [%2] == [Expansion] goto :Expansion
if /I [%2] == [ExpansionRetail] goto :ExpansionRetail
if /I [%2] == [ExpansionDemo] goto :ExpansionDemo
if /I [%2] == [ExpansionRetailReview] goto :ExpansionRetailReview

if /I [%2] == [HeliSim] goto :HeliSim
if /I [%2] == [HeliSimRetail] goto :HeliSimRetail
if /I [%2] == [TakeOnH] goto :TakeOnH
if /I [%2] == [TakeOnHRetail] goto :TakeOnHRetail
if /I [%2] == [TakeOnHPublicBeta] goto :TakeOnHPublicBeta
if /I [%2] == [TakeOnHdemo] goto :TakeOnHdemo






echo ProtectInternal.bat: Unknown protecting rule!
goto :end

:TakeOnH
echo Protecting internal Expansion version (o:\TakeOnH\TakeOnHInt.exe)
java -jar c:\bin\SecuROM\signedOETKApplet-5.15t-port8002.jar -s "o:\TakeOnH\TakeOnHIntUnprotected.exe" -d "o:\TakeOnH\TakeOnHInt.exe" -p "BI Internal$ArmA 2 Internal" -c "W:\c\Poseidon\lib\Protect\ArmA2_Internal.oet"
goto :end

:TakeOnHRetail
echo Protecting internal Expansion version (o:\TakeOnH\TakeOnH.exe)
java -jar c:\bin\SecuROM\signedOETKApplet-5.15t-port8002.jar -s "o:\TakeOnH\TakeOnHUnprotected.exe" -d "o:\TakeOnH\TakeOnH.exe" -p "Take On Helicopters" -c "w:\C_Branch\Poseidon\HeliSim\lib\Protect\TKOH_withDVD.oet"
goto :end

:TakeOnHdemo
echo Protecting internal Expansion version (o:\TakeOnH\TakeOnH.exe)
java -jar c:\bin\SecuROM\signedOETKApplet-5.15t-port8002.jar -s "o:\TakeOnH_demo\TakeOnHdemoUnprotected.exe" -d "o:\TakeOnH_demo\TakeOnHdemo.exe" -p "Take On Helicopters" -c "w:\C_Branch\Poseidon\HeliSim\lib\Protect\TKOH_withDVD.oet"
goto :end

:TakeOnHPublicBeta
echo Protecting internal Expansion version (o:\TakeOnH\TakeOnHPublicBeta.exe)
java -jar c:\bin\SecuROM\signedOETKApplet-5.15t-port8002.jar -s "o:\TakeOnH\TakeOnHPublicBetaUnprotected.exe" -d "o:\TakeOnH\TakeOnHPublicBeta.exe" -p "BI Internal$ArmA 2 Internal" -c "W:\c\Poseidon\lib\Protect\ArmA2_Internal.oet"
goto :end

:Expansion
echo Protecting internal Expansion version (o:\Arma2_Expansion\ArmA2ExpInt.exe)
java -jar c:\bin\SecuROM\signedOETKApplet-5.15t-port8002.jar -s "o:\Arma2_Expansion\ArmA2ExpIntUnprotected.exe" -d "o:\Arma2_Expansion\ArmA2ExpInt.exe" -p "BI Internal$ArmA 2 Internal" -c "W:\c\Poseidon\lib\Protect\ArmA2_Internal.oet"
goto :end

:ExpansionRetail
echo Protecting internal Expansion version (o:\Arma2_Expansion\ArmA2Exp.exe)
java -jar c:\bin\SecuROM\signedOETKApplet-5.15t-port8002.jar -s "o:\Arma2_Expansion\ArmA2ExpUnprotected.exe" -d "o:\Arma2_Expansion\ArmA2Exp.exe" -p "ArmA2_OA_retail" -c "w:\C_branch\Poseidon\Arrowhead\lib\Protect\ARMA2_OA_Retail.oet"
goto :end

:ExpansionDemo
echo Protecting internal Expansion version (o:\Arma2_Expansion\ArmA2ExpDemo.exe)
java -jar c:\bin\SecuROM\signedOETKApplet-5.15t-port8002.jar -s "o:\Arma2_Expansion\ArmA2ExpDemoUnprotected.exe" -d "o:\Arma2_Expansion\ArmA2ExpDemo.exe" -p "ArmA2_OA" -c "w:\C_branch\Poseidon\Arrowhead\lib\Protect\ARMA2_OA.oet"
goto :end

:ExpansionRetailReview
echo Protecting internal Expansion version (o:\Arma2_Expansion\ArmA2Exp.exe)
java -jar c:\bin\SecuROM\signedOETKApplet-5.15t-port8002.jar -s "o:\Arma2_Expansion\ArmA2ExpUnprotected.exe" -d "o:\Arma2_Expansion\ArmA2Exp.exe" -p "ArmA2_OA" -c "w:\C_branch\Poseidon\Arrowhead\lib\Protect\ARMA2_OA_Retail.oet"
goto :end


:Demo
echo Protecting internal version (o:\arma\ArmA2DemoUnprotected.exe)
java -jar c:\bin\SecuROM\signedOETKApplet-5.15t-port8002.jar -s "o:\arma\ArmA2DemoUnprotected.exe" -d "o:\arma\ArmA2Demo.exe" -p "BI Internal$ArmA 2 Internal" -c "W:\c\Poseidon\lib\Protect\ArmA2_Internal.oet"
goto :end

:Arma
echo Protecting internal version (o:\arma\ArmA2IntUnprotected.exe)
java -jar c:\bin\SecuROM\signedOETKApplet-5.15t-port8002.jar -s "o:\arma\ArmA2IntUnprotected.exe" -d "o:\arma\ArmA2Int.exe" -p "BI Internal$ArmA 2 Internal" -c "W:\c\Poseidon\lib\Protect\ArmA2_Internal.oet"
goto :end

:SuperRelease
:Retail
echo Protecting public version version (o:\arma\ArmA2Unprotected.exe)
rem Protect Game and Server using Idea Games key
java -jar c:\bin\SecuROM\signedOETKApplet-5.15t-port8002.jar -s "o:\arma\ArmA2Unprotected.exe" -d "o:\arma\ArmA2.exe" -p "ArmA 2" -c "W:\c\Poseidon\lib\Protect\ArmA2.oet"
goto :end



:FSM
java -jar c:\bin\SecuROM\signedOETKApplet-5.15t-port8002.jar -s "o:\fsmeditor\FSMEditorUnprotected.exe" -d "o:\fsmeditor\FSMEditor.exe" -p "BI Internal$ArmA 2 Internal" -c "W:\c\Poseidon\lib\Protect\ArmA2_Internal.oet"
goto :end

:News
REM java -jar c:\bin\SecuROM\signedOETKApplet-5.15t-port8002.jar -s "o:\newsreader\NewsReaderUnprotected.exe" -d "o:\newsreader\NewsReader.exe" -p "BI Internal$ArmA 2 Internal" -c "W:\c\Poseidon\lib\Protect\ArmA2_Internal.oet"

goto :end

:end

echo Protection applied...
