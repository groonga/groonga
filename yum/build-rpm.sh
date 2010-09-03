#!/bin/sh

LANG=C

PACKAGE=$(cat /tmp/build-package)
USER_NAME=$(cat /tmp/build-user)
VERSION=$(cat /tmp/build-version)
DEPENDED_PACKAGES=$(cat /tmp/depended-packages)
BUILD_SCRIPT=/tmp/build-${PACKAGE}.sh

run()
{
    "$@"
    if test $? -ne 0; then
	echo "Failed $@"
	exit 1
    fi
}

distribution=$(cut -d ' ' -f 1 /etc/redhat-release | tr 'A-Z' 'a-z')
distribution_version=$(cut -d ' ' -f 3 /etc/redhat-release)
if ! rpm -q ${distribution}-release > /dev/null 2>&1; then
    packages_dir=/var/cache/yum/core/packages
    release_rpm=${distribution}-release-${distribution_version}-*.rpm
    run rpm -Uvh --force ${packages_dir}/${release_rpm}
    run rpm -Uvh --force ${packages_dir}/ca-certificates-*.rpm
fi

run yum update -y
if [ "$distribution" = "centos" ] && ! rpm -q mecab-devel > /dev/null; then
    run yum install -y wget libtool gcc make

    cat <<EOF > $BUILD_SCRIPT
#!/bin/sh

base=http://download.fedoraproject.org/pub/fedora/linux/releases/13/Everything/source/SRPMS
srpm=\$1

if [ ! -f ~/.rpmmacros ]; then
    cat <<EOM > ~/.rpmmacros
%_topdir \$HOME/rpm
EOM
fi

rm -rf rpm

mkdir -p rpm/BUILD
mkdir -p rpm/RPMS
mkdir -p rpm/SRPMS
mkdir -p rpm/SOURCES
mkdir -p rpm/SPECS

mkdir -p dependencies/RPMS
mkdir -p dependencies/SRPMS

mkdir -p tmp
cd tmp
wget \$base/\$srpm
rpm2cpio \$srpm | cpio -id
mv \$srpm ~/dependencies/SRPMS/
mv *.spec ~/rpm/SPECS/
mv * ~/rpm/SOURCES/
cd ..
rm -rf tmp
rpmbuild -ba rpm/SPECS/*.spec

cp -p rpm/RPMS/*/*.rpm dependencies/RPMS/
EOF

    run chmod +x $BUILD_SCRIPT
    for rpm in mecab-0.98-1.fc12.src.rpm \
               mecab-ipadic-2.7.0.20070801-3.fc13.1.src.rpm \
               mecab-jumandic-5.1.20070304-4.fc12.src.rpm; do
	run su - $USER_NAME $BUILD_SCRIPT $rpm
	run rpm -Uvh /home/$USER_NAME/rpm/RPMS/*/*.rpm
    done
fi
run yum install -y rpm-build tar ${DEPENDED_PACKAGES}
run yum clean packages

if ! id $USER_NAME >/dev/null 2>&1; then
    run useradd -m $USER_NAME
fi

cat <<EOF > $BUILD_SCRIPT
#!/bin/sh

if [ ! -f ~/.rpmmacros ]; then
    cat <<EOM > ~/.rpmmacros
%_topdir \$HOME/rpm
EOM
fi

rm -rf rpm
mkdir -p rpm/SOURCES
mkdir -p rpm/SPECS
mkdir -p rpm/BUILD
mkdir -p rpm/RPMS
mkdir -p rpm/SRPMS

cp /tmp/${PACKAGE}-$VERSION.tar.gz rpm/SOURCES/
cp /tmp/${PACKAGE}.spec rpm/SPECS/

chmod o+rx . rpm rpm/RPMS rpm/SRPMS

rpmbuild -ba rpm/SPECS/${PACKAGE}.spec
EOF

run chmod +x $BUILD_SCRIPT
run su - $USER_NAME $BUILD_SCRIPT
