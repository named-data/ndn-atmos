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

'''This is the config file parser module.
Input = object with command line parameters.
Output = list of components for different config sections'''
import configparser
import sys, traceback

class ParseConf(object):
    '''parses the name schema file and returns name mappings for translated output'''
    def __init__(self, confName):
        self.confName = confName
        if __debug__:
            print("Config file name: %s" %(self.confName))

        self.filenameMap = []
        self.ndnNameMap = []
        self.seperatorsMap = []
        self.userDefinedConfDir = {}
        self.translator = []

        #initialize the parser
        self.parser = configparser.SafeConfigParser()
        self.parser.optionxform=str
        self.parser.read(self.confName)
        self.fullConf = {}

        #do the mapping
        res = self.getMappings(confName)
        if res is False:
          print("Error getting values from config file")
          raise error.with_traceback(sys.exc_info()[2])


    def _parseConf(self):
        #iterate over them and store the name components in fullConf
        try:
            for sectionName in self.parser.sections():
                self.conf = {}
                for name, value in self.parser.items(sectionName):
                    self.conf[name] = value
                self.fullConf[sectionName] = self.conf
            if __debug__:
                print(self.fullConf)
        except KeyError:
            print("Key %s is not found in config file" %(name))
            print(sys.exc_info()[2])
        except TypeError:
            print("TypeError while parsing config file")
            print(sys.exc_info()[2])
        return self.fullConf

    def _doParsing(self):
        #parser now contain a dictionary with the sections in conf
        # first elements are section and second ones are variables defined in config file
        try:
            self.filenameMap = self.fullConf['Name']['filenameMapping'].replace(" ", "").split(',')
            self.ndnNameMap = self.fullConf['Name']['ndnMapping'].replace(" ", "").split(',')

            # user defined components look like this
            #activity:cmip5, subactivity:atmos, organization:csu, ensemble:r3i1p1
            userDefinedConf = self.fullConf['Name']['userDefinedComps'].replace(" ", "").split(',')
            for item in userDefinedConf:
                key, value = item.split(":")
                self.userDefinedConfDir[key] = [value]
            self.seperatorsMap = self.fullConf['Name']['seperators'].replace(" ", "").split(',')
            #reads which translator to use
            self.translator = self.fullConf['Translator']['translator'].replace(" ", "")
        except KeyError:
            print("Key %s is not found in config file" %(name))
            print(sys.exc_info()[2])
        except TypeError:
            print("TypeError while parsing config file")
            print(sys.exc_info()[2])

    def getMappings(self, confName):
        '''parses the schema file and provides name mappings'''
        fullConf = self._parseConf()
        #if dict is not empty
        if fullConf:
          res = self._doParsing()
          if len(self.filenameMap) == 0 or len(self.ndnNameMap) == 0 or len(self.translator) == 0:
            return False
          else:
            return True
        else:
          return False

