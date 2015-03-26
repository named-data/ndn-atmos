#!/usr/bin/env bash
set -x
set -e

# Prepare environment
sudo rm -Rf ~/.ndn

# Run unit tests
./build/catalog/unit-tests -l test_suite
