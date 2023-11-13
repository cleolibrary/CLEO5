## 5.0.0

- support for CLEO modules feature https://github.com/sannybuilder/dev/issues/264
- new [DebugUtils](https://github.com/cleolibrary/CLEO5/tree/master/cleo_plugins/DebugUtils) plugin
  - new opcode **00C3 ([debug_on](https://library.sannybuilder.com/#/sa/debug/00C3))**
  - new opcode **00C4 ([debug_off](https://library.sannybuilder.com/#/sa/debug/00C4))**
  - new opcode **2100 ([breakpoint](https://library.sannybuilder.com/#/sa/debug/2100))**
  - new opcode **2101 ([trace](https://library.sannybuilder.com/#/sa/debug/2101))**
  - new opcode **2102 ([log_to_file](https://library.sannybuilder.com/#/sa/debug/2102))**
  - implemented support of opcodes **0662**, **0663** and **0664** (original Rockstar's script debugging opcodes. See DebugUtils.ini)
- new and updated opcodes
  - **0DD5 ([get_game_platform](https://library.sannybuilder.com/#/sa/CLEO/0DD5))**
  - **2000 ([resolve_filepath](https://library.sannybuilder.com/#/sa/CLEO/2000))**
  - **2001 ([get_script_filename](https://library.sannybuilder.com/#/sa/CLEO/2001))**
  - **2002 ([cleo_return_with](https://library.sannybuilder.com/#/sa/CLEO/2002))**
  - **2003 ([cleo_return_false](https://library.sannybuilder.com/#/sa/CLEO/2003))**
  - 'argument count' parameter of **0AB1 (cleo_call)** is now optional. `cleo_call @LABEL args 0` can be written as `cleo_call @LABEL`
  - 'argument count' parameter of **0AB2 (cleo_return)** is now optional. `cleo_return 0` can be written as `cleo_return`
  - opcodes **0AAB**, **0AE4**, **0AE5**, **0AE6**, **0AE7** and **0AE8** moved to the [FileSystemOperations](https://github.com/cleolibrary/CLEO5/tree/master/cleo_plugins/FileSystemOperations) plugin
  - SCM functions **(0AB1)** now keep their own GOSUB's call stack
  - new opcode **0B1E ([sign_extend](https://library.sannybuilder.com/#/sa/bitwise/0B1E))**
- changes in file operations
  - file paths can now use 'virtual absolute paths'. Use prefix in file path strings to access predefined locations: 
    - `root:\` for _game root_ directory
    - `userfiles:\` for _game save files_ directory
    - `.\` for _this script file_ directory
    - `cleo:\` for _CLEO_ directory
    - `modules:\` for _CLEO\cleo_modules_ directory
  - rewritten opcode **0A99 (set_current_directory)**. It no longer affects internal game state and other scripts
- improved error handling
  - more detailed error messages in multiple scenarios
  - some errors now cause the script to pause, instead of crashing the game
- updated included Silent's ASI Loader to version 1.3

### Bug Fixes
- fixed error in **004E (terminate_this_script)** allowing to run multiple missions
- fixed handling of strings longer than 128 characters causing errors in some cases
- fixed error in handling of first string argument in **0AF5 (write_string to_ini_file)**
- fixed resolution dependent aspect ratio of CLEO text in main menu
- fixed clearing mission locals when new CLEO mission is started
- when reading less than 4 bytes with **0A9D (readfile)** now remaining bytes of the target variable are set to zero

#### SDK AND PLUGINS
- now all opcodes in range **0-7FFF** can be registered by plugins
- plugins moved to _cleo\cleo_plugins_ directory
- new SDK method: CLEO_RegisterCallback
- new SDK method: CLEO_GetVarArgCount
- new SDK method: CLEO_SkipUnusedVarArgs
- new SDK method: CLEO_ReadParamsFormatted
- new SDK method: CLEO_GetScriptVersion
- new SDK method: CLEO_GetScriptInfoStr
- new SDK method: CLEO_ResolvePath
- new SDK method: CLEO_GetScriptDebugMode
- new SDK method: CLEO_SetScriptDebugMode
- new SDK method: CLEO_Log

#### CLEO internal
- project migrated to VS 2022
- configured game debugging settings
- plugins moved into single solution
- configured automatic releases on GitHub
- added setup_env.bat script

#### Special Thanks
- **123nir** for the alpha-testing, troubleshooting and valuable bug reports

## Older
For previous changes, see [CLEO4 changelog](https://github.com/cleolibrary/CLEO4/blob/master/CHANGELOG.md)
