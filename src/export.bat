@echo off
if defined MSYSTEM (
    echo This .bat file is for Windows CMD.EXE shell only. When using MSYS, run:
    echo   . ./export.sh.
    goto :eof
)

echo.
echo Setup environment for developing...
echo.

:: Infer IDF_PATH from script location
set IDF_PATH=%~dp0
set IDF_PATH=%IDF_PATH:~0,-4%libs\esp_idf
echo Setting IDF_PATH to %IDF_PATH%

rem C:\Program Files\xtensa-lx106-elf\bin
rem C:\Program Files\mconf-v4.6.0.0-idf-20190628-win32
rem 

set OLD_PATH=%PATH%
echo Old PATH=%PATH%
set DEV_TOOLS_BIN_PATH=C:\Program Files\xtensa-lx106-elf\bin
set CONF_TOOLS_PATH=C:\Program Files\mconf-v4.6.0.0-idf-20190628-win32
set PATH=%DEV_TOOLS_BIN_PATH%;%CONF_TOOLS_PATH%;%IDF_PATH%;%PATH%
echo New PATH=%PATH%

cd build

echo.
echo Use this command:
echo   ninja clean - to clean project
echo   ninja - to build project
echo   ninja menuconfig - to configure project
echo.

