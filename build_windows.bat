@rem Groonga Windows build
@rem NOTE: execute vcvars32.bat first!
@rem c:\Program Files\Microsoft Visual Studio 8\VC\bin\vcvars32.bat
@rem c:\Program Files\Microsoft Visual Studio 9.0\vc\bin\vcvars32.bat

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

@set INCLUDE=%OLD_INCLUDE%
@set LIB=%OLD_LIB%
