@echo off
rem Deploy X-mod config into Xbox working area
xcopy /i /r /y "O:\StatusQuo\X\bin\*.hpp" "x:\bin"
xcopy /i /r /y "O:\StatusQuo\X\bin\*.h" "x:\bin"
xcopy /i /r /y "O:\StatusQuo\X\bin\*.cpp" "x:\bin"
xcopy /i /r /y "O:\StatusQuo\X\bin\*.csv" "x:\bin"
xcopy /i /r /y "O:\StatusQuo\X\bin\*.hlsl" "x:\bin"
