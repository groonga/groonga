cd lib
nmake -f Makefile.msvc install
cd ..

cd src
nmake -f Makefile.msvc install
cd ..

cd plugins\suggest
nmake -f Makefile.msvc install
cd ..\..

cd plugins\tokenizers
nmake -f Makefile.msvc install
cd ..\..
