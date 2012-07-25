#!/bin/sh

# Usage: check-utility.sh [--install-groonga]
#                         [--check-install]
#                         [--check-address]
#                         [--enable-repository]
#
# CODES="squeeze wheezy unstable lucid natty oneiric precise"
# DISTRIBUTIONS="centos fedora"

CHROOT_ROOT=/var/lib/chroot
CHECK_ADDRESS=0
CHECK_INSTALL=0
ENABLE_REPOSITORY=0
DISABLE_REPOSITORY=0
INSTALL_SCRIPT=0
INSTALL_GROONGA=0
UNINSTALL_GROONGA=0

echo_packages_repository_address ()
{
    root_dir=$1
    code=$2
    arch=$3
    address=`grep "packages.groonga.org" $root_dir/etc/hosts | grep -v "#"`
    if [ -z "$address" ]; then
	echo "$code-$arch: default"
    else
	echo "$code-$arch: $address"
    fi
}

check_packages_repository_address ()
{
    for code in $CODES; do
	for arch in $DEB_ARCHITECTURES; do
	    root_dir=$CHROOT_ROOT/$code-$arch
	    echo_packages_repository_address "$root_dir" "$code" "$arch"
	done
    done
    for dist in $DISTRIBUTIONS; do
	case $dist in
	    "fedora")
		DISTRIBUTIONS_VERSION="17"
		;;
	    "centos")
		DISTRIBUTIONS_VERSION="5 6"
		;;
	esac
	for ver in $DISTRIBUTIONS_VERSION; do
	    for arch in $RPM_ARCHITECTURES; do
		root_dir=$CHROOT_ROOT/$dist-$ver-$arch
		echo_packages_repository_address "$root_dir" "$dist-$ver" "$arch"
	    done
	done
    done
}

host_address ()
{
    ifconfig_result=`LANG=C /sbin/ifconfig wlan0`
    inet_addr=`echo "$ifconfig_result" | grep "inet addr:192"`
    address=`echo $inet_addr | ruby -ne '/inet addr:(.+?)\s/ =~ $_ && puts($1)'`
    HOST_ADDRESS=$address
}

check_installed_groonga_packages ()
{
    cat > check-deb-groonga.sh <<EOF
#!/bin/sh
dpkg -l | grep roonga
dpkg -l | grep mysql
EOF
    cat > check-rpm-groonga.sh <<EOF
#!/bin/sh
rpm -qa | grep roonga
rpm -qa | grep mysql
EOF
    for code in $CODES; do
	for arch in $DEB_ARCHITECTURES; do
	    root_dir=$CHROOT_ROOT/$code-$arch
	    CHECK_SCRIPT=check-deb-groonga.sh
	    echo "copy check script $CHECK_SCRIPT to $root_dir/tmp"
	    sudo rm -f $root_dir/tmp/$CHECK_SCRIPT
	    cp $CHECK_SCRIPT $root_dir/tmp
	    sudo chmod 755 $root_dir/tmp/$CHECK_SCRIPT
	    sudo chname $code-$arch chroot $root_dir /tmp/$CHECK_SCRIPT
	done
    done
    for dist in $DISTRIBUTIONS; do
	case $dist in
	    "fedora")
		DISTRIBUTIONS_VERSION="17"
		;;
	    "centos")
		DISTRIBUTIONS_VERSION="5 6"
		;;
	esac
	for ver in $DISTRIBUTIONS_VERSION; do
	    for arch in $RPM_ARCHITECTURES; do
		CHECK_SCRIPT=check-rpm-groonga.sh
		root_dir=$CHROOT_ROOT/$dist-$ver-$arch
		echo "copy check script $CHECK_SCRIPT to $root_dir/tmp"
		sudo rm -f $root_dir/tmp/$CHECK_SCRIPT
		cp $CHECK_SCRIPT $root_dir/tmp
		sudo chmod 755 $root_dir/tmp/$CHECK_SCRIPT
		sudo chname $code-$ver-$arch chroot $root_dir /tmp/$CHECK_SCRIPT
	    done
	done
    done
}

