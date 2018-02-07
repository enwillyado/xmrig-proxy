@echo off

cd %~dp0

if /i "%1"=="help" goto help
if /i "%1"=="--help" goto help
if /i "%1"=="-help" goto help
if /i "%1"=="/help" goto help
if /i "%1"=="?" goto help
if /i "%1"=="-?" goto help
if /i "%1"=="--?" goto help
if /i "%1"=="/?" goto help

@rem Process arguments.
set config=vs-release
set target=Build
set target_arch=x64
set target_env=
set noprojgen=
set nobuild=
set run=
set vs_toolset=x86
set msbuild_platform=x64
set library=static_library

:next-arg
if "%1"=="" goto args-done
if /i "%1"=="release"      set config=vs-release&goto arg-ok
if /i "%1"=="debug"        set config=vs-debug&goto arg-ok
if /i "%1"=="x64"          set target_arch=x64&set msbuild_platform=x64&set vs_toolset=x64&goto arg-ok
if /i "%1"=="ia32"         set target_arch=ia32&set msbuild_platform=Win32&set vs_toolset=x86&goto arg-ok
if /i "%1"=="x64"          set target_arch=x64&set msbuild_platform=x64&set vs_toolset=x64&goto arg-ok
:arg-ok
shift
goto next-arg
:args-done

@rem Look for Visual Studio 2017 only if explicitly requested.
if "%target_env%" NEQ "vs2017" goto vs-set-2015
echo Looking for Visual Studio 2017
@rem Check if VS2017 is already setup, and for the requested arch.
if "_%VisualStudioVersion%_" == "_15.0_" if "_%VSCMD_ARG_TGT_ARCH%_"=="_%vs_toolset%_" goto found_vs2017
set "VSINSTALLDIR="
call tools\vswhere_usability_wrapper.cmd
if "_%VCINSTALLDIR%_" == "__" goto vs-set-2015
@rem Need to clear VSINSTALLDIR for vcvarsall to work as expected.
@rem Keep current working directory after call to vcvarsall
set "VSCMD_START_DIR=%CD%"
set vcvars_call="%VCINSTALLDIR%\Auxiliary\Build\vcvarsall.bat" %vs_toolset%
echo calling: %vcvars_call%
call %vcvars_call%

:found_vs2017
echo Found MSVS version %VisualStudioVersion%
if %VSCMD_ARG_TGT_ARCH%==x64 set target_arch=x64&set msbuild_platform=x64&set vs_toolset=x64
set GYP_MSVS_VERSION=2017
goto select-target

@rem Look for Visual Studio 2015
:vs-set-2015
if not defined VS140COMNTOOLS goto vc-set-2013
if not exist "%VS140COMNTOOLS%\..\..\vc\vcvarsall.bat" goto vc-set-2013
call "%VS140COMNTOOLS%\..\..\vc\vcvarsall.bat" %vs_toolset%
set GYP_MSVS_VERSION=2015
echo Using Visual Studio 2015
goto select-target

:vc-set-2013
@rem Look for Visual Studio 2013
if not defined VS120COMNTOOLS goto vc-set-2012
if not exist "%VS120COMNTOOLS%\..\..\vc\vcvarsall.bat" goto vc-set-2012
call "%VS120COMNTOOLS%\..\..\vc\vcvarsall.bat" %vs_toolset%
set GYP_MSVS_VERSION=2013
echo Using Visual Studio 2013
goto select-target

:vc-set-2012
@rem Look for Visual Studio 2012
if not defined VS110COMNTOOLS goto vc-set-2010
if not exist "%VS110COMNTOOLS%\..\..\vc\vcvarsall.bat" goto vc-set-2010
call "%VS110COMNTOOLS%\..\..\vc\vcvarsall.bat" %vs_toolset%
set GYP_MSVS_VERSION=2012
echo Using Visual Studio 2012
goto select-target

:vc-set-2010
@rem Look for Visual Studio 2010
if not defined VS100COMNTOOLS goto vc-set-2008
if not exist "%VS100COMNTOOLS%\..\..\vc\vcvarsall.bat" goto vc-set-2008
call "%VS100COMNTOOLS%\..\..\vc\vcvarsall.bat" %vs_toolset%
set GYP_MSVS_VERSION=2010
echo Using Visual Studio 2010
goto select-target

:vc-set-2008
@rem Look for Visual Studio 2008
if not defined VS90COMNTOOLS goto vc-set-notfound
if not exist "%VS90COMNTOOLS%\..\..\vc\vcvarsall.bat" goto vc-set-notfound
call "%VS90COMNTOOLS%\..\..\vc\vcvarsall.bat" %vs_toolset%
set GYP_MSVS_VERSION=2008
echo Using Visual Studio 2008
goto select-target

:vc-set-notfound
echo Warning: Visual Studio not found

:select-target
:project-gen
:msbuild-found
echo Configuration=%config% Platform=%msbuild_platform%
msbuild xmrig-proxy.sln /m /t:%target% /p:Configuration=%config% /p:Platform="%msbuild_platform%" /clp:NoSummary;NoItemAndPropertyList;Verbosity=minimal /nologo
if errorlevel 1 exit /b 1
goto exit

:help
echo vcbuild.bat [x86/x64]
echo Examples:
echo   vcbuild.bat ia32         : builds release ia32 build
echo   vcbuild.bat x64          : builds release x64 build
echo   vcbuild.bat release ia32 : builds release ia32 build
echo   vcbuild.bat release x64  : builds release x64 build
echo   vcbuild.bat debug ia32   : builds debug ia32 build
echo   vcbuild.bat debug x64    : builds debug x64 build
goto exit

:exit
pause
