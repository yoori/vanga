%global python3_pkgversion 3.12

Name: python3-vanga
Version: %{vanga_version}
Release: 1%{?dist}
Summary: Vanga Python tree training library for Python 3.12
License: GPL-3.0-only
Source0: vanga-%{version}.tar.gz
BuildArch: noarch
AutoReqProv: no
BuildRequires: python%{python3_pkgversion}-rpm-macros
BuildRequires: python%{python3_pkgversion}-devel
BuildRequires: pyproject-rpm-macros
Requires: python3.12
Requires: python3.12dist(torch) >= 2.2
Requires: python3.12dist(torch) < 3
Requires: python3.12-pytorch-tabular = 1.2.0
Requires: python3.12-catboost >= 1.2.8
Requires: python3.12-catboost < 2
Requires: python3.12-numpy >= 1.24
Requires: python3.12-numpy < 3
Requires: python3.12-scipy >= 1.11
Requires: python3.12-scikit-learn >= 1.4
Requires: python3.12-joblib >= 1.3
Requires: python3.12-requests >= 2.31
Provides: python3.12dist(vanga) = %{version}

%description
Python Dataset adapters, differentiable NODE tree models, trainers and export
tools. Binary labels and probability targets share one float contract.
Dependencies are supplied by their own RPMs. Contains no C++ Vanga binaries,
PyTorch/CUDA libraries, model weights or private Python environment.

%prep
%setup -q -n vanga-%{version}

%generate_buildrequires
%pyproject_buildrequires -R

%build
%pyproject_wheel

%install
%pyproject_install
%pyproject_save_files -L vanga

%files -f %{pyproject_files}
%license LICENSE
%doc README.rst
%{_bindir}/vanga-train
%{_bindir}/vanga-adaptive-train
%{_bindir}/vanga-search
