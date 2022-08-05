# Dockerfile - docker/pinentry-qt4
# Copyright (C) 2022 g10 Code GmbH
#
# Software engineering by Ingo Klöcker <dev@ingo-kloecker.de>
#
# This file is part of GnuPG.
#
# Gpg4win is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Gpg4win is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <https://www.gnu.org/licenses/>.
#
# SPDX-License-Identifier: GPL-2.0+

FROM centos:7

# Import the CentOS 7 package signing key after verifying its fingerprint.
# Then add the software collections repository from the CentOS SCLo SIG and
# import its package signing key after verifying the fingerprint.
# Ditto for the EPEL 7 package signing key.
RUN test $(gpg --with-colons --with-fingerprint < /etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-7 | grep ^pub: | wc -l) = 1 \
    && gpg --with-colons --with-fingerprint < /etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-7 | grep -q ^fpr:::::::::6341AB2753D78A78A7C27BB124C6A8A7F4A80EB5 \
    && rpmkeys --import /etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-7 \
    && yum -y update \
    && rm -rf /root/.gnupg

RUN yum -y install \
        automake \
        binutils \
        bzip2 \
        gcc \
        gcc-c++ \
        libassuan-devel \
        make \
        qt-devel

# pinentry requires libgpg-error-1.16 (CentOS 7 includes 1.12)
RUN mkdir /tmp/libgpg-error \
    && cd /tmp/libgpg-error \
    && curl --remote-name https://gnupg.org/ftp/gcrypt/libgpg-error/libgpg-error-1.16.tar.gz \
    && tar -xzf libgpg-error-1.16.tar.gz \
    && cd libgpg-error-1.16 \
    && ./configure \
    && make install \
    && cd \
    && rm -rf /tmp/libgpg-error
