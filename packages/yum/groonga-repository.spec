Summary: groonga RPM repository configuration
Name: groonga-repository
Version: 1.0.1
Release: 0
License: LGPLv2
URL: http://packages.groonga.org/
Source: groonga-repository.tar.gz
Group: System Environment/Base
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-%(%{__id_u} -n)
BuildArchitectures: noarch

%description
groonga RPM repository configuration.

%prep
%setup -c

%build

%install
%{__rm} -rf %{buildroot}

%{__install} -Dp -m0644 RPM-GPG-KEY-groonga %{buildroot}%{_sysconfdir}/pki/rpm-gpg/RPM-GPG-KEY-groonga

%{__install} -Dp -m0644 groonga.repo %{buildroot}%{_sysconfdir}/yum.repos.d/groonga.repo

%clean
%{__rm} -rf %{buildroot}

%files
%defattr(-, root, root, 0755)
%doc *
%pubkey RPM-GPG-KEY-groonga
%dir %{_sysconfdir}/yum.repos.d/
%config(noreplace) %{_sysconfdir}/yum.repos.d/groonga.repo
%dir %{_sysconfdir}/pki/rpm-gpg/
%{_sysconfdir}/pki/rpm-gpg/RPM-GPG-KEY-groonga

%changelog
* Sun Apr 29 2012 Kouhei Sutou <kou@clear-code.com>
- Update GPG key.

* Thu Sep 02 2010 Kouhei Sutou <kou@clear-code.com>
- (1.0.0-0)
- Initial package.
