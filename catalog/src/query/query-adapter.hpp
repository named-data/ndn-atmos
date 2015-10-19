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

#ifndef ATMOS_QUERY_QUERY_ADAPTER_HPP
#define ATMOS_QUERY_QUERY_ADAPTER_HPP

#include "util/catalog-adapter.hpp"
#include "util/mysql-util.hpp"
#include "util/config-file.hpp"

#include <thread>

#include <json/reader.h>
#include <json/value.h>
#include <json/writer.h>

#include <ndn-cxx/data.hpp>
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/interest-filter.hpp>
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/time.hpp>
#include <ndn-cxx/encoding/encoding-buffer.hpp>
#include <ndn-cxx/util/in-memory-storage-lru.hpp>
#include <ndn-cxx/util/string-helper.hpp>
#include <ChronoSync/socket.hpp>

#include "mysql/mysql.h"

#include <map>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <array>
#include <utility>

#include "util/logger.hpp"


namespace atmos {
namespace query {
#ifdef HAVE_LOG4CXX
  INIT_LOGGER("QueryAdapter");
#endif

// todo: calculate payload limit by get the size of a signed empty Data packet
static const size_t PAYLOAD_LIMIT = 7000;

/**
 * QueryAdapter handles the Query usecases for the catalog
 */
template <typename DatabaseHandler>
class QueryAdapter : public atmos::util::CatalogAdapter {
public:
  /**
   * Constructor
   *
   * @param face:       Face that will be used for NDN communications
   * @param keyChain:   KeyChain that will be used for data signing
   * @param syncSocket: ChronoSync socket
   */
  QueryAdapter(const std::shared_ptr<ndn::Face>& face,
               const std::shared_ptr<ndn::KeyChain>& keyChain,
               const std::shared_ptr<chronosync::Socket>& syncSocket);

  virtual
  ~QueryAdapter();

  /**
   * Helper function to specify section handler
   */
  void
  setConfigFile(util::ConfigFile& config,
                const ndn::Name& prefix,
                const std::vector<std::string>& nameFields,
                const std::string& databaseTable);

protected:
  /**
   * Helper function for configuration parsing
   */
  void
  onConfig(const util::ConfigSection& section,
           bool isDryDun,
           const std::string& fileName,
           const ndn::Name& prefix);

  /**
   * Handles incoming query requests by stripping the filter off the Interest to get the
   * actual request out. This removes the need for a 2-step Interest-Data retrieval.
   *
   * @param filter:   InterestFilter that caused this Interest to be routed
   * @param interest: Interest that needs to be handled
   */
  virtual void
  onQueryInterest(const ndn::InterestFilter& filter, const ndn::Interest& interest);

  /**
   * Handles requests for responses to an existing query
   *
   * @param filter:   InterestFilter that caused this Interest to be routed
   * @param interest: Interest that needs to be handled
   */
  virtual void
  onQueryResultsInterest(const ndn::InterestFilter& filter, const ndn::Interest& interest);

  /**
   * Handles requests for responses to an filter initialization request
   *
   * @param filter:   InterestFilter that caused this Interest to be routed
   * @param interest: Interest that needs to be handled
   */
  virtual void
  onFiltersInitializationInterest(const ndn::InterestFilter& filter, const ndn::Interest& interest);

  /**
   * Helper function that generates query results from a Json query carried in the Interest
   *
   * @param interest:  Interest that needs to be handled
   */
  void
  populateFiltersMenu(std::shared_ptr<const ndn::Interest> interest);

  void
  getFiltersMenu(Json::Value& value);

  /**
   * Helper function that makes query-results data
   *
   * @param segmentPrefix:  Name that identifies the Prefix for the Data
   * @param value:          Json::Value to be sent in the Data
   * @param segmentNo:      uint64_t the segment for this Data
   * @param isFinalBlock:   bool to indicate whether this needs to be flagged in the Data as the
   *                         last entry
   * @param isAutocomplete: bool to indicate whether this is an autocomplete message
   * @param resultCount:    the number of records in the query results
   * @param viewStart:      the start index of the record in the query results payload
   * @param viewEnd:        the end index of the record in the query results payload
   * @param lastComponent:  flag to indicate the content contains the last component for
                            autocompletion query
   */
  std::shared_ptr<ndn::Data>
  makeReplyData(const ndn::Name& segmentPrefix,
                const Json::Value& value,
                uint64_t segmentNo,
                bool isFinalBlock,
                bool isAutocomplete,
                uint64_t resultCount,
                uint64_t viewStart,
                uint64_t viewEnd,
                bool lastComponent);

  /**
   * Helper function that generates query results from a Json query carried in the Interest
   *
   * @param interest:  Interest that needs to be handled
   */
  void
  runJsonQuery(std::shared_ptr<const ndn::Interest> interest);

