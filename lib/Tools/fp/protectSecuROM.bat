rem %1 - SourceDirSR
rem %2 - SourceProductSR
rem %3 - SourceDirServer
rem %4 - SourceProductServer
rem %5 - DestinationProduct
rem %6 - DestinationBuild

rem Empty intermediate directories
del /Q "%1\Protected\*.*"
del /Q "%1\ProtectedUS\*.*"
del /Q "%1\ProtectedUSOnline\*.*"
del /Q "%3\Protected\*.*"

rem Prepare Morphicon key store
b:\Protect\CopyKeyStore c:\bin\SecuROM\Morphicon_v2.ks

rem Protect Game and Server using Morphicon key
c:\bis\dev\run {java -jar c:\bin\SecuROM\signedOETKApplet-5.02-port8002.jar -s "%1\%2.exe" -d "%1\Protected\%2.exe" -p "Armed Assault$Game" -c "W:\c\Poseidon\lib\Protect\Game.oet"} {java -jar c:\bin\SecuROM\signedOETKApplet-5.02-port8002.jar -s "%3\%4.exe" -d "%3\Protected\%4.exe" -p "Armed Assault$Server" -c "W:\c\Poseidon\lib\Protect\Server.oet"}

rem Copy to the final destination
mkdir u:\mapfiles\%5\%6\SecuROM
copy "%1\Protected\%2.exe" "u:\mapfiles\%5\%6\SecuROM\%5.exe"
copy "%3\Protected\%4.exe" "u:\mapfiles\%5\%6\SecuROM\"

rem Prepare Atari key store
b:\Protect\CopyKeyStore c:\bin\SecuROM\Atari_v2.ks

rem Protect Game using Atari key - DVD and Online distribution
c:\bis\dev\run {java -jar c:\bin\SecuROM\signedOETKApplet-5.02-port8002.jar -s "%1\%2.exe" -d "%1\ProtectedUS\%2.exe" -p "Armed Assault$Game" -c "W:\c\Poseidon\lib\Protect\GameUS.oet"} {java -jar c:\bin\SecuROM\signedOETKApplet-5.02-port8002.jar -s "%1\%2.exe" -d "%1\ProtectedUSOnline\%2.exe" -p "Armed Assault$Online" -c "W:\c\Poseidon\lib\Protect\GameUSOnline.oet"}

rem Copy to the final destination
mkdir u:\mapfiles\%5\%6\SecuROMAtari
copy "%1\ProtectedUS\%2.exe" "u:\mapfiles\%5\%6\SecuROMAtari\%5.exe"
mkdir u:\mapfiles\%5\%6\SecuROMAtariOnline
copy "%1\ProtectedUSOnline\%2.exe" "u:\mapfiles\%5\%6\SecuROMAtariOnline\%5.exe"
