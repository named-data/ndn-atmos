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
from ndn_cmmap_translators import atmos_translator

def createTables(cursor):
  TABLES = {}
  TABLES['cmip5'] = (
    "CREATE TABLE `cmip5` ("
    "  `id` int(100) AUTO_INCREMENT NOT NULL,"
    "  `sha256` varchar(64) UNIQUE NOT NULL,"
    "  `name` varchar(1000) NOT NULL,"
    "  `activity` varchar(100) NOT NULL,"
    "  `product` varchar(100) NOT NULL,"
    "  `organization` varchar(100) NOT NULL,"
    "  `model` varchar(100) NOT NULL,"
    "  `experiment` varchar(100) NOT NULL,"
    "  `frequency` varchar(100) NOT NULL,"
    "  `modeling_realm` varchar(100) NOT NULL,"
    "  `variable_name` varchar(100) NOT NULL,"
    "  `ensemble` varchar(100) NOT NULL,"
    "  `time` varchar(100) NOT NULL,"
    "  PRIMARY KEY (`id`)"
    ") ENGINE=InnoDB")

  #check if tables exist, if not create them
  #create tables per schema in the wiki
  for tableName, query in TABLES.items():
    try:
      print("Creating table {}: ".format(tableName))
      cursor.execute(query)
      print("Created table {}: ".format(tableName))
      return 0
    except sql.Error as err:
      if err.errno == errorcode.ER_TABLE_EXISTS_ERROR:
        print("Table already exists: %s " %(tableName))
        return 1
      else:
        print("Failed to create table: %s" %(err.msg))
        return -1

def insertNames(cursor, name):
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
    print("Name to insert in database %s" %(splitName))

  addRecord = ("INSERT INTO cmip5 "
               "(name, sha256, activity, product, organization, model, experiment, frequency,\
               modeling_realm, variable_name, ensemble, time) "
               "VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)")

  try:
    cursor.execute(addRecord, splitName)
    print("Inserted record %s" %(name))
    return 0
  except sql.Error as err:
    print("Error inserting name %s" %(err.msg))
    return -1

if __name__ == '__main__':
  datafilePath = input("File/directory to translate: ")
  configFilepath = input("Schema file path for the above file/directory: ")

  #do the translation, the library should provide error messages, if any
  ndnNames = atmos_translator.args_for_translation(datafilePath, configFilepath)
  if len(ndnNames) == 0:
    print("No name returned from the translator, exiting.")
    sys.exit(-1)
  if __debug__:
    print("Returned NDN names: %s" %(ndnNames))

  #open connection to db
  dbName = input("Database to store NDN names?")
  dbUsername = input("Database username?")
  dbPasswd = getpass.getpass("Database password?")

  try:
    con = sql.connect(user=dbUsername, database=dbName, password=dbPasswd)
    cursor = con.cursor()
    if __debug__:
      print("successfully connected to database")

    #if tables do not exist, create tables
    res = createTables(cursor)
    if __debug__:
      print("Return Code for create_tables: %s" %(res))

    #if error, exit
    if res == -1:
      sys.exit(-1)

    #if table already exists, or creation successful, continue
    if res == 1 or res == 0:
      for ndnName in ndnNames:
        #get list of files and insert into database
        res_insertNames = insertNames(cursor, ndnName)
        if __debug__:
          print("Return Code is %s for inserting name: %s" %(res_insertNames, ndnName))
      con.commit()
  except sql.Error as err:
    if err.errno == errorcode.ER_ACCESS_DENIED_ERROR:
      print("Incorrect username or password")
    elif err.errno == errorcode.ER_BAD_DB_ERROR:
      print("Database does not exist")
    else:
      print("Error connecting to Database: %s" %(err.msg))
  finally:
    con.close()
