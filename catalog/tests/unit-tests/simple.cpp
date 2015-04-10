/** NDN-Atmos: Cataloging Service for distributed data originally developed
 *  for atmospheric science data
 *  Copyright (C) 2015 Colorado State University
 *
 *  NDN-Atmos is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  NDN-Atmos is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#include <boost/test/unit_test.hpp>

#include <json/value.h>
#include <json/writer.h>
#include <json/reader.h>

#include <iostream>

namespace atmos {
namespace test {

BOOST_AUTO_TEST_SUITE(MasterSuite)

BOOST_AUTO_TEST_CASE(SimpleTest)
{
  BOOST_CHECK(0==0);
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
} //namespace atmos
