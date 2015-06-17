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

'''Translates a netcdf filename to a NDN name'''

from ndn_cmmap_translators.atmos2ndn_parser import conf_file_parser, cmd_arg_parser
from ndn_cmmap_translators.atmos2ndn_translators import translate

def argsForTranslation(dataFilepath, configPath):
    '''this module does the actual translation calls'''
    if __debug__:
        print("config file '%s', data path '%s'" %(configPath, dataFilepath))

    #pass the config file to parser module, which will return and object
    #with all the mappings. The mappings tell us which name component
    #comes from where and what should be the order of the components

    #library would throw exceptions, if any
    parsedConfig = conf_file_parser.ParseConf(configPath)


    #do the translation
    ndnNames = translate.translate(parsedConfig, dataFilepath)
    if __debug__:
      print("NDN names in atmos_translate module: %s" %(ndnNames))
    return ndnNames


def main():

    '''This main is for debug only, run with the debug flag on.
    Otherwise call argsForTranslation from a wrapper function'''

    userArgs = cmd_arg_parser.InputParser()
    userArgs.parse()
    configFile = userArgs.confFilepath
    if __debug__:
        print("config file '%s', data path '%s'" %(configFile, userArgs.dataFilepath))

    #call the translator module
    ndnNames = argsForTranslation(userArgs.dataFilepath, configFile)


if __name__ == '__main__':
    main()
