#!/bin/sh

script_base_dir=`dirname $0`

if [ $# != 2 ]; then
    echo "Usage: $0 GPG_UID DISTRIBUTIONS"
    echo " e.g.: $0 'F10399C0' 'fedora centos'"
    exit 1
fi

GPG_UID=$1
DISTRIBUTIONS=$2

run()
{
    "$@"
    if test $? -ne 0; then
	echo "Failed $@"
	exit 1
    fi
}

for distribution in ${DISTRIBUTIONS}; do
    run rpm \
	-D "_gpg_name ${GPG_UID}" \
	-D "__gpg /usr/bin/gpg2" \
	-D "__gpg_check_password_cmd /bin/true true" \
	-D "__gpg_sign_cmd %{__gpg} gpg --batch --no-verbose --no-armor %{?_gpg_digest_algo:--digest-algo %{_gpg_digest_algo}} --no-secmem-warning -u \"%{_gpg_name}\" -u \"1C837F31\" -sbo %{__signature_filename} %{__plaintext_filename}" \
	--resign $script_base_dir/${distribution}/*/*/*/*.rpm
done
