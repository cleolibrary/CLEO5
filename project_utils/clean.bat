@echo off
pushd "%~dp0.."
echo Cleaning build outputs and intermediate files...
if exist ".output" rmdir /s /q ".output"
if exist "cleo_plugins\.output" rmdir /s /q "cleo_plugins\.output"
popd
echo Done.
pause
