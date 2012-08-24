#!/bin/sh

if [ $# != 11 ]; then
    echo "Usage: $0 PACKAGE VERSION SOURCE_BASE_NAME SPEC_DIR DESTINATION CHROOT_BASE ARCHITECTURES DISTRIBUTIONS HAVE_DEVELOPMENT_BRANCH USE_RPMFORGE USE_ATRPMS"
    echo " e.g.: $0 milter-manager 1.1.1 ../milter-manager ../rpm repositories/ /var/lib/chroot 'i386 x86_64' 'fedora centos' yes no no"
    exit 1
fi

PACKAGE=$1
VERSION=$2
SOURCE_BASE_NAME=$3
SPEC_DIR=$4
DESTINATION=$5
CHROOT_BASE=$6
ARCHITECTURES=$7
DISTRIBUTIONS=$8
HAVE_DEVELOPMENT_BRANCH=$9
USE_RPMFORGE=$10
USE_ATRPMS=$11

PATH=/usr/local/sbin:/usr/sbin:$PATH

script_base_dir=`dirname $0`

if test "$PARALLEL" = "yes"; then
    parallel="yes"
else
    parallel="no"
fi

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
	if [ "$distribution_name-$distribution_version" = "centos-5" ]; then
	    distribution_architecture=$architecture
	else
	    distribution_architecture=i686
	fi
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
    distribution_version=$3

    target=${distribution}-${distribution_version}-${architecture}
    base_dir=${CHROOT_BASE}/${target}
    if [ ! -d $base_dir ]; then
	run build_chroot $architecture $distribution $distribution_version
    fi

    build_user=${PACKAGE}-build
    build_user_dir=${base_dir}/home/${build_user}
    rpm_base_dir=${build_user_dir}/rpmbuild
    rpm_dir=${rpm_base_dir}/RPMS/${architecture}
    srpm_dir=${rpm_base_dir}/SRPMS
    pool_base_dir=${DESTINATION}${distribution}/${distribution_version}
    if test "${HAVE_DEVELOPMENT_BRANCH}" = "yes"; then
	minor_version=$(echo $VERSION | ruby -pe '$_.gsub!(/\A\d+\.(\d+)\..*/, "\\1")')
	if test $(expr ${minor_version} % 2) -eq 0; then
	    branch_name=stable
	else
	    branch_name=development
	fi
	pool_base_dir=${pool_base_dir}/${branch_name}
    fi
    binary_pool_dir=$pool_base_dir/$architecture/Packages
    source_pool_dir=$pool_base_dir/source/SRPMS
    if test -f ${SOURCE_BASE_NAME}-${VERSION}-*.src.rpm; then
	run cp ${SOURCE_BASE_NAME}-${VERSION}-*.src.rpm \
	    ${CHROOT_BASE}/$target/tmp/
    else
	run cp ${SOURCE_BASE_NAME}-${VERSION}.* \
	    ${CHROOT_BASE}/$target/tmp/
	run cp ${SPEC_DIR}/${distribution}/${PACKAGE}.spec \
	    ${CHROOT_BASE}/$target/tmp/
    fi
    run echo $PACKAGE > ${CHROOT_BASE}/$target/tmp/build-package
    run echo $VERSION > ${CHROOT_BASE}/$target/tmp/build-version
    run echo $(basename ${SOURCE_BASE_NAME}) > \
	${CHROOT_BASE}/$target/tmp/build-source-base-name
    run echo $build_user > ${CHROOT_BASE}/$target/tmp/build-user
    run cp ${script_base_dir}/${PACKAGE}-depended-packages \
	${CHROOT_BASE}/$target/tmp/depended-packages
    run echo $USE_RPMFORGE > ${CHROOT_BASE}/$target/tmp/build-use-rpmforge
    run echo $USE_ATRPMS > ${CHROOT_BASE}/$target/tmp/build-use-atrpms
    run cp ${script_base_dir}/${PACKAGE}-build-options \
	${CHROOT_BASE}/$target/tmp/build-options
    run cp ${script_base_dir}/build-rpm.sh ${CHROOT_BASE}/$target/tmp/
    run_sudo rm -rf $rpm_dir $srpm_dir
    run_sudo su -c "chroot ${CHROOT_BASE}/$target /tmp/build-rpm.sh"
    run mkdir -p $binary_pool_dir
    run mkdir -p $source_pool_dir
    run cp -p $rpm_dir/*-${VERSION}* $binary_pool_dir
    run cp -p $srpm_dir/*-${VERSION}* $source_pool_dir

    dependencies_dir=${build_user_dir}/dependencies
    dependencies_rpm_dir=${dependencies_dir}/RPMS
    dependencies_srpm_dir=${dependencies_dir}/SRPMS
    if [ -d "${dependencies_rpm_dir}" ]; then
	run cp -p ${dependencies_rpm_dir}/* $binary_pool_dir
    fi
    if [ -d "${dependencies_srpm_dir}" ]; then
	run cp -p ${dependencies_srpm_dir}/* $source_pool_dir
    fi
}

for architecture in $ARCHITECTURES; do
    for distribution in $DISTRIBUTIONS; do
	case $distribution in
	    fedora)
		distribution_versions="17"
		;;
	    centos)
		distribution_versions="5 6"
		if [ -n "$CENTOS_VERSIONS" ]; then
		    distribution_versions="$CENTOS_VERSIONS"
		fi
		;;
	esac
	for distribution_version in $distribution_versions; do
	    if test "$parallel" = "yes"; then
		build $architecture $distribution $distribution_version &
	    else
		mkdir -p tmp
		build_log=tmp/build-$distribution-$distribution_version-$architecture.log
		build $architecture $distribution $distribution_version 2>&1 | tee $build_log
	    fi;
	done;
    done;
done

if test "$parallel" = "yes"; then
    wait
fi
