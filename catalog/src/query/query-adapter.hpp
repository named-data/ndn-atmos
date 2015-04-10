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

#include "mysql/mysql.h"

#include <iostream>

#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>

namespace atmos {
namespace query {

static const size_t MAX_SEGMENT_SIZE = ndn::MAX_NDN_PACKET_SIZE >> 1;

/**
 * QueryAdapter handles the Query usecases for the catalog
 */
template <typename DatabaseHandler>
class QueryAdapter : public atmos::util::CatalogAdapter<DatabaseHandler> {
public:
  /**
   * Constructor
   *
   * @param face:            Face that will be used for NDN communications
   * @param keyChain:        KeyChain to sign query responses and evaluate the incoming query
   *                          and ChronoSync requests against
   * @param databaseHandler: <typename DatabaseHandler> to the database that stores our catalog
   * @param prefix:          Name that will define the prefix to all queries requests that will be
   *                          routed to this specific Catalog Instance
   */
  QueryAdapter(std::shared_ptr<ndn::Face> face, std::shared_ptr<ndn::KeyChain> keyChain,
               std::shared_ptr<DatabaseHandler> databaseHandler, const ndn::Name& prefix);

  /**
   * Destructor
   */
  virtual
  ~QueryAdapter();

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

private:
  /**
   * Helper function that generates query results
   *
   * @param face:            Face that will be used for NDN communications
   * @param keyChain:        KeyChain to sign query responses and evaluate the incoming query
   *                          and ChronoSync requests against
   * @param databaseHandler: <typename DatabaseHandler> to the database that stores our catalog
   */
  void
  query(std::shared_ptr<ndn::Face> face, std::shared_ptr<ndn::KeyChain> keyChain,
        std::shared_ptr<const ndn::Interest> interest,
        std::shared_ptr<DatabaseHandler> databaseHandler);

  /**
   * Helper function that publishes JSON
   *
   * @param face:          Face that will send the Data out on
   * @param keyChain:      KeyChain to sign the Data we're creating
   * @param segmentPrefix: Name that identifies the Prefix for the Data
   * @param value:         Json::Value to be sent in the Data
   * @param segmentNo:     uint64_t the segment for this Data
   * @param isFinalBlock:   bool to indicate whether this needs to be flagged in the Data as the last entry
   * @param isAutocomplete: bool to indicate whether this is an autocomplete message
   */
  void
  publishJson(std::shared_ptr<ndn::Face> face, std::shared_ptr<ndn::KeyChain> keyChain,
              const ndn::Name& segmentPrefix, const Json::Value& value,
              uint64_t segmentNo, bool isFinalBlock, bool isAutocomplete);

  /**
   * Helper function that publishes char*
   *
   * @param face:          Face that will send the Data out on
   * @param keyChain:      KeyChain to sign the Data we're creating
   * @param segmentPrefix: Name that identifies the Prefix for the Data
   * @param payload:       char* to be sent in the Data
   * @param payloadLength: size_t to indicate how long payload is
   * @param segmentNo:     uint64_t the segment for this Data
   * @param isFinalBlock:   bool to indicate whether this needs to be flagged in the Data as the last entry
   */
  void
  publishSegment(std::shared_ptr<ndn::Face> face, std::shared_ptr<ndn::KeyChain> keyChain,
                 const ndn::Name& segmentPrefix, const char* payload, size_t payloadLength,
                 uint64_t segmentNo, bool isFinalBlock);

