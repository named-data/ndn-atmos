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

'''This module is for inserting NDN names in underlying database'''

import mysql.connector as sql
from mysql.connector import errorcode
import sys
import getpass
import hashlib
from hep_translator import hep_translator_module

def createTables(cursor):

    #name format: <campaign>/<physics process>/<event content>/<calibration version>/<production directory>/<production file hash>.root

  TABLES = {}
  TABLES['hep_data'] = (
    "CREATE TABLE `hep_data` ("
    "  `id` int(100) AUTO_INCREMENT NOT NULL,"
    "  `sha256` varchar(64) UNIQUE NOT NULL,"
    "  `name` varchar(1000) NOT NULL,"
    "  `campaign` varchar(100) NOT NULL,"
    "  `physics_process` varchar(100) NOT NULL,"
    "  `event_content` varchar(100) NOT NULL,"
    "  `calibration_version` varchar(100) NOT NULL,"
    "  `production_dir` varchar(100) NOT NULL,"
    "  `production_file_hash` varchar(100) NOT NULL,"
    "  PRIMARY KEY (`id`)"
    ") ENGINE=InnoDB")

  #check if tables exist, if not create them
  #create tables per schema in the wiki
  for tableName, query in TABLES.items():
    try:
      print("Creating table {}: ".format(tableName))
      cursor.execute(query)
      print("Created table {}: ".format(tableName))
      return True
    except sql.Error as err:
      if err.errno == errorcode.ER_TABLE_EXISTS_ERROR:
        print("Table already exists: %s " %(tableName))
        return True
      else:
        print("Failed to create table: %s" %(err.msg))
        return False

def insertName(cursor, name):
  if __debug__:
    print("Name to insert %s " %(name))

  #hashvalue is needed since name is too long for primary key (must be <767 bytes)
  hashValue =  hashlib.sha256(str(name).encode('utf-8')).hexdigest()
  if __debug__:
    print("Hash of name %s " %(hashValue))

  ##NOTE:must use %s to prevent sql injection in all sql queries
  splitName = list(filter(None, name.split("/")))
  splitName.insert(0, hashValue)
  splitName.insert(0, name)
  splitName = tuple(splitName)
  if __debug__:
    print("Name to insert in database %s" %(splitName,))

  addRecord = ("INSERT INTO hep_data "
               "(name, sha256, campaign, physics_process, event_content, calibration_version, \
                production_dir, production_file_hash) "
               "VALUES (%s, %s, %s, %s, %s, %s, %s, %s)")

  try:
    cursor.execute(addRecord, splitName)
    print("Inserted record %s" %(name))
    return True
  except sql.Error as err:
    print("Error inserting name %s, \nError:%s" %(name, err.msg))
    return False

if __name__ == '__main__':
  datafilePath = input("File/directory to translate: ")
  configFilepath = input("Schema file path for the above file/directory: ")

  #do the translation, the library should provide error messages, if any
  ndnNames = hep_translator_module.argsForTranslation(datafilePath, configFilepath)
  if ndnNames is False:
    print("Error parsing config file, exiting")
    sys.exit(-1)

  if len(ndnNames) == 0:
    print("No name returned from the translator, exiting.")
    sys.exit(-1)
  if __debug__:
    print("Returned NDN names: %s" %(ndnNames))

  #open connection to db
  dbHost = input("Database Host?")
  dbName = input("Database to store NDN names?")
  dbUsername = input("Database username?")
  dbPasswd = getpass.getpass("Database password?")

  try:
    con = sql.connect(user=dbUsername, database=dbName, password=dbPasswd, host=dbHost)
    cursor = con.cursor()
    if __debug__:
      print("successfully connected to database")

    #if tables do not exist, create tables
    #if table already exists, or creation successful, continue
    if createTables(cursor):
      for ndnName in ndnNames:
        #get list of files and insert into database
        resInsertnames = insertName(cursor, ndnName[1])
        if __debug__:
          print("Return Code is %s for inserting name: %s" %(resInsertnames, ndnName[1]))
      con.commit()
    else:
      print("Error creating tables, exiting.")

  except sql.Error as err:
    if err.errno == errorcode.ER_ACCESS_DENIED_ERROR:
      print("Incorrect username or password")
    elif err.errno == errorcode.ER_BAD_DB_ERROR:
      print("Database does not exist")
    else:
      print("Error connecting to Database: %s" %(err.msg))
  finally:
    con.close()
