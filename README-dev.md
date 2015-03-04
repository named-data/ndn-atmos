Notes for ndn-atmos developers
==============================

Requirements
------------

Contributions to ndn-atmos must be licensed under GPL 3.0 or compatible license.  If you are
choosing GPL 3.0, please use the following license boilerplate in all `.hpp` and `.cpp`
files:

Include the following license boilerplate into all `.hpp` and `.cpp` files:

    /* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
    /**
     * Copyright (c) [Year(s)],  [Copyright Holder(s)].
     *
     * This file is part of ndn-atmos.
     *
     * ndn-atmos is free software: you can redistribute it and/or modify it under the terms
     * of the GNU General Public License as published by the Free Software Foundation,
     * either version 3 of the License, or (at your option) any later version.
     *
     * ndn-atmos is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
     * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     * PURPOSE.  See the GNU General Public License for more details.
     *
     * You should have received a copy of the GNU General Public License along with
     * ndn-atmos, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
     *
     * See AUTHORS.md for complete list of ndn-atmos authors and contributors.
     */


Recommendations
---------------

ndn-atmos code is subject to NFD [code style]
(http://redmine.named-data.net/projects/nfd/wiki/CodeStyle).


Running unit-tests
------------------

To run unit tests, ndn-atmos needs to be configured and build with unit test support:

    ./waf configure --with-tests
    ./waf

The simplest way to run tests, is just to run the compiled binary without any parameters:

    # Run ndn-atmos catalog unit tests
    ./build/catalog/unit-tests

However, [Boost.Test framework](http://www.boost.org/doc/libs/1_48_0/libs/test/doc/html/)
is very flexible and allows a number of run-time customization of what tests should be run.
For example, it is possible to choose to run only a specific test suite, only a specific
test case within a suite, or specific test cases within specific test suites.

By default, Boost.Test framework will produce verbose output only when a test case fails.
If it is desired to see verbose output (result of each test assertion), add `-l all`
option to `./build/catalog/unit-tests` command.  To see test progress, you can use `-l test_suite`
or `-p` to show progress bar.

There are many more command line options available, information about
which can be obtained either from the command line using `--help`
switch, or online on [Boost.Test library](http://www.boost.org/doc/libs/1_48_0/libs/test/doc/html/)
website.
