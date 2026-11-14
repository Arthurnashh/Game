del x:\buldozer.exe
copy /Y p:\buldozer.exe x:\buldozer.exe
del x:\buldozer.map
copy /Y p:\buldozer.map x:\buldozer.map
echo Deployed from %COMPUTERNAME% > x:\Buldozer.txt
echo Deployed by %USERNAME% >> x:\Buldozer.txt
