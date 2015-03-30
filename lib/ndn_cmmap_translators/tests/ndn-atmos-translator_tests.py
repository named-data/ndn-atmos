#!/usr/bin/env python3

# -*- Mode:python; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
#
# Copyright (c) 2015, Colorado State University.
#
# This file is part of ndn-atmos.
#
# ndn-atmos is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later version.
#
# ndn-atmos is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
#
# You should have received copies of the GNU General Public License and GNU Lesser
# General Public License along with ndn-atmos, e.g., in COPYING.md file.  If not, see
# <http://www.gnu.org/licenses/>.
#
# See AUTHORS.md for complete list of ndn-atmos authors and contributors.

import unittest
import inspect
import argparse
import ConfigParser
import sys

sys.path.append("../")
sys.path.append("../../")

import atmos2ndn_translators
from atmos2ndn_translators.parser import parser, user_conf_parser
from netcdf2ndn.translators import core_translator, cam_translator


class TestCommandParser(unittest.TestCase):

    maxDiff = None
    def setUp(self):
        self.parse_file = None
        self.parsed_config = None
        self.test_conf_file = "netcdf2ndn/etc/cesm-0.conf"
        self.user_conf_file = "netcdf2ndn/etc/user_spec_comps.conf"
        self.filename_mapping = ''
        self.parse_file = parser.parse_conf(self.test_conf_file)
        self.parse_user_file = user_conf_parser.parse_user_conf(self.user_conf_file)

#    def CmdArgParse(self):
# don't know how to test

    def TestFilenameMapping(self):
        '''test original filename mapping is fine'''
        parsed = self.parse_file.filename_mapping()
        expected_filename_mapping=['experiment', 'experiment', 'model', 'granularity', 'year', 'month', 'day',
        'time', 'filetype']
        print parsed, expected_filename_mapping
        self.assertEqual(expected_filename_mapping, parsed)

    def TestNDNNameMapping(self):
        '''test ndn name component mapping is fine'''
        parsed = self.parse_file.ndn_name_mapping()
        expected_ndn_name_mapping=['activity', 'subactivity', 'organization', 'ensemble', 'experiment',
        'model', 'granularity', 'start_time']
        self.assertEqual(expected_ndn_name_mapping, parsed)

    def TestSeperatorsMapping(self):
        '''test seperators mapping is parsed fine'''
        parsed = self.parse_file.seperators_mapping()
        expected_seperators_mapping=['_', '.', '-']
        self.assertEqual(expected_seperators_mapping, parsed)

    def TestCompFromDataMapping(self):
        '''test component from data mapping is parsed fine'''
        parsed = self.parse_file.comp_from_data()
        expected_comp_mapping=['activity', 'subactivity', 'organization', 'ensemble', 'granularity', 'start_time']
        self.assertEqual(expected_comp_mapping, parsed)

    def TestParseUserConf(self):
        '''test correct parsing of user defined mappings'''
        parsed = self.parse_user_file.user_defined_mapping()
        print parsed
        expected_mapping = {'organization': ['csu'], 'granularity': [''], 'ensemble': ['test'], 'subactivity': ['atmos'], 'activity': ['gcrm']}
        self.assertEqual(expected_mapping, parsed)

    def TestDoTranslate(self):
        '''test translation function'''
        expected_translated_name = {'orig_name': 'spcesm-ctrl.cam2.h0.1891-01.nc', 'final_ndn_name_str':
        '/gcrm/atmos/csu/test/spcesm-ctrl/cam2/1M/1891-01/'}
        translated_name = core_translator.do_translate('/home/susmit/atmos_filesystem/CESM_sample_output/spcesm-ctrl.cam2.h0.1891-01.nc',
        ['experiment', 'stream', 'granularity', 'start_time', 'filetype'], ['activity', 'subactivity', 'organization','ensemble', 'experiment', 'stream', 'granularity', 'start_time'], ['_', '.'], {'organization': ['csu'], 'granularity': [''],'ensemble': ['test'], 'subactivity': ['atmos'], 'activity': ['gcrm']})
        self.assertEqual(expected_translated_name, translated_name)


if __name__ == '__main__':
        unittest.main()

