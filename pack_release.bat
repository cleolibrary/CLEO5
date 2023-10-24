@echo off

SET zip="C:\Program Files\7-Zip\7z.exe"

echo Preparing GTA SA CLEO
FOR /F "USEBACKQ" %%F IN (`powershell -NoLogo -NoProfile -Command ^(Get-Item ".output\Release\CLEO.asi"^).VersionInfo.FileVersion`) DO (SET fileVersion=%%F)
echo Detected version: %fileVersion%
SET outputFile=".\CLEO.SA_v%fileVersion%.zip"
if exist %outputFile% del %outputFile% /q

mkdir pack_tmp
mkdir pack_tmp\cleo
mkdir pack_tmp\cleo\cleo_modules
mkdir pack_tmp\cleo\cleo_plugins
mkdir pack_tmp\cleo\cleo_saves
mkdir pack_tmp\cleo\cleo_text
%zip% a -tzip %outputFile% ".\pack_tmp\*" -r -bso0
rmdir /s /q pack_tmp

%zip% a -tzip %outputFile% ".\.output\Release\CLEO.asi" -bb2 | findstr "+"
%zip% a -tzip %outputFile% ".\third-party\bass\bass.dll" -bb2 | findstr "+"
%zip% a -tzip %outputFile% ".\third-party\asi_loader\vorbisFile.dll" -bb2 | findstr "+"
%zip% a -tzip %outputFile% ".\third-party\asi_loader\vorbisHooked.dll" -bb2 | findstr "+"

%zip% a -tzip %outputFile% ".\third-party\asi_loader\ReadMe.txt" -bb2 | findstr "+"
%zip% rn %outputFile% "ReadMe.txt" "cleo_readme\ASI Loader Readme.txt" -bso0


%zip% a -tzip %outputFile% ".\.output\Changelog.html" -bb2 | findstr "+"
%zip% rn %outputFile% "Changelog.html" "cleo_readme\Changelog.html" -bso0

%zip% a -tzip %outputFile% ".\.output\Readme.html" -bb2 | findstr "+" 
%zip% rn %outputFile% "Readme.html" "cleo_readme\Readme.html" -bso0

%zip% a -tzip %outputFile% ".\source\cleo_config.ini" -bb2 | findstr "+" 
%zip% rn %outputFile% "cleo_config.ini" "cleo\.cleo_config.ini" -bso0

%zip% a -tzip %outputFile% "cleo_plugins\.output\*.cleo" -bb2 | findstr "+"
%zip% a -tzip %outputFile% "cleo_plugins\.output\*.ini" -bb2 | findstr "+"
%zip% rn %outputFile% "cleo_plugins\.output" "cleo\cleo_plugins" -bso0

pause
