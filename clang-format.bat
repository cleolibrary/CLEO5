@echo off
echo Formatting all C/C++ files using clang-format...

setlocal enabledelayedexpansion


for %%e in (cpp h c hpp) do (
    for /r %%f in (*.%%e) do (
        echo %%f | findstr /i "third-party" >nul
        if !errorlevel! neq 0 (
            clang-format.exe -i --verbose "%%f"
        )
    )
)

echo.
echo Formatting complete!
pause