install_groonga_packages ()
{
    cat > install-aptitude-groonga.sh <<EOF
#!/bin/sh
sudo aptitude clean
rm -f /var/lib/apt/lists/packages.groonga.org_*
rm -f /var/lib/apt/lists/partial/packages.groonga.org_*
sudo aptitude update
sudo aptitude -V -D -y --allow-untrusted install groonga-keyring
sudo aptitude update
sudo aptitude -V -D -y install groonga
sudo aptitude -V -D -y install groonga-tokenizer-mecab
sudo aptitude -V -D -y install groonga-munin-plugins
EOF
    cat > install-aptget-groonga.sh <<EOF
#!/bin/sh
sudo apt-get clean
rm -f /var/lib/apt/lists/packages.groonga.org_*
rm -f /var/lib/apt/lists/partial/packages.groonga.org_*
sudo apt-get update
sudo apt-get -y --allow-unauthenticated install groonga-keyring
sudo apt-get update
sudo apt-get -y install groonga
sudo apt-get -y install groonga-tokenizer-mecab
sudo apt-get -y install groonga-munin-plugins
EOF
    for code in $CODES; do
	for arch in $DEB_ARCHITECTURES; do
	    root_dir=$CHROOT_ROOT/$code-$arch
	    INSTALL_SCRIPT=""
	    case $code in
		squeeze|unstable)
		    INSTALL_SCRIPT=install-aptitude-groonga.sh
		    ;;
		*)
		    INSTALL_SCRIPT=install-aptget-groonga.sh
		    ;;
	    esac
	    echo "copy install script $INSTALL_SCRIPT to $root_dir/tmp"
	    sudo rm -f $root_dir/tmp/$INSTALL_SCRIPT
	    cp $INSTALL_SCRIPT $root_dir/tmp
	    chmod 755 $root_dir/tmp/$INSTALL_SCRIPT
	    sudo chname $code-$arch chroot $root_dir /tmp/$INSTALL_SCRIPT
	done
    done
    cat > install-centos5-groonga.sh <<EOF
sudo rpm -ivh http://packages.groonga.org/centos/groonga-release-1.1.0-0.noarch.rpm
sudo yum makecache
sudo yum install -y groonga
sudo yum install -y groonga-tokenizer-mecab
wget http://download.fedoraproject.org/pub/epel/5/i386/epel-release-5-4.noarch.rpm
sudo rpm -ivh epel-release-5-4.noarch.rpm
sudo yum install -y groonga-munin-plugins
EOF
    cat > install-centos6-groonga.sh <<EOF
sudo rpm -ivh http://packages.groonga.org/centos/groonga-release-1.1.0-0.noarch.rpm
sudo yum makecache
sudo yum install -y groonga
sudo yum install -y groonga-tokenizer-mecab
sudo rpm -ivh http://download.fedoraproject.org/pub/epel/6/i386/epel-release-6-7.noarch.rpm
sudo yum install -y groonga-munin-plugins
EOF
    cat > install-fedora-groonga.sh <<EOF
sudo rpm -ivh http://packages.groonga.org/fedora/groonga-release-1.1.0-0.noarch.rpm
sudo yum makecache
sudo yum install -y groonga
sudo yum install -y groonga-tokenizer-mecab
sudo yum install -y groonga-munin-plugins
EOF
    for dist in $DISTRIBUTIONS; do
	case $dist in
	    "fedora")
		DISTRIBUTIONS_VERSION="17"
		;;
	    "centos")
		DISTRIBUTIONS_VERSION="5 6"
		;;
	esac
	for ver in $DISTRIBUTIONS_VERSION; do
	    for arch in $RPM_ARCHITECTURES; do
		case "$dist-$ver" in
		    centos-5)
			INSTALL_SCRIPT=install-centos5-groonga.sh
			;;
		    centos-6)
			INSTALL_SCRIPT=install-centos6-groonga.sh
			;;
		    fedora-17)
			INSTALL_SCRIPT=install-fedora-groonga.sh
			;;
		    *)
			;;
		esac
		root_dir=$CHROOT_ROOT/$dist-$ver-$arch
		echo "copy install script $INSTALL_SCRIPT to $root_dir/tmp"
		sudo rm -f $root_dir/tmp/$INSTALL_SCRIPT
		cp $INSTALL_SCRIPT $root_dir/tmp
		chmod 755 $root_dir/tmp/$INSTALL_SCRIPT
		sudo chname $code-$ver-$arch chroot $root_dir /tmp/$INSTALL_SCRIPT
	    done
	done
    done
}


