@echo off
copy /y "W:\c_branch\Poseidon\Arrowhead\Drobnosti\Security\DSSignFile\Release\DSSignFile.exe" "b:\Security\"
if errorlevel 1 pause
copy /y "W:\c_branch\Poseidon\Arrowhead\Drobnosti\Security\DSSignFile\Release\DSSignFile.map" "b:\Security\"
if errorlevel 1 pause