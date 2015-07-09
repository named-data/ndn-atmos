# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

"""
Copyright (c) 2014-2015,  Regents of the University of California,
                          Arizona Board of Regents,
                          Colorado State University,
                          University Pierre & Marie Curie, Sorbonne University,
                          Washington University in St. Louis,
                          Beijing Institute of Technology

This file is part of NFD (Named Data Networking Forwarding Daemon).
See AUTHORS.md for complete list of NFD authors and contributors.

NFD is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
"""


VERSION='0.1'
APPNAME='ndn-atmos'

from waflib import Configure, Utils, Logs, Context
import os

def options(opt):
    opt.load(['compiler_cxx', 'gnu_dirs'])

    opt.load(['default-compiler-flags', 'boost'],
              tooldir=['.waf-tools'])

    opt = opt.add_option_group('ndn-atmos Options')

    opt.add_option('--with-tests', action='store_true', default=False,
                   dest='with_tests', help='''build unit tests''')

def configure(conf):
    conf.load(['compiler_cxx', 'default-compiler-flags', 'boost', 'gnu_dirs'])

    conf.find_program('bash', var='BASH')

    if not os.environ.has_key('PKG_CONFIG_PATH'):
        os.environ['PKG_CONFIG_PATH'] = ':'.join([
            '/usr/local/lib/pkgconfig',
            '/usr/local/lib32/pkgconfig',
            '/usr/local/lib64/pkgconfig',
            '/opt/local/lib/pkgconfig'])

    conf.check_cfg(package='libndn-cxx', args=['--cflags', '--libs'],
                   uselib_store='NDN_CXX', mandatory=True)

    conf.check_cfg(package='ChronoSync', args=['ChronoSync >= 0.1', '--cflags', '--libs'],
                   uselib_store='SYNC', mandatory=True)

    conf.check_cfg(package='jsoncpp', args=['--cflags', '--libs'],
                   uselib_store='JSON', mandatory=True)

    conf.check_cfg(path='mysql_config', args=['--cflags', '--libs'], package='',
                   uselib_store='MYSQL', mandatory=True)

    boost_libs = 'system random thread filesystem'

    if conf.options.with_tests:
        conf.env['WITH_TESTS'] = 1
        conf.define('WITH_TESTS', 1);
        boost_libs += ' unit_test_framework'

    conf.check_boost(lib=boost_libs, mandatory=True)
    if conf.env.BOOST_VERSION_NUMBER < 104800:
        Logs.error("Minimum required boost version is 1.48.0")
        Logs.error("Please upgrade your distribution or install custom boost libraries" +
                    " (http://redmine.named-data.net/projects/nfd/wiki/Boost_FAQ)")
        return

    conf.define('DEFAULT_CONFIG_FILE', '%s/ndn-atmos/catalog.conf' % conf.env['SYSCONFDIR'])

    conf.write_config_header('config.hpp')

def build (bld):
    ndn_atmos_objects = bld(
        target='ndn_atmos_objects',
        name='ndn_atmos_objects',
        features='cxx',
        source=bld.path.ant_glob(['catalog/src/**/*.cpp'],
                                 excl=['catalog/src/main.cpp']),
        use='NDN_CXX BOOST JSON MYSQL SYNC',
        includes='catalog/src .',
        export_includes='catalog/src .'
    )

    bld(
        target='bin/atmos-catalog',
        features='cxx cxxprogram',
        source='catalog/src/main.cpp',
        use='ndn_atmos_objects'
    )

    bld.recurse('tools')

    # Catalog unit tests
    if bld.env['WITH_TESTS']:
        bld.recurse('catalog/tests')

    bld(
        features="subst",
        source='catalog.conf.sample.in',
        target='catalog.conf.sample',
        install_path="${SYSCONFDIR}/ndn-atmos"
    )
