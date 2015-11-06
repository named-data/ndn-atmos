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

'''this is the xrootd translator module'''

import re
import sys
import os
import traceback

class HepNameTranslator(object):
    def __init__(self):
       self.finalName = '/'
       self.fullFilePath = '';

    def translate(self, fullPath, parsedConfig):
      '''translates xrootd filename to ndn name. For example,
      /tmp/store/mc/RunIISpring15DR74/ZprimeToWW_narrow_M-1800_13TeV-madgraph/AODSIM/
      Asympt25ns_MCRUN2_74_V9-v1/70000. Upto mc/ will be removed and the rest translated.'''

      fileName = fullPath.split('/')[-1]
      if "mc" in fullPath:
          index_for_removal = fullPath.index("mc")
      self.fullFilePath = fullPath[index_for_removal + 3:]
      filenameMap = parsedConfig.filenameMap
      ndnNameMap = parsedConfig.ndnNameMap
      seperatorsMap = parsedConfig.seperatorsMap
      confName = parsedConfig.confName
      fileCompDict = {}
      metadataCompDict = {}


      #split the input filename, we will use this for ndn name create a regexp
      if __debug__:
          print("Full Path:", self.fullFilePath)

      combinedSeperators = '|'.join(map(re.escape, seperatorsMap))
      splitFileName = re.split(combinedSeperators, self.fullFilePath)
      if __debug__:
          print("Split file name", splitFileName)

      #check if input file maps to conf mapping
      if len(splitFileName) != len(filenameMap):
          print("Error: Input file %s does not match schema described in configuration file in" \
          "%s, skipping" %(fileName, confName))

      #create a directory mapping NDN components to filename components
      for item in range(len(ndnNameMap)):
          if filenameMap[item].strip() in fileCompDict:
              fileCompDict[filenameMap[item].strip()] = fileCompDict[filenameMap[item].strip()] + '_' + splitFileName[item]
          else:
              try:
                  fileCompDict[filenameMap[item].strip()] = splitFileName[item]
              except IndexError:
                  return False
      #create the name
      #for item in sorted(fileCompDict):
      for item in ndnNameMap:
          print("Item: ", item)
          self.finalName += fileCompDict[item] + '/'
      #if final name has any spaces, remove them
      self.finalName = self.finalName.replace(" ", "")
      self.finalName = self.finalName[:-1]
      if __debug__:
          print (self.finalName, self.fullFilePath)
      return True
