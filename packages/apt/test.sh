#!/bin/sh

apt install /repositories/debian/pool/stretch/main/g/groonga/*.deb
groonga --version
git clone https://github.com/groonga/groonga.git
git clone https://github.com/clear-code/cutter.git
cd groonga
sh autogen.sh
./configure --with-cutter-source-path=../cutter/cutter
GROONGA=/usr/bin/groonga /groonga/test/command/run-test.sh
GROONGA=/usr/bin/groonga /groonga/test/command_line/run-test.sh
GROONGA=/usr/bin/groonga /groonga/test/unit/run-test.sh
