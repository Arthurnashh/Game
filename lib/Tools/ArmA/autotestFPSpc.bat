@echo off
set errorlvl=%errorlevel%

set outputdir=O:\TakeOnH\\verHistory\output
set debuglogdir=C:\Documents and Settings\%username%\Local Settings\Application Data\Take On Helicopters\debug.log
set autotestdir=O:\TakeOnH\\autotest
set exeparams= -nopause -nosplash

md %outputdir%

pushd O:\TakeOnH\
 
REM -useCB -cpuCount=4
echo TakeOnHInt.exe %exeparams% -autotest=%autotestdir%\autotestFunct_TKOH.cfg 
TakeOnHInt.exe %exeparams% -autotest=%autotestdir%\autotestFunct_TKOH.cfg 
if errorlevel 1 if errorlevel %errorlvl% echo TKOH Autotest failed! - autotestFPSpc failed on errorlevel: %errorlevel%
if errorlevel %errorlvl% set errorlvl=%errorlevel%

DEL /Q %outputdir%\bedar_fps0.rpt
c:\bis\bedar\deriveFPS.pl "%debuglogdir%" %outputdir%\bedar_fps0.rpt

type %outputdir%\bedar_fps0.rpt >  %outputdir%\bedar_fpsPerftest.rpt
rem type %outputdir%\bedar_fps1.rpt >> %outputdir%\bedar_fpsPerftest.rpt
rem type %outputdir%\bedar_fps2.rpt >> %outputdir%\bedar_fpsPerftest.rpt
rem type %outputdir%\bedar_fps3.rpt >> %outputdir%\bedar_fpsPerftest.rpt


REM Last line! :)
popd