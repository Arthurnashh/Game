REM execution: autotestFps.bat (NOT)Regular (NOT)Retail|SuperRelease other_arguments_passed_to_arma

@ECHO OFF
:BEGIN
ECHO Autotesting - FPS (%1 %2 %3 %4)...

O:
cd \Arma2_Expansion\


if [%2] == [SuperRelease] goto :SuperRelease
if [%2] == [Retail] goto :Retail

ArmA2ExpInt.exe -nosplash -mod=expansion -autotest=O:\Arma2_Expansion\autotest\autotestFunct_EXP.cfg -noCB %3 %4 %5 %6
goto :result

:Retail
:SuperRelease
ArmA2Exp.exe -nosplash -mod=expansion -autotest=O:\Arma2_Expansion\autotest\autotestFunct_EXP.cfg -noCB %3 %4 %5 %6

:result
SET ERRLVL=%ERRORLEVEL%

REM SET ERRLVL = %ERRORLEVEL% <- BAD!!! spaces make it not work; news:fbge5u$lee$1@new_server.localdomain
REM ECHO ERRLVL je %ERRLVL%

if ERRORLEVEL 9999 goto CONFIGNOT
if ERRORLEVEL 1 goto FAILED

REM ECHO all ok.
ECHO %DATE% %TIME% OK (funct)>>autotest/at.log
ECHO %DATE% %TIME% All ok. This should not be sent.>autotest/sendthis.msg
GOTO END

:FAILED
ECHO FAILED %ERRORLEVEL% TEST(S)!
ECHO %DATE% %TIME% FAILED: %ERRORLEVEL% TEST! (funct)>>autotest/at.log
ECHO %DATE% %TIME% Functionality test failed. Errorlevel: %ERRORLEVEL%.>autotest/sendthis.msg
call O:\Arma2_Expansion\autotest\reportToNews_EXP.bat %1 %2 %3
GOTO END

:CONFIGNOT
ECHO ERROR: Autotest config not found! (verify Poseidon\lib\Tools\arma\autotest.bat)
ECHO %DATE% %TIME% FAILED: NO CONFIG! (funct)>>autotest/at.log
ECHO %DATE% %TIME% Functionality test failed. Config not found!>autotest/sendthis.msg
call O:\Arma2_Expansion\autotest\reportToNews_EXP.bat %1 %2 %3


:END
echo Restoring errorlevel from arma execution (was %errorlevel%, will be %errlvl%)...
EXIT /B %ERRLVL%