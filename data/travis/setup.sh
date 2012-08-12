#!/bin/sh

set -e
set -x

distribution=$(lsb_release --short --id | tr 'A-Z' 'a-z')
code_name=$(lsb_release --short --codename)
case $distribution in
    debian)
	component=main
	;;
    ubunutu)
	component=universe
	;;
esac
cat <<EOF | sudo tee /etc/apt/sources.list.d/groonga.list
deb http://packages.groonga.org/${distribution}/ ${code_name} ${component}
deb-src http://packages.groonga.org/${distribution}/ ${code_name} ${component}
EOF

sudo apt-get update
sudo apt-get -y --allow-unauthenticated install groonga-keyring
sudo apt-get -y purge zeromq
sudo apt-get update
sudo apt-get -y install libgroonga-dev
