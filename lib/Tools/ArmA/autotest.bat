@ECHO OFF
:BEGIN
ECHO Autotesting - UNIT (exedir: %1 exe (w/o ext): %2 folder: %3  regular: %4 buildVer: %5 )...

if [%1]==[] Echo   --default dir: O:\TakeOnH\
if [%1]==[] cd /d O:\TakeOnH\
cd /d %1

set exe=%2
if [%exe%]==[] Echo   --default exe: Arma2Int
if [%exe%]==[] set exe=TakeOnHInt.exe

set folder=%3
if [%folder%]==[] Echo   --default folder: O:\TakeOnH\autotest\
if [%folder%]==[] set folder=O:\TakeOnH\autotest\

%exe%.exe -window -nosplash -mod=O:\TakeOnH -autotest=%folder%autotestUnit_TKOH.cfg

:result

SET ERRLVL=%ERRORLEVEL%

REM SET ERRLVL = %ERRORLEVEL% <- BAD!!! spaces make it not work; news:fbge5u$lee$1@new_server.localdomain
REM ECHO ERRLVL je %ERRLVL%
REM errorlevel 1 - program exited with exit(X) where X > 0 - that means that 1 or more tests failed

if ERRORLEVEL 9999 goto CONFIGNOT
if ERRORLEVEL 1 goto FAILED

REM ECHO all ok.
ECHO %DATE% %TIME% OK >>autotest/at.log
ECHO %DATE% %TIME% All ok. This should not be sent.>autotest/sendthis.msg
GOTO END

:FAILED
ECHO FAILED %ERRORLEVEL% TEST(S)!
ECHO %DATE% %TIME% FAILED: %ERRORLEVEL% TEST(S)!>>autotest/at.log
ECHO %DATE% %TIME% Unit test failed. Errorlevel: %ERRORLEVEL%.>autotest/sendthis.msg
call %folder%reportToNews.bat %2 %4 %5
GOTO END

:CONFIGNOT
ECHO ERROR: Autotest config not found! (verify Poseidon\lib\Tools\arma\autotest.bat)
ECHO %DATE% %TIME% FAILED: NO CONFIG!>>autotest/at.log
ECHO %DATE% %TIME% Unit test failed. Config not found!>autotest/sendthis.msg
call %folder%reportToNews.bat %2 %4 %5

:END
echo Restoring errorlevel from arma execution (was %errorlevel%, will be %errlvl%)...
EXIT /B %ERRLVL%