uninstall_groonga_packages ()
{
    UNINSTALL_SCRIPT=uninstall-deb-groonga.sh
    cat > $UNINSTALL_SCRIPT <<EOF
#!/bin/sh
sudo apt-get purge groonga-* mysql-*
EOF
    for code in $CODES; do
	for arch in $DEB_ARCHITECTURES; do
	    root_dir=$CHROOT_ROOT/$code-$arch
	    echo "copy uninstall script $UNINSTALL_SCRIPT to $root_dir/tmp"
	    sudo rm -f $root_dir/tmp/$UNINSTALL_SCRIPT
	    cp $UNINSTALL_SCRIPT $root_dir/tmp
	    chmod 755 $root_dir/tmp/$UNINSTALL_SCRIPT
	    sudo chname $code-$arch chroot $root_dir /tmp/$UNINSTALL_SCRIPT
	done
    done
    UNINSTALL_SCRIPT=uninstall-rpm-groonga.sh
    cat > $UNINSTALL_SCRIPT <<EOF
#!/bin/sh
sudo yum remove groonga-* mysql-*
EOF
    for dist in $DISTRIBUTIONS; do
	case $dist in
	    "fedora")
		DISTRIBUTIONS_VERSION="17"
		;;
	    "centos")
		DISTRIBUTIONS_VERSION="5 6"
		;;
	esac
	for ver in $DISTRIBUTIONS_VERSION; do
	    for arch in $RPM_ARCHITECTURES; do
		root_dir=$CHROOT_ROOT/$dist-$ver-$arch
		echo "copy install script $UNINSTALL_SCRIPT to $root_dir/tmp"
		sudo rm -f $root_dir/tmp/$UNINSTALL_SCRIPT
		cp $UNINSTALL_SCRIPT $root_dir/tmp
		chmod 755 $root_dir/tmp/$UNINSTALL_SCRIPT
		sudo chname $code-$ver-$arch chroot $root_dir /tmp/$UNINSTALL_SCRIPT
	    done
	done
    done
}



enable_temporaly_groonga_repository ()
{
    cat > enable-repository.sh <<EOF
#!/bin/sh

grep -v "packages.groonga.org" /etc/hosts > /tmp/hosts
echo "$HOST_ADDRESS packages.groonga.org" >> /tmp/hosts
cp -f /tmp/hosts /etc/hosts
EOF
    for code in $CODES; do
	for arch in $DEB_ARCHITECTURES; do
	    root_dir=$CHROOT_ROOT/$code-$arch
	    today=`date '+%Y%m%d.%s'`
	    sudo cp $root_dir/etc/hosts $root_dir/etc/hosts.$today
	    sudo cp enable-repository.sh $root_dir/tmp
	    sudo chname $code-$arch chroot $root_dir /tmp/enable-repository.sh
	done
    done
    for dist in $DISTRIBUTIONS; do
	case $dist in
	    "fedora")
		DISTRIBUTIONS_VERSION="17"
		;;
	    "centos")
		DISTRIBUTIONS_VERSION="5 6"
		;;
	esac
	for ver in $DISTRIBUTIONS_VERSION; do
	    for arch in $RPM_ARCHITECTURES; do
		root_dir=$CHROOT_ROOT/$dist-$ver-$arch
		today=`date '+%Y%m%d.%s'`
		sudo cp $root_dir/etc/hosts $root_dir/etc/hosts.$today
		sudo cp enable-repository.sh $root_dir/tmp
		sudo chname $code-$arch chroot $root_dir /tmp/enable-repository.sh
	    done
	done
    done

    check_packages_repository_address
}

