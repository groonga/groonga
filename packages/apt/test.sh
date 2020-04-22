#!/bin/sh

apt install /repositories/debian/pool/stretch/main/g/groonga/*.deb
groonga --version
git clone https://github.com/groonga/groonga.git
cd groonga
sh autogen.sh
./configure --prefix=/tmp/local --with-ruby
make
GROONGA=/usr/bin/groonga /groonga/test/command/run-test.sh
