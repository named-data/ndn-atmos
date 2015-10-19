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
 *  along with NDN-Atmos.  If not, see <http://www.gnu.org/licenses/>.
**/

#include "publish/publish-adapter.hpp"
#include "boost-test.hpp"
#include "../../unit-test-time-fixture.hpp"
#include "util/config-file.hpp"

#include <boost/mpl/list.hpp>
#include <boost/thread.hpp>
#include <ndn-cxx/util/dummy-client-face.hpp>
#include <boost/property_tree/info_parser.hpp>

namespace atmos{
namespace tests{
  using ndn::util::DummyClientFace;
  using ndn::util::makeDummyClientFace;

  class PublishAdapterTest : public publish::PublishAdapter<std::string>
  {
  public:
    PublishAdapterTest(const std::shared_ptr<ndn::util::DummyClientFace>& face,
                       const std::shared_ptr<ndn::KeyChain>& keyChain,
                       std::shared_ptr<chronosync::Socket>& syncSocket)
      : publish::PublishAdapter<std::string>(face, keyChain, syncSocket)
    {
    }

    virtual
    ~PublishAdapterTest()
    {
    }

    void
    setDatabaseTable(const std::string& databaseTable)
    {
      m_databaseTable = databaseTable;
    }

    void
    setTableFields(const std::vector<std::string>& tableFields)
    {
      m_tableColumns = tableFields;
    }

    const ndn::Name
    getPrefix()
    {
      return m_prefix;
    }

    const ndn::Name
    getSigningId()
    {
      return m_signingId;
    }

    const ndn::Name
    getSyncPrefix()
    {
      return m_syncPrefix;
    }

    void
    configAdapter(const util::ConfigSection& section,
                  const ndn::Name& prefix)
    {
      onConfig(section, false, std::string("test.txt"), prefix);
    }

    bool
    testJson2Sql(std::stringstream& sqlString,
                 Json::Value& jsonValue,
                 util::DatabaseOperation operation)
    {
      return json2Sql(sqlString, jsonValue, operation);
    }

    bool
    testName2Fields(std::stringstream& sqlString,
                    std::string& fileName)
    {
      return name2Fields(sqlString, fileName);
    }

    bool
    testValidatePublicationChanges(const std::shared_ptr<const ndn::Data>& data)
    {
      return validatePublicationChanges(data);
    }
  };

  class PublishAdapterFixture : public UnitTestTimeFixture
  {
  public:
    PublishAdapterFixture()
      : face(makeDummyClientFace(io))
      , keyChain(new ndn::KeyChain())
      , databaseTable("cmip5")
      , publishAdapterTest1(face, keyChain, syncSocket)
      , publishAdapterTest2(face, keyChain, syncSocket)
    {
      std::string cx("sha256"), c0("name"), c1("activity"), c2("product"), c3("organization");
      std::string c4("model"), c5("experiment"), c6("frequency"), c7("modeling_realm");
      std::string c8("variable_name"), c9("ensemble"), c10("time");
      tableFields.push_back(cx);
      tableFields.push_back(c0);
      tableFields.push_back(c1);
      tableFields.push_back(c2);
      tableFields.push_back(c3);
      tableFields.push_back(c4);
      tableFields.push_back(c5);
      tableFields.push_back(c6);
      tableFields.push_back(c7);
      tableFields.push_back(c8);
      tableFields.push_back(c9);
      tableFields.push_back(c10);
      publishAdapterTest1.setDatabaseTable(databaseTable);
      publishAdapterTest1.setTableFields(tableFields);
      publishAdapterTest2.setDatabaseTable(databaseTable);
      publishAdapterTest2.setTableFields(tableFields);
    }

    virtual
    ~PublishAdapterFixture()
    {
    }

  protected:
    void
    initializePublishAdapterTest1()
    {
      util::ConfigSection section;
      try {
        std::stringstream ss;
        ss << "database                  \
             {                           \
              dbServer localhost         \
              dbName testdb              \
              dbUser testuser            \
              dbPasswd testpwd           \
             }                           \
             sync                        \
             {                           \
              prefix ndn:/ndn/broadcast1 \
             }";
        boost::property_tree::read_info(ss, section);
      }
      catch (boost::property_tree::info_parser_error &e) {
        std::cout << "Failed to read config file " << e.what() << std::endl;
      }

      publishAdapterTest1.configAdapter(section, ndn::Name("/test"));
    }