disable_temporaly_groonga_repository ()
{
    cat > disable-repository.sh <<EOF
#!/bin/sh

grep -v "packages.groonga.org" /etc/hosts > /tmp/hosts
cp -f /tmp/hosts /etc/hosts
EOF
    DISABLE_SCRIPT=disable-repository.sh
    for code in $CODES; do
	for arch in $DEB_ARCHITECTURES; do
	    root_dir=$CHROOT_ROOT/$code-$arch
	    today=`date '+%Y%m%d.%s'`
	    sudo cp $root_dir/etc/hosts $root_dir/etc/hosts.$today
	    cp $DISABLE_SCRIPT $root_dir/tmp
	    chmod 755 $root_dir/tmp/$DISABLE_SCRIPT
	    sudo chname $code-$arch chroot $root_dir /tmp/$DISABLE_SCRIPT
	done
    done
    for dist in $DISTRIBUTIONS; do
	case $dist in
	    "fedora")
		DISTRIBUTIONS_VERSION="17"
		;;
	    "centos")
		DISTRIBUTIONS_VERSION="5 6"
		;;
	esac
	for ver in $DISTRIBUTIONS_VERSION; do
	    for arch in $RPM_ARCHITECTURES; do
		root_dir=$CHROOT_ROOT/$dist-$ver-$arch
		today=`date '+%Y%m%d.%s'`
		sudo cp $root_dir/etc/hosts $root_dir/etc/hosts.$today
		cp $DISABLE_SCRIPT $root_dir/tmp
		chmod 755 $root_dir/tmp/$DISABLE_SCRIPT
		sudo chname $code-$arch chroot $root_dir /tmp/$DISABLE_SCRIPT
	    done
	done
    done

    check_packages_repository_address
}

host_address
echo $HOST_ADDRESS

while [ $# -ne 0 ]; do
    case $1 in
	--check-install)
	    CHECK_INSTALL=1
	    shift
	    ;;
	--check-address)
	    CHECK_ADDRESS=1
	    shift
	    ;;
	--enable-repository)
	    ENABLE_REPOSITORY=1
	    shift
	    ;;
	--disable-repository)
	    DISABLE_REPOSITORY=1
	    shift
	    ;;
	--install-groonga)
	    INSTALL_GROONGA=1
	    shift
	    ;;
	--uninstall-groonga)
	    UNINSTALL_GROONGA=1
	    shift
	    ;;
	--code)
	    shift
	    CODES=$1
	    shift
	    ;;
	--code-arch)
	    shift
	    DEB_ARCHITECTURES=$1
	    shift
	    ;;
	--dist)
	    shift
	    DISTRIBUTIONS=$1
	    shift
	    ;;
	--dist-arch)
	    shift
	    RPM_ARCHITECTURES=$1
	    shift
	    ;;
	*)
	    shift
	    ;;
    esac
done

if [ -z "$DEB_ARCHITECTURES" ]; then
    DEB_ARCHITECTURES="i386 amd64"
fi
if [ -z "$RPM_ARCHITECTURES" ]; then
    RPM_ARCHITECTURES="i386 x86_64"
fi

if [ $CHECK_INSTALL -ne 0 ]; then
    check_installed_groonga_packages
fi
if [ $CHECK_ADDRESS -ne 0 ]; then
    check_packages_repository_address
fi
if [ $ENABLE_REPOSITORY -ne 0 ]; then
    enable_temporaly_groonga_repository
fi
if [ $DISABLE_REPOSITORY -ne 0 ]; then
    disable_temporaly_groonga_repository
fi
if [ $INSTALL_GROONGA -ne 0 ]; then
    install_groonga_packages
fi
if [ $UNINSTALL_GROONGA -ne 0 ]; then
    uninstall_groonga_packages
fi

