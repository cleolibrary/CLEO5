@REM Compile all .txt files in the current directory to .cs files using Sanny Builder 4
@REM Usage: set SANNY="path\to\sanny.exe" && compile.bat

for /f "delims=" %%i in ('dir /b /s *.txt') do %SANNY% --compile "%%i" "%%~pi%%~ni.cs" --no-splash --mode sa_sbl