  // mutex to control critical sections
  std::mutex m_mutex;
  // @{ needs m_mutex protection
  // The Queries we are currently writing to
  std::map<std::string, std::shared_ptr<ndn::Data>> m_activeQueryToFirstResponse;
  // @}
};


template <typename DatabaseHandler>
QueryAdapter<DatabaseHandler>::QueryAdapter(std::shared_ptr<ndn::Face> face,
                           std::shared_ptr<ndn::KeyChain> keyChain,
                           std::shared_ptr<DatabaseHandler> databaseHandler,
                           const ndn::Name& prefix)
  : atmos::util::CatalogAdapter<DatabaseHandler>(face, keyChain, databaseHandler, prefix)
{
  face->setInterestFilter(ndn::InterestFilter(ndn::Name(prefix).append("query")),
                           bind(&atmos::query::QueryAdapter<DatabaseHandler>::onQueryInterest,
                                this, _1, _2),
                           bind(&atmos::query::QueryAdapter<DatabaseHandler>::onRegisterSuccess,
                                this, _1),
                           bind(&atmos::query::QueryAdapter<DatabaseHandler>::onRegisterFailure,
                                this, _1, _2));

  face->setInterestFilter(ndn::InterestFilter(ndn::Name(prefix).append("query-results")),
                           bind(&atmos::query::QueryAdapter<DatabaseHandler>::onQueryResultsInterest,
                                this, _1, _2),
                           bind(&atmos::query::QueryAdapter<DatabaseHandler>::onRegisterSuccess,
                                this, _1),
                           bind(&atmos::query::QueryAdapter<DatabaseHandler>::onRegisterFailure,
                                this, _1, _2));
}

template <typename DatabaseHandler>
QueryAdapter<DatabaseHandler>::~QueryAdapter(){
  // empty
}

template <typename DatabaseHandler>
void
QueryAdapter<DatabaseHandler>::onQueryInterest(const ndn::InterestFilter& filter, const ndn::Interest& interest)
{
  // strictly enforce query initialization namespace. Name should be our local prefix + "query" + parameters
  if (interest.getName().size() != filter.getPrefix().size() + 1) {
    // @todo: return a nack
    return;
  }

  std::shared_ptr<const ndn::Interest> interestPtr = interest.shared_from_this();

  std::thread queryThread(&QueryAdapter<DatabaseHandler>::query, this,
                          atmos::util::CatalogAdapter<DatabaseHandler>::m_face,
                          atmos::util::CatalogAdapter<DatabaseHandler>::m_keyChain, interestPtr,
                          atmos::util::CatalogAdapter<DatabaseHandler>::m_databaseHandler);
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
  std::cout << "Got to query result" << std::endl;
}

template <typename DatabaseHandler>
void
QueryAdapter<DatabaseHandler>::publishJson(std::shared_ptr<ndn::Face> face,
                                              std::shared_ptr<ndn::KeyChain> keyChain,
                                              const ndn::Name& segmentPrefix,
                                              const Json::Value& value,
                                              uint64_t segmentNo, bool isFinalBlock,
                                              bool isAutocomplete)
{
  Json::Value entry;
  Json::FastWriter fastWriter;
  if (isAutocomplete) {
    entry["next"] = value;
  } else {
    entry["results"] = value;
  }
  const std::string jsonMessage = fastWriter.write(entry);
  publishSegment(face, keyChain, segmentPrefix, jsonMessage.c_str(), jsonMessage.size() + 1,
                 segmentNo, isFinalBlock);
}

template <typename DatabaseHandler>
void
QueryAdapter<DatabaseHandler>::publishSegment(std::shared_ptr<ndn::Face> face,
                                              std::shared_ptr<ndn::KeyChain> keyChain,
                                              const ndn::Name& segmentPrefix,
                                              const char* payload, size_t payloadLength,
                                              uint64_t segmentNo, bool isFinalBlock)
{
  ndn::Name segmentName(segmentPrefix);
  if (isFinalBlock) {
    segmentName.append("END");
  } else {
    segmentName.appendSegment(segmentNo);
  }

  std::shared_ptr<ndn::Data> data = std::make_shared<ndn::Data>(segmentName);
  data->setContent(reinterpret_cast<const uint8_t*>(payload), payloadLength);
  data->setFreshnessPeriod(ndn::time::milliseconds(10000));

  if (isFinalBlock) {
    data->setFinalBlockId(segmentName[-1]);
  }
  keyChain->sign(*data);
  face->put(*data);
}

template <typename DatabaseHandler>
void
QueryAdapter<DatabaseHandler>::query(std::shared_ptr<ndn::Face> face,
                                     std::shared_ptr<ndn::KeyChain> keyChain,
                                     std::shared_ptr<const ndn::Interest> interest,
                                     std::shared_ptr<DatabaseHandler> databaseHandler)
{
  // @todo: we should return a NACK as we have no database
}


template <>
void
QueryAdapter<MYSQL>::query(std::shared_ptr<ndn::Face> face,
                           std::shared_ptr<ndn::KeyChain> keyChain,
                           std::shared_ptr<const ndn::Interest> interest,
                           std::shared_ptr<MYSQL> databaseHandler)
{
  std::cout << "Running query" << std::endl;
  // 1) Strip the prefix off the ndn::Interest's ndn::Name
  // +1 to grab JSON component after "query" component
  const std::string jsonQuery(reinterpret_cast<const char*>(interest->getName()[m_prefix.size()+1].value()));

  // For efficiency, do a double check. Once without the lock, then with it.
  if (m_activeQueryToFirstResponse.find(jsonQuery) != m_activeQueryToFirstResponse.end()) {
    m_mutex.lock();
    { // !!! BEGIN CRITICAL SECTION !!!
      // If this fails upon locking, we removed it during our search.
      // An unusual race-condition case, which requires things like PIT aggregation to be off.
      auto iter = m_activeQueryToFirstResponse.find(jsonQuery);
      if (iter != m_activeQueryToFirstResponse.end()) {
        face->put(*(iter->second));
        m_mutex.unlock(); //escape lock
        return;
      }
    } // !!!  END  CRITICAL SECTION !!!
    m_mutex.unlock();
  }

  // 2) From the remainder of the ndn::Interest's ndn::Name, get the JSON out
  Json::Value parsedFromString;
  Json::Reader reader;
  if (reader.parse(jsonQuery, parsedFromString)) {
    const ndn::name::Component version = ndn::name::Component::fromVersion(ndn::time::toUnixTimestamp(ndn::time::system_clock::now()).count());

    // JSON parsed ok, so we can acknowledge successful receipt of the query
    ndn::Name ackName(interest->getName());
    ackName.append(version);
    ackName.append("OK");

    std::shared_ptr<ndn::Data> ack(std::make_shared<ndn::Data>(ackName));

    m_mutex.lock();
    { // !!! BEGIN CRITICAL SECTION !!!
      // An unusual race-condition case, which requires things like PIT aggregation to be off.
      auto iter = m_activeQueryToFirstResponse.find(jsonQuery);
      if (iter != m_activeQueryToFirstResponse.end()) {
        face->put(*(iter->second));
        m_mutex.unlock(); // escape lock
        return;
      }
      // This is where things are expensive so we save them for the lock
      keyChain->sign(*ack);
      face->put(*ack);
      m_activeQueryToFirstResponse.insert(std::pair<std::string,
                                                    std::shared_ptr<ndn::Data>>(jsonQuery, ack));
    } // !!!  END  CRITICAL SECTION !!!
    m_mutex.unlock();

    // 3) Convert the JSON Query into a MySQL one
    bool autocomplete = false;
    std::stringstream mysqlQuery;
    mysqlQuery << "SELECT name FROM cmip5";
    bool input = false;
    for (Json::Value::iterator iter = parsedFromString.begin(); iter != parsedFromString.end(); ++iter)
    {
      Json::Value key = iter.key();
      Json::Value value = (*iter);

      if (input) {
        mysqlQuery << " AND";
      } else {
        mysqlQuery << " WHERE";
      }

      // Auto-complete case
      if (key.asString().compare("?") == 0) {
        mysqlQuery << " name REGEXP '^" << value.asString() << "'";
        autocomplete = true;
      }
      // Component case
      else {
        mysqlQuery << " " << key.asString() << "='" << value.asString() << "'";
      }
      input = true;
    }

    if (!input) { // Force it to be the empty set
      mysqlQuery << " limit 0";
    }
    mysqlQuery << ";";

    // 4) Run the Query
    // We're assuming that databaseHandler has already been connected to the database
    std::shared_ptr<MYSQL_RES> results = atmos::util::PerformQuery(databaseHandler, mysqlQuery.str());

    MYSQL_ROW row;
    ndn::Name segmentPrefix(m_prefix);
    segmentPrefix.append("query-results");
    segmentPrefix.append(version);

    size_t usedBytes = 0;
    const size_t PAYLOAD_LIMIT = 7000;
    uint64_t segmentNo = 0;
    Json::Value array;
    while ((row = mysql_fetch_row(results.get())))
    {
      size_t size = strlen(row[0]) + 1;
      if (usedBytes + size > PAYLOAD_LIMIT) {
        publishJson(face, keyChain, segmentPrefix, array, segmentNo, false, autocomplete);
        array.clear();
        usedBytes = 0;
        segmentNo++;
      }
      array.append(row[0]);
      usedBytes += size;
    }
    publishJson(face, keyChain, segmentPrefix, array, segmentNo, true, autocomplete);
  }
}

} // namespace query
} // namespace atmos
#endif //ATMOS_QUERY_QUERY_ADAPTER_HPP
