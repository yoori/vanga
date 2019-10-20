#!/bin/bash

# create folders for RPM build environment
mkdir -vp  `rpm -E '%_tmppath %_rpmdir %_builddir %_sourcedir %_specdir %_srcrpmdir'`

VERSION=1.0.0.2
GEARS_SOURCE_ROOT=$HOME/projects/vanga/gears
VANGA_SOURCE_ROOT=$HOME/projects/vanga/vanga

RES_RPMS=build/RPMS/
RPM_SOURCES_DIR=`rpm -E '%_sourcedir'`
RPM_TMP_DIR=`rpm -E '%_tmppath'`

# cleanup target folders if exists
rm -rf $RPM_TMP_DIR/vanga-$VERSION 2>/dev/null
rm -rf $RPM_SOURCES_DIR/vanga-$VERSION.tar.gz 2>/dev/null
rm -rf $RES_RPMS/vanga-*.rpm 2>/dev/null

mkdir -p $RES_RPMS
mkdir -p $RPM_SOURCES_DIR || { echo "can't create RPM sources dir: $RPM_SOURCES_DIR" ; exit 1 ; }
mkdir -p $RPM_TMP_DIR/vanga-$VERSION/vanga || { echo "can't create RPM temporary dir: $RPM_TMP_DIR" ; exit 1 ; }
mkdir -p $RPM_TMP_DIR/vanga-$VERSION/gears || { echo "can't create RPM temporary dir: $RPM_TMP_DIR" ; exit 1 ; }

# skip hidden files (git,svn) and build dir
rsync -r --exclude='.*' --exclude='/build' $VANGA_SOURCE_ROOT/ $RPM_TMP_DIR/vanga-$VERSION/vanga
rsync -r --exclude='.*' --exclude='/build' $GEARS_SOURCE_ROOT/ $RPM_TMP_DIR/vanga-$VERSION/gears

# create tarball for rpmbuild
pushd .
cd $RPM_TMP_DIR
tar -czf $RPM_SOURCES_DIR/vanga-$VERSION.tar.gz vanga-$VERSION
popd

rpmbuild -ba $VANGA_SOURCE_ROOT/RPM/SPECS/vanga.spec || { echo "rpmbuild failed" ; exit 1 ; }

# cleanup temporary folders and tarball
rm -rf $RPM_TMP_DIR/vanga-$VERSION 2>/dev/null
rm -rf $RPM_SOURCES_DIR/vanga-$VERSION.tar.gz 2>/dev/null

SOURCE_RPM_FOLDER=`rpm -E '%_srcrpmdir'`
BIN_RPM_FOLDER=`rpm -E '%_rpmdir/%_arch'`

cp $SOURCE_RPM_FOLDER/vanga-$VERSION-ssv1.el6.src.rpm $RES_RPMS/
cp $BIN_RPM_FOLDER/vanga-debuginfo-$VERSION-ssv1.el6.x86_64.rpm $RES_RPMS/
cp $BIN_RPM_FOLDER/vanga-$VERSION-ssv1.el6.x86_64.rpm $RES_RPMS/

echo 'RPM packages successfully built: '
echo "Source RPM: $RES_RPMS/vanga-$VERSION-ssv1.el6.src.rpm"
echo "Debug Info RPM: $RES_RPMS/vanga-debuginfo-$VERSION-ssv1.el6.x86_64.rpm"
echo "Install RPM: $RES_RPMS/vanga-$VERSION-ssv1.el6.x86_64.rpm"
