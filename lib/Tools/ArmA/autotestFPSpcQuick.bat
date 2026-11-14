@echo off
set errorlvl=%errorlevel%

pushd
md o:\Arma\verHistory\output

O:
cd \arma\

REM -useCB -cpuCount=4
echo "-useCB -cpuCount=4" > o:\arma\verhistory\output\bedar_fps3.rpt
echo Arma2Int.exe -nopause -nosplash -autotest=O:\arma\autotest\autotestFPS.cfg -useCB -cpuCount=4
Arma2Int.exe -nopause -nosplash -autotest=O:\arma\autotest\autotestFPS.cfg -useCB -cpuCount=4
if errorlevel 1 if errorlevel %errorlvl% echo Autotest failed! - autotestFPSpc failed on (-useCB -cpuCount=4) errorlevel: %errorlevel%
if errorlevel %errorlvl% set errorlvl=%errorlevel%

c:\bis\bedar\deriveFPS.pl "C:\Documents and Settings\%username%\Local Settings\Application Data\ArmA 2\debug.log" o:\arma\verhistory\output\bedar_fps3.rpt
c:\bis\bedar\deriveSlowFrames.pl "C:\Documents and Settings\%username%\Local Settings\Application Data\ArmA 2\debug.log" o:\arma\verhistory\output\bedar_SlowFrames.rpt

type o:\arma\verhistory\output\bedar_fps3.rpt > o:\arma\verhistory\output\bedar_fpsPerftest.rpt

popd