#!/bin/sh

if [ $# != 5 ]; then
    echo "Usage: $0 PACKAGE VERSION CHROOT_BASE ARCHITECTURES DISTRIBUTIONS"
    echo " e.g.: $0 groonga 1.1.1 /var/lib/chroot 'i386 x86_64' 'fedora centos'"
    exit 1
fi

PACKAGE=$1
VERSION=$2
CHROOT_BASE=$3
ARCHITECTURES=$4
DISTRIBUTIONS=$5

PATH=/usr/local/sbin:/usr/sbin:$PATH

script_base_dir=`dirname $0`

run()
{
    "$@"
    if test $? -ne 0; then
	echo "Failed $@"
	exit 1
    fi
}

run_sudo()
{
    run sudo "$@"
}

build_chroot()
{
    architecture=$1
    distribution_name=$2
    distribution_version=$3

    if [ $architecture = "x86_64" ]; then
	rinse_architecture="amd64"
        distribution_architecture=$architecture
    else
	rinse_architecture=$architecture
	case $distribution_name in
	    fedora)
		distribution_architecture=i686
		;;
	    *)
		distribution_architecture=$architecture
		;;
	esac
    fi

    run_sudo mkdir -p ${base_dir}/etc/rpm
    rpm_platform=${distribution_architecture}-${distribution}-linux
    run_sudo sh -c "echo ${rpm_platform} > ${base_dir}/etc/rpm/platform"
    run_sudo rinse \
	--arch $rinse_architecture \
	--distribution $distribution_name-$distribution_version \
	--directory $base_dir
    run_sudo rinse --arch $rinse_architecture --clean-cache

    run_sudo sh -c "echo >> /etc/fstab"
    run_sudo sh -c "echo /dev ${base_dir}/dev none bind 0 0 >> /etc/fstab"
    run_sudo sh -c "echo devpts-chroot ${base_dir}/dev/pts devpts defaults 0 0 >> /etc/fstab"
    run_sudo sh -c "echo proc-chroot ${base_dir}/proc proc defaults 0 0 >> /etc/fstab"
    run_sudo mount ${base_dir}/dev
    run_sudo mount ${base_dir}/dev/pts
    run_sudo mount ${base_dir}/proc
}

build()
{
    architecture=$1
    distribution=$2

    case $distribution in
	fedora)
	    distribution_version=13
	    ;;
	centos)
	    distribution_version=5
	    ;;
    esac
    target=${distribution}-${distribution_version}-${architecture}
    base_dir=${CHROOT_BASE}/${target}
    if [ ! -d $base_dir ]; then
	run build_chroot $architecture $distribution $distribution_version
    fi

    source_dir=${script_base_dir}/..
    build_user=${PACKAGE}-build
    build_user_dir=${base_dir}/home/${build_user}
    rpm_base_dir=${build_user_dir}/rpm
    rpm_dir=${rpm_base_dir}/RPMS/${architecture}
    srpm_dir=${rpm_base_dir}/SRPMS
    pool_base_dir=${distribution}/${distribution_version}
    binary_pool_dir=$pool_base_dir/$architecture/Packages
    source_pool_dir=$pool_base_dir/source/SRPMS
    run cp $source_dir/${PACKAGE}-${VERSION}.tar.gz \
	${CHROOT_BASE}/$target/tmp/
    run cp $source_dir/rpm/${distribution}/${PACKAGE}.spec \
	${CHROOT_BASE}/$target/tmp/
    run echo $PACKAGE > ${CHROOT_BASE}/$target/tmp/build-package
    run echo $VERSION > ${CHROOT_BASE}/$target/tmp/build-version
    run echo $build_user > ${CHROOT_BASE}/$target/tmp/build-user
    run cp ${script_base_dir}/${PACKAGE}-depended-packages \
	${CHROOT_BASE}/$target/tmp/depended-packages
    run cp ${script_base_dir}/build-rpm.sh \
	${CHROOT_BASE}/$target/tmp/
    run_sudo rm -rf $rpm_dir $srpm_dir
    run_sudo su -c "chroot ${CHROOT_BASE}/$target /tmp/build-rpm.sh"
    run mkdir -p $binary_pool_dir
    run mkdir -p $source_pool_dir
    run cp -p $rpm_dir/*-${VERSION}* $binary_pool_dir
    run cp -p $srpm_dir/*-${VERSION}* $source_pool_dir
}

for architecture in $ARCHITECTURES; do
    for distribution in $DISTRIBUTIONS; do
	build $architecture $distribution
    done;
done
