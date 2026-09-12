@ECHO OFF
REM tests\_build_tests.bat - build the CLEO test scripts into a game folder.
REM
REM Double-click friendly, like clang-format.bat: it asks for whatever it cannot
REM detect, streams the build progress and waits at the end so a failure stays
REM readable. Pure cmd.exe - no bash, no MSYS/Git-for-Windows needed.
REM
REM What it does - always these three steps, nothing conditional:
REM   1. copies tests\cleo_tests -> <GTA_SA_DIR>\cleo\cleo_tests
REM   2. deletes the *.s there and recompiles every *.txt in place. The output
REM      extension comes from the file's "{$CLEO .ext}" header (default ".s"),
REM      so testMission.txt -> .cm and testModule.txt -> .mod.
REM   3. compiles .cleo_tests_runner.txt -> <GTA_SA_DIR>\cleo\cleo_tests_runner.cs
REM
REM A script only counts as OK when its output exists AND sanny's compile.log
REM reports no error for it (sanny produces a file even when it logs an error).
REM Any failure is loud: a banner, the failing file with sanny's error line, the
REM bad output deleted, and exit code 1.
REM
REM Exit codes: 0 ok, 1 build failed, 2 bad setup (cancelled/gave up).
REM
REM Model ids are numeric MODEL_* constants declared in the test files, because
REM sanny can only resolve "#MODELNAME" when a game directory is configured in
REM its own settings.ini - which this script never touches.

SETLOCAL EnableDelayedExpansion
TITLE CLEO 5 - build test scripts

SET "REPO_TESTS=%~dp0cleo_tests"
SET "REPO_RUNNER=%~dp0.cleo_tests_runner.txt"
SET "GAME_MODE=sa_sbl"
SET "SANNY_ARGS=-o CustomNames::UseCustomVariables 1 --no-splash --mode %GAME_MODE% --compile"

ECHO ============================================================
ECHO   CLEO 5 - build test scripts
ECHO ============================================================
ECHO:

IF NOT EXIST "%REPO_TESTS%\" GOTO :NO_SOURCES
IF NOT EXIST "%REPO_RUNNER%" GOTO :NO_SOURCES

SET "TRIES=0"

REM --- game folder ------------------------------------------------------
REM GTA_SA_DIR is the same variable the .vcxproj builds use for the game
REM folder. It is cleared here (this session only) so a bad value falls
REM through to the prompt instead of failing three times.

:GET_GAME
SET "GAME="
IF DEFINED GTA_SA_DIR SET "GAME=%GTA_SA_DIR%"
SET "GTA_SA_DIR="
IF NOT DEFINED GAME SET /P "GAME=Game folder: "
IF DEFINED GAME SET "GAME=%GAME:"=%"
IF EXIST "%GAME%\cleo\" GOTO :GOT_GAME
ECHO    ERROR: "%GAME%" is not a game folder - no cleo\ subfolder found.
SET /A TRIES+=1
IF %TRIES% GEQ 3 GOTO :GIVE_UP
GOTO :GET_GAME
:GOT_GAME

REM --- sanny ------------------------------------------------------------
REM SANNY may be the folder holding sanny.exe, or sanny.exe itself.

:GET_SANNY
SET "SANY="
IF DEFINED SANNY SET "SANY=%SANNY%"
SET "SANNY="
IF NOT DEFINED SANY SET /P "SANY=Sanny folder: "
IF DEFINED SANY SET "SANY=%SANY:"=%"
SET "SANY_BIN="
IF EXIST "%SANY%\sanny.exe" SET "SANY_BIN=%SANY%\sanny.exe"
IF EXIST "%SANY%" IF NOT EXIST "%SANY%\" SET "SANY_BIN=%SANY%"
IF NOT DEFINED SANY_BIN GOTO :BAD_SANNY
GOTO :GOT_SANNY
:BAD_SANNY
ECHO    ERROR: no sanny.exe found at "%SANY%".
SET /A TRIES+=1
IF %TRIES% GEQ 3 GOTO :GIVE_UP
GOTO :GET_SANNY
:GOT_SANNY

REM --- confirm, then build ----------------------------------------------
ECHO:
ECHO   game folder : %GAME%
ECHO   sanny       : %SANY_BIN%
ECHO:
SET "ANSWER="
SET /P "ANSWER=Deploy the test scripts into this game folder (Y) or select another folder (N)? "
IF /I "%ANSWER%"=="y" GOTO :DO_BUILD
IF /I "%ANSWER%"=="yes" GOTO :DO_BUILD
GOTO :GET_GAME
:DO_BUILD

