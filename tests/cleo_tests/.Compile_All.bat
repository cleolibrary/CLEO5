@REM Compile all .txt files in the current directory to .s files using Sanny Builder 4
@REM Usage: set SANNY="path\to\sanny.exe" && .Compile_All.bat

@echo off
SETLOCAL EnableDelayedExpansion

@REM Delete all .s files in the current directory and subdirectories
for /f "delims=" %%i in ('dir /b /s *.s') do (
    set p=%%i 
    echo Deleting !p:%__CD__%=!
    del "%%i"
)
echo.

@REM Compile all .txt files in the current directory and subdirectories
for /f "delims=" %%i in ('dir /b /s *.txt') do (
    if not "%%~nxi" == "cleo_tester.txt" (
        set p=%%i 
        echo Compiling !p:%__CD__%=!
        
        @REM Parse the file to find {$CLEO .ext} and extract the extension
        set "ext=.s"
        for /f "tokens=*" %%l in ('findstr /i /c:"{$CLEO" "%%i"') do (
            set "line=%%l"
            for /f "tokens=2 delims= " %%e in ("!line!") do (
                set "ext=%%e"
                set "ext=!ext:}=!"
            )
        )

        %SANNY% --compile "%%i" "%%~dpni!ext!" --no-splash --mode sa_sbl    
        if not exist "%%~dpni!ext!" (
            echo ERROR: Failed to build !p:%__CD__%=!
        )        
    )
)
echo.

echo Done.
pause
