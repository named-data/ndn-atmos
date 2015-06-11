#!/usr/bin/env bash
set -e

JDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source "$JDIR"/util.sh

if has OSX $NODE_LABELS; then
    set -x
    brew update
    brew upgrade
    brew install boost sqlite3 pkg-config mysql jsoncpp
    brew cleanup
fi

if has Ubuntu $NODE_LABELS; then
    BOOST_PKG=libboost-all-dev
    if has Ubuntu-12.04 $NODE_LABELS; then
        BOOST_PKG=libboost1.48-all-dev
    fi

    set -x
    sudo apt-get update -qq -y
    sudo apt-get -qq -y install build-essential pkg-config $BOOST_PKG libssl-dev \
                                libcrypto++-dev libsqlite3-dev libmysqlclient-dev \
                                libjsoncpp-dev protobuf-compiler libprotobuf-dev
fi
