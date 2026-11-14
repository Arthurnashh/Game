@echo off

o:
cd o:\ArmA
call o:\ArmA\ArmA2Int.exe -nosplash -window -GenerateShaders
if errorlevel 1 goto fail

copy "w:\c\Poseidon\Cfg\bin\*.hpp" "X:\fp\bin"
copy "w:\c\Poseidon\Cfg\*.hpp" "X:\fp\bin"
copy "w:\c\Poseidon\Cfg\bin\*.h" "X:\fp\bin"
copy "w:\c\Poseidon\Cfg\bin\config.cpp" "X:\fp\bin"
copy "w:\c\Poseidon\Cfg\bin\*.csv" "X:\fp\bin"
copy "o:\ArmA\bin\*.shdc" "X:\fp\bin"
copy "o:\ArmA\ArmA2Int.exe" "X:\fp\ArmA2Int.exe"
copy "o:\ArmA\ArmA2Int.exe.cfg" "X:\fp\ArmA2Int.exe.cfg"
copy "o:\ArmA\ArmA2Int.map" "X:\fp\ArmA2Int.map"
copy "o:\ArmA\ArmA2Int.pdb" "X:\fp\ArmA2Int.pdb"

if [%1]==[] (set targetComputer=%COMPUTERNAME%) else (set targetComputer=%1)
echo ["pack","p:\ofp2\bin","%targetComputer%",false] >> x:\packserver\packbot\requests.lst

set mapdir=u:\mapfiles\arma2int\
del "%mapdir%recent-30\bin\*.*" /f /q 
rmdir /S /Q "%mapdir%recent-30"
rename "%mapdir%recent-29" "recent-30"
rename "%mapdir%recent-28" "recent-29"
rename "%mapdir%recent-27" "recent-28"
rename "%mapdir%recent-26" "recent-27"
rename "%mapdir%recent-25" "recent-26"
rename "%mapdir%recent-24" "recent-25"
rename "%mapdir%recent-23" "recent-24"
rename "%mapdir%recent-22" "recent-23"
rename "%mapdir%recent-21" "recent-22"
rename "%mapdir%recent-20" "recent-21"
rename "%mapdir%recent-19" "recent-20"
rename "%mapdir%recent-18" "recent-19"
rename "%mapdir%recent-17" "recent-18"
rename "%mapdir%recent-16" "recent-17"
rename "%mapdir%recent-15" "recent-16"
rename "%mapdir%recent-14" "recent-15"
rename "%mapdir%recent-13" "recent-14"
rename "%mapdir%recent-12" "recent-13"
rename "%mapdir%recent-11" "recent-12"
rename "%mapdir%recent-10" "recent-11"
rename "%mapdir%recent-9" "recent-10"
rename "%mapdir%recent-8" "recent-9"
rename "%mapdir%recent-7" "recent-8"
rename "%mapdir%recent-6" "recent-7"
rename "%mapdir%recent-5" "recent-6"
rename "%mapdir%recent-4" "recent-5"
rename "%mapdir%recent-3" "recent-4"
rename "%mapdir%recent-2" "recent-3"
rename "%mapdir%recent-1" "recent-2"
rename "%mapdir%recent" "recent-1"
mkdir "%mapdir%recent"
mkdir "%mapdir%recent\bin"

copy "o:\ArmA\ArmA2Int.pdb" "%mapdir%recent\ArmA2Int.pdb"
copy "o:\ArmA\ArmA2Int.exe" "%mapdir%recent\ArmA2Int.exe"
copy "o:\ArmA\ArmA2Int.exe.cfg" "%mapdir%recent\ArmA2Int.exe.cfg"
copy "o:\ArmA\ArmA2Int.map" "%mapdir%recent\ArmA2Int.map"
xcopy "o:\ArmA\bin\*.*" "%mapdir%recent\bin" /q

echo Deployed from %COMPUTERNAME% > X:\fp\bin\ArmA2Int_deploy.txt
echo Deployed by %USERNAME% >> X:\fp\bin\ArmA2Int_deploy.txt
echo Deployed at %DATE% %TIME%>> X:\fp\bin\ArmA2Int_deploy.txt

goto :EOF

:fail

echo Shader compilation failed


