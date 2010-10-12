cd lib
nmake -f Makefile.msvc install
cd ..

cd src
nmake -f Makefile.msvc install
cd ..

cd modules\suggest
nmake -f Makefile.msvc install
cd ..\..

cd modules\tokenizers
nmake -f Makefile.msvc install
cd ..\..
