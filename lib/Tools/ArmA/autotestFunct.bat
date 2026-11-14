@ECHO OFF

REM Parameter 1 - exe file (without .exe extension)

:BEGIN
ECHO Autotesting - FUNCT (exedir:%1 exefile (w/o ext): %2 %3 %4)...
REM regular must be 3rd parameter at most (see reportToNews call)


REM Test if we really have .exe - not working (%1 takes file with letters potentially in other case) + (.exe not sent anymore)
REM if [%1]==[%~d1%~p1%~n1.exe] goto :start
REM if [%1]==[%~p1%~n1.exe] goto :start
REM if [%1]==[%~n1.exe] goto :start

if [%1]==[] Echo   --default dir: o:\arma\
if [%1]==[] cd /d o:\arma\

cd /d %1
set exe=%2

if [%2]==[] set exe=Arma2int

REM echo Executable file tested: %exe%
%exe%.exe -nosplash -autotest=O:\arma\autotest\autotestFunct.cfg -noCB

:result
SET ERRLVL=%ERRORLEVEL%

REM SET ERRLVL = %ERRORLEVEL% <- BAD!!! spaces make it not work; news:fbge5u$lee$1@new_server.localdomain
REM ECHO ERRLVL je %ERRLVL% 

if ERRORLEVEL 9999 goto CONFIGNOT
if ERRORLEVEL 1 goto FAILED 

REM ECHO all ok.
ECHO %DATE% %TIME% OK (funct) [%1 %2 %3 %4 %5 %6]>>autotest/at.log
ECHO %DATE% %TIME% All ok. This should not be sent.>autotest/sendthis.msg
GOTO END

:FAILED
ECHO FAILED %ERRORLEVEL% TEST(S)!
ECHO %DATE% %TIME% FAILED: %ERRORLEVEL% TEST! (funct)  [%1 %2 %3 %4 %5 %6]>>autotest/at.log
ECHO %DATE% %TIME% Functionality test failed. Errorlevel: %ERRORLEVEL%.>autotest/sendthis.msg
call O:\arma\autotest\reportToNews.bat %2 %3 %4
GOTO END

:CONFIGNOT
ECHO ERROR: Autotest config not found! (verify Poseidon\lib\Tools\arma\autotest.bat)
ECHO %DATE% %TIME% FAILED: NO CONFIG! (funct)  [%1 %2 %3 %4 %5 %6]>>autotest/at.log
ECHO %DATE% %TIME% Functionality test failed. Config not found!>autotest/sendthis.msg
call O:\arma\autotest\reportToNews.bat %2 %3 %4


:END
echo Restoring errorlevel from arma execution (was %errorlevel%, will be %errlvl%)...
EXIT /B %ERRLVL%