SET "DST_TESTS=%GAME%\cleo\cleo_tests"
SET "DST_RUNNER=%GAME%\cleo\cleo_tests_runner.cs"
SET "FAILLOG=%TEMP%\cleo_tests_failed.txt"
SET "FILELIST=%TEMP%\cleo_tests_files.txt"
DEL "%FAILLOG%" "%FILELIST%" >NUL 2>&1

REM sanny writes its compile log next to the exe, overwritten per invocation
REM and only on errors.
FOR %%D IN ("%SANY_BIN%") DO SET "SLOG=%%~dpDcompile.log"

REM --- 0. preflight: make sure sanny actually compiles ------------------------
REM A running Sanny Builder hands command line compiles over to itself and
REM compiles nothing, which would otherwise show up as 150 confusing failures.
REM This runs before anything in the game folder is touched.
IF NOT EXIST "%DST_TESTS%\" MD "%DST_TESTS%"
SET "PROBE=%DST_TESTS%\.sanny_probe.txt"
SET "PROBE_OUT=%DST_TESTS%\.sanny_probe.cs"
> "%PROBE%" ECHO {$CLEO .cs}
>>"%PROBE%" ECHO script_name 'probe'
>>"%PROBE%" ECHO terminate_this_custom_script
IF EXIST "%PROBE_OUT%" DEL "%PROBE_OUT%"
"%SANY_BIN%" %SANNY_ARGS% "%PROBE%" "%PROBE_OUT%"
IF EXIST "%PROBE%" DEL "%PROBE%"
IF NOT EXIST "%PROBE_OUT%" GOTO :NO_PROBE
DEL "%PROBE_OUT%" >NUL 2>&1

REM --- 1. copy the sources into the game folder -------------------------------
ECHO:
ECHO ==^> Copying "%REPO_TESTS%" to "%DST_TESTS%"
ROBOCOPY "%REPO_TESTS%" "%DST_TESTS%" /MIR /NFL /NDL /NJH /NJS /NP >NUL
IF ERRORLEVEL 8 GOTO :COPY_FAILED

REM remove stale outputs first (mirrors the old .Compile_All.bat)
DEL /S /Q "%DST_TESTS%\*.s" >NUL 2>&1

REM --- 2. compile every test script in place ----------------------------------
DIR /B /S /A-D "%DST_TESTS%\*.txt" | SORT > "%FILELIST%"
FOR /F %%C IN ('TYPE "%FILELIST%" ^| FIND /C /V ""') DO SET "TOTAL=%%C"
ECHO:
ECHO ==^> Compiling !TOTAL! scripts in place
ECHO:

SET "OK=0"
SET "FAIL=0"
SET "N=0"

FOR /F "usebackq delims=" %%F IN ("%FILELIST%") DO (
    SET /A N+=1
    SET "SRC=%%F"
    SET "REL=!SRC:%DST_TESTS%\=!"
    SET "REL=!REL:\=/!"

    REM [ 1/149] Audio/0AAC.txt  - pad the counter to 3 digits
    SET "PAD="
    IF !N! LSS 100 SET "PAD= "
    IF !N! LSS 10 SET "PAD=  "
    ECHO [!PAD!!N!/!TOTAL!] !REL!

    REM output extension from the "{$CLEO .ext}" header, default ".s"
    SET "EXT=.s"
    SET "HDR="
    FOR /F "delims=" %%L IN ('FINDSTR /I /C:"{$CLEO" "!SRC!"') DO IF NOT DEFINED HDR SET "HDR=%%L"
    IF DEFINED HDR (
        FOR /F "tokens=2" %%E IN ("!HDR!") DO SET "EXT=%%E"
        SET "EXT=!EXT:}=!"
    )
    FOR %%P IN ("!SRC!") DO SET "OUT=%%~dpnP!EXT!"

    REM sanny only writes its compile.log on errors - delete it first so any
    REM new content belongs to this invocation only
    IF EXIST "!SLOG!" DEL "!SLOG!"
    "%SANY_BIN%" %SANNY_ARGS% "!SRC!" "!OUT!"

    SET "ERR="
    IF EXIST "!SLOG!" FOR /F "delims=" %%L IN ('FINDSTR /I /C:"error" "!SLOG!"') DO IF NOT DEFINED ERR SET "ERR=%%L"

    SET "GOOD="
    IF EXIST "!OUT!" IF NOT DEFINED ERR SET "GOOD=1"
    IF DEFINED GOOD (
        SET /A OK+=1
    ) ELSE (
        SET /A FAIL+=1
        REM sanny writes an output file even when it reports an error - never
        REM leave one of those behind for the game to load
        IF EXIST "!OUT!" DEL "!OUT!"
        ECHO       -^> FAILED
        >>"%FAILLOG%" ECHO   !SRC!
        IF DEFINED ERR (
            ECHO          !ERR!
            >>"%FAILLOG%" ECHO          !ERR!
        )
    )
)

