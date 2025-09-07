@echo off
echo Formatting all C/C++ files using clang-format...

setlocal enabledelayedexpansion

set "ignore_files="
for /f "delims=" %%i in (./.clang-format-ignore) do (
    set "ignore_files=!ignore_files! %%i"
)

for %%e in (cpp h c hpp) do (
    for /r %%f in (*.%%e) do (
        echo %%f | findstr /i "!ignore_files!" >nul
        if !errorlevel! neq 0 (
            clang-format.exe -i --verbose "%%f"
        )
    )
)

echo.
echo Formatting complete!
pause