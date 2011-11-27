#!/bin/sh

LANG=C

PACKAGE=$(cat /tmp/build-package)
USER_NAME=$(cat /tmp/build-user)
VERSION=$(cat /tmp/build-version)
DEPENDED_PACKAGES=$(cat /tmp/depended-packages)
BUILD_SCRIPT=/tmp/build-deb-in-chroot.sh

run()
{
    "$@"
    if test $? -ne 0; then
	echo "Failed $@"
	exit 1
    fi
}

distribution=debian
lsb_release=/etc/lsb-release
if [ -f $lsb_release ]; then
    case $(grep "DISTRIB_ID=" $lsb_release) in
	*Ubuntu*)
	    distribution=ubuntu
	    ;;
    esac
fi

sources_list=/etc/apt/sources.list
if [ "$distribution" = "ubuntu" ] && \
    ! (grep '^deb' $sources_list | grep -q universe); then
    run sed -i'' -e 's/main$/main universe/g' $sources_list
fi

if [ ! -x /usr/bin/aptitude ]; then
    run apt-get update
    run apt-get install -y aptitude
fi
run aptitude update -V -D
run aptitude safe-upgrade -V -D -y

run aptitude install -V -D -y ruby

if aptitude show libmsgpack-dev > /dev/null 2>&1; then
    DEPENDED_PACKAGES="${DEPENDED_PACKAGES} libmsgpack-dev"
else
    ruby -i'' -ne 'print $_ unless /libmsgpack/' /tmp/${PACKAGE}-debian/control
fi

if aptitude show libzmq-dev > /dev/null 2>&1; then
    DEPENDED_PACKAGES="${DEPENDED_PACKAGES} libzmq-dev"
else
    ruby -i'' -ne 'print $_ unless /libzmq/' /tmp/${PACKAGE}-debian/control
fi

if aptitude show libevent-dev > /dev/null 2>&1; then
    DEPENDED_PACKAGES="${DEPENDED_PACKAGES} libevent-dev"
else
    ruby -i'' -ne 'print $_ unless /libevent/' /tmp/${PACKAGE}-debian/control
fi

if aptitude show liblzo2-dev > /dev/null 2>&1; then
    DEPENDED_PACKAGES="${DEPENDED_PACKAGES} liblzo2-dev"
else
    ruby -i'' -ne 'print $_ unless /liblzo2-dev/' /tmp/${PACKAGE}-debian/control
fi

run aptitude install -V -D -y devscripts ${DEPENDED_PACKAGES}
run aptitude clean

if ! id $USER_NAME >/dev/null 2>&1; then
    run useradd -m $USER_NAME
fi

cat <<EOF > $BUILD_SCRIPT
#!/bin/sh

rm -rf build
mkdir -p build

cp /tmp/${PACKAGE}-${VERSION}.tar.gz build/${PACKAGE}_${VERSION}.orig.tar.gz
cd build
tar xfz ${PACKAGE}_${VERSION}.orig.tar.gz
cd ${PACKAGE}-${VERSION}/
cp -rp /tmp/${PACKAGE}-debian debian
# export DEB_BUILD_OPTIONS=noopt
debuild -us -uc
EOF

run chmod +x $BUILD_SCRIPT
run su - $USER_NAME $BUILD_SCRIPT
