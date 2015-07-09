#!/usr/bin/env bash
set -x
set -e

pushd /tmp >/dev/null

INSTALLED_VERSION=$((cd ChronoSync && git rev-parse HEAD) 2>/dev/null || echo NONE)

sudo rm -Rf ChronoSync-latest
git clone --depth 1 git://github.com/named-data/ChronoSync ChronoSync-latest
LATEST_VERSION=$((cd ChronoSync-latest && git rev-parse HEAD) 2>/dev/null || echo UNKNOWN)

if [[ $INSTALLED_VERSION != $LATEST_VERSION ]]; then
    sudo rm -Rf ChronoSync
    mv ChronoSync-latest ChronoSync
else
    sudo rm -Rf ChronoSync-latest
fi

sudo rm -Rf /usr/local/include/ChronoSync
sudo rm -f /usr/local/lib/libChronoSync*
sudo rm -f /usr/local/lib/pkgconfig/ChronoSync*

pushd ChronoSync >/dev/null

export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:\
/usr/local/lib32/pkgconfig:\
/usr/local/lib64/pkgconfig

sudo ldconfig || true
./waf configure -j1 --color=yes
./waf -j1 --color=yes
sudo ./waf install -j1 --color=yes

popd >/dev/null
popd >/dev/null
