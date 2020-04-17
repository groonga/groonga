#!/bin/sh

apt install /repositories/debian/pool/stretch/main/g/groonga/*.deb
groonga --version
git clone https://github.com/groonga/groonga.git
cd groonga
sh autogen.sh
./configure
GROONGA=/usr/bin/groonga /groonga/test/unit/run-test.sh
GROONGA=/usr/bin/groonga /groonga/test/command/run-test.sh
GROONGA=/usr/bin/groonga /groonga/test/command_line/run-test.sh
