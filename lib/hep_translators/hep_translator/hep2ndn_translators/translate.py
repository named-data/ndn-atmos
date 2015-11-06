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

'''Translates a xrootd filename to a NDN name'''

import sys, traceback
import configparser
import re
import glob
import os
from . import hep_translator
#from hep_translator.hep2ndn_parser import conf_file_parser, cmd_arg_parser

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
    if translator == 'hep_translator':
        translatorFunction =  hep_translator.HepNameTranslator
    else:
        raise RuntimeError("Error: Invalid translator specified by user. Choice is 'hep_translator'")

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
                            translatedFileNames.append([translateObj.fullFilePath,translateObj.finalName])
                except Exception as err:
                    traceback.print_exc(file=sys.stdout)
                    #don't stop for a garbled file
                    pass
        print("Traslated filename: ", translatedFileNames)
        return translatedFileNames
