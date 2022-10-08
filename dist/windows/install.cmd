@echo off

set install_dir="%USERPROFILE%\.hoichess"

echo ----------------------------------------------------------------------
echo HoiChess Installer for Windows
echo ----------------------------------------------------------------------
echo.
echo This will install HoiChess into
echo %install_dir%
echo.

rem choice /C yn /M "Continue? [y/n] "
rem if errorlevel 2 exit /b
set /P ANS="Continue? [y/n] "
if /I "%ANS%"=="y" goto continue
exit /b
:continue

mkdir %install_dir%
copy bin\* %install_dir%
copy doc\* %install_dir%
copy share\* %install_dir%

echo.
echo ----------------------------------------------------------------------
echo HoiChess has been installed into
echo %install_dir%
echo To uninstall, simply delete that directory.
echo ----------------------------------------------------------------------
