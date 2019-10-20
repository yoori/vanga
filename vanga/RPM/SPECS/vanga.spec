%if %{?__target:1}%{!?__target:0}
%define _target_cpu             %{__target}
%endif

%define __product                    vanga
%define __inst_root                  /opt/foros/%{__product}
%define __osbe_build_dir             .
%define __cmake_build_dir            build

Name:    vanga
Version: 1.0.0.1
Release: ssv1%{?dist}
Summary: Vanga ML library
License: Commercial
Group:   System Environment/Daemons
Source0: %{name}-%{version}.tar.gz

BuildRoot: %{_tmppath}/%{name}-buildroot

BuildRequires: libstdc++ = 4.4.7-18.el6
Requires: libstdc++ = 4.4.7-18.el6
BuildRequires: glibc-devel
Requires: glibc
BuildRequires: gcc-c++
BuildRequires: autoconf >= 2.63
BuildRequires: OpenSBE >= 1.0.54
BuildRequires: OpenSBE-defs >= 1.0.24.0
BuildRequires: bison >= 2.4.1

%description
Vanga ML library

%prep
%setup -q -n %{name}-%{version}

mkdir -p gears/%{__osbe_build_dir}
cpp -DOS_%{_os_release} -DARCH_%{_target_cpu} -DARCH_FLAGS='%{__arch_flags}' \
    gears/default.config.t > gears/%{__osbe_build_dir}/default.config

mkdir -p %{__product}/%{__osbe_build_dir}
cpp -DOS_%{_os_release} -DARCH_%{_target_cpu} -DARCH_FLAGS='%{__arch_flags}' \
    -DGEARS_ROOT=%{_builddir}/vanga-%{version}/gears/%{__osbe_build_dir} \
    -DGEARS_INCLUDE=src \
    -DGEARS_DEF=%{_builddir}/%{name}-%{version}/gears/libdefs \
    %{__product}/default.config.t > %{__product}/%{__osbe_build_dir}/default.config

%build
%ifnarch noarch
pushd gears
osbe
product_root=`pwd`
cd %{__osbe_build_dir}
${product_root}/configure --enable-no-questions --enable-guess-location=no --prefix=%{__inst_root}
%{__make} %{_smp_mflags}
popd

pushd %{__product}
osbe
product_root=`pwd`
cd %{__osbe_build_dir}
${product_root}/configure --enable-no-questions --enable-guess-location=no --prefix=%{__inst_root}
%{__make} %{_smp_mflags}
popd
%endif

%install

rm -rf %{buildroot}
mkdir -p %{buildroot}%{__inst_root}

%ifnarch noarch
rm -rf %{buildroot}%{__inst_root}/include
rm -rf `find %{buildroot}%{__inst_root}/lib -type f -name '*.a'`

make -C gears/%{__osbe_build_dir} install destdir=%{buildroot}
make -C %{__product}/%{__osbe_build_dir} install destdir=%{buildroot}
%endif

%files
%defattr(-, root, root)
%__inst_root/bin/
%__inst_root/lib/
%__inst_root/include/

%clean
rm -rf ${RPM_BUILD_ROOT}

%changelog
