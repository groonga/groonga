#!/bin/sh

run()
{
    "$@"
    if test $? -ne 0; then
        echo "Failed $@"
        exit 1
    fi
}

rpmbuild_options=

. /vagrant/env.sh

swap_file=/tmp/swap
run sudo dd if=/dev/zero of="$swap_file" bs=1024 count=4096K
run sudo /sbin/mkswap "$swap_file"
run sudo /sbin/swapon "$swap_file"

distribution=$(cut -d " " -f 1 /etc/redhat-release | tr "A-Z" "a-z")
if grep -q Linux /etc/redhat-release; then
    distribution_version=$(cut -d " " -f 4 /etc/redhat-release)
else
    distribution_version=$(cut -d " " -f 3 /etc/redhat-release)
fi
distribution_version=$(echo ${distribution_version} | sed -e 's/\..*$//g')

architecture="$(arch)"
case "${architecture}" in
    i*86)
        architecture=i386
        ;;
esac

run sudo yum install -y epel-release
run sudo yum groupinstall -y "Development Tools"

have_rapidjson=yes
if ! yum info rapidjson-devel > /dev/null 2>&1; then
  have_rapidjson=no
fi

if [ "${have_rapidjson}" = "no" ]; then
  DEPENDED_PACKAGES="$(echo ${DEPENDED_PACKAGES} | sed -e 's/rapidjson-devel//')"
fi

have_autoconf_archive=yes
if ! yum info autoconf-archive > /dev/null 2>&1; then
  have_autoconf_archive=no
fi

if [ "${have_autoconf_archive}" = "no" ]; then
  DEPENDED_PACKAGES="$(echo ${DEPENDED_PACKAGES} | sed -e 's/autoconf-archive//')"
fi

run sudo yum install -y rpm-build rpmdevtools tar ${DEPENDED_PACKAGES}

if [ -x /usr/bin/rpmdev-setuptree ]; then
    rm -rf .rpmmacros
    run rpmdev-setuptree
else
    run cat <<EOM > ~/.rpmmacros
%_topdir ${HOME}/rpmbuild
EOM
    run mkdir -p ~/rpmbuild/SOURCES
    run mkdir -p ~/rpmbuild/SPECS
    run mkdir -p ~/rpmbuild/BUILD
    run mkdir -p ~/rpmbuild/RPMS
    run mkdir -p ~/rpmbuild/SRPMS
fi

repository="/vagrant/repositories/${distribution}/${distribution_version}"
rpm_dir="${repository}/${architecture}/Packages"
srpm_dir="${repository}/source/SRPMS"
run mkdir -p "${rpm_dir}" "${srpm_dir}"

build_fedora_srpm()
{
    base=http://download.fedoraproject.org/pub/fedora/linux/releases/29/Everything/source/tree/Packages/m
    update=http://download.fedoraproject.org/pub/fedora/linux/updates/29/SRPMS
    srpm="$1"
    srpm_base="$2"

    run cd

    run mkdir -p tmp
    run cd tmp
    wget "${update}/${srpm}"
    if [ $? -ne 0 ]; then
        run wget "${base}/${srpm}"
    fi
    run rpm2cpio "${srpm}" | run cpio -id
    run rm "${srpm}"

    case "${srpm}" in
        mecab-ipadic*)
            patch -p0 < /vagrant/patches/mecab-ipadic-provides-mecab-dic.diff
            ;;
        mecab-jumandic-*)
            patch -p0 < /vagrant/patches/mecab-jumandic-provides-mecab-dic.diff
            ;;
    esac
    run rm -rf ~/rpmbuild/SPECS/
    run mkdir -p ~/rpmbuild/SPECS/
    run mv *.spec ~/rpmbuild/SPECS/
    run mv * ~/rpmbuild/SOURCES/
    run cd -
    run rm -rf tmp

    mecab_build_options="--buildroot ${HOME}/rpmbuild/BUILDROOT/${srpm_base}"
    case "${architecture}" in
        i*86)
            run rpmbuild -ba rpmbuild/SPECS/*.spec ${mecab_build_options} \
                --define "optflags -O2 -g -march=i586"
            ;;
        *)
            run rpmbuild -ba rpmbuild/SPECS/*.spec ${mecab_build_options}
            ;;
    esac

    run sudo rpm -Uvh rpmbuild/RPMS/*/*.rpm
    run mv rpmbuild/RPMS/*/*.rpm "${rpm_dir}/"
    run mv rpmbuild/SRPMS/*.rpm "${srpm_dir}/"
}

if ! rpm -q mecab-devel > /dev/null; then
    run sudo yum install -y wget

    for rpm in mecab-0.996-2.fc29.1.src.rpm \
               mecab-ipadic-2.7.0.20070801-17.fc29.src.rpm \
               mecab-jumandic-5.1.20070304-18.fc29.src.rpm; do
       srpm_base=`echo $rpm | sed -e 's/\.fc29.*//g'`
       run build_fedora_srpm "${rpm}" "${srpm_base}"
    done
fi

# for debug
# rpmbuild_options="$rpmbuild_options --define 'optflags -O0 -g3'"

cd

run cp /vagrant/tmp/${PACKAGE}-${VERSION}.* rpmbuild/SOURCES/
run cp /vagrant/tmp/${distribution}/${PACKAGE}.spec rpmbuild/SPECS/

run rpmbuild -ba ${rpmbuild_options} rpmbuild/SPECS/${PACKAGE}.spec

run mv rpmbuild/RPMS/*/* "${rpm_dir}/"
run mv rpmbuild/SRPMS/* "${srpm_dir}/"
