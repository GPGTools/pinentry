#! /bin/bash

cd $(dirname $0)

docker build -t g10-build-pinentry-qt4:centos7 pinentry-qt4
