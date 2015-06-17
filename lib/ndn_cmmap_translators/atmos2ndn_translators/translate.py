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

import sys, traceback
import configparser
import re
import netCDF4
import glob
import os
from ndn_cmmap_translators.atmos2ndn_parser import cmd_arg_parser
from ndn_cmmap_translators.atmos2ndn_parser import conf_file_parser
from . import cmip5_translator

def translate(parsedConfig, dataFilepath):
    '''translate module is called by wrapper functions
    with file/directory name and returns NDN name'''

    translatedFileNames = []
    filenameMap = parsedConfig.filenameMap
    ndnNameMap = parsedConfig.ndnNameMap
    seperatorsMap = parsedConfig.seperatorsMap
    userDefinedMapping =  parsedConfig.userDefinedConfDir
    fullPath = dataFilepath
    translator= parsedConfig.translator

    if __debug__:
        print("In translate")
        print("Translator=", translator)

    translatorFunction = None
    if translator == 'cmip5_translator':
        translatorFunction =  cmip5_translator.Cmip5NameTranslator
    else:
        raise RuntimeError("Error: Invalid translator specified by user. \
        Choice is 'cmip5_translator'")

    if os.path.isdir(fullPath) is True:
        #we have been given a directory, walk it
        for root, subdirs, files in os.walk(fullPath):
            if __debug__:
                print('='*60)
                print('Working on  = ' + root)
                print('='*60)

            for fileName in files:
                if __debug__:
                    print("fileName, filenameMap, ndnNameMap, \
                    seperatorsMap, userDefinedMapping", fileName, \
                    filenameMap, ndnNameMap, seperatorsMap, \
                    userDefinedMapping)
                try:
                    #this is where translation happens
                    if os.path.isfile(os.path.join(root, fileName)):
                        if __debug__:
                            print("-"*80)
                            print("Original File Name: %s \n" %(fileName))
                        translateObj = translatorFunction()
                        res = translateObj.translate(os.path.join(root, fileName), parsedConfig)
                        if res:
                            if __debug__:
                                print("NDN Name: %s\n" %(translateObj.finalName))
                                print("-"*80)
                            translatedFileNames.append(translateObj.finalName)

                except Exception as err:
                    traceback.print_exc(file=sys.stdout)
                    #don't stop for a garbled file
                    pass
        return translatedFileNames
    #else work only on the given file
    else:
        try:
            if __debug__:
                print("fileName, filenameMap, ndnNameMap, seperatorsMap, userDefinedMapping",\
                fullPath, filenameMap, ndnNameMap, seperatorsMap, userDefinedMapping)
            root = os.path.dirname(fullPath)
            fileName = os.path.split(fullPath)[-1]
            if __debug__:
                print("-"*80)
                print("Original File Name: %s \n" %(fileName))
            translateObj = translatorFunction()
            res = translateObj.translate(fullPath, parsedConfig)
            if res:
                if __debug__:
                    print("NDN Name: %s\n" %(translateObj.finalName))
                    print("-"*80)
                return [translateObj.finalName]

        except Exception as err:
            traceback.print_exc(file=sys.stdout)
            return None
