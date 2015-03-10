/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2015,  Colorado State University.
 *
 * This file is part of ndn-atmos.
 *
 * ndn-atmos is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * ndn-atmos is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received copies of the GNU General Public License and GNU Lesser
 * General Public License along with ndn-atmos, e.g., in COPYING.md file.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * See AUTHORS.md for complete list of ndn-atmos authors and contributors.
 */

#include <boost/test/unit_test.hpp>
#include <libpq-fe.h>
#include <json/value.h>
#include <json/writer.h>
#include <json/reader.h>
#include <iostream>

namespace NdnAtmos {
namespace test {

BOOST_AUTO_TEST_SUITE(MasterSuite)

BOOST_AUTO_TEST_CASE(SimpleTest)
{
  BOOST_CHECK(0==0);
}

BOOST_AUTO_TEST_CASE(DBTest)
{
  PGconn *conn;
  conn = PQconnectdb("dbname=test sslmode=disable");
  BOOST_CHECK_EQUAL(PQstatus(conn) != CONNECTION_OK, true);
}

BOOST_AUTO_TEST_CASE(JsonTest)
{
  Json::Value original;
  original["command"] = "test";

  Json::FastWriter fastWriter;
  std::string jsonMessage = fastWriter.write(original);

  Json::Value parsedFromString;
  Json::Reader reader;
  bool result;
  BOOST_CHECK_EQUAL(result = reader.parse(jsonMessage, parsedFromString), true);
  if (result) {
    BOOST_CHECK_EQUAL(original, parsedFromString);
  }
}

BOOST_AUTO_TEST_SUITE_END()

} //namespace test
} //namespace ndn-atmos
