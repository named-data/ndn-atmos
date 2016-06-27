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
  onIncomingQueryInterest(const ndn::InterestFilter& filter, const ndn::Interest& interest);

  /**
   * Handles requests for responses to an filter initialization request
   *
   * @param interest: Interest that needs to be handled
   */
  virtual void
  onFiltersInitializationInterest(std::shared_ptr<const ndn::Interest> interest);

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
   * Helper function that signs the data
   */
  void
  signData(ndn::Data& data);

  /**
   * Helper function that publishes query-results data segments
   */
  virtual void
  prepareSegmentsBySqlString(const ndn::Name& segmentPrefix,
                             const std::string& sqlString,
                             bool lastComponent,
                             const std::string& nameField);

  virtual void
  prepareSegmentsByParams(std::vector<std::pair<std::string, std::string>>& queryParams,
                          const ndn::Name& segmentPrefix);

  void
  generateSegments(ResultSet_T& res,
                   const ndn::Name& segmentPrefix,
                   int resultCount,
                   bool autocomplete,
                   bool lastComponent);

  /**
   * Helper function to set the DatabaseHandler
   */
  void
  setDatabaseHandler(const util::ConnectionDetails&  databaseId);

  void
  closeDatabaseHandler();

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
   * @param nameField:     stringstream to save the nameField string
   */
  bool
  json2AutocompletionSql(std::stringstream& sqlQuery,
                         Json::Value& jsonValue,
                         bool& lastComponent,
                         std::stringstream& nameField);

  bool
  doPrefixBasedSearch(Json::Value& jsonValue,
                      std::vector<std::pair<std::string, std::string>>& typedComponents);

  bool
  doFilterBasedSearch(Json::Value& jsonValue,
                      std::vector<std::pair<std::string, std::string>>& typedComponents);

  ndn::Name
  getQueryResultsName(std::shared_ptr<const ndn::Interest> interest,
                      const ndn::Name::Component& version);

  std::string
  getChronoSyncDigest();