    void
    initializePublishAdapterTest2()
    {
      util::ConfigSection section;
      try {
        std::stringstream ss;
        ss << "\
             signingId /prefix/signingId \
             database                    \
             {                           \
              dbServer localhost         \
              dbName testdb              \
              dbUser testuser            \
              dbPasswd testpwd           \
             }                           \
             sync                        \
             {                           \
             }";
        boost::property_tree::read_info(ss, section);
      }
      catch (boost::property_tree::info_parser_error &e) {
        std::cout << "Failed to read config file " << e.what() << std::endl;;
      }

      publishAdapterTest2.configAdapter(section, ndn::Name("/test"));
    }

  protected:
    std::shared_ptr<DummyClientFace> face;
    std::shared_ptr<ndn::KeyChain> keyChain;
    std::shared_ptr<chronosync::Socket> syncSocket;
    std::vector<std::string> tableFields;
    std::string databaseTable;
    PublishAdapterTest publishAdapterTest1;
    PublishAdapterTest publishAdapterTest2;
  };

  BOOST_FIXTURE_TEST_SUITE(PublishAdapterTestSuite, PublishAdapterFixture)

  BOOST_AUTO_TEST_CASE(BasicPublishAdapterTest1)
  {
    BOOST_CHECK(publishAdapterTest1.getPrefix() == ndn::Name());
    BOOST_CHECK(publishAdapterTest1.getSigningId() == ndn::Name());
    BOOST_CHECK(publishAdapterTest1.getSyncPrefix() == ndn::Name());
  }

  BOOST_AUTO_TEST_CASE(BasicPublishAdapterTest2)
  {
    initializePublishAdapterTest1();
    BOOST_CHECK(publishAdapterTest1.getPrefix() == ndn::Name("/test"));
    BOOST_CHECK(publishAdapterTest1.getSigningId() == ndn::Name());
    BOOST_CHECK(publishAdapterTest1.getSyncPrefix() == ndn::Name("ndn:/ndn/broadcast1"));

    initializePublishAdapterTest2();
    BOOST_CHECK(publishAdapterTest2.getPrefix() == ndn::Name("/test"));
    BOOST_CHECK(publishAdapterTest2.getSigningId() == ndn::Name("/prefix/signingId"));
    BOOST_CHECK(publishAdapterTest2.getSyncPrefix() ==
                ndn::Name("ndn:/ndn-atmos/broadcast/chronosync"));
  }

  BOOST_AUTO_TEST_CASE(PublishAdapterName2FieldsNormalTest)
  {
    std::string testFileName1 = "/1/2/3/4/5/6/7/8/9/10";
    std::stringstream ss;
    std::string expectString1 = ",'1','2','3','4','5','6','7','8','9','10'";
    BOOST_CHECK_EQUAL(publishAdapterTest1.testName2Fields(ss, testFileName1), true);
    BOOST_CHECK_EQUAL(ss.str(), expectString1);

    ss.str("");
    ss.clear();
    std::string testFileName2 = "ndn:/1/2/3/4/5/6/777/8/99999/10";
    std::string expectString2 = ",'1','2','3','4','5','6','777','8','99999','10'";
    BOOST_CHECK_EQUAL(publishAdapterTest1.testName2Fields(ss, testFileName2), true);
    BOOST_CHECK_EQUAL(ss.str(), expectString2);
  }

  BOOST_AUTO_TEST_CASE(PublishAdapterName2FieldsFailureTest)
  {
    std::string testFileName1 = "/1/2/3/4/5/6/7/8/9/10/11";//too much components
    std::stringstream ss;
    BOOST_CHECK_EQUAL(publishAdapterTest1.testName2Fields(ss, testFileName1), false);

    ss.str("");
    ss.clear();
    std::string testFileName2 = "1234567890";
    BOOST_CHECK_EQUAL(publishAdapterTest1.testName2Fields(ss, testFileName2), false);

    ss.str("");
    ss.clear();
    std::string testFileName3 = "ndn:/1/2/3/4/5"; //too little components
    BOOST_CHECK_EQUAL(publishAdapterTest1.testName2Fields(ss, testFileName3), false);
  }

