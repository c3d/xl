Name:		xl
Version:	0.6.1
Release:	1%{?dist}
Summary:	An extensible programming language based on parse-tree rewrites
License:	GPLv3+
Url:		https://github.com/c3d/%{name}
Source:		https://github.com/c3d/%{name}/archive/tao-%{version}/%{name}-%{version}.tar.gz
BuildRequires:	make >= 3.82
BuildRequires:  make-it-quick >= 0.2
BuildRequires:  gcc-c++ >= 7.0
BuildRequires:  llvm35-devel

%description
XL is an extensible programming language based on parse-tree rewrites.
It is used as the base language for Tao3D

%package devel
Summary:        The XL programming language implemented as a library
%description devel
XL programming language implemented as a library for use in other programs

%prep
%autosetup -n %{name}-%{version}

%build
%make_build opt COLORIZE=

%check
%make_build test COLORIZE=

%install
%make install COLORIZE=

%files
%{_bindir}/xl
%license COPYING
%doc README.md

%files devel
%{_libdir}/lib%{name}.so
%{_libdir}/lib%{name}.so.*
%{_includedir}/%{name}/*
%{_datadir}/pkgconfig/%{name}.pc

%changelog
