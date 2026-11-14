rem %1 - SourceDir
rem %2 - DestinationProduct
rem %3 - DestinationBuild

rem StarForce Metaboli
mkdir "%1\StarForceMetaboli"
del /Q "%1\StarForceMetaboli\*.*"
C:\StarForce\Protect.exe -user:ArmaMetaboli@K72mjJ30CPVBiD -project "W:\c\Poseidon\lib\protect\ArmA Metaboli.psp" -log "%1\StarForceMetaboli\StarForce.log" -close
mkdir u:\mapfiles\%2\%3\StarForceMetaboli
copy "%1\StarForceMetaboli\*.*" "u:\mapfiles\%2\%3\StarForceMetaboli\"

rem StarForce DVD
mkdir "%1\StarForce"
del /Q "%1\StarForce\*.*"
C:\StarForce\Protect.exe -user:Bohemia001@dslsQWXfspfh -project "W:\c\Poseidon\lib\protect\ArmA DVD.psp" -log "%1\StarForce\StarForce.log" -close
mkdir u:\mapfiles\%2\%3\StarForce
copy "%1\StarForce\*.*" "u:\mapfiles\%2\%3\StarForce\"

rem StarForce ProActive
mkdir "%1\ProActive"
del /Q "%1\ProActive\*.*"
C:\StarForce\Protect.exe -user:Bohemia001@dslsQWXfspfh -project "W:\c\Poseidon\lib\protect\ArmA ProActive.psp" -log "%1\ProActive\StarForce.log" -close
mkdir u:\mapfiles\%2\%3\ProActive
copy "%1\ProActive\*.*" "u:\mapfiles\%2\%3\ProActive\"

