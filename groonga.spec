%define version 0.0.1
%define release 1

Summary: An Embeddable Fulltext Search Engine
Name: groonga
Version: %{version}
Release: %{release}
License: LGPL
Group: local
Packager: Brazil Ltd. <senna@razil.jp>
Source: %{name}-%{version}.tar.gz
BuildRoot: /var/tmp/%{name}
Requires: mecab >= 0.80

%description
An Embeddable Fulltext Search Engine.

%package devel
Summary: Libraries and header files for Groonga
Group: Development/Libraries

%description devel
Libraries and header files for Groonga

%prep
%setup -q

%build
export CFLAGS="$RPM_OPT_FLAGS"
export CXXFLAGS="$RPM_OPT_FLAGS"
%configure
make

%install
[ "$RPM_BUILD_ROOT" != "/" ] && %{__rm} -rf $RPM_BUILD_ROOT
%makeinstall

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && %{__rm} -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%defattr(-, root, root)
%doc README AUTHORS COPYING doc/*
%{_libdir}/*.so.*
%{_bindir}/*

%files devel
%defattr(-, root, root)
%{_includedir}/groonga/*
%{_libdir}/*.so
%{_libdir}/*.a
%{_libdir}/*.la
