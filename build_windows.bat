@rem Groonga for Windows build script.

@if "%VSINSTALLDIR%"=="" goto error_no_VSINSTALLDIR
@if "%VCINSTALLDIR%"=="" goto error_no_VCINSTALLDIR

@set MECAB_SDK_PATH=C:\Program Files\MeCab\sdk

@set OLD_INCLUDE=%INCLUDE%
@set OLD_LIB=%LIB%

@set INCLUDE=%INCLUDE%;.;..;%MECAB_SDK_PATH%
@set LIB=%LIB%;%MECAB_SDK_PATH%

@rem build libgroonga.dll
cd lib
nmake -f Makefile.msvc clean
nmake -f Makefile.msvc
cd ..

@rem build groonga.exe
cd src
nmake -f Makefile.msvc clean
nmake -f Makefile.msvc
cd ..

@rem build suggest
cd plugins\suggest
nmake -f Makefile.msvc clean
nmake -f Makefile.msvc
cd ..\..

@rem build mecab
cd plugins\tokenizers
nmake -f Makefile.msvc clean
nmake -f Makefile.msvc
cd ..\..

@set INCLUDE=%OLD_INCLUDE%
@set LIB=%OLD_LIB%

@goto end

:error_no_VSINSTALLDIR
:error_no_VCINSTALLDIR
@echo ERROR: execute vcvarsall.bat first!
@echo.
@echo Visual Studio 2005
@echo c:\Program Files\Microsoft Visual Studio 8\VC\vcvarsall.bat
@echo Visual Studio 2008
@echo c:\Program Files\Microsoft Visual Studio 9.0\VC\vcvarsall.bat
@echo.
@echo if you want to get x86 binary on x86/x64 environment,
@echo you have to pass "x86" to vcvarsall.bat.
@echo if you want to get x64 binary on x86 environment,
@echo you have to pass "x86_amd64" to vcvarsall.bat.
@echo if you want to get x64 binary on x64 environment,
@echo you have to pass "amd64" to vcvarsall.bat.
@goto end

:end