REM --- 3. compile the runner --------------------------------------------------
ECHO:
ECHO ==^> Compiling runner to "%DST_RUNNER%"
IF EXIST "%SLOG%" DEL "%SLOG%"
"%SANY_BIN%" %SANNY_ARGS% "%REPO_RUNNER%" "%DST_RUNNER%"
SET "ERR="
IF EXIST "%SLOG%" FOR /F "delims=" %%L IN ('FINDSTR /I /C:"error" "%SLOG%"') DO IF NOT DEFINED ERR SET "ERR=%%L"
SET "RUNNER_OK="
IF EXIST "%DST_RUNNER%" IF NOT DEFINED ERR SET "RUNNER_OK=1"
IF DEFINED RUNNER_OK (
    SET /A OK+=1
) ELSE (
    SET /A FAIL+=1
    IF EXIST "%DST_RUNNER%" DEL "%DST_RUNNER%"
    ECHO       -^> FAILED
    >>"%FAILLOG%" ECHO   %REPO_RUNNER%
    IF DEFINED ERR (
        ECHO          %ERR%
        >>"%FAILLOG%" ECHO          %ERR%
    )
)

REM --- 4. summary -------------------------------------------------------------
ECHO:
IF !FAIL! GTR 0 GOTO :FAILED

ECHO ==^> Summary: !OK! compiled, 0 failed (!TOTAL! scripts + runner)
ECHO:
ECHO Deployed in place: %DST_TESTS%
ECHO Launch gta_sa.exe in %GAME% to run the tests;
ECHO:
DEL "%FILELIST%" >NUL 2>&1
ECHO *** test build finished ***
ECHO:
PAUSE
EXIT /B 0

:FAILED
SET /A TOTAL_FILES=TOTAL+1
ECHO ########################################################################
ECHO #  TEST BUILD FAILED
ECHO #  !FAIL! of !TOTAL_FILES! file(s) did not compile
ECHO ########################################################################
ECHO:
ECHO The failed scripts have been REMOVED from the game folder (their output is
ECHO deleted so the game cannot silently run a miscompiled script). Running the
ECHO suite now would just skip them - fix the sources and build again:
ECHO:
FOR /F "usebackq delims=" %%F IN ("%FAILLOG%") DO ECHO %%F
ECHO:
ECHO Game folder: %DST_TESTS%
FINDSTR /I /C:"model ID" "%FAILLOG%" >NUL 2>&1
IF NOT ERRORLEVEL 1 CALL :MODEL_HINT
ECHO:
ECHO Do not run the test suite until this build is green (see RUNNING_TESTS.md).
ECHO ########################################################################
DEL "%FILELIST%" >NUL 2>&1
ECHO:
PAUSE
EXIT /B 1

:MODEL_HINT
ECHO:
ECHO Some failures are about model ids: sanny can only resolve '#MODELNAME' when a
ECHO game directory is configured in its settings, so the tests use numeric ids
ECHO instead. Declare one per file next to the other constants and use it:
ECHO:
ECHO     const MODEL_LANDSTAL = 400
ECHO     carHandle = SpawnCar(MODEL_LANDSTAL)
EXIT /B

REM ---------------------------------------------------------------- errors ---
:NO_SOURCES
ECHO    ERROR: cleo_tests sources not found next to this script.
ECHO            Expected: "%REPO_TESTS%"
ECHO            and:      "%REPO_RUNNER%"
ECHO:
PAUSE
EXIT /B 2

:NO_PROBE
DEL "%PROBE%" >NUL 2>&1
ECHO    ERROR: sanny did not compile a trivial probe script.
ECHO            Sanny Builder is single-instance: if the Sanny Builder window is
ECHO            open (or a stuck sanny.exe is running) every command line compile
ECHO            is handed over to it and compiles nothing.
ECHO            Close it, or run: taskkill /F /IM sanny.exe
ECHO            Then retry. Nothing in the game folder was modified.
ECHO:
PAUSE
EXIT /B 2

:COPY_FAILED
ECHO    ERROR: copying the test sources into "%DST_TESTS%" failed.
ECHO:
PAUSE
EXIT /B 2

:GIVE_UP
ECHO:
ECHO Giving up. Set GTA_SA_DIR and SANNY, or edit this script.
ECHO:
PAUSE
EXIT /B 2
