#!/bin/sh

set -e
set -x

rvm use 1.9.3

./autogen.sh

./configure --with-ruby19
