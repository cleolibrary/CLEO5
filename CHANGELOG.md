## 5.0.0

- support for CLEO modules feature https://github.com/sannybuilder/dev/issues/264
- new [Audio](https://github.com/cleolibrary/CLEO5/tree/master/cleo_plugins/Audio) plugin
  - audio related opcodes moved from CLEO core into separated plugin
  - CLEO's audio now obey game's volume settings
  - implemented Doppler effect for 3d audio streams (fast moving sound sources)
  - CLEO's audio now follows game speed changes
  - sound device can be now manually selected in .ini file
  - new opcode **2500 ([is_audio_stream_playing](https://library.sannybuilder.com/#/sa/audio/2500))**
  - new opcode **2501 ([get_audio_stream_duration](https://library.sannybuilder.com/#/sa/audio/2501))**
  - new opcode **2502 ([get_audio_stream_speed](https://library.sannybuilder.com/#/sa/audio/2502))**
  - new opcode **2503 ([set_audio_stream_speed](https://library.sannybuilder.com/#/sa/audio/2503))**
  - new opcode **2504 ([set_audio_stream_volume_with_transition](https://library.sannybuilder.com/#/sa/audio/2504))**
  - new opcode **2505 ([set_audio_stream_speed_with_transition](https://library.sannybuilder.com/#/sa/audio/2505))**
  - new opcode **2506 ([set_audio_stream_source_size](https://library.sannybuilder.com/#/sa/audio/2506))**
  - new opcode **2507 ([get_audio_stream_progress](https://library.sannybuilder.com/#/sa/audio/2507))**
  - new opcode **2508 ([set_audio_stream_progress](https://library.sannybuilder.com/#/sa/audio/2508))**
  - new opcode **2509 ([get_audio_stream_type](https://library.sannybuilder.com/#/sa/audio/2509))**
  - new opcode **250A ([set_audio_stream_type](https://library.sannybuilder.com/#/sa/audio/250A))**
- new [DebugUtils](https://github.com/cleolibrary/CLEO5/tree/master/cleo_plugins/DebugUtils) plugin
  - new opcode **00C3 ([debug_on](https://library.sannybuilder.com/#/sa/debug/00C3))**
  - new opcode **00C4 ([debug_off](https://library.sannybuilder.com/#/sa/debug/00C4))**
  - new opcode **2100 ([breakpoint](https://library.sannybuilder.com/#/sa/debug/2100))**
  - new opcode **2101 ([trace](https://library.sannybuilder.com/#/sa/debug/2101))**
  - new opcode **2102 ([log_to_file](https://library.sannybuilder.com/#/sa/debug/2102))**
  - implemented support of opcodes **0662**, **0663** and **0664** (original Rockstar's script debugging opcodes. See DebugUtils.ini)
- new [FileSystemOperations](https://github.com/cleolibrary/CLEO5/tree/master/cleo_plugins/FileSystemOperations) plugin
  - file related opcodes moved from CLEO core into separated plugin
  - opcode **0A9E ([write_to_file](https://library.sannybuilder.com/#/sa/file/0A9E))** now supports literal numbers and strings
  - fixed bug causing file stream opcodes not working correctly when read-write modes are used
  - fixed buffer overflows in file stream read opcodes
  - added/fixed support of all file stream opcodes in legacy mode (Cleo3)
  - new opcode **2300 ([get_file_position](https://library.sannybuilder.com/#/sa/file/2300))**
  - new opcode **2301 ([read_block_from_file](https://library.sannybuilder.com/#/sa/file/2301))**
  - new opcode **2302 ([write_block_to_file](https://library.sannybuilder.com/#/sa/file/2302))**
  - new opcode **2303 ([resolve_filepath](https://library.sannybuilder.com/#/sa/file/2303))**
  - new opcode **2304 ([get_script_filename](https://library.sannybuilder.com/#/sa/file/2304))**
- new [MemoryOperations](https://github.com/cleolibrary/CLEO5/tree/master/cleo_plugins/MemoryOperations) plugin
  - memory related opcodes moved from CLEO core into separated plugin
  - validation of input and output parameters for all opcodes
  - opcode **0A8C ([write_memory](https://library.sannybuilder.com/#/sa/memory/0A8C))** now supports strings
  - new opcode **2400 ([copy_memory](https://library.sannybuilder.com/#/sa/memory/2400))**
  - new opcode **2401 ([read_memory_with_offset](https://library.sannybuilder.com/#/sa/memory/2401))**
  - new opcode **2402 ([writememory_with_offset](https://library.sannybuilder.com/#/sa/memory/2402))**
  - new opcode **2403 ([forget_memory](https://library.sannybuilder.com/#/sa/memory/2403))**
  - new opcode **2404 ([get_script_struct_just_created](https://library.sannybuilder.com/#/sa/memory/2404))**
  - new opcode **2405 ([is_script_running](https://library.sannybuilder.com/#/sa/memory/2405))**
  - new opcode **2406 ([get_script_struct_from_filename](https://library.sannybuilder.com/#/sa/memory/2406))**
  - new opcode **2407 ([is_memory_equal](https://library.sannybuilder.com/#/sa/memory/2407))**
- new [Text](https://github.com/cleolibrary/CLEO5/tree/master/cleo_plugins/Text) plugin
  - text related opcodes moved from CLEO core into separated plugin
  - new opcode **2600 ([is_text_empty](https://library.sannybuilder.com/#/sa/text/2600))**
  - new opcode **2601 ([is_text_equal](https://library.sannybuilder.com/#/sa/text/2601))**
  - new opcode **2602 ([is_text_in_text](https://library.sannybuilder.com/#/sa/text/2602))**
  - new opcode **2603 ([is_text_prefix](https://library.sannybuilder.com/#/sa/text/2603))**
  - new opcode **2604 ([is_text_suffix](https://library.sannybuilder.com/#/sa/text/2604))**
- new and updated opcodes
  - implemented support for **memory pointer string** arguments for all game's native opcodes
  - **0B1E ([sign_extend](https://library.sannybuilder.com/#/sa/bitwise/0B1E))**
  - **0DD5 ([get_game_platform](https://library.sannybuilder.com/#/sa/CLEO/0DD5))**
  - **2002 ([cleo_return_with](https://library.sannybuilder.com/#/sa/CLEO/2002))**
  - **2003 ([cleo_return_fail](https://library.sannybuilder.com/#/sa/CLEO/2003))**
  - 'argument count' parameter of **0AB1 (cleo_call)** is now optional. `cleo_call @LABEL args 0` can be written as `cleo_call @LABEL`
  - 'argument count' parameter of **0AB2 (cleo_return)** is now optional. `cleo_return 0` can be written as `cleo_return`
  - **cleo_return_\*** opcodes now can pass strings as return arguments
  - SCM functions **(0AB1)** now keep their own GOSUB's call stack
  - fixed bug in **0AD4 ([scan_string](https://library.sannybuilder.com/#/sa/text/2604))** causing data overruns when reading strings longer than target variable
  - fixed result register not being cleared before function call in opcodes **0AA7** and **0AA8**
- changes in file operations
  - file paths can now use 'virtual absolute paths'. Use prefix in file path strings to access predefined locations: 
    - `root:\` for _game root_ directory
    - `userfiles:\` for _game save files_ directory
    - `.\` for _this script file_ directory
    - `cleo:\` for _CLEO_ directory
    - `modules:\` for _CLEO\cleo_modules_ directory
  - rewritten opcode **0A99 (set_current_directory)**. Now it no longer affects internal game state or current directory in other scripts
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
- fixed invalid 7 characters length limit of **0AAA (get_script_struct_named)**

#### SDK AND PLUGINS
- now all opcodes in range **0-7FFF** can be registered by plugins
- plugins moved to _cleo\cleo_plugins_ directory
- new SDK method: CLEO_RegisterCommand
- new SDK method: CLEO_RegisterCallback
- new SDK method: CLEO_GetVarArgCount
- new SDK method: CLEO_PeekIntOpcodeParam
- new SDK method: CLEO_PeekFloatOpcodeParam
- new SDK method: CLEO_PeekPointerToScriptVariable
- new SDK method: CLEO_SkipUnusedVarArgs
- new SDK method: CLEO_ReadParamsFormatted
- new SDK method: CLEO_ReadStringParamWriteBuffer
- new SDK method: CLEO_GetOpcodeParamsArray
- new SDK method: CLEO_GetParamsHandledCount
- new SDK method: CLEO_GetScriptVersion
- new SDK method: CLEO_GetScriptInfoStr
- new SDK method: CLEO_GetScriptFilename
- new SDK method: CLEO_GetScriptWorkDir
- new SDK method: CLEO_SetScriptWorkDir
- new SDK method: CLEO_ResolvePath
- new SDK method: CLEO_ListDirectory
- new SDK method: CLEO_ListDirectoryFree
- new SDK method: CLEO_GetScriptByName
- new SDK method: CLEO_GetScriptByFilename
- new SDK method: CLEO_GetScriptDebugMode
- new SDK method: CLEO_SetScriptDebugMode
- new SDK method: CLEO_Log


#### CLEO internal
- introduced unit test scripts
- project migrated to VS 2022
- configured game debugging settings
- plugins moved into single solution
- configured automatic releases on GitHub
- added setup_env.bat script

#### Special Thanks
- **123nir** for the alpha-testing, troubleshooting and valuable bug reports

## Older
For previous changes, see [CLEO4 changelog](https://github.com/cleolibrary/CLEO4/blob/master/CHANGELOG.md)
