REM @echo off
REM Usage: GenerateWIXPackage.bat [PACKAGE_NAME] [BRANCH_NAME] [TARGET_VERSION]
REM [PACKAGE_NAME] 		: Name of package, e.g. [PACKAGE_NAME]-1.0.0-win64.msi
REM [BRANCH_NAME]       : Name of the branch this msi is being generated from

REM consider renaming "target" since it conflicts with the WiX definition for "target"
REM in this script, "target" refers to the upgrade version
REM [TARGET_VERSION]	: Version number for target package

REM @echo on

set SCRIPT_DIRECTORY=%~dp0
set LSA_ROOT=%SCRIPT_DIRECTORY%..

::Arguments 
set PACKAGE_NAME=%1
set BRANCH_NAME=%2
set TARGET_VERSION=%3

::Constants
set PATH=%PATH%;%WIX%\bin;
set PACKAGE_ARCH=win64

::Paths
set PACKAGE_DIRECTORY=%LSA_ROOT%\packages
set TARGET_PATH=%PACKAGE_DIRECTORY%\%PACKAGE_NAME%-%TARGET_VERSION%\_CPack_Packages\%PACKAGE_ARCH%\WIX

set TARGET_MSI=%TARGET_PATH%\%PACKAGE_NAME%-%TARGET_VERSION%-%PACKAGE_ARCH%.msi

REM @echo off

rmdir packages\%PACKAGE_NAME%-%TARGET_VERSION%

cpack --config %LSA_ROOT%\build\CPackConfig.cmake

COPY packages\%PACKAGE_NAME%-%TARGET_VERSION%\_CPack_Packages\win64\WIX\%PACKAGE_NAME%-%TARGET_VERSION%-%PACKAGE_ARCH%.msi %LSA_MSI_TARGET%\%PACKAGE_NAME%-%BRANCH_NAME%-%TARGET_VERSION%-%PACKAGE_ARCH%.msi
