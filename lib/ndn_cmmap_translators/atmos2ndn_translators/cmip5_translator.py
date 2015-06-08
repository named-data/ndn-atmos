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

'''this is the cmip5 translator module'''

import re
import sys
import netCDF4
import os
import traceback

class Cmip5NameTranslator(object):
    def __init__(self):
       self.finalName = '/'

    def checkGranularity(self, ncFile, fullFilePath):
      '''check the granularity/time frequency of values from metatdata
      and crosscheck by calculating it from data'''

      #get first two values of time variable
      time = ncFile.variables['time'][0:2]

      #get time unit
      unit = ncFile.variables['time'].units

      #get granularity in metadata
      metadataGranularity = getattr(ncFile, 'frequency')

      #if day appears in unit, the index is based on day
      if 'day' in unit:
          timeFromData = str(int((time[1] - time[0])*24)) + 'hr'

      #does this ever happen? Check
      else:
          print("Warning: Data not in day granularity")
          return False

      #if granularity from metadata and data does not match, error, else keep record
      if timeFromData != metadataGranularity:
          print("Error in file %s. Name component 'time' is '%s' in data and '%s' in  metadata "
          %(fullFilePath, timeFromData, metadataGranularity))
          return False

      else:
          if __debug__:
              print("file, time from data , time from metadata," ,ncFile, timeFromData,\
              metadataGranularity)
          return timeFromData

    def translate(self, fullPath, parsedConfig):
      '''translates netcdf filename to ndn name. For example,
       psl_6hrPlev_MIROC5_historical_r3i1p1_1968010100-1968123118.nc  becomes
      /CMIP5/output/MIROC/MIROC5/historical/6hr/atmos/psl/r1i1p1/1968010100-1968123118/'''

      fileName = fullPath.split('/')[-1]
      fullFilePath = fullPath
      filenameMap = parsedConfig.filenameMap
      ndnNameMap = parsedConfig.ndnNameMap
      seperatorsMap = parsedConfig.seperatorsMap
      confName = parsedConfig.confName
      fileCompDict = {}
      metadataCompDict = {}


      #split the input filename, we will use this for ndn name create a regexp
      combinedSeperators = '|'.join(map(re.escape, seperatorsMap))
      splitFileName = re.split(combinedSeperators, fileName)
      if __debug__:
          print("Split file name", splitFileName)

      #check if input file maps to conf mapping
      if len(splitFileName) != len(filenameMap):
          print("Error: Input file %s does not match schema described in configuration file in" \
          "%s, skipping" %(fileName, confName))


      #create a directory mapping DRS components to filename components
      for item in range(len(filenameMap)):
          fileCompDict[filenameMap[item].strip()] = splitFileName[item]
      if __debug__:
          print(fileCompDict)

      #open the file and read the metadata
      try:
        with netCDF4.Dataset(fullFilePath, 'r') as ncFile:
          #for each item in ndnMapping list, try getting it from metadata
          for nameComp in ndnNameMap:
              try:
                  metadataCompDict[nameComp.strip()] = getattr(ncFile, nameComp.strip())
              except:
                  if __debug__:
                      print("'%s' not found in metadata" %(nameComp.strip()))
                  pass
          if __debug__:
              print(metadataCompDict)
              print(ndnNameMap)
          for item in ndnNameMap:
              if (item == 'frequency'):
                  #get it from data, if frequency differs in metadata and data, throw error
                  freq = self.checkGranularity(ncFile, fullFilePath)
                  if freq is False:
                      self.finalName = ''
                      return False
              else:
                  if item in metadataCompDict:
                      if item in metadataCompDict and item in fileCompDict:
                         if metadataCompDict[item] != fileCompDict[item]:
                              print("Error in file %s" %(fullFilePath))
                              print("Name component '%s' is '%s' in name and '%s' in  metadata " \
                              %(item, fileCompDict[item], metadataCompDict[item]))
                              self.finalName = ''
                              return False
              #create the name
              if item in fileCompDict:
                  self.finalName += fileCompDict[item] + '/'
              else:
                  try:
                      self.finalName += metadataCompDict[item] + '/'
                  except KeyError:
                      print("Error: %s does not have '%s' component needed for translation"
                      %(fileName, item))
                      traceback.print_exc(file=sys.stdout)
                      return False
      except (RuntimeError, IOError):
          print("%s is not a valid netCDF file" %(fullFilePath))
          traceback.print_exc(file=sys.stdout)
          return False
      #if final name has any spaces, remove them
      self.finalName = self.finalName.replace(" ", "")
      return True
