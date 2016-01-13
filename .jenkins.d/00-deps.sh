#!/usr/bin/env bash
set -e

JDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source "$JDIR"/util.sh

if has OSX $NODE_LABELS; then
    set -x
    brew update
    brew upgrade
    brew install boost sqlite3 pkg-config mysql jsoncpp libzdb
    brew cleanup
fi

if has Ubuntu $NODE_LABELS; then
    BOOST_PKG=libboost-all-dev
    ZDB_PKG=libzdb-dev
    if has Ubuntu-12.04 $NODE_LABELS; then
        BOOST_PKG=libboost1.48-all-dev
        unset ZDB_PKG
        sudo apt-get update -qq -y
        sudo apt-get -qq -y install autoconf libtool re2c
        pushd /tmp >/dev/null
        sudo rm -Rf libzdb
        git clone https://bitbucket.org/tildeslash/libzdb.git
        pushd libzdb >/dev/null
        git checkout -b 3-1 release-3-1
        ./bootstrap
        ./configure
        make
        sudo make install
        popd >/dev/null
        popd >/dev/null
    fi

    set -x
    sudo apt-get update -qq -y
    sudo apt-get -qq -y install build-essential pkg-config $BOOST_PKG libssl-dev \
                                libcrypto++-dev libsqlite3-dev libmysqlclient-dev \
                                libjsoncpp-dev protobuf-compiler libprotobuf-dev $ZDB_PKG
fi
