# CLEO Library for GTA San Andreas (Windows PC)

CLEO Library is an open-source extensible plugin for the game Grand Theft Auto: San Andreas by Rockstar Games, allowing the use of thousands of unique mods which change or expand the gameplay. You may find more information about CLEO on the official website https://cleo.li

## Installation

An ASI loader is required for CLEO 5 to work. CLEO 5 comes pre-packaged with several popular ASI Loaders ([Silent's ASI Loader](https://cookieplmonster.github.io/mods/gta-sa/#asiloader) and [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader/)).

- If you don't have an ASI loader installed already or are unsure which one to download, download **CLEO with Silent's ASI Loader**.
- If you prefer the Ultimate ASI Loader, download **CLEO with Ultimate ASI Loader**.
- If you have an ASI loader installed already, download the archive that contains **only CLEO 5** (library and plugins).

Unzip the archive to the GTA San Andreas game directory.

CLEO 5 itself doesn't replace any original game file - the ASI Loader replaces `vorbisFile.dll`, so be sure to back it up. Correct installation can be verified by the CLEO text shown in the bottom-left corner of the game's main menu.

New features are provided by user-made scripts (see CLEO Scripts).

## CLEO Scripts

CLEO allows the installation of 'CLEO scripts', which often use the extension `.cs`. These third-party scripts are entirely user-made and are in no way supported by the developers of this library. While CLEO itself should work in a wide range of game installations, individual scripts are known to have their own compatibility restrictions and cannot be guaranteed to work.
CLEO scripts can be found on Grand Theft Auto fansites and modding sites such as:
- https://libertycity.net/files/gta-san-andreas/mods/cleo-scripts/
- https://www.gtainside.com/en/sanandreas/mods-322/
- http://hotmist.ddo.jp/cleomod/index.html
- https://zazmahall.de/CLEO.htm

## CLEO Plugins

CLEO plugins are optional modules that extend CLEO with additional opcodes. They are installed to the `cleo\cleo_plugins\` folder:

- **[SA.Audio.cleo](https://library.sannybuilder.com/#/sa/script/extensions/audio)** - audio playback utilities powered by BASS.dll library
- **[SA.DebugUtils.cleo](https://library.sannybuilder.com/#/sa/script/extensions/debug)** - script debugging utilities
- **[SA.FileSystemOperations.cleo](https://library.sannybuilder.com/#/sa/script/extensions/file)** - file operations
- **[SA.GameEntities.cleo](https://library.sannybuilder.com/#/sa/script/extensions/CLEO)** - cars/peds/objects/pickups related utilities
- **[SA.IniFiles.cleo](https://library.sannybuilder.com/#/sa/script/extensions/ini)** - .ini config files handling
- **[SA.Input.cleo](https://library.sannybuilder.com/#/sa/script/extensions/input)** - keyboard/mouse related utilities
- **[SA.Math.cleo](https://library.sannybuilder.com/#/sa/script/extensions/math)** - additional math operations
- **[SA.MemoryOperations.cleo](https://library.sannybuilder.com/#/sa/script/extensions/memory)** - memory and .dll libraries utilities
- **[SA.Text.cleo](https://library.sannybuilder.com/#/sa/script/extensions/text)** - text processing and screen drawing
- **[NewOpcodes.cleo](https://library.sannybuilder.com/#/sa/script/extensions/NewOpcodes)** - NewOpcodes plugin, bundled from the [NewOpcodes](https://github.com/cleolibrary/NewOpcodes) repo

All plugins are optional, however they may be required by various CLEO scripts.

## Compatibility Mode

CLEO is continually being improved and extended over time. In very rare circumstances, new major releases may break some older scripts. To fix this, CLEO provides a 'compatibility mode' to closely emulate behavior of previous versions and improve stability of old scripts.
- To run a script with maximum compatibility with 'CLEO 4', change the script extension from `.cs` to `.cs4`.
- To run a script with maximum compatibility with 'CLEO 3', change the script extension from `.cs` to `.cs3`.
[Read more about differences in legacy modes](https://github.com/cleolibrary/CLEO5/wiki/CLEO4-Compat-Mode-(.cs4))

## Creating CLEO Scripts

- [CLEO scripting introduction](https://tutorial.sannybuilder.com/)
- [Example CLEO5 scripts](https://github.com/cleolibrary/CLEO5/tree/master/examples)

## Credits

- Seemann - the author and original developer of the CLEO library
- Alien, Deji - the lead developers of CLEO 4
- Miran - the lead developer of CLEO 5
- all contributors to the GitHub project at https://github.com/cleolibrary/CLEO5

Special thanks to:

- Stanislav Golovin (a.k.a. listener) for his great work in exploration of the GTA series.
- NTAuthority and LINK/2012 for additional support with CLEO 4.3.
- mfisto for the alpha-testing of CLEO 4, his support and advices.
- 123nir for the alpha-testing of CLEO 5.0.0, troubleshooting and valuable bug reports.
- Hamal for the beta-testing of CLEO 5.0.0, troubleshooting and valuable bug reports.

The developers are not affiliated with Take 2 Interactive or Rockstar Games.
By using this product or any of the additional products included, you take your own personal responsibility for any negative consequences should they arise.
