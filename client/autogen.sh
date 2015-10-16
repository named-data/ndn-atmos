#!/bin/sh

if [ ! hash git 2>/dev/null ]; then
  echo "git is required for this repo to function properly! Please install it.";
  exit 1;
fi

git submodule init ndn-js && git submodule update
cd ndn-js && ./waf configure && ./waf && cd ..

if [ ! hash npm 2>/dev/null ]; then
  echo "npm is required to build the production site. Only the dev site will be available.";

else

  if [ ! hash gulp 2>/dev/null ]; then
    npm install -g gulp
    npm install
  fi

  npm upgrade

  gulp

fi

