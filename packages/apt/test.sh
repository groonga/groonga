#!/bin/sh

apt install /repositories/debian/pool/stretch/main/g/groonga/*.deb
groonga --version
git clone https://github.com/groonga/groonga.git
cd groonga
GROONGA=/usr/bin/groonga /groonga/test/command/run-test.sh
