@REM Compile all .txt files in the current directory to .s files using Sanny Builder 4
@REM Usage: set SANNY="path\to\sanny.exe" && compile.bat

@echo off

for /f "delims=" %%i in ('dir /b /s *.txt') do (
    echo Compiling "%%i"...
    %SANNY% --compile "%%i" "%%~pi%%~ni.s" --no-splash --mode sa_sbl
)

echo Done.
pause