  /**
   * Helper function that makes ACK data
   *
   * @param interest: Intersts that needs to be handled
   * @param version:  Version that needs to be in the data name
   */
  std::shared_ptr<ndn::Data>
  makeAckData(std::shared_ptr<const ndn::Interest> interest,
              const ndn::Name::Component& version);

  /**
   * Helper function that sends NACK
   *
   * @param dataPrefix: prefix for the data packet
   */
  void
  sendNack(const ndn::Name& dataPrefix);

  /**
   * Helper function that generates the sqlQuery string for component-based query
   * @param sqlQuery:     stringstream to save the sqlQuery string
   * @param jsonValue:    Json value that contains the query information
   */
  bool
  json2Sql(std::stringstream& sqlQuery,
           Json::Value& jsonValue);

  /**
   * Helper function that signs the data
   */
  void
  signData(ndn::Data& data);

  /**
   * Helper function that publishes query-results data segments
   */
  virtual void
  prepareSegments(const ndn::Name& segmentPrefix,
                  const std::string& sqlString,
                  bool autocomplete,
                  bool lastComponent);

  /**
   * Helper function to set the DatabaseHandler
   */
  void
  setDatabaseHandler(const util::ConnectionDetails&  databaseId);

  /**
   * Helper function that set filters to make the adapter work
   */
  void
  setFilters();

  void
  setCatalogId();

  /**
   * Helper function that generates the sqlQuery string for autocomplete query
   * @param sqlQuery:      stringstream to save the sqlQuery string
   * @param jsonValue:     Json value that contains the query information
   * @param lastComponent: Flag to mark the last component query
   */
  bool
  json2AutocompletionSql(std::stringstream& sqlQuery,
                         Json::Value& jsonValue,
                         bool& lastComponent);

  bool
  json2PrefixBasedSearchSql(std::stringstream& sqlQuery,
                            Json::Value& jsonValue);

  ndn::Name
  getQueryResultsName(std::shared_ptr<const ndn::Interest> interest,
                      const ndn::Name::Component& version);

  std::string
  getChronoSyncDigest();

protected:
  typedef std::unordered_map<ndn::Name, const ndn::RegisteredPrefixId*> RegisteredPrefixList;
  // Handle to the Catalog's database
  std::shared_ptr<DatabaseHandler> m_databaseHandler;
  const std::shared_ptr<chronosync::Socket>& m_socket;

