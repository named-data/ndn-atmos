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

#include "query/query-adapter.hpp"
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

  class QueryAdapterTest : public query::QueryAdapter<std::string>
  {
  public:
    QueryAdapterTest(const std::shared_ptr<ndn::util::DummyClientFace>& face,
                     const std::shared_ptr<ndn::KeyChain>& keyChain)
      : query::QueryAdapter<std::string>(face, keyChain)
    {
    }

    virtual
    ~QueryAdapterTest()
    {
    }

    void setPrefix(const ndn::Name& prefix)
    {
      m_prefix = prefix;
    }

    void setSigningId(const ndn::Name& signingId)
    {
      m_signingId = signingId;
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

    std::shared_ptr<ndn::Data>
    getAckData(std::shared_ptr<const ndn::Interest> interest, const ndn::Name::Component& version)
    {
      return makeAckData(interest, version);
    }

    void
    parseJsonTest(std::string& targetSql,
                  Json::Value& parsedFromString,
                  bool& autocomplete)
    {
      std::stringstream resultSql;
      json2Sql(resultSql, parsedFromString, autocomplete);
      targetSql.assign(resultSql.str());
    }

    std::shared_ptr<ndn::Data>
    getReplyData(const ndn::Name& segmentPrefix,
                 const Json::Value& value,
                 uint64_t segmentNo,
                 bool isFinalBlock,
                 bool isAutocomplete)
    {
      return makeReplyData(segmentPrefix, value, segmentNo, isFinalBlock, isAutocomplete);
    }

    void
    queryTest(std::shared_ptr<const ndn::Interest> interest)
    {
      runJsonQuery(interest);
    }

    void
    prepareSegments(const ndn::Name& segmentPrefix,
                    const std::string& sqlString,
                    bool autocomplete)
    {
      BOOST_CHECK_EQUAL(sqlString, "SELECT name FROM cmip5 WHERE name=\'test\';");
      Json::Value fileList;
      fileList.append("/ndn/test1");
      fileList.append("/ndn/test2");
      fileList.append("/ndn/test3");

      std::shared_ptr<ndn::Data> data = makeReplyData(segmentPrefix,
                                                      fileList,
                                                      0,
                                                      true,
                                                      false);
      m_mutex.lock();
      m_cache.insert(*data);
      m_mutex.unlock();
    }

    std::shared_ptr<const ndn::Data>
    getDataFromActiveQuery(const std::string& jsonQuery)
    {
      m_mutex.lock();
      if (m_activeQueryToFirstResponse.find(jsonQuery) != m_activeQueryToFirstResponse.end()) {
        auto iter = m_activeQueryToFirstResponse.find(jsonQuery);
        if (iter != m_activeQueryToFirstResponse.end()) {
          m_mutex.unlock();
          return iter->second;
        }
      }
      m_mutex.unlock();
      return std::shared_ptr<const ndn::Data>();
    }

    std::shared_ptr<const ndn::Data>
    getDataFromCache(const ndn::Interest& interest)
    {
      return m_cache.find(interest);
    }

    void
    configAdapter(const util::ConfigSection& section,
                  const ndn::Name& prefix)
    {
      onConfig(section, false, std::string("test.txt"), prefix);
    }
  };

  class QueryAdapterFixture : public UnitTestTimeFixture
  {
  public:
    QueryAdapterFixture()
      : face(makeDummyClientFace(io))
      , keyChain(new ndn::KeyChain())
      , queryAdapterTest1(face, keyChain)
      , queryAdapterTest2(face, keyChain)
    {
    }

    virtual
    ~QueryAdapterFixture()
    {
    }

  protected:
    void
    initializeQueryAdapterTest1()
    {
      util::ConfigSection section;
      try {
        std::stringstream ss;
        ss << "signingId /test/signingId\
             database                   \
             {                          \
              dbServer localhost        \
              dbName testdb             \
              dbUser testuser           \
              dbPasswd testpwd          \
             }";
        boost::property_tree::read_info(ss, section);
      }
      catch (boost::property_tree::info_parser_error &e) {
        std::cout << "Failed to read config file " << e.what() << std::endl;;
      }
      queryAdapterTest1.configAdapter(section, ndn::Name("/test"));
    }

    void
    initializeQueryAdapterTest2()
    {
      util::ConfigSection section;
      try {
        std::stringstream ss;
        ss << "database\
             {                                  \
              dbServer localhost                \
              dbName testdb                     \
              dbUser testuser                   \
              dbPasswd testpwd                  \
             }";
        boost::property_tree::read_info(ss, section);
      }
      catch (boost::property_tree::info_parser_error &e) {
        std::cout << "Failed to read config file " << e.what() << std::endl;;
      }
      queryAdapterTest2.configAdapter(section, ndn::Name("/test"));
    }

  protected:
    std::shared_ptr<DummyClientFace> face;
    std::shared_ptr<ndn::KeyChain> keyChain;
    QueryAdapterTest queryAdapterTest1;
    QueryAdapterTest queryAdapterTest2;
  };

  BOOST_FIXTURE_TEST_SUITE(QueryAdapterTestSuite, QueryAdapterFixture)

  BOOST_AUTO_TEST_CASE(BasicQueryAdapterTest1)
  {
    BOOST_CHECK(queryAdapterTest1.getPrefix() == ndn::Name());
    BOOST_CHECK(queryAdapterTest1.getSigningId() == ndn::Name());
  }

  BOOST_AUTO_TEST_CASE(BasicQueryAdapterTest2)
  {
    initializeQueryAdapterTest1();
    BOOST_CHECK(queryAdapterTest1.getPrefix() == ndn::Name("/test"));
    BOOST_CHECK(queryAdapterTest1.getSigningId() == ndn::Name("/test/signingId"));
  }

  BOOST_AUTO_TEST_CASE(QueryAdapterJsonParseNormalTest)
  {
    Json::Value testJson;
    testJson["name"] = "test";
    testJson["activity"] = "testActivity";
    testJson["product"] = "testProduct";

    std::string dstString;
    bool autocomplete = false;
    queryAdapterTest1.parseJsonTest(dstString, testJson, autocomplete);
    BOOST_CHECK_EQUAL(dstString, "SELECT name FROM cmip5 WHERE\
 activity=\'testActivity\' AND name='test\' AND product=\'testProduct\';");
    BOOST_CHECK_EQUAL(autocomplete, false);
  }

  BOOST_AUTO_TEST_CASE(QueryAdapterJsonParseEmptyTest)
  {
    Json::Value testJson;

    std::string dstString;
    bool autocomplete = false;
    queryAdapterTest1.parseJsonTest(dstString, testJson, autocomplete);
    BOOST_CHECK_EQUAL(dstString, "SELECT name FROM cmip5 limit 0;");
    BOOST_CHECK_EQUAL(autocomplete, false);
  }

  BOOST_AUTO_TEST_CASE(QueryAdapterJsonParseAllItemsTest)
  {
    Json::Value testJson;
    testJson["name"] = "test";
    testJson["activity"] = "testActivity";
    testJson["product"] = "testProduct";
    testJson["origanization"] = "testOrg";
    testJson["model"] = "testModel";
    testJson["experiment"] = "testExperiment";
    testJson["frequency"] = "testFrenquency";
    testJson["modeling realm"] = "testModeling";
    testJson["variable name"] = "testVarName";
    testJson["ensemble member"] = "testEnsembleMember";
    testJson["ensemble"] = "testEnsemble";
    testJson["sample granularity"] = "testSampleGranularity";
    testJson["start time"] = "testStartTime";
    testJson["field campaign"] = "testFieldCampaign";
    testJson["optical properties for radiation"] = "testOptProperties";
    testJson["grid resolution"] = "testGridResolution";
    testJson["output type"] = "testOutputType";
    testJson["timestamp"] = "testTimestamp";

    std::string dstString;
    bool autocomplete = false;
    queryAdapterTest1.parseJsonTest(dstString, testJson, autocomplete);
    BOOST_CHECK_EQUAL(dstString, "SELECT name FROM cmip5 WHERE activity=\'testActivity\' AND \
ensemble=\'testEnsemble\' AND ensemble member=\'testEnsembleMember\' AND \
experiment=\'testExperiment\' AND field campaign=\'testFieldCampaign\' AND \
frequency=\'testFrenquency\' AND grid resolution=\'testGridResolution\' AND \
model=\'testModel\' AND modeling realm=\'testModeling\' AND name=\'test\' AND \
optical properties for radiation=\'testOptProperties\' AND origanization=\'testOrg\' AND \
output type=\'testOutputType\' AND product=\'testProduct\' AND sample \
granularity=\'testSampleGranularity\' AND start time=\'testStartTime\' AND \
timestamp=\'testTimestamp\' AND variable name=\'testVarName\';");
    BOOST_CHECK_EQUAL(autocomplete, false);
  }

  BOOST_AUTO_TEST_CASE(QueryAdapterJsonParseSearchTest)
  {
    Json::Value testJson;
    testJson["name"] = "test";
    testJson["?"] = "serchTest";

    std::string dstString;
    bool autocomplete = false;
    queryAdapterTest1.parseJsonTest(dstString, testJson, autocomplete);
    BOOST_CHECK_EQUAL(dstString,
      "SELECT name FROM cmip5 WHERE name REGEXP \'^serchTest\' AND name=\'test\';");
    BOOST_CHECK_EQUAL(autocomplete, true);
  }

  BOOST_AUTO_TEST_CASE(QueryAdapterMakeAckDataTest)
  {
    ndn::Interest interest(ndn::Name("/test/ack/data/json"));
    interest.setInterestLifetime(ndn::time::milliseconds(1000));
    interest.setMustBeFresh(true);
    std::shared_ptr<const ndn::Interest> interestPtr = std::make_shared<ndn::Interest>(interest);

    const ndn::name::Component version
      = ndn::name::Component::fromVersion(1);

    std::shared_ptr<ndn::Data> data = queryAdapterTest2.getAckData(interestPtr, version);
    BOOST_CHECK_EQUAL(data->getName().toUri(), "/test/ack/data/json/%FD%01/OK");
    BOOST_CHECK_EQUAL(data->getContent().value_size(), 0);
  }

  BOOST_AUTO_TEST_CASE(QueryAdapterMakeReplyDataTest1)
  {
    Json::Value fileList;
    fileList.append("/ndn/test1");
    fileList.append("/ndn/test2");

    const ndn::Name prefix("/atmos/test/prefix");

    std::shared_ptr<ndn::Data> data = queryAdapterTest2.getReplyData(prefix,
      fileList,
      1,
      false,
      false);
    BOOST_CHECK_EQUAL(data->getName().toUri(), "/atmos/test/prefix/%00%01");
    BOOST_CHECK_EQUAL(data->getFinalBlockId(), ndn::Name::Component(""));
    const std::string jsonRes(reinterpret_cast<const char*>(data->getContent().value()));
    Json::Value parsedFromString;
    Json::Reader reader;
    BOOST_CHECK_EQUAL(reader.parse(jsonRes, parsedFromString), true);
    BOOST_CHECK_EQUAL(parsedFromString["results"].size(), 2);
    BOOST_CHECK_EQUAL(parsedFromString["results"][0], "/ndn/test1");
    BOOST_CHECK_EQUAL(parsedFromString["results"][1], "/ndn/test2");
  }

  BOOST_AUTO_TEST_CASE(QueryAdapterMakeReplyDataTest2)
  {
    Json::Value fileList;
    fileList.append("/ndn/test1");
    const ndn::Name prefix("/atmos/test/prefix");

    std::shared_ptr<ndn::Data> data = queryAdapterTest2.getReplyData(prefix,
      fileList,
      2,
      true,
      true);
    // the finalBlock does not work for jsNDN
    BOOST_CHECK_EQUAL(data->getName().toUri(), "/atmos/test/prefix/%00%02");
    BOOST_CHECK_EQUAL(data->getFinalBlockId(), ndn::Name::Component::fromSegment(2));
    const std::string jsonRes(reinterpret_cast<const char*>(data->getContent().value()));
    Json::Value parsedFromString;
    Json::Reader reader;
    BOOST_CHECK_EQUAL(reader.parse(jsonRes, parsedFromString), true);
    BOOST_CHECK_EQUAL(parsedFromString["next"].size(), 1);
    BOOST_CHECK_EQUAL(parsedFromString["next"][0], "/ndn/test1");
  }

  BOOST_AUTO_TEST_CASE(QueryAdapterQueryProcessTest)
  {
    initializeQueryAdapterTest2();
    Json::Value query;
    query["name"] = "test";
    Json::FastWriter fastWriter;
    std::string jsonMessage = fastWriter.write(query);
    jsonMessage.erase(std::remove(jsonMessage.begin(), jsonMessage.end(), '\n'), jsonMessage.end());
    std::shared_ptr<ndn::Interest> queryInterest
      = std::make_shared<ndn::Interest>(ndn::Name("/test/query").append(jsonMessage.c_str()));

    queryAdapterTest2.queryTest(queryInterest);
    auto ackData = queryAdapterTest2.getDataFromActiveQuery(jsonMessage);

    BOOST_CHECK(ackData);
    if (ackData) {
      BOOST_CHECK_EQUAL(ackData->getName().getPrefix(3),
      ndn::Name("/test/query/%7B%22name%22%3A%22test%22%7D"));
      BOOST_CHECK_EQUAL(ackData->getName().getSubName(4, 1), ndn::Name("OK"));
      BOOST_CHECK_EQUAL(ackData->getContent().value_size(), 0);
    }

    std::shared_ptr<ndn::Interest> resultInterest
      = std::make_shared<ndn::Interest>(ndn::Name("/test/query-results"));
    auto replyData = queryAdapterTest2.getDataFromCache(*resultInterest);
    BOOST_CHECK(replyData);
    if (replyData){
      BOOST_CHECK_EQUAL(replyData->getName().getPrefix(2), ndn::Name("/test/query-results"));
      const std::string jsonRes(reinterpret_cast<const char*>(replyData->getContent().value()));
      Json::Value parsedFromString;
      Json::Reader reader;
      BOOST_CHECK_EQUAL(reader.parse(jsonRes, parsedFromString), true);
      BOOST_CHECK_EQUAL(parsedFromString["results"].size(), 3);
      BOOST_CHECK_EQUAL(parsedFromString["results"][0], "/ndn/test1");
      BOOST_CHECK_EQUAL(parsedFromString["results"][1], "/ndn/test2");
      BOOST_CHECK_EQUAL(parsedFromString["results"][2], "/ndn/test3");
    }
  }

  BOOST_AUTO_TEST_SUITE_END()

}//tests
}//atmos
