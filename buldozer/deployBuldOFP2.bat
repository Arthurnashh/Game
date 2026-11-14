xcopy /i /r /y  p:\buldozer.exe U:\Tools\Buldozer\dist
xcopy /i /r /y  p:\buldozer.map U:\Tools\Buldozer\dist
xcopy /i /r /y  p:\bin U:\Tools\Buldozer\dist\bin

echo Deployed from %COMPUTERNAME% > U:\Tools\Buldozer\dist\Buldozer.txt
echo Deployed by %USERNAME% >> U:\Tools\Buldozer\dist\Buldozer.txt
