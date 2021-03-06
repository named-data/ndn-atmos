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

  class QueryAdapterTest : public query::QueryAdapter<std::string>
  {
  public:
    QueryAdapterTest(const std::shared_ptr<ndn::util::DummyClientFace>& face,
                     const std::shared_ptr<ndn::KeyChain>& keyChain,
                     const std::shared_ptr<chronosync::Socket>& syncSocket)
      : query::QueryAdapter<std::string>(face, keyChain, syncSocket)
    {
    }

    virtual
    ~QueryAdapterTest()
    {
    }

    void setDatabaseTable(const std::string& databaseTable)
    {
      m_databaseTable.assign(databaseTable);
    }

    void setNameFields(const std::vector<std::string>& nameFields)
    {
      m_nameFields = nameFields;
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

    std::shared_ptr<ndn::Data>
    getReplyData(const ndn::Name& segmentPrefix,
                 const Json::Value& value,
                 uint64_t segmentNo,
                 bool isFinalBlock,
                 bool isAutocomplete,
                 uint64_t resultCount,
                 uint64_t viewStart,
                 uint64_t viewEnd)
    {
      return makeReplyData(segmentPrefix, value, segmentNo, isFinalBlock,
                           isAutocomplete, resultCount, viewStart, viewEnd, false);
    }

    void
    queryTest(std::shared_ptr<const ndn::Interest> interest)
    {
      runJsonQuery(interest);
    }

    void
    prepareSegmentsByParams(std::vector<std::pair<std::string, std::string>>& queryParams,
                            const ndn::Name& segmentPrefix)
    {
      //BOOST_CHECK_EQUAL(sqlString, "SELECT name FROM cmip5 WHERE name=\'test\';");
      for (auto it = queryParams.begin() ; it != queryParams.end(); ++it) {
        std::cout << it->first << " " << it->second << std::endl;
      }

      Json::Value fileList;
      fileList.append("/ndn/test1");
      fileList.append("/ndn/test2");
      fileList.append("/ndn/test3");

      std::shared_ptr<ndn::Data> data = makeReplyData(segmentPrefix,
                                                      fileList, 0, true, false,
                                                      3, 0, 2, true);
      m_mutex.lock();
      m_cache.insert(*data);
      m_mutex.unlock();
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

    bool
    json2AutocompletionSqlTest(std::stringstream& sqlQuery,
                               Json::Value& jsonValue,
                               bool& lastComponent,
                               std::stringstream& nameField)
    {
      return json2AutocompletionSql(sqlQuery, jsonValue, lastComponent, nameField);
    }

    bool
    testDoPrefixBasedSearch(Json::Value& jsonValue,
                            std::vector<std::pair<std::string, std::string>>& typedComponents)
    {
      return doPrefixBasedSearch(jsonValue, typedComponents);
    }

    bool
    testDoFilterBasedSearch(Json::Value& jsonValue,
                            std::vector<std::pair<std::string, std::string>>& typedComponents)
    {
      return doFilterBasedSearch(jsonValue, typedComponents);
    }


  };

  class QueryAdapterFixture : public UnitTestTimeFixture
  {
  public:
    QueryAdapterFixture()
      : face(std::make_shared<ndn::util::DummyClientFace>(io))
      , keyChain(new ndn::KeyChain())
      , databaseTable("cmip5")
      , queryAdapterTest1(face, keyChain, syncSocket)
      , queryAdapterTest2(face, keyChain, syncSocket)
    {
      std::string c1("activity"), c2("product"), c3("organization"), c4("model");
      std::string c5("experiment"), c6("frequency"), c7("modeling_realm"), c8("variable_name");
      std::string c9("ensemble"), c10("time");
      nameFields.push_back(c1);
      nameFields.push_back(c2);
      nameFields.push_back(c3);
      nameFields.push_back(c4);
      nameFields.push_back(c5);
      nameFields.push_back(c6);
      nameFields.push_back(c7);
      nameFields.push_back(c8);
      nameFields.push_back(c9);
      nameFields.push_back(c10);

      queryAdapterTest1.setDatabaseTable(databaseTable);
      queryAdapterTest1.setNameFields(nameFields);
      queryAdapterTest2.setDatabaseTable(databaseTable);
      queryAdapterTest2.setNameFields(nameFields);
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
             filterCategoryNames activity,product,organization,model,experiment,frequency,modeling_realm,variable_name,ensemble\
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
        ss << "\
             filterCategoryNames activity,product,organization,model,experiment,frequency,modeling_realm,variable_name,ensemble\
             database\
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
    std::shared_ptr<ndn::util::DummyClientFace> face;
    std::shared_ptr<ndn::KeyChain> keyChain;
    std::shared_ptr<chronosync::Socket> syncSocket;
    std::string databaseTable;
    std::vector<std::string> nameFields;
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

  // use real data instead of ack data
  BOOST_AUTO_TEST_CASE(QueryAdapterMakeAckDataTest)
  {
    ndn::Interest interest(ndn::Name("/test/ack/data/json"));
    interest.setInterestLifetime(ndn::time::milliseconds(1000));
    interest.setMustBeFresh(true);
    std::shared_ptr<const ndn::Interest> interestPtr = std::make_shared<ndn::Interest>(interest);

    const ndn::name::Component version
      = ndn::name::Component::fromVersion(1);

    std::shared_ptr<ndn::Data> data = queryAdapterTest2.getAckData(interestPtr, version);
    BOOST_CHECK_EQUAL(data->getName().toUri(), "/test/ack/data/json");
  }

  BOOST_AUTO_TEST_CASE(QueryAdapterMakeReplyDataTest1)
  {
    Json::Value fileList;
    fileList.append("/ndn/test1");
    fileList.append("/ndn/test2");

    const ndn::Name prefix("/atmos/test/prefix");

    std::shared_ptr<ndn::Data> data = queryAdapterTest2.getReplyData(prefix, fileList,
                                                                     1, false, false, 2, 0, 1);
    BOOST_CHECK_EQUAL(data->getName().toUri(), "/atmos/test/prefix/%00%01");
    BOOST_CHECK_EQUAL(data->getFinalBlockId(), ndn::Name::Component(""));
    const std::string jsonRes(reinterpret_cast<const char*>(data->getContent().value()),
                              data->getContent().value_size());
    Json::Value parsedFromString;
    Json::Reader reader;
    BOOST_CHECK_EQUAL(reader.parse(jsonRes, parsedFromString), true);
    BOOST_CHECK_EQUAL(parsedFromString["resultCount"], 2);
    BOOST_CHECK_EQUAL(parsedFromString["results"].size(), 2);
    BOOST_CHECK_EQUAL(parsedFromString["results"][0], "/ndn/test1");
    BOOST_CHECK_EQUAL(parsedFromString["results"][1], "/ndn/test2");
  }

  BOOST_AUTO_TEST_CASE(QueryAdapterMakeReplyDataTest2)
  {
    Json::Value fileList;
    fileList.append("/ndn/test1");
    const ndn::Name prefix("/atmos/test/prefix");

    std::shared_ptr<ndn::Data> data = queryAdapterTest2.getReplyData(prefix, fileList,
                                                                     2, true, true, 1, 0, 0);

    BOOST_CHECK_EQUAL(data->getName().toUri(), "/atmos/test/prefix/%00%02");
    BOOST_CHECK_EQUAL(data->getFinalBlockId(), ndn::Name::Component::fromSegment(2));
    const std::string jsonRes(reinterpret_cast<const char*>(data->getContent().value()),
                              data->getContent().value_size());
    Json::Value parsedFromString;
    Json::Reader reader;
    BOOST_CHECK_EQUAL(reader.parse(jsonRes, parsedFromString), true);
    BOOST_CHECK_EQUAL(parsedFromString["resultCount"], 1);
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

    // TODO: the code below should be enabled when queryAdapter can get the correct the
    // ChronoSync state; currently, we don't need the activeQuery to save the ACK data;
    //auto ackData = queryAdapterTest2.getDataFromActiveQuery(jsonMessage);
    //
    //BOOST_CHECK(ackData);
    //if (ackData) {
    //  BOOST_CHECK_EQUAL(ackData->getName().getPrefix(3),
    //  ndn::Name("/test/query/%7B%22name%22%3A%22test%22%7D"));
    //  BOOST_CHECK_EQUAL(ackData->getName().at(ackData->getName().size() - 1),
    //                    ndn::Name::Component("OK"));
    //  BOOST_CHECK_EQUAL(ackData->getContent().value_size(), 0);
    //}

    // there is no query-results namespace data
    auto replyData = queryAdapterTest2.getDataFromCache(*queryInterest);
    BOOST_CHECK(replyData);
    if (replyData){
      BOOST_CHECK_EQUAL(replyData->getName().getPrefix(2), ndn::Name("/test/query"));
      const std::string jsonRes(reinterpret_cast<const char*>(replyData->getContent().value()),
                                replyData->getContent().value_size());
      Json::Value parsedFromString;
      Json::Reader reader;
      BOOST_CHECK_EQUAL(reader.parse(jsonRes, parsedFromString), true);
      BOOST_CHECK_EQUAL(parsedFromString["resultCount"], 3);
      BOOST_CHECK_EQUAL(parsedFromString["results"].size(), 3);
      BOOST_CHECK_EQUAL(parsedFromString["results"][0], "/ndn/test1");
      BOOST_CHECK_EQUAL(parsedFromString["results"][1], "/ndn/test2");
      BOOST_CHECK_EQUAL(parsedFromString["results"][2], "/ndn/test3");
    }
  }

  BOOST_AUTO_TEST_CASE(QueryAdapterAutocompletionSqlSuccessTest)
  {
    initializeQueryAdapterTest2();

    std::stringstream ss, nameField;
    Json::Value testJson;
    bool lastComponent = false;
    testJson["?"] = "/";
    BOOST_CHECK_EQUAL(true,
                      queryAdapterTest2.json2AutocompletionSqlTest(ss,
                                                                   testJson,
                                                                   lastComponent,
                                                                   nameField));
    BOOST_CHECK_EQUAL(lastComponent, false);
    BOOST_CHECK_EQUAL(";", ss.str());
    BOOST_CHECK_EQUAL("activity", nameField.str());

    ss.str("");
    ss.clear();
    nameField.str("");
    nameField.clear();
    testJson.clear();
    testJson["?"] = "/Activity/";
    BOOST_CHECK_EQUAL(true,
                      queryAdapterTest2.json2AutocompletionSqlTest(ss,
                                                                   testJson,
                                                                   lastComponent,
                                                                   nameField));
    BOOST_CHECK_EQUAL(lastComponent, false);
    BOOST_CHECK_EQUAL(" WHERE activity='Activity';", ss.str());
    BOOST_CHECK_EQUAL("product", nameField.str());

    ss.str("");
    ss.clear();
    nameField.str("");
    nameField.clear();
    testJson.clear();
    testJson["?"] = "/Activity/Product/Organization/Model/Experiment/";
    BOOST_CHECK_EQUAL(true,
                      queryAdapterTest2.json2AutocompletionSqlTest(ss,
                                                                   testJson,
                                                                   lastComponent,
                                                                   nameField));
    BOOST_CHECK_EQUAL(lastComponent, false);
    BOOST_CHECK_EQUAL(" WHERE activity='Activity' AND \
experiment='Experiment' AND model='Model' AND organization='Organization' AND product='Product';",
     ss.str());
    BOOST_CHECK_EQUAL("frequency", nameField.str());

    ss.str("");
    ss.clear();
    nameField.str("");
    nameField.clear();
    testJson.clear();
    testJson["?"] = "/Activity/Product/Organization/Model/Experiment/Frequency/Modeling/\
Variable/Ensemble/";
    BOOST_CHECK_EQUAL(true,
                      queryAdapterTest2.json2AutocompletionSqlTest(ss,
                                                                   testJson,
                                                                   lastComponent,
                                                                   nameField));
    BOOST_CHECK_EQUAL(lastComponent, true);
    BOOST_CHECK_EQUAL(" WHERE activity='Activity' AND ensemble=\
'Ensemble' AND experiment='Experiment' AND frequency='Frequency' AND model='Model' AND \
modeling_realm='Modeling' AND organization='Organization' AND product='Product' AND variable_name=\
'Variable';",ss.str());
    BOOST_CHECK_EQUAL("time", nameField.str());
  }

  BOOST_AUTO_TEST_CASE(QueryAdapterAutocompletionSqlFailTest)
  {
    initializeQueryAdapterTest2();

    std::stringstream ss, nameField;
    Json::Value testJson;
    bool lastComponent = false;
    testJson["?"] = "serchTest";
    BOOST_CHECK_EQUAL(false,
                      queryAdapterTest2.json2AutocompletionSqlTest(ss,
                                                                   testJson,
                                                                   lastComponent,
                                                                   nameField));

    ss.str("");
    ss.clear();
    nameField.str("");
    nameField.clear();
    testJson.clear();
    testJson["?"] = "/cmip5";
    BOOST_CHECK_EQUAL(false,
                      queryAdapterTest2.json2AutocompletionSqlTest(ss,
                                                                   testJson,
                                                                   lastComponent,
                                                                   nameField));

    ss.str("");
    ss.clear();
    nameField.str("");
    nameField.clear();
    Json::Value testJson2; //simply clear does not work
    testJson2[0] = "test";
    BOOST_CHECK_EQUAL(false,
                      queryAdapterTest2.json2AutocompletionSqlTest(ss,
                                                                   testJson,
                                                                   lastComponent,
                                                                   nameField));

    ss.str("");
    ss.clear();
    nameField.str("");
    nameField.clear();
    Json::Value testJson3;
    testJson3 = Json::Value(Json::arrayValue);
    BOOST_CHECK_EQUAL(false,
                      queryAdapterTest2.json2AutocompletionSqlTest(ss,
                                                                   testJson,
                                                                   lastComponent,
                                                                   nameField));

    ss.str("");
    ss.clear();
    nameField.str("");
    nameField.clear();
    Json::Value testJson4;
    Json::Value param;
    param[0] = "test";
    testJson4["name"] = param;
    BOOST_CHECK_EQUAL(false,
                      queryAdapterTest2.json2AutocompletionSqlTest(ss,
                                                                   testJson,
                                                                   lastComponent,
                                                                   nameField));
  }

  BOOST_AUTO_TEST_CASE(QueryAdapterDoFilterBasedSearchTest)
  {
    Json::Value testJson;
    testJson["activity"] = "testActivity";
    testJson["product"] = "testProduct";

    std::vector<std::pair<std::string, std::string>> resultComponents;

    BOOST_CHECK_EQUAL(true, queryAdapterTest1.testDoFilterBasedSearch(testJson, resultComponents));
    BOOST_CHECK_EQUAL(2, resultComponents.size());
    for (auto it=resultComponents.begin(); it != resultComponents.end(); it++) {
      if (it->first == "activity")
        BOOST_CHECK_EQUAL(it->second, "testActivity");
      if (it->first == "product")
        BOOST_CHECK_EQUAL(it->second, "testProduct");
    }

    testJson.clear();
    resultComponents.clear();

    BOOST_CHECK_EQUAL(true, queryAdapterTest1.testDoFilterBasedSearch(testJson, resultComponents));
    BOOST_CHECK_EQUAL(0, resultComponents.size());

    testJson.clear();
    resultComponents.clear();

    testJson["name"] = Json::nullValue;
    BOOST_CHECK_EQUAL(false, queryAdapterTest1.testDoFilterBasedSearch(testJson, resultComponents));

    testJson.clear();
    resultComponents.clear();
    Json::Value param;
    param[0] = "test";
    testJson["name"] = param;

    BOOST_CHECK_EQUAL(false, queryAdapterTest1.testDoFilterBasedSearch(testJson, resultComponents));

    testJson.clear();
    resultComponents.clear();
    Json::Value tmp;
    tmp[0] = "test";

    BOOST_CHECK_EQUAL(false, queryAdapterTest1.testDoFilterBasedSearch(tmp, resultComponents));
  }

  BOOST_AUTO_TEST_CASE(QueryAdapterDoPrefixBasedSearchTest)
  {
    Json::Value testJson;
    testJson["??"] = "/";

    std::vector<std::pair<std::string, std::string>> resultComponents;

    BOOST_CHECK_EQUAL(true, queryAdapterTest2.testDoPrefixBasedSearch(testJson, resultComponents));
    BOOST_CHECK_EQUAL(0, resultComponents.size());

    testJson.clear();
    resultComponents.clear();

    testJson["??"] = "/Activity/Product";
    BOOST_CHECK_EQUAL(true, queryAdapterTest2.testDoPrefixBasedSearch(testJson, resultComponents));
    BOOST_CHECK_EQUAL(2, resultComponents.size());

    for (auto it=resultComponents.begin(); it != resultComponents.end(); it++) {
      if (it->first == "activity")
        BOOST_CHECK_EQUAL(it->second, "Activity");
      if (it->first == "product")
        BOOST_CHECK_EQUAL(it->second, "Product");
    }

    testJson.clear();
    resultComponents.clear();

    testJson["??"] = "";
    BOOST_CHECK_EQUAL(false, queryAdapterTest2.testDoPrefixBasedSearch(testJson, resultComponents));

    testJson.clear();
    resultComponents.clear();

    testJson = Json::Value(Json::arrayValue);
    BOOST_CHECK_EQUAL(false, queryAdapterTest2.testDoPrefixBasedSearch(testJson, resultComponents));

    testJson.clear();
    resultComponents.clear();

    Json::Value tmp;
    tmp[0] = "test";
    BOOST_CHECK_EQUAL(false, queryAdapterTest2.testDoPrefixBasedSearch(tmp, resultComponents));

    testJson.clear();
    resultComponents.clear();

    Json::Value param, testJson2;
    param[0] = "test";
    testJson2["activity"] = param;
    BOOST_CHECK_EQUAL(false, queryAdapterTest2.testDoPrefixBasedSearch(testJson2, resultComponents));
  }

  BOOST_AUTO_TEST_SUITE_END()

}//tests
}//atmos
