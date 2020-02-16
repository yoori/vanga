%define __product               vanga
%define __inst_root             /opt/foros/%{__product}

Name:    vanga
Version: %{version}
Release: ssv1%{?dist}
Summary: Vanga
License: Commercial
Group:   System Environment/Daemons

BuildRoot: %{_tmppath}/%{name}-buildroot
Source0: vanga-%{version}.tar.gz

BuildRequires: libstdc++
Requires: libstdc++
BuildRequires: glibc-devel
Requires: glibc

BuildRequires: gcc-c++
BuildRequires: cmake
BuildRequires: make >= 1:3.81-20.el6
BuildRequires: flex >= 2.5.35-8.el6
BuildRequires: bison >= 2.4.1

%description

%package -n %{name}-devel
Summary: Vanga (Devel)
Group:   Development/Libraries/C and C++
Requires: %{name} = %{version}
%description -n %{name}-devel

%prep
%setup -q -n vanga-%{version}

%build
mkdir build
pushd build
cmake ..
%{__make} %{_smp_mflags}
popd

%install
mkdir -p %{buildroot}%{__inst_root}

pushd build
make install DESTDIR=%{buildroot}%{__inst_root}
popd

find %{buildroot}%{__inst_root}

%files -n %{name}
%defattr(777,root,root)
%{__inst_root}/bin/*
%defattr(444,root,root)
%{__inst_root}/lib/*.so

%files -n %{name}-devel
%defattr(-,root,root)
%{__inst_root}/include/DTree
%{__inst_root}/include/Gears

%clean
rm -rf %{buildroot}

%changelog
