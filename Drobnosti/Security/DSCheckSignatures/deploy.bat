@echo off
copy /y "W:\c_branch\Poseidon\Arrowhead\Drobnosti\Security\DSCheckSignatures\Release\DSCheckSignatures.exe" "b:\Security\"
if errorlevel 1 pause
copy /y "W:\c_branch\Poseidon\Arrowhead\Drobnosti\Security\DSCheckSignatures\Release\DSCheckSignatures.map" "b:\Security\"
if errorlevel 1 pause