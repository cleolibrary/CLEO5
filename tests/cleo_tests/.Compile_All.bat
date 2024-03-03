@REM Compile all .txt files in the current directory to .s files using Sanny Builder 4
@REM Usage: set SANNY="path\to\sanny.exe" && .Compile_All.bat

@echo off

@REM Delete all .s files in the current directory and subdirectories
for /f "delims=" %%i in ('dir /b /s *.s') do (
    echo Deleting "%%~ni%%~xi"...
    del "%%i"
)

@REM Compile all .txt files in the current directory and subdirectories
for /f "delims=" %%i in ('dir /b /s *.txt') do (
    echo Compiling "%%~ni%%~xi"...
    %SANNY% --compile "%%i" "%%~dpi%%~ni.s" --no-splash --mode sa_sbl
)

echo Done.
pause
