@echo off
call dbg.bat %0 Starting copy sequence  2>nul
call dbg.bat %0 (copy /y "W:\C_Branch\Poseidon\HeliSim\binarize\Release\binarize.???" "b:\Binarize_HeliSim\")  2>nul

md b:\binarize_HeliSim 2>nul
md b:\binarize_HeliSim\bin 2>nul

set outputDir=B:\binarize_HeliSim

call dbg.bat %0 (copy /y "W:\C_Branch\Poseidon\HeliSim\binarize\binarize.???" "%outputDir%\Bin")  2>nul

copy /y "W:\C_Branch\Poseidon\HeliSim\binarize\Release\binarize.exe" %outputDir%
if errorlevel 1 exit /b %errorlevel%
call dbg.bat %0 1 2>nul

copy /y "W:\C_Branch\Poseidon\HeliSim\binarize\Release\binarize.map" %outputDir%
if errorlevel 1 exit /b %errorlevel%
call dbg.bat %0 2 2>nul

copy /y "W:\C_Branch\Poseidon\HeliSim\binarize\Release\binarize.pdb" %outputDir%
if errorlevel 1 exit /b %errorlevel%
call dbg.bat %0 3 2>nul

echo Deployed from %COMPUTERNAME% > %outputDir%\_binarize_deploy.txt
echo Deployed by %USERNAME% >> %outputDir%\_binarize_deploy.txt
echo Deployed at %DATE% %TIME%>> %outputDir%\_binarize_deploy.txt


call dbg.bat %0 (copy /y "W:\C_Branch\Poseidon\HeliSim\cfg\Bin\*.???" "%outputDir%\Bin")  2>nul

copy /y "W:\C_Branch\Poseidon\HeliSim\cfg\Bin\*.*" "%outputDir%\Bin"
if errorlevel 1 exit /b %errorlevel%
call dbg.bat %0 4 2>nul

copy /y "W:\C_Branch\Poseidon\HeliSim\cfg\resincl.hpp" "%outputDir%\Bin"
if errorlevel 1 exit /b %errorlevel%
call dbg.bat %0 5 2>nul

copy /y "W:\C_Branch\Poseidon\HeliSim\lib\dikCodes.h" "%outputDir%\Bin"
if errorlevel 1 exit /b %errorlevel%
call dbg.bat %0 6 2>nul

copy /y "W:\C_Branch\Poseidon\HeliSim\lib\languages.hpp" "%outputDir%\Bin"
if errorlevel 1 exit /b %errorlevel%
call dbg.bat %0 Deploy end ...all ok 2>nul

echo Deployed from %COMPUTERNAME% > %outputDir%\bin\_binarizeBin_deploy.txt
echo Deployed by %USERNAME% >> %outputDir%\bin\_binarizeBin_deploy.txt
echo Deployed at %DATE% %TIME%>> %outputDir%\bin\_binarizeBin_deploy.txt





REM copy /y "W:\C_Branch\Poseidon\HeliSim\binarize\Release\binarize.exe" "U:\Tools\Binarize_HeliSim\dist\"
REM if errorlevel 1 exit /b %errorlevel%
REM call dbg.bat %0 4 2>nul
REM copy /y "W:\C_Branch\Poseidon\HeliSim\binarize\Release\binarize.map" "U:\Tools\Binarize_HeliSim\dist\"
REM if errorlevel 1 exit /b %errorlevel%
REM call dbg.bat %0 5 2>nul

REM call dbg.bat %0 (copy /y "O:\fp\bin\*.???" "U:\Tools\Binarize_HeliSim\dist\Bin")  2>nul
REM copy /y "O:\fp\bin\*.hpp" "U:\Tools\Binarize_HeliSim\dist\Bin"
REM if errorlevel 1 exit /b %errorlevel%
REM call dbg.bat %0 6 2>nul
REM
REM copy /y "O:\fp\bin\*.h" "U:\Tools\Binarize_HeliSim\dist\Bin"
REM if errorlevel 1 exit /b %errorlevel%
REM call dbg.bat %0 7 2>nul
REM
REM copy /y "O:\fp\bin\*.cpp" "U:\Tools\Binarize_HeliSim\dist\Bin"
REM if errorlevel 1 exit /b %errorlevel%
REM call dbg.bat %0 8 2>nul
REM
REM copy /y "O:\fp\bin\*.csv" "U:\Tools\Binarize_HeliSim\dist\Bin"
REM if errorlevel 1 exit /b %errorlevel%




