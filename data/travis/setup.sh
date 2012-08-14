#!/bin/sh

set -e
set -x

if [ "$GROONGA_MASTER" = "yes" ]; then
    sudo apt-get -y -V install autotools-dev pkg-config libmecab-dev \
	libmsgpack-dev libevent-dev
    git clone https://github.com/groonga/groonga.git
    cd groonga
    ./autogen.sh
    ./configure --prefix=/usr --localstatedir=/var --with-debug
    make -j$(grep '^processor' /proc/cpuinfo | wc -l) > /dev/null
    sudo make install > /dev/null
    cd ..
else
distribution=$(lsb_release --short --id | tr 'A-Z' 'a-z')
code_name=$(lsb_release --short --codename)
case $distribution in
    debian)
	component=main
	;;
    ubuntu)
	component=universe
	;;
esac
apt_url_base=http://packages.groonga.org
cat <<EOF | sudo tee /etc/apt/sources.list.d/groonga.list
deb ${apt_url_base}/${distribution}/ ${code_name} ${component}
deb-src ${apt_url_base}/${distribution}/ ${code_name} ${component}
EOF

sudo apt-get update
sudo apt-get -y --allow-unauthenticated install groonga-keyring
sudo apt-get -y purge zeromq
sudo apt-get update
sudo apt-get -y -V install groonga libgroonga-dev
fi