  // mutex to control critical sections
  std::mutex m_mutex;
  // @{ needs m_mutex protection
  // The Queries we are currently writing to
  //std::map<std::string, std::shared_ptr<ndn::Data>> m_activeQueryToFirstResponse;
  ndn::util::InMemoryStorageLru m_activeQueryToFirstResponse;
  ndn::util::InMemoryStorageLru m_cache;
  std::string m_chronosyncDigest;
  // @}
  RegisteredPrefixList m_registeredPrefixList;
  ndn::Name m_catalogId; // should be replaced with the PK digest
  std::vector<std::string> m_filterCategoryNames;
};

template <typename DatabaseHandler>
QueryAdapter<DatabaseHandler>::QueryAdapter(const std::shared_ptr<ndn::Face>& face,
                                            const std::shared_ptr<ndn::KeyChain>& keyChain,
                                            const std::shared_ptr<chronosync::Socket>& syncSocket)
  : util::CatalogAdapter(face, keyChain)
  , m_socket(syncSocket)
  , m_activeQueryToFirstResponse(100000)
  , m_cache(250000)
  , m_chronosyncDigest("0")
  , m_catalogId("catalogIdPlaceHolder") // initialize for unitests
{
}

template <typename DatabaseHandler>
void
QueryAdapter<DatabaseHandler>::setFilters()
{
  ndn::Name queryPrefix = ndn::Name(m_prefix).append("query");
  m_registeredPrefixList[queryPrefix] = m_face->setInterestFilter(ndn::InterestFilter(queryPrefix),
                            bind(&query::QueryAdapter<DatabaseHandler>::onQueryInterest,
                                 this, _1, _2),
                            bind(&query::QueryAdapter<DatabaseHandler>::onRegisterSuccess,
                                 this, _1),
                            bind(&query::QueryAdapter<DatabaseHandler>::onRegisterFailure,
                                 this, _1, _2));

  ndn::Name queryResultsPrefix = ndn::Name(m_prefix).append("query-results");
  m_registeredPrefixList[queryResultsPrefix] =
    m_face->setInterestFilter(ndn::InterestFilter(ndn::Name(m_prefix)
                                                  .append("query-results").append(m_catalogId)),
                            bind(&query::QueryAdapter<DatabaseHandler>::onQueryResultsInterest,
                                 this, _1, _2),
                            bind(&query::QueryAdapter<DatabaseHandler>::onRegisterSuccess,
                                 this, _1),
                            bind(&query::QueryAdapter<DatabaseHandler>::onRegisterFailure,
                                 this, _1, _2));

  ndn::Name filtersInitializationPrefix = ndn::Name(m_prefix).append("filters-initialization");
  m_registeredPrefixList[filtersInitializationPrefix] =
    m_face->setInterestFilter(ndn::InterestFilter(ndn::Name(m_prefix).append("filters-initialization")),
                            bind(&query::QueryAdapter<DatabaseHandler>::onFiltersInitializationInterest,
                                 this, _1, _2),
                            bind(&query::QueryAdapter<DatabaseHandler>::onRegisterSuccess,
                                 this, _1),
                            bind(&query::QueryAdapter<DatabaseHandler>::onRegisterFailure,
                                 this, _1, _2));
}

template <typename DatabaseHandler>
void
QueryAdapter<DatabaseHandler>::setConfigFile(util::ConfigFile& config,
                                             const ndn::Name& prefix,
                                             const std::vector<std::string>& nameFields,
                                             const std::string& databaseTable)
{
  m_nameFields = nameFields;
  m_databaseTable = databaseTable;
  config.addSectionHandler("queryAdapter", bind(&QueryAdapter<DatabaseHandler>::onConfig, this,
                                                _1, _2, _3, prefix));
}

template <typename DatabaseHandler>
void
QueryAdapter<DatabaseHandler>::onConfig(const util::ConfigSection& section,
                                        bool isDryRun,
                                        const std::string& filename,
                                        const ndn::Name& prefix)
{
  using namespace util;
  if (isDryRun) {
    return;
  }
  std::string signingId, dbServer, dbName, dbUser, dbPasswd;
  for (auto item = section.begin();
       item != section.end();
       ++item)
  {
    if (item->first == "signingId") {
      signingId = item->second.get_value<std::string>();
      if (signingId.empty()) {
        throw Error("Empty value for \"signingId\""
                    " in \"query\" section");
      }
    }
    if (item->first == "filterCategoryNames") {
      std::istringstream ss(item->second.get_value<std::string>());
      std::string token;
      while(std::getline(ss, token, ',')) {
        m_filterCategoryNames.push_back(token);
      }
    }
    if (item->first == "database") {
      const util::ConfigSection& dataSection = item->second;
      for (auto subItem = dataSection.begin();
           subItem != dataSection.end();
           ++subItem)
      {
        if (subItem->first == "dbServer") {
          dbServer = subItem->second.get_value<std::string>();
        }
        if (subItem->first == "dbName") {
          dbName = subItem->second.get_value<std::string>();
        }
        if (subItem->first == "dbUser") {
          dbUser = subItem->second.get_value<std::string>();
        }
        if (subItem->first == "dbPasswd") {
          dbPasswd = subItem->second.get_value<std::string>();
        }
      }

      if (dbServer.empty()){
        throw Error("Invalid value for \"dbServer\""
                    " in \"query\" section");
      }
      if (dbName.empty()){
        throw Error("Invalid value for \"dbName\""
                    " in \"query\" section");
      }
      if (dbUser.empty()){
        throw Error("Invalid value for \"dbUser\""
                    " in \"query\" section");
      }
      if (dbPasswd.empty()){
        throw Error("Invalid value for \"dbPasswd\""
                    " in \"query\" section");
      }
    }
  }

  if (m_filterCategoryNames.empty()) {
    throw Error("Empty value for \"filterCategoryNames\" in \"query\" section");
  }

  m_prefix = prefix;

  m_signingId = ndn::Name(signingId);
  setCatalogId();

  util::ConnectionDetails mysqlId(dbServer, dbUser, dbPasswd, dbName);
  setDatabaseHandler(mysqlId);
  setFilters();
}

template <typename DatabaseHandler>
void
QueryAdapter<DatabaseHandler>::setCatalogId()
{
  //empty
}

template <>
void
QueryAdapter<MYSQL>::setCatalogId()
{
  // use public key digest as the catalog ID
  ndn::Name keyId;
  if (m_signingId.empty()) {
    keyId = m_keyChain->getDefaultKeyNameForIdentity(m_keyChain->getDefaultIdentity());
  } else {
    keyId = m_keyChain->getDefaultKeyNameForIdentity(m_signingId);
  }

  std::shared_ptr<ndn::PublicKey> pKey = m_keyChain->getPib().getPublicKey(keyId);
  ndn::Block keyDigest = pKey->computeDigest();
  m_catalogId.clear();
  m_catalogId.append(ndn::toHex(*keyDigest.getBuffer()));
}

template <typename DatabaseHandler>
void
QueryAdapter<DatabaseHandler>::setDatabaseHandler(const util::ConnectionDetails& databaseId)
{
  //empty
}

template <>
void
QueryAdapter<MYSQL>::setDatabaseHandler(const util::ConnectionDetails& databaseId)
{
  std::shared_ptr<MYSQL> conn = atmos::util::MySQLConnectionSetup(databaseId);

  m_databaseHandler = conn;
}

template <typename DatabaseHandler>
QueryAdapter<DatabaseHandler>::~QueryAdapter()
{
  for (const auto& itr : m_registeredPrefixList) {
    if (static_cast<bool>(itr.second))
      m_face->unsetInterestFilter(itr.second);
  }
}

template <typename DatabaseHandler>
void
QueryAdapter<DatabaseHandler>::onQueryInterest(const ndn::InterestFilter& filter,
                                               const ndn::Interest& interest)
{
  _LOG_DEBUG(">> QueryAdapter::onQueryInterest");

  if (interest.getName().size() != filter.getPrefix().size() + 1) {
    // @todo: return a nack
    return;
  }

  std::shared_ptr<const ndn::Interest> interestPtr = interest.shared_from_this();

  _LOG_DEBUG("Interest : " << interestPtr->getName());

  // @todo: use thread pool
  std::thread queryThread(&QueryAdapter<DatabaseHandler>::runJsonQuery,
                          this,
                          interestPtr);
  queryThread.join();
}

template <typename DatabaseHandler>
void
QueryAdapter<DatabaseHandler>::onQueryResultsInterest(const ndn::InterestFilter& filter,
                                                      const ndn::Interest& interest)
{
  // FIXME Results are currently getting served out of the forwarder's
  // CS so we just ignore any retrieval Interests that hit us for
  // now. In the future, this should check some form of
  // InMemoryStorage.

  _LOG_DEBUG(">> QueryAdapter::onQueryResultsInterest");

  auto data = m_cache.find(interest.getName());
  if (data) {
    m_face->put(*data);
  }

  _LOG_DEBUG("<< QueryAdapter::onQueryResultsInterest");
}

template <typename DatabaseHandler>
void
QueryAdapter<DatabaseHandler>::onFiltersInitializationInterest(const ndn::InterestFilter& filter,
                                                               const ndn::Interest& interest)
{
  _LOG_DEBUG(">> QueryAdapter::onFiltersInitializationInterest");
  std::shared_ptr<const ndn::Interest> interestPtr = interest.shared_from_this();

  _LOG_DEBUG("Interest : " << interestPtr->getName());

  // TODO: save the content in memory, first check the memory, if not exists, start thread to generate it
  // Note that if ChronoSync state changes, we need to clear the saved value, and regenerate it

  auto data = m_cache.find(interest.getName());
  if (data) {
    m_face->put(*data);
  }
  else {
    std::thread queryThread(&QueryAdapter<DatabaseHandler>::populateFiltersMenu,
                            this,
                            interestPtr);
    queryThread.join();
  }

  _LOG_DEBUG("<< QueryAdapter::onFiltersInitializationInterest");
}

template <typename DatabaseHandler>
void
QueryAdapter<DatabaseHandler>::populateFiltersMenu(std::shared_ptr<const ndn::Interest> interest)
{
  _LOG_DEBUG(">> QueryAdapter::populateFiltersMenu");
  Json::Value filters;
  Json::FastWriter fastWriter;
  getFiltersMenu(filters);

  const std::string filterValue = fastWriter.write(filters);

  if (!filters.empty()) {
    ndn::Name filterDataName(interest->getName());
    filterDataName.append("stateVersion");// TODO: should replace with a state version

    const char* payload = filterValue.c_str();
    size_t payloadLength = filterValue.size();
    size_t startIndex = 0, seqNo = 0;

    if (filterValue.length() > PAYLOAD_LIMIT) {
      payloadLength = PAYLOAD_LIMIT;
      ndn::Name segmentName = ndn::Name(filterDataName).appendSegment(seqNo);
      std::shared_ptr<ndn::Data> filterData = std::make_shared<ndn::Data>(segmentName);
      filterData->setFreshnessPeriod(ndn::time::milliseconds(10000));
      filterData->setContent(reinterpret_cast<const uint8_t*>(payload + startIndex), payloadLength);

      signData(*filterData);

      _LOG_DEBUG("Populate Filter Data :" << segmentName);

      m_mutex.lock();
      m_cache.insert(*filterData);
      try {
        m_face->put(*filterData);
      }
      catch (std::exception& e) {
        _LOG_ERROR(e.what());
      }
      m_mutex.unlock();

      seqNo++;
      startIndex = payloadLength * seqNo + 1;
    }
    payloadLength = filterValue.size() - PAYLOAD_LIMIT * seqNo;

    ndn::Name lastSegment = ndn::Name(filterDataName).appendSegment(seqNo);
    std::shared_ptr<ndn::Data> filterData = std::make_shared<ndn::Data>(lastSegment);
    filterData->setFreshnessPeriod(ndn::time::milliseconds(10000));
    filterData->setContent(reinterpret_cast<const uint8_t*>(payload + startIndex), payloadLength);
    filterData->setFinalBlockId(ndn::Name::Component::fromSegment(seqNo));

    signData(*filterData);
    m_mutex.lock();
    m_cache.insert(*filterData);
    m_face->put(*filterData);
    m_mutex.unlock();
  }
  _LOG_DEBUG("<< QueryAdapter::populateFiltersMenu");
}

template <typename DatabaseHandler>
void
QueryAdapter<DatabaseHandler>::getFiltersMenu(Json::Value& value)
{
  // empty
}

// get distinct value of each column
template <>
void
QueryAdapter<MYSQL>::getFiltersMenu(Json::Value& value)
{
  _LOG_DEBUG(">> QueryAdapter::getFiltersMenu");
  Json::Value tmp;

  for (size_t i = 0; i < m_filterCategoryNames.size(); i++) {
    std::string columnName = m_filterCategoryNames[i];
    std::string getFilterSql("SELECT DISTINCT " + columnName +
                             " FROM " + m_databaseTable + ";");
    std::string errMsg;
    bool success;

    std::shared_ptr<MYSQL_RES> results
      = atmos::util::MySQLPerformQuery(m_databaseHandler, getFilterSql,
                                       util::QUERY, success, errMsg);
    if (!success) {
      _LOG_ERROR(errMsg);
      value.clear();
      return;
    }

    while (MYSQL_ROW row = mysql_fetch_row(results.get()))
    {
      tmp[columnName].append(row[0]);
    }
    value.append(tmp);
    tmp.clear();
  }

  _LOG_DEBUG("<< QueryAdapter::getFiltersMenu");
}

template <typename DatabaseHandler>
void
QueryAdapter<DatabaseHandler>::signData(ndn::Data& data)
{
  if (m_signingId.empty())
    m_keyChain->sign(data);
  else {
    ndn::Name keyName = m_keyChain->getDefaultKeyNameForIdentity(m_signingId);
    ndn::Name certName = m_keyChain->getDefaultCertificateNameForKey(keyName);
    m_keyChain->sign(data, certName);
  }
}

template <typename DatabaseHandler>
ndn::Name
QueryAdapter<DatabaseHandler>::getQueryResultsName(std::shared_ptr<const ndn::Interest> interest,
                                                   const ndn::Name::Component& version)
{
  // the server side should conform: http://redmine.named-data.net/projects/ndn-atmos/wiki/Query
  // for now, should be /<prefix>/query-results/<catalog-id>/<query-parameters>/<version>

  ndn::Name queryResultName(m_prefix);
  queryResultName.append("query-results")
    .append(m_catalogId)
    .append(interest->getName().get(-1))
    .append(version);
  return queryResultName;
}

template <typename DatabaseHandler>
std::shared_ptr<ndn::Data>
QueryAdapter<DatabaseHandler>::makeAckData(std::shared_ptr<const ndn::Interest> interest,
                                           const ndn::Name::Component& version)
{
  std::string queryResultNameStr(getQueryResultsName(interest, version).toUri());

  std::shared_ptr<ndn::Data> ack = std::make_shared<ndn::Data>(interest->getName());
  ack->setContent(reinterpret_cast<const uint8_t*>(queryResultNameStr.c_str()),
                  queryResultNameStr.length());
  ack->setFreshnessPeriod(ndn::time::milliseconds(10000));

  signData(*ack);

  _LOG_DEBUG("Make ACK : " << queryResultNameStr);

  return ack;
}

template <typename DatabaseHandler>
void
QueryAdapter<DatabaseHandler>::sendNack(const ndn::Name& dataPrefix)
{
  uint64_t segmentNo = 0;

  std::shared_ptr<ndn::Data> nack =
    std::make_shared<ndn::Data>(ndn::Name(dataPrefix).appendSegment(segmentNo));
  nack->setFreshnessPeriod(ndn::time::milliseconds(10000));
  nack->setFinalBlockId(ndn::Name::Component::fromSegment(segmentNo));

  signData(*nack);

  _LOG_DEBUG("Send Nack: " << ndn::Name(dataPrefix).appendSegment(segmentNo));

  m_mutex.lock();
  m_cache.insert(*nack);
  m_mutex.unlock();
}


template <typename DatabaseHandler>
bool
QueryAdapter<DatabaseHandler>::json2Sql(std::stringstream& sqlQuery,
                                        Json::Value& jsonValue)
{
  _LOG_DEBUG(">> QueryAdapter::json2Sql");

  _LOG_DEBUG(jsonValue.toStyledString());

  if (jsonValue.type() != Json::objectValue) {
    return false;
  }

  sqlQuery << "SELECT name FROM " << m_databaseTable;
  bool input = false;
  for (Json::Value::iterator iter = jsonValue.begin(); iter != jsonValue.end(); ++iter)
  {
    Json::Value key = iter.key();
    Json::Value value = (*iter);

    if (key == Json::nullValue || value == Json::nullValue) {
      _LOG_ERROR("Null key or value in JsonValue");
      return false;
    }

    // cannot convert to string
    if (!key.isConvertibleTo(Json::stringValue) || !value.isConvertibleTo(Json::stringValue)) {
      _LOG_ERROR("Malformed JsonQuery string");
      return false;
    }

    if (key.asString().compare("?") == 0) {
      continue;
    }

    if (input) {
      sqlQuery << " AND";
    } else {
      sqlQuery << " WHERE";
    }

    sqlQuery << " " << key.asString() << "='" << value.asString() << "'";
    input = true;
  }

  if (!input) { // Force it to be the empty set
     return false;
  }
  sqlQuery << ";";
  return true;
}

template <typename DatabaseHandler>
bool
QueryAdapter<DatabaseHandler>::json2AutocompletionSql(std::stringstream& sqlQuery,
                                                      Json::Value& jsonValue,
                                                      bool& lastComponent)
{
  _LOG_DEBUG(">> QueryAdapter::json2AutocompletionSql");

  _LOG_DEBUG(jsonValue.toStyledString());

  if (jsonValue.type() != Json::objectValue) {
    return false;
  }

  std::string typedString;
  // get the string in the jsonValue
  for (Json::Value::iterator iter = jsonValue.begin(); iter != jsonValue.end(); ++iter)
  {
    Json::Value key = iter.key();
    Json::Value value = (*iter);

    if (key == Json::nullValue || value == Json::nullValue) {
      _LOG_ERROR("Null key or value in JsonValue");
      return false;
    }

    // cannot convert to string
    if (!key.isConvertibleTo(Json::stringValue) || !value.isConvertibleTo(Json::stringValue)) {
      _LOG_ERROR("Malformed JsonQuery string");
      return false;
    }

    if (key.asString().compare("?") == 0) {
      typedString = value.asString();
      // since the front end triggers the autocompletion when users typed '/',
      // there must be a '/' at the end, and the first char must be '/'
      if (typedString.at(typedString.length() - 1) != '/' || typedString.find("/") != 0)
        return false;
      break;
    }
  }

  // 1. get the expected column number by parsing the typedString, so we can get the filed name
  size_t pos = 0;
  size_t start = 1; // start from the 1st char which is not '/'
  size_t count = 0; // also the name to query for
  std::string token;
  std::string delimiter = "/";
  std::map<std::string, std::string> typedComponents;
  while ((pos = typedString.find(delimiter, start)) != std::string::npos) {
    token = typedString.substr(start, pos - start);
    if (count >= m_nameFields.size() - 1) {
      return false;
    }

    // add column name and value (token) into map
    typedComponents.insert(std::pair<std::string, std::string>(m_nameFields[count], token));
    count++;
    start = pos + 1;
  }

  // 2. generate the sql string (append what appears in the typed string, like activity='xxx'),
  // return true
  if (count == m_nameFields.size() - 1)
    lastComponent = true; // indicate this query is to query the last component

  bool more = false;
  sqlQuery << "SELECT DISTINCT " << m_nameFields[count] << " FROM " << m_databaseTable;
  for (std::map<std::string, std::string>::iterator it = typedComponents.begin();
       it != typedComponents.end(); ++it) {
    if (more)
      sqlQuery << " AND";
    else
      sqlQuery << " WHERE";

    sqlQuery << " " << it->first << "='" << it->second << "'";

    more = true;
  }
  sqlQuery << ";";
  return true;
}

template <typename DatabaseHandler>
bool
QueryAdapter<DatabaseHandler>::json2PrefixBasedSearchSql(std::stringstream& sqlQuery,
                                                         Json::Value& jsonValue)
{
  _LOG_DEBUG(">> QueryAdapter::json2CompleteSearchSql");

  _LOG_DEBUG(jsonValue.toStyledString());

  if (jsonValue.type() != Json::objectValue) {
    return false;
  }

  std::string typedString;
  // get the string in the jsonValue
  for (Json::Value::iterator iter = jsonValue.begin(); iter != jsonValue.end(); ++iter)
  {
    Json::Value key = iter.key();
    Json::Value value = (*iter);

    if (key == Json::nullValue || value == Json::nullValue) {
      _LOG_ERROR("Null key or value in JsonValue");
      return false;
    }

    // cannot convert to string
    if (!key.isConvertibleTo(Json::stringValue) || !value.isConvertibleTo(Json::stringValue)) {
      _LOG_ERROR("Malformed JsonQuery string");
      return false;
    }

    if (key.asString().compare("??") == 0) {
      typedString = value.asString();
      if (typedString.empty() || typedString.find("/") != 0)
        return false;
      break;
    }
  }

  // 1. get the expected column number by parsing the typedString, so we can get the filed name
  size_t pos = 0;
  size_t start = 1; // start from the 1st char which is not '/'
  size_t count = 0; // also the name to query for
  size_t typedStringLen = typedString.length();
  std::string token;
  std::string delimiter = "/";
  std::vector<std::pair<std::string, std::string>> typedComponents;
  while ((pos = typedString.find(delimiter, start)) != std::string::npos) {
    token = typedString.substr(start, pos - start);
    if (count >= m_nameFields.size()) {
      return false;
    }

    // add column name and value (token) into map
    typedComponents.push_back(std::make_pair(m_nameFields[count], token));

    count++;
    start = pos + 1;
  }

  // we may have a component after the last "/"
  if (start < typedStringLen) {
    typedComponents.push_back(std::make_pair(m_nameFields[count],
                                             typedString.substr(start, typedStringLen - start)));
  }

  // 2. generate the sql string (append what appears in the typed string, like activity='xxx'),
  // return true
  bool more = false;
  sqlQuery << "SELECT name FROM " << m_databaseTable;
  for (std::vector<std::pair<std::string, std::string>>::iterator it = typedComponents.begin();
       it != typedComponents.end(); ++it) {
    if (more)
      sqlQuery << " AND";
    else
      sqlQuery << " WHERE";

    sqlQuery << " " << it->first << "='" << it->second << "'";

    more = true;
  }
  sqlQuery << ";";
  return true;
}

template <typename DatabaseHandler>
void
QueryAdapter<DatabaseHandler>::runJsonQuery(std::shared_ptr<const ndn::Interest> interest)
{
  _LOG_DEBUG(">> QueryAdapter::runJsonQuery");

  // 1) Strip the prefix off the ndn::Interest's ndn::Name
  // +1 to grab JSON component after "query" component

  ndn::Name::Component jsonStr = interest->getName()[m_prefix.size()+1];
  // This one cannot parse the JsonQuery correctly, and should be moved to runJsonQuery
  const std::string jsonQuery(reinterpret_cast<const char*>(jsonStr.value()), jsonStr.value_size());

  if (jsonQuery.length() <= 0) {
    // no JSON query, send Nack?
    return;
  }

  // the version should be replaced with ChronoSync state digest
  ndn::name::Component version;

  if(m_socket != nullptr) {
    const ndn::ConstBufferPtr digestPtr = m_socket->getRootDigest();
    std::string digestStr = ndn::toHex(digestPtr->buf(), digestPtr->size());
    _LOG_DEBUG("Original digest" << m_chronosyncDigest);
    _LOG_DEBUG("New digest : " << digestStr);
    // if the m_chronosyncDigest and the rootdigest are not equal
    if (digestStr != m_chronosyncDigest) {
      // (1) update chronosyncDigest
      // (2) clear all staled ACK data
      m_mutex.lock();
      m_chronosyncDigest = digestStr;
      m_activeQueryToFirstResponse.erase(ndn::Name("/"));
      m_mutex.unlock();
      _LOG_DEBUG("Change digest to " << m_chronosyncDigest);
    }
    version = ndn::name::Component::fromEscapedString(digestStr);
  }
  else {
    version = ndn::name::Component::fromEscapedString(m_chronosyncDigest);
  }

  // try to respond with the inMemoryStorage
  m_mutex.lock();
  { // !!! BEGIN CRITICAL SECTION !!!
    auto data = m_activeQueryToFirstResponse.find(interest->getName());
    if (data) {
      _LOG_DEBUG("Answer with Data in IMS : " << data->getName());
      m_face->put(*data);
      m_mutex.unlock();
      return;
    }
  } // !!! END CRITICAL SECTION !!!
  m_mutex.unlock();

  // 2) From the remainder of the ndn::Interest's ndn::Name, get the JSON out
  Json::Value parsedFromString;
  Json::Reader reader;
  if (!reader.parse(jsonQuery, parsedFromString)) {
    // @todo: send NACK?
    _LOG_ERROR("Cannot parse the JsonQuery");
    return;
  }

  std::shared_ptr<ndn::Data> ack = makeAckData(interest, version);

  m_mutex.lock();
  { // !!! BEGIN CRITICAL SECTION !!!
    // An unusual race-condition case, which requires things like PIT aggregation to be off.
    auto data = m_activeQueryToFirstResponse.find(interest->getName());
    if (data) {
      m_face->put(*data);
      m_mutex.unlock();
      return;
    }

    // This is where things are expensive so we save them for the lock
    // note that we ack the query with the cached ACK messages, but we should remove the ACKs
    // that conatin the old version when ChronoSync is updated
    m_activeQueryToFirstResponse.insert(*ack);

    m_face->put(*ack);
  } // !!!  END  CRITICAL SECTION !!!
  m_mutex.unlock();

  // 3) Convert the JSON Query into a MySQL one
  bool autocomplete = false, lastComponent = false;
  std::stringstream sqlQuery;

  ndn::Name segmentPrefix(getQueryResultsName(interest, version));

  Json::Value tmp;
  // expect the autocomplete and the component-based query are separate
  // if JSON::Value contains ? as key, is autocompletion
  if (parsedFromString.get("?", tmp) != tmp) {
    autocomplete = true;
    if (!json2AutocompletionSql(sqlQuery, parsedFromString, lastComponent)) {
      sendNack(segmentPrefix);
      return;
    }
  }
  else if (parsedFromString.get("??", tmp) != tmp) {
    if (!json2PrefixBasedSearchSql(sqlQuery, parsedFromString)) {
      sendNack(segmentPrefix);
      return;
    }
  }
  else {
    if (!json2Sql(sqlQuery, parsedFromString)) {
      sendNack(segmentPrefix);
      return;
    }
  }

  // 4) Run the Query
  prepareSegments(segmentPrefix, sqlQuery.str(), autocomplete, lastComponent);
}

template <typename DatabaseHandler>
void
QueryAdapter<DatabaseHandler>::prepareSegments(const ndn::Name& segmentPrefix,
                                               const std::string& sqlString,
                                               bool autocomplete,
                                               bool lastComponent)
{
  // empty
}

// prepareSegments specilization function
template<>
void
QueryAdapter<MYSQL>::prepareSegments(const ndn::Name& segmentPrefix,
                                     const std::string& sqlString,
                                     bool autocomplete,
                                     bool lastComponent)
{
  _LOG_DEBUG(">> QueryAdapter::prepareSegments");

  _LOG_DEBUG(sqlString);

  std::string errMsg;
  bool success;
  // 4) Run the Query
  std::shared_ptr<MYSQL_RES> results
    = atmos::util::MySQLPerformQuery(m_databaseHandler, sqlString, util::QUERY, success, errMsg);
  if (!success)
    _LOG_ERROR(errMsg);

  if (!results) {
    _LOG_ERROR("NULL MYSQL_RES for" << sqlString);

    // @todo: throw runtime error or log the error message?
    return;
  }

  uint64_t resultCount = mysql_num_rows(results.get());

  _LOG_DEBUG("Query resuls contain " << resultCount << "rows");

  MYSQL_ROW row;
  uint64_t segmentNo = 0;
  Json::Value tmp;
  Json::Value resultJson;
  Json::FastWriter fastWriter;

  uint64_t viewStart = 0, viewEnd = 0;
  while ((row = mysql_fetch_row(results.get())))
  {
    tmp.append(row[0]);
    const std::string tmpString = fastWriter.write(tmp);
    if (tmpString.length() > PAYLOAD_LIMIT) {
      std::shared_ptr<ndn::Data> data
        = makeReplyData(segmentPrefix, resultJson, segmentNo, false,
                        autocomplete, resultCount, viewStart, viewEnd, lastComponent);
      m_mutex.lock();
      m_cache.insert(*data);
      m_mutex.unlock();
      tmp.clear();
      resultJson.clear();
      segmentNo++;
      viewStart = viewEnd + 1;
    }
    resultJson.append(row[0]);
    viewEnd++;
  }

  std::shared_ptr<ndn::Data> data
    = makeReplyData(segmentPrefix, resultJson, segmentNo, true,
                    autocomplete, resultCount, viewStart, viewEnd, lastComponent);
  m_mutex.lock();
  m_cache.insert(*data);
  m_mutex.unlock();
}

template <typename DatabaseHandler>
std::shared_ptr<ndn::Data>
QueryAdapter<DatabaseHandler>::makeReplyData(const ndn::Name& segmentPrefix,
                                             const Json::Value& value,
                                             uint64_t segmentNo,
                                             bool isFinalBlock,
                                             bool isAutocomplete,
                                             uint64_t resultCount,
                                             uint64_t viewStart,
                                             uint64_t viewEnd,
                                             bool lastComponent)
{
  Json::Value entry;
  Json::FastWriter fastWriter;

  entry["resultCount"] = Json::UInt64(resultCount);;
  entry["viewStart"] = Json::UInt64(viewStart);
  entry["viewEnd"] = Json::UInt64(viewEnd);

  if (lastComponent)
    entry["lastComponent"] = Json::Value(true);

  _LOG_DEBUG("resultCount " << resultCount << ";"
             << "viewStart " << viewStart << ";"
             << "viewEnd " << viewEnd);

  if (isAutocomplete) {
    entry["next"] = value;
  } else {
    entry["results"] = value;
  }
  const std::string jsonMessage = fastWriter.write(entry);
  const char* payload = jsonMessage.c_str();
  size_t payloadLength = jsonMessage.size() + 1;
  ndn::Name segmentName(segmentPrefix);
  segmentName.appendSegment(segmentNo);

  std::shared_ptr<ndn::Data> data = std::make_shared<ndn::Data>(segmentName);
  data->setContent(reinterpret_cast<const uint8_t*>(payload), payloadLength);
  data->setFreshnessPeriod(ndn::time::milliseconds(10000));

  if (isFinalBlock) {
    data->setFinalBlockId(ndn::Name::Component::fromSegment(segmentNo));
  }

  _LOG_DEBUG(segmentName);

  signData(*data);
  return data;
}


} // namespace query
} // namespace atmos
#endif //ATMOS_QUERY_QUERY_ADAPTER_HPP
