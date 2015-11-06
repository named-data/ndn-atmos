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

'''this module parsed the command line arguments and return an object with
the data filename and config filename'''
import argparse
import sys, traceback
import os

class InputParser(object):
    '''parse the command line arguments
    * -c or --conf for naming schema files
    * -f is for filename to translate
    * -d if for directory containing the files to translate
    '''

    def __init__(self):
        self.confFilepath = None
        self.dataFilepath = None

    def parse(self):
        '''Parse the command line arguments and get file/directory name
        to translate. Also, get the schema config filename'''

        parser = argparse.ArgumentParser(description='Translates netcdf names\
        to NDN names')
        parser.add_argument("-c", "--conf", required=True, help='full path to\
        name configuration file')

        #the translator either takes a filename or a directory with files
        group = parser.add_mutually_exclusive_group(required=True)
        group.add_argument("-f", "--filename", help='takes a netcdf filename')
        group.add_argument("-d", "--datadir", help='If specified, converts all\
        files in the specified directory')
        args = parser.parse_args()

        #assign values to filename and confFile
        try:
            self.confFilepath = args.conf
            self.dataFilepath = args.filename or args.datadir

            #check if config file exists
            confFile = open(self.confFilepath)
            confFile.close()

            #check if data directory or datafile exists
            if args.filename is not None:
                dataF = open(args.filename)
                dataF.close()

            #if datadir does not exists, raise exception
            elif args.datadir is not None and os.path.isdir(args.datadir) is False:
                    raise Exception("No such directory '%s'" %(args.datadir))

        except (IOError, OSError):
            traceback.print_exc(file=sys.stdout)
            sys.exit(1)
        return 0
