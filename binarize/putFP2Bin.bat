@echo off
call dbg.bat %0 copy /y "O:\fp\bin\*.hpp" "b:\Binarize\Bin" 2>nul

copy /y "O:\fp\bin\*.hpp" "b:\Binarize\Bin"
copy /y "O:\fp\bin\*.h" "b:\Binarize\Bin"
copy /y "O:\fp\bin\*.cpp" "b:\Binarize\Bin"
copy /y "O:\fp\bin\*.csv" "b:\Binarize\Bin"

call dbg.bat %0 copy 4 files, errorlevel: %errorlevel% 2>nul