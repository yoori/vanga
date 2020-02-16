#!/bin/bash

VERSION=$1
UNIXCOMMONSDIR=$2

# script require 'sudo rpm' for install RPM packages
# create build/RPMS folder - all built packages will be duplicated here
RES_TMP=build/TMP/
RES_RPMS=build/RPMS/
rm -rf "$RES_TMP"

mkdir -p $RES_TMP
mkdir -p $RES_RPMS

# download and install packages required for build
yum -y install spectool yum-utils rpmdevtools redhat-rpm-config rpm-build epel-rpm-macros || \
  { echo "can't install RPM build packages" >&2 ; exit 1 ; }

# create folders for RPM build environment
mkdir -vp  `rpm -E '%_tmppath %_rpmdir %_builddir %_sourcedir %_specdir %_srcrpmdir %_rpmdir/%_arch'`

BIN_RPM_FOLDER=`rpm -E '%_rpmdir/%_arch'`
SPEC_FILE=`rpm -E %_specdir`/server.spec

rm -f "$SPEC_FILE"

# download sources from git tag, pack and pass to sources dir
echo "to download sources from dev branch"
RPM_SOURCES_DIR=`rpm -E %_sourcedir`

cp RPM/SPECS/vanga.spec "$SPEC_FILE"

#git archive --format tar.gz --output "$RPM_SOURCES_DIR/stream-dsp-$VERSION.tar.gz" dev
rm -rf "$RES_TMP/vanga-$VERSION"
mkdir "$RES_TMP/"
rsync -av --exclude '/build' --exclude ".git" --exclude ".svn" ./ "$RES_TMP/vanga-$VERSION"
rm -f "$RPM_SOURCES_DIR/vanga-$VERSION.tar.gz"
pushd "$RES_TMP"
#echo 'tar -czvf '"$RPM_SOURCES_DIR/foros-server-$VERSION.tar.gz"' '"$RES_TMP/foros-server-$VERSION"
tar -czvf "$RPM_SOURCES_DIR/vanga-$VERSION.tar.gz" "vanga-$VERSION"
popd

#tar -czvf "$RPM_SOURCES_DIR/foros-server-$VERSION.tar.gz" `ls -1 -f | grep -v build | grep -v -E '^[.]'` --exclude ".git" --exclude ".svn"
#tar -czvf  "$RPM_SOURCES_DIR/stream-dsp-$VERSION.tar.gz" *  --exclude ".git" --exclude ".svn" --exclude "./build"

#popd
#popd


#yum-builddep -y --define "__server_type central" --define "version $VERSION" "$SPEC_FILE" || { echo "can't install build requirements" >&2 ; exit 1 ; }

rpmbuild --force -ba --define "version $VERSION" "$SPEC_FILE" || \
  { echo "can't build RPM" >&2 ; exit 1 ; }

cp $BIN_RPM_FOLDER/vanga*.rpm $RES_RPMS/

