@echo off
copy /y "W:\c_branch\Poseidon\Arrowhead\Drobnosti\Security\DSCreateKey\Release\DSCreateKey.exe" "b:\Security\"
if errorlevel 1 pause
copy /y "W:\c_branch\Poseidon\Arrowhead\Drobnosti\Security\DSCreateKey\Release\DSCreateKey.map" "b:\Security\"
if errorlevel 1 pause