  BOOST_AUTO_TEST_CASE(PublishAdapterSqlStringNormalTest)
  {
    Json::Value testJson;
    testJson["add"][0] = "/1/2/3/4/5/6/7/8/9/10";
    testJson["add"][1] = "ndn:/a/b/c/d/eee/f/gg/h/iiii/j";
    testJson["remove"][0] = "ndn:/1/2/3/4/5/6/7/8/9/10";
    testJson["remove"][1] = "/a/b/c/d";
    testJson["remove"][2] = "/test/for/remove";

    std::stringstream ss;
    std::string expectRes1 = "INSERT INTO cmip5 (sha256, name, activity, product, organization, \
model, experiment, frequency, modeling_realm, variable_name, ensemble, time) VALUES(\
'3738C9C0E0297DE7FE0EE538030597442DEEFF0F2C88778404D7B6E4BAD589F6','/1/2/3/4/5/6/7/8/9/10',\
'1','2','3','4','5','6','7','8','9','10'),\
('F93128EE9B7769105C6BDF6AA0FAA8CB4ED429395DDBC2CDDBFBA05F35B320FB','ndn:/a/b/c/d/eee/f/gg/h/iiii/j'\
,'a','b','c','d','eee','f','gg','h','iiii','j');";
    BOOST_CHECK_EQUAL(publishAdapterTest1.testJson2Sql(ss, testJson, util::ADD), true);
    BOOST_CHECK_EQUAL(ss.str(), expectRes1);

    ss.str("");
    ss.clear();
    std::string expectRes2 = "DELETE FROM cmip5 WHERE name IN ('ndn:/1/2/3/4/5/6/7/8/9/10',\
'/a/b/c/d','/test/for/remove');";
    BOOST_CHECK_EQUAL(publishAdapterTest1.testJson2Sql(ss, testJson, util::REMOVE), true);
    BOOST_CHECK_EQUAL(ss.str(), expectRes2);
  }

  BOOST_AUTO_TEST_CASE(PublishAdapterSqlStringFailureTest)
  {
    Json::Value testJson;
    testJson["add"][0] = "/1/2/3/4/5/6/7/8/9/10";
    testJson["add"][1] = "/a/b/c/d/eee/f/gg/h/iiii/j/kkk"; //too much components
    std::stringstream ss;
    bool res = publishAdapterTest1.testJson2Sql(ss, testJson, util::REMOVE);
    BOOST_CHECK(res == false);
  }

  BOOST_AUTO_TEST_CASE(PublishAdapterValidateDataTestSuccess)
  {
    ndn::Name dataName("/test/publisher/12345"); // data name must be prefix+nonce
    Json::Value testJson;
    testJson["add"][0] = "/test/publisher/1";
    testJson["add"][1] = "/test/publisher/2";
    testJson["remove"][0] = "/test/publisher/5";

    Json::FastWriter fastWriter;
    const std::string jsonMessage = fastWriter.write(testJson);
    const char* payload = jsonMessage.c_str();
    size_t payLoadLength = jsonMessage.size() + 1;

    std::shared_ptr<ndn::Data> data = std::make_shared<ndn::Data>(dataName);
    data->setContent(reinterpret_cast<const uint8_t*>(payload), payLoadLength);
    data->setFreshnessPeriod(ndn::time::milliseconds(10000));

    BOOST_CHECK_EQUAL(true, publishAdapterTest1.testValidatePublicationChanges(data));

    ndn::Name dataName2("/"); // short data name
    Json::Value testJson2;
    testJson2["add"][0] = "/test/publisher2/1";
    testJson2["remove"][0] = "/test/publisher/1/2/3";

    const std::string jsonMessage2 = fastWriter.write(testJson2);
    const char* payload2 = jsonMessage2.c_str();
    size_t payLoadLength2 = jsonMessage2.size() + 1;

    std::shared_ptr<ndn::Data> data2 = std::make_shared<ndn::Data>(dataName2);
    data2->setContent(reinterpret_cast<const uint8_t*>(payload2), payLoadLength2);
    data2->setFreshnessPeriod(ndn::time::milliseconds(10000));
    BOOST_CHECK_EQUAL(true, publishAdapterTest1.testValidatePublicationChanges(data2));
  }

  BOOST_AUTO_TEST_CASE(PublishAdapterValidateDataTestFail)
  {
    ndn::Name dataName1("test/publisher2/12345"); // data name must be prefix+nonce
    Json::Value testJson1;
    testJson1["add"][0] = "/test/publisher2/1";
    testJson1["remove"][0] = "/test/publisher/1/2/3";
    testJson1["remove"][1] = "/test/publisher2/4";

    Json::FastWriter fastWriter;
    const std::string jsonMessage1 = fastWriter.write(testJson1);
    const char* payload1 = jsonMessage1.c_str();
    size_t payLoadLength1 = jsonMessage1.size() + 1;

    std::shared_ptr<ndn::Data> data1 = std::make_shared<ndn::Data>(dataName1);
    data1->setContent(reinterpret_cast<const uint8_t*>(payload1), payLoadLength1);
    data1->setFreshnessPeriod(ndn::time::milliseconds(10000));

    BOOST_CHECK_EQUAL(false, publishAdapterTest1.testValidatePublicationChanges(data1));
  }

  BOOST_AUTO_TEST_SUITE_END()

}//tests
}//atmos
