#!/usr/bin/env bash
set -x
set -e

# Prepare environment
sudo rm -Rf ~/.ndn

export LD_LIBRARY_PATH=/usr/local/lib/:$LD_LIBRARY_PATH

# Run unit tests
./build/catalog/unit-tests -l test_suite
