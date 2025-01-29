#! /bin/bash

set -e

if [ "$1" = "--devel" ]; then
    devmode=
else
    devmode=1
fi

sourcedir=$(cd $(dirname $0)/..; pwd)

if [ -z "$devmode" ]; then
    buildroot="/tmp/build-pinentry-qt4.$(id -un).d"
    [ -d "$buildroot" ] || mkdir "$buildroot"
else
    buildroot=$(mktemp -d --tmpdir build-pinentry-qt4.XXXXXXXXXX)
fi
echo Using ${buildroot}


docker run -it --rm --user "$(id -u):$(id -g)" \
    --volume ${sourcedir}:/src \
    --volume ${buildroot}:/build \
    g10-build-pinentry-qt4:centos7

echo "You can find the build in ${buildroot} (if the build succeeded)."

# Unpack the tarball of pinentry and then ./configure and make to test
# building pinentry for Qt4.
