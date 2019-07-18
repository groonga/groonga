Summary: Groonga release files
Name: groonga-release
Version: 1.5.0
Release: 1
License: LGPLv2
URL: https://packages.groonga.org/
Source: groonga-release.tar.gz
Group: System Environment/Base
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-%(%{__id_u} -n)
BuildArchitectures: noarch
Requires: epel-release
Obsoletes: groonga-repository <= 1.0.1-0

%description
Groonga release files

%prep
%setup -c

%build

%install
%{__rm} -rf %{buildroot}

%{__install} -Dp -m0644 RPM-GPG-KEY-groonga %{buildroot}%{_sysconfdir}/pki/rpm-gpg/RPM-GPG-KEY-groonga
%{__install} -Dp -m0644 RPM-GPG-KEY-groonga-RSA4096 %{buildroot}%{_sysconfdir}/pki/rpm-gpg/RPM-GPG-KEY-groonga-RSA4096
%{__install} -Dp -m0644 groonga.repo %{buildroot}%{_sysconfdir}/yum.repos.d/groonga.repo


%clean
%{__rm} -rf %{buildroot}

%files
%defattr(-, root, root, 0755)
%doc *
%pubkey RPM-GPG-KEY-groonga
%pubkey RPM-GPG-KEY-groonga-RSA4096
%dir %{_sysconfdir}/yum.repos.d/
%config(noreplace) %{_sysconfdir}/yum.repos.d/groonga.repo
%dir %{_sysconfdir}/pki/rpm-gpg/
%{_sysconfdir}/pki/rpm-gpg/RPM-GPG-KEY-groonga
%{_sysconfdir}/pki/rpm-gpg/RPM-GPG-KEY-groonga-RSA4096

%post
if grep -q 'Amazon Linux release 2' /etc/system-release 2>/dev/null; then
  cat <<EOR > %{_sysconfdir}/yum.repos.d/groonga.repo
[groonga]
name=Groonga for Amazon Linux 2 - \$basearch
baseurl=https://packages.groonga.org/centos/7/\$basearch/
gpgcheck=1
enabled=1
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-groonga
       file:///etc/pki/rpm-gpg/RPM-GPG-KEY-groonga-RSA4096
EOR
elif
  cat <<EOR > %{buildroot}%{_sysconfdir}/yum.repos.d/groonga.repo
[groonga]
name=Groonga for CentOS \$releasever - \$basearch
baseurl=https://packages.groonga.org/centos/\$releasever/\$basearch/
gpgcheck=1
enabled=1
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-groonga
       file:///etc/pki/rpm-gpg/RPM-GPG-KEY-groonga-RSA4096
EOR
fi


%changelog
* Fri Jul 12 2019 Horimoto Yasuhiro <horimoto@clear-code.com> - 1.5.0-1
- Add support for Amazon Linux 2

* Mon Jan 29 2018 Kentaro Hayashi <hayashi@clear-code.com> - 1.4.0-1
- Add new signing key for transition from weak key (1024bit)

* Sat Mar 25 2017 Kentaro Hayashi <hayashi@clear-code.com> - 1.3.0-1
- Use https instead of http in groonga.repo.

* Fri Nov 11 2016 Kouhei Sutou <kou@clear-code.com> - 1.2.0-0
- Require epel-release for msgpack-devel, lz4-devel and libzstd-devel.

* Thu Nov 29 2012 HAYASHI Kentaro <hayashi@clear-code.com>
- Fix to specify the version of last released groonga-repository package
  as Obsoletes.

* Tue May 29 2012 Kouhei Sutou <kou@clear-code.com>
- Rename to groonga-release from groonga-repository to follow
  convention such as centos-release and fedora-release.

* Sun Apr 29 2012 Kouhei Sutou <kou@clear-code.com>
- Update GPG key.

* Thu Sep 02 2010 Kouhei Sutou <kou@clear-code.com>
- (1.0.0-0)
- Initial package.