protected:
  typedef std::unordered_map<ndn::Name, const ndn::RegisteredPrefixId*> RegisteredPrefixList;
  // Handle to the Catalog's database
  std::shared_ptr<DatabaseHandler> m_dbConnPool;
  const std::shared_ptr<chronosync::Socket>& m_socket;

  // mutex to control critical sections
  std::mutex m_mutex;
  // @{ needs m_mutex protection
  // The Queries we are currently writing to
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
  m_registeredPrefixList[m_prefix] = m_face->setInterestFilter(ndn::InterestFilter(m_prefix),
                            bind(&query::QueryAdapter<DatabaseHandler>::onIncomingQueryInterest,
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
QueryAdapter<ConnectionPool_T>::setCatalogId()
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
QueryAdapter<ConnectionPool_T>::setDatabaseHandler(const util::ConnectionDetails& databaseId)
{
  m_dbConnPool = zdbConnectionSetup(databaseId);
}

template <typename DatabaseHandler>
void
QueryAdapter<DatabaseHandler>::closeDatabaseHandler()
{
}

template <>
void
QueryAdapter<ConnectionPool_T>::closeDatabaseHandler()
{
  ConnectionPool_stop(*m_dbConnPool);
}


template <typename DatabaseHandler>
QueryAdapter<DatabaseHandler>::~QueryAdapter()
{
  for (const auto& itr : m_registeredPrefixList) {
    if (static_cast<bool>(itr.second))
      m_face->unsetInterestFilter(itr.second);
  }

  closeDatabaseHandler();
}

template <typename DatabaseHandler>
void
QueryAdapter<DatabaseHandler>::onIncomingQueryInterest(const ndn::InterestFilter& filter,
                                                       const ndn::Interest& interest)
{
  _LOG_DEBUG(">> QueryAdapter::onIncomingQueryInterest");

  // Interest must carry component "initialization" or "query"
  if (interest.getName().size() < filter.getPrefix().size()) {
    // must NACK incorrect interest
    sendNack(interest.getName());
    return;
  }

  _LOG_DEBUG("Interest : " << interest.getName());
  std::shared_ptr<const ndn::Interest> interestPtr = interest.shared_from_this();

  if (interest.getName()[filter.getPrefix().size()] == ndn::Name::Component("filters-initialization")) {
    std::thread queryThread(&QueryAdapter<DatabaseHandler>::onFiltersInitializationInterest,
                            this,
                            interestPtr);
    queryThread.detach();
  }
  else if (interest.getName()[filter.getPrefix().size()] == ndn::Name::Component("query")) {

    auto data = m_cache.find(interest);
    if (data) {
      m_face->put(*data);
      return;
    }

    // catalog must strip sequence number in an Interest for further process
    if (interest.getName().size() > (filter.getPrefix().size() + 2)) {
      // Interest carries sequence number, only grip the main part
      // e.g., /hep/query/<query-params>/<version>/#seq
      ndn::Interest queryInterest(interest.getName().getPrefix(filter.getPrefix().size() + 2));

      auto data = m_cache.find(queryInterest);
      if (data) {
        // catalog has generated some data, but still working on it
        return;
      }
      interestPtr = std::make_shared<ndn::Interest>(queryInterest);
    }

    std::thread queryThread(&QueryAdapter<DatabaseHandler>::runJsonQuery,
                            this,
                            interestPtr);
    queryThread.detach();
  }

  // ignore other Interests
}

template <typename DatabaseHandler>
void
QueryAdapter<DatabaseHandler>::onFiltersInitializationInterest(std::shared_ptr<const ndn::Interest> interest)
{
  _LOG_DEBUG(">> QueryAdapter::onFiltersInitializationInterest");

  if(m_socket != nullptr) {
    const ndn::ConstBufferPtr digestPtr = m_socket->getRootDigest();
    std::string digestStr = ndn::toHex(digestPtr->buf(), digestPtr->size());
    _LOG_DEBUG("Original digest :" << m_chronosyncDigest);
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
  }

  auto data = m_activeQueryToFirstResponse.find(*interest);
  if (data) {
    m_face->put(*data);
  }
  else {
    populateFiltersMenu(interest);
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
    // use /<prefix>/filters-initialization/<seg> as data name
    ndn::Name filterDataName(interest->getName().getPrefix(-1));

    const char* payload = filterValue.c_str();
    size_t payloadLength = filterValue.size();
    size_t startIndex = 0, seqNo = 0;

    if (filterValue.length() > PAYLOAD_LIMIT) {
      payloadLength = PAYLOAD_LIMIT;
      ndn::Name segmentName = ndn::Name(filterDataName).appendSegment(seqNo);
      std::shared_ptr<ndn::Data> filterData = std::make_shared<ndn::Data>(segmentName);
      // freshnessPeriod 0 means permanent?
      filterData->setFreshnessPeriod(ndn::time::milliseconds(10));
      filterData->setContent(reinterpret_cast<const uint8_t*>(payload + startIndex), payloadLength);

      signData(*filterData);

      _LOG_DEBUG("Populate Filter Data :" << segmentName);

      m_mutex.lock();
      // save the filter results in the activeQueryToFirstResponse structure
      // when version changes, the activeQueryToFirstResponse should be cleaned
      m_activeQueryToFirstResponse.insert(*filterData);
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
    filterData->setFreshnessPeriod(ndn::time::milliseconds(10));
    filterData->setContent(reinterpret_cast<const uint8_t*>(payload + startIndex), payloadLength);
    filterData->setFinalBlockId(ndn::Name::Component::fromSegment(seqNo));

    signData(*filterData);
    m_mutex.lock();
    m_activeQueryToFirstResponse.insert(*filterData);
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
QueryAdapter<ConnectionPool_T>::getFiltersMenu(Json::Value& value)
{
  _LOG_DEBUG(">> QueryAdapter::getFiltersMenu");
  Json::Value tmp;

  Connection_T conn = ConnectionPool_getConnection(*m_dbConnPool);
  if (!conn) {
    _LOG_DEBUG("No available database connections");
    return;
  }

  for (size_t i = 0; i < m_filterCategoryNames.size(); i++) {
    std::string columnName = m_filterCategoryNames[i];
    std::string getFilterSql("SELECT DISTINCT " + columnName +
                             " FROM " + m_databaseTable + ";");

    ResultSet_T res4ColumnName;
    TRY {
      res4ColumnName = Connection_executeQuery(conn, reinterpret_cast<const char*>(getFilterSql.c_str()), getFilterSql.size());
    }
    CATCH(SQLException) {
      _LOG_ERROR(Connection_getLastError(conn));
    }
    END_TRY;

    while (ResultSet_next(res4ColumnName)) {
      tmp[columnName].append(ResultSet_getString(res4ColumnName, 1));
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
  // use generic name, instead of specific one
  ndn::Name queryResultName = interest->getName();
  queryResultName.append(version);
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
  m_face->put(*nack);
  m_mutex.unlock();
}

template <typename DatabaseHandler>
bool
QueryAdapter<DatabaseHandler>::json2AutocompletionSql(std::stringstream& sqlQuery,
                                                      Json::Value& jsonValue,
                                                      bool& lastComponent,
                                                      std::stringstream& fieldName)
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

  fieldName << m_nameFields[count];
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

template <typename databasehandler>
bool
QueryAdapter<databasehandler>::doPrefixBasedSearch(Json::Value& jsonValue,
                                                   std::vector<std::pair<std::string, std::string>>& typedComponents)
{
  _LOG_DEBUG(">> QueryAdapter::doPrefixBasedSearch");

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
      _LOG_ERROR("null key or value in jsonValue");
      return false;
    }

    // cannot convert to string
    if (!key.isConvertibleTo(Json::stringValue) || !value.isConvertibleTo(Json::stringValue)) {
      _LOG_ERROR("malformed jsonquery string");
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
  size_t typedStringlen = typedString.length();
  std::string token;
  std::string delimiter = "/";

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
  if (start < typedStringlen) {
    typedComponents.push_back(std::make_pair(m_nameFields[count],
                                             typedString.substr(start, typedStringlen - start)));
  }

  return true;
}


template <typename databasehandler>
bool
QueryAdapter<databasehandler>::doFilterBasedSearch(Json::Value& jsonValue,
                                                   std::vector<std::pair<std::string, std::string>>& typedComponents)
{
  _LOG_DEBUG(">> QueryAdapter::doFilterBasedSearch");

  if (jsonValue.type() != Json::objectValue) {
    return false;
  }

  for (Json::Value::iterator iter = jsonValue.begin(); iter != jsonValue.end(); ++iter)
  {
    Json::Value key = iter.key();
    Json::Value value = (*iter);

    if (key == Json::nullValue || value == Json::nullValue) {
      _LOG_ERROR("null key or value in jsonValue");
      return false;
    }

    // cannot convert to string
    if (!key.isConvertibleTo(Json::stringValue) || !value.isConvertibleTo(Json::stringValue)) {
      _LOG_ERROR("malformed jsonQuery string");
      return false;
    }

    if (key.asString().compare("?") == 0 || key.asString().compare("??") == 0) {
      continue;
    }

    _LOG_DEBUG(key.asString() << " " << value.asString());
    typedComponents.push_back(std::make_pair(key.asString(), value.asString()));
  }

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
    // no JSON query, send Nack
    sendNack(interest->getName());
    return;
  }

  // the version should be replaced with ChronoSync state digest
  ndn::name::Component version;

  if(m_socket != nullptr) {
    const ndn::ConstBufferPtr digestPtr = m_socket->getRootDigest();
    std::string digestStr = ndn::toHex(digestPtr->buf(), digestPtr->size());
    _LOG_DEBUG("Original digest " << m_chronosyncDigest);
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

  // 2) From the remainder of the ndn::Interest's ndn::Name, get the JSON out
  Json::Value parsedFromString;
  Json::Reader reader;
  if (!reader.parse(jsonQuery, parsedFromString)) {
    // json object is broken
    sendNack(interest->getName());
    _LOG_ERROR("Cannot parse the JsonQuery");
    return;
  }

  // 3) Convert the JSON Query into a MySQL one
  ndn::Name segmentPrefix(getQueryResultsName(interest, version));
  _LOG_DEBUG("segmentPrefix :" << segmentPrefix);

  Json::Value tmp;
  std::vector<std::pair<std::string, std::string>> typedComponents;

  // expect the autocomplete and the component-based query are separate
  // if Json::Value contains ? as key, is autocompletion
  if (parsedFromString.get("?", tmp) != tmp) {
    bool lastComponent = false;
    std::stringstream sqlQuery, fieldName;

    // must generate the sql string for autocomple, the selected column is changing
    if (!json2AutocompletionSql(sqlQuery, parsedFromString, lastComponent, fieldName)) {
      sendNack(segmentPrefix);
      return;
    }
    prepareSegmentsBySqlString(segmentPrefix, sqlQuery.str(), lastComponent, fieldName.str());
  }
  else if (parsedFromString.get("??", tmp) != tmp) {
    if (!doPrefixBasedSearch(parsedFromString, typedComponents)) {
      sendNack(segmentPrefix);
      return;
    }
    prepareSegmentsByParams(typedComponents, segmentPrefix);
  }
  else {
    if (!doFilterBasedSearch(parsedFromString, typedComponents)) {
      sendNack(segmentPrefix);
      return;
    }
    prepareSegmentsByParams(typedComponents, segmentPrefix);
  }

}

template <typename databasehandler>
void
QueryAdapter<databasehandler>::
prepareSegmentsByParams(std::vector<std::pair<std::string, std::string>>& queryParams,
                        const ndn::Name& segmentprefix)
{
}

template <>
void
QueryAdapter<ConnectionPool_T>::
prepareSegmentsByParams(std::vector<std::pair<std::string, std::string>>& queryParams,
                        const ndn::Name& segmentPrefix)
{
  _LOG_DEBUG(">> QueryAdapter::prepareSegmentsByParams");

  // the prepared_statement cannot improve the performance, but can simplify the code
  Connection_T conn = ConnectionPool_getConnection(*m_dbConnPool);
  if (!conn) {
    // do not answer for this request due to lack of connections, request will come back later
    _LOG_DEBUG("No available database connections");
    return;
  }
  std::string getRecordNumSqlStr("SELECT count(name) FROM ");
  getRecordNumSqlStr += m_databaseTable;
  getRecordNumSqlStr += " WHERE ";
  for (size_t i = 0; i < m_nameFields.size(); i++) {
    getRecordNumSqlStr += m_nameFields[i];
    getRecordNumSqlStr += " LIKE ?";
    if (i != m_nameFields.size() - 1) {
      getRecordNumSqlStr += " AND ";
    }
  }

  PreparedStatement_T ps4RecordNum =
    Connection_prepareStatement(conn, reinterpret_cast<const char*>(getRecordNumSqlStr.c_str()), getRecordNumSqlStr.size());

  // before query, initialize all params for statement
  for (size_t i = 0; i < m_nameFields.size(); i++) {
    PreparedStatement_setString(ps4RecordNum, i + 1, "%");
  }

  // reset params based on the query
  for (std::vector<std::pair<std::string, std::string>>::iterator it = queryParams.begin();
       it != queryParams.end(); ++it) {
    // dictionary is faster
    for (size_t i = 0; i < m_nameFields.size(); i++) {
      if (it->first == m_nameFields[i]) {
        PreparedStatement_setString(ps4RecordNum, i + 1, it->second.c_str());
      }
    }
  }

  ResultSet_T res4RecordNum;
  TRY {
    res4RecordNum = PreparedStatement_executeQuery(ps4RecordNum);
  }
  CATCH(SQLException) {
    _LOG_ERROR(Connection_getLastError(conn));
  }
  END_TRY;

  uint64_t resultCount = 0; // use count sql to get

  // result for record number
  while (ResultSet_next(res4RecordNum)) {
    resultCount = ResultSet_getInt(res4RecordNum, 1);
  }

  // get name list statement
  std::string getNameListSqlStr("SELECT name, has_metadata FROM ");
  getNameListSqlStr += m_databaseTable;
  getNameListSqlStr += " WHERE ";
  for (size_t i = 0; i < m_nameFields.size(); i++) {
    getNameListSqlStr += m_nameFields[i];
    getNameListSqlStr += " LIKE ?";
    if (i != m_nameFields.size() - 1) {
      getNameListSqlStr += " AND ";
    }
  }

  PreparedStatement_T ps4Name =
    Connection_prepareStatement(conn, reinterpret_cast<const char*>(getNameListSqlStr.c_str()), getNameListSqlStr.size());

  // before query, initialize all params for statement
  for (size_t i = 0; i < m_nameFields.size(); i++) {
    PreparedStatement_setString(ps4Name, i + 1, "%");
  }

  // reset params based on the query
  for (std::vector<std::pair<std::string, std::string>>::iterator it = queryParams.begin();
       it != queryParams.end(); ++it) {
    // dictionary is faster
    for (size_t i = 0; i < m_nameFields.size(); i++) {
      if (it->first == m_nameFields[i]) {
        PreparedStatement_setString(ps4Name, i + 1, it->second.c_str());
      }
    }
  }

  ResultSet_T res4Name;
  TRY {
    res4Name = PreparedStatement_executeQuery(ps4Name);
  }
  CATCH(SQLException) {
    _LOG_ERROR(Connection_getLastError(conn));
  }
  END_TRY;

  generateSegments(res4Name, segmentPrefix, resultCount, false, false);

  Connection_close(conn);
}

template <typename DatabaseHandler>
void
QueryAdapter<DatabaseHandler>::generateSegments(ResultSet_T& res,
                                                const ndn::Name& segmentPrefix,
                                                int resultCount,
                                                bool autocomplete,
                                                bool lastComponent)
{
  uint64_t segmentno = 0;
  Json::Value tmp, buf, resultjson;
  Json::FastWriter fastWriter;

  bool twoColumns = false;
  if (ResultSet_getColumnCount(res) > 1) {
    twoColumns = true;
  }

  uint64_t viewstart = 0, viewend = 0;
  while (ResultSet_next(res)) {
    tmp["name"] = ResultSet_getString(res, 1);
    if (twoColumns) {
      tmp["has_metadata"] = ResultSet_getInt(res, 2);
    } else {
      tmp["has_metadata"] = 0;
    }
    buf.append(tmp);
    const std::string tmpString = fastWriter.write(buf);
    if (tmpString.length() > PAYLOAD_LIMIT) {
      std::shared_ptr<ndn::Data> data
        = makeReplyData(segmentPrefix, resultjson, segmentno, false,
                        autocomplete, resultCount, viewstart, viewend, lastComponent);
      m_mutex.lock();
      m_cache.insert(*data);
      m_face->put(*data);
      m_mutex.unlock();

      buf.clear();
      resultjson.clear();
      segmentno++;
      viewstart = viewend + 1;
    }
    resultjson.append(tmp);
    buf = resultjson;
    tmp.clear();
    viewend++;
  }
  std::shared_ptr<ndn::Data> data
    = makeReplyData(segmentPrefix, resultjson, segmentno, true,
                    autocomplete, resultCount, viewstart, viewend, lastComponent);
  m_mutex.lock();
  m_cache.insert(*data);
  m_face->put(*data);
  m_mutex.unlock();
}

template <typename DatabaseHandler>
void
QueryAdapter<DatabaseHandler>::prepareSegmentsBySqlString(const ndn::Name& segmentPrefix,
                                                          const std::string& sqlString,
                                                          bool lastComponent,
                                                          const std::string& nameField)
{
  // empty
}


template <>
void
QueryAdapter<ConnectionPool_T>::prepareSegmentsBySqlString(const ndn::Name& segmentPrefix,
                                                          const std::string& sqlString,
                                                          bool lastComponent,
                                                          const std::string& nameField)
{
  _LOG_DEBUG(">> QueryAdapter::prepareSegmentsBySqlString");

  _LOG_DEBUG(sqlString);

  Connection_T conn = ConnectionPool_getConnection(*m_dbConnPool);
  if (!conn) {
    _LOG_DEBUG("No available database connections");
    return;
  }

  //// just for get the rwo count ...
  std::string getRecordNumSqlStr("SELECT COUNT( DISTINCT ");
  getRecordNumSqlStr += nameField;
  getRecordNumSqlStr += ") FROM ";
  getRecordNumSqlStr += m_databaseTable;
  getRecordNumSqlStr += sqlString;

  ResultSet_T res4RecordNum;
  TRY {
    res4RecordNum = Connection_executeQuery(conn, reinterpret_cast<const char*>(getRecordNumSqlStr.c_str()), getRecordNumSqlStr.size());
  }
  CATCH(SQLException) {
    _LOG_ERROR(Connection_getLastError(conn));
  }
  END_TRY;

  uint64_t resultCount = 0;
  while (ResultSet_next(res4RecordNum)) {
    resultCount = ResultSet_getInt(res4RecordNum, 1);
  }
  ////

  std::string getNextFieldsSqlStr("SELECT DISTINCT ");
  getNextFieldsSqlStr += nameField;
  getNextFieldsSqlStr += " FROM ";
  getNextFieldsSqlStr += m_databaseTable;
  getNextFieldsSqlStr += sqlString;

  ResultSet_T res4NextFields;
  TRY {
    res4NextFields = Connection_executeQuery(conn, reinterpret_cast<const char*>(getNextFieldsSqlStr.c_str()), getNextFieldsSqlStr.size());
  }
  CATCH(SQLException) {
    _LOG_ERROR(Connection_getLastError(conn));
  }
  END_TRY;

  generateSegments(res4NextFields, segmentPrefix, resultCount, true, lastComponent);

  Connection_close(conn);
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

  _LOG_DEBUG("resultCount " << resultCount << "; "
             << "viewStart " << viewStart << "; "
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
