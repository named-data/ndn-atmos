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

#ifndef ATMOS_PUBLISH_PUBLISH_ADAPTER_HPP
#define ATMOS_PUBLISH_PUBLISH_ADAPTER_HPP

#include "util/catalog-adapter.hpp"
#include "util/mysql-util.hpp"
#include <mysql/mysql.h>

#include <json/reader.h>
#include <json/value.h>
#include <json/writer.h>

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/interest-filter.hpp>
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/validator-config.hpp>
#include <ndn-cxx/util/string-helper.hpp>

#include <ChronoSync/socket.hpp>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

#include "util/logger.hpp"

namespace atmos {
namespace publish {
#ifdef HAVE_LOG4CXX
  INIT_LOGGER("PublishAdapter");
#endif

#define RETRY_WHEN_TIMEOUT 2

/**
 * PublishAdapter handles the Publish usecases for the catalog
 */
template <typename DatabaseHandler>
class PublishAdapter : public atmos::util::CatalogAdapter {
public:
  /**
   * Constructor
   *
   * @param face:       Face that will be used for NDN communications
   * @param keyChain:   KeyChain that will be used for data signing
   * @param syncSocket: ChronoSync socket
   */
  PublishAdapter(const std::shared_ptr<ndn::Face>& face,
                 const std::shared_ptr<ndn::KeyChain>& keyChain,
                 std::shared_ptr<chronosync::Socket>& syncSocket);

  virtual
  ~PublishAdapter();

  /**
   * Helper function that subscribe to a publish section for the config file
   */
  void
  setConfigFile(util::ConfigFile& config,
                const ndn::Name& prefix,
                const std::vector<std::string>& nameFields,
                const std::string& databaseTable);

protected:
  /**
   * Helper function that configures piblishAdapter instance according to publish section
   * in config file
   */
  void
  onConfig(const util::ConfigSection& section,
           bool isDryDun,
           const std::string& fileName,
           const ndn::Name& prefix);

   /**
   * Initial "please publish this" Interests
   *
   * @param filter:   InterestFilter that caused this Interest to be routed
   * @param interest: Interest that needs to be handled
   */
  virtual void
  onPublishInterest(const ndn::InterestFilter& filter, const ndn::Interest& interest);

  virtual void
  onTimeout(const ndn::Interest& interest);

  /**
   * Data containing the actual thing we need to publish
   *
   * @param interest: Interest that caused this Data to be routed
   * @param data:     Data that needs to be handled
   */
  virtual void
  onPublishedData(const ndn::Interest& interest, const ndn::Data& data);

  /**
   * Helper function to initialize the DatabaseHandler
   */
  void
  initializeDatabase(const util::ConnectionDetails&  databaseId);

  /**
   * Helper function that sets filters to make the adapter work
   */
  void
  setFilters();

  void
  setCatalogId();

   /**
   * Function to validate publication changes against the trust model, which is, all file
   * names must be under the publisher's prefix. This function should be called by a callback
   * function invoked by validator
   *
   * @param data: received data from the publisher
   */
  bool
  validatePublicationChanges(const std::shared_ptr<const ndn::Data>& data);


  /**
   * Helper function that processes the sync update
   *
   * @param updates: vector that contains all the missing data information
   */
  void
  processSyncUpdate(const std::vector<chronosync::MissingDataInfo>& updates);

  /**
   * Helper function that processes the update data
   *
   * @param data: shared pointer for the fetched update data
   */
  void
  processUpdateData(const std::shared_ptr<const ndn::Data>& data);

  /**
   * Helper function that add data to or remove data from database
   *
   * @param sql: sql string to do the add or remove jobs
   * @param op:  enum value indicates the database operation, could be REMOVE, ADD
   */
  virtual void
  operateDatabase(const std::string& sql,
                  util::DatabaseOperation op);

  /**
   * Helper function that parses jsonValue to generate sql string, return value indicates
   * if it is successfully
   *
   * @param sqlString: streamstream to save the sql string
   * @param jsonValue: Json value that contains the update information
   * @param op:        enum value indicates the database operation, could be REMOVE, ADD
   */
  bool
  json2Sql(std::stringstream& sqlString,
           Json::Value& jsonValue,
           util::DatabaseOperation op);

  /**
   * Helper function to generate sql string based on file name, return value indicates
   * if it is successfully
   *
   * @param sqlString: streamstream to save the sql string
   * @param fileName:  ndn uri string for a file name
   */
  bool
  name2Fields(std::stringstream& sqlstring,
              std::string& fileName);

  /**
   * Check the local database for the latest sequence number for a ChronoSync update
   *
   * @param update: the MissingDataInfo object
   */
  chronosync::SeqNo
  getLatestSeqNo(const chronosync::MissingDataInfo& update);

  /**
   * Update the local database with the update message
   *
   * @param update: the MissingDataInfo object
   */
  void
  renewUpdateInformation(const chronosync::MissingDataInfo& update);

  /**
   * Insert the update message into the local database
   *
   * @param update: the MissingDataInfo object
   */
  void
  addUpdateInformation(const chronosync::MissingDataInfo& update);

  void
  onFetchUpdateDataTimeout(const ndn::Interest& interest);

  void
  onValidationFailed(const std::shared_ptr<const ndn::Data>& data,
                     const std::string& failureInfo);

  void
  validatePublishedDataPaylod(const std::shared_ptr<const ndn::Data>& data);

protected:
  typedef std::unordered_map<ndn::Name, const ndn::RegisteredPrefixId*> RegisteredPrefixList;
  // Prefix for ChronoSync
  ndn::Name m_syncPrefix;
  // Handle to the Catalog's database
  std::shared_ptr<DatabaseHandler> m_databaseHandler;
  std::unique_ptr<ndn::ValidatorConfig> m_publishValidator;
  RegisteredPrefixList m_registeredPrefixList;
  std::shared_ptr<chronosync::Socket>& m_socket; // SyncSocket
  std::vector<std::string> m_tableColumns;
  // mutex to control critical sections
  std::mutex m_mutex;
  // TODO: create thread for each request, and the variables below should be within the thread
  bool m_mustBeFresh;
  bool m_isFinished;
  ndn::Name m_catalogId;
};


template <typename DatabaseHandler>
PublishAdapter<DatabaseHandler>::PublishAdapter(const std::shared_ptr<ndn::Face>& face,
                                                const std::shared_ptr<ndn::KeyChain>& keyChain,
                                                std::shared_ptr<chronosync::Socket>& syncSocket)
  : util::CatalogAdapter(face, keyChain)
  , m_socket(syncSocket)
  , m_mustBeFresh(true)
  , m_isFinished(false)
  , m_catalogId("catalogIdPlaceHolder")
{
}

template <typename DatabaseHandler>
void
PublishAdapter<DatabaseHandler>::setCatalogId()
{
  // empty
}

template <>
void
PublishAdapter<MYSQL>::setCatalogId()
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
PublishAdapter<DatabaseHandler>::setFilters()
{
  ndn::Name publishPrefix = ndn::Name(m_prefix).append("publish");
    m_registeredPrefixList[publishPrefix] =
      m_face->setInterestFilter(publishPrefix,
                                bind(&PublishAdapter<DatabaseHandler>::onPublishInterest,
                                     this, _1, _2),
                                bind(&publish::PublishAdapter<DatabaseHandler>::onRegisterSuccess,
                                     this, _1),
                                bind(&publish::PublishAdapter<DatabaseHandler>::onRegisterFailure,
                                     this, _1, _2));

    ndn::Name catalogSync = ndn::Name(m_prefix).append("sync").append(m_catalogId);
    m_socket.reset(new chronosync::Socket(m_syncPrefix,
                                          catalogSync,
                                          *m_face,
                                          bind(&PublishAdapter<DatabaseHandler>::processSyncUpdate,
                                               this, _1)));
}

template <typename DatabaseHandler>
PublishAdapter<DatabaseHandler>::~PublishAdapter()
{
  for (const auto& itr : m_registeredPrefixList) {
    if (static_cast<bool>(itr.second))
      m_face->unsetInterestFilter(itr.second);
  }
}

template <typename DatabaseHandler>
void
PublishAdapter<DatabaseHandler>::setConfigFile(util::ConfigFile& config,
                                               const ndn::Name& prefix,
                                               const std::vector<std::string>& nameFields,
                                               const std::string& databaseTable)
{
  m_nameFields = nameFields;

  //initialize m_tableColumns
  m_tableColumns = nameFields;
  auto it = m_tableColumns.begin();
  it = m_tableColumns.insert(it, std::string("name"));
  it = m_tableColumns.insert(it, std::string("sha256"));

  m_databaseTable = databaseTable;
  config.addSectionHandler("publishAdapter",
                           bind(&PublishAdapter<DatabaseHandler>::onConfig, this,
                                _1, _2, _3, prefix));
}

template <typename DatabaseHandler>
void
PublishAdapter<DatabaseHandler>::onConfig(const util::ConfigSection& section,
                                          bool isDryRun,
                                          const std::string& filename,
                                          const ndn::Name& prefix)
{
  using namespace util;
  if (isDryRun) {
    return;
  }

  std::string signingId, dbServer, dbName, dbUser, dbPasswd;
  std::string syncPrefix("ndn:/ndn-atmos/broadcast/chronosync");

  for (auto item = section.begin();
       item != section.end();
       ++item)
  {
    if (item->first == "signingId") {
      signingId = item->second.get_value<std::string>();
      if (signingId.empty()) {
        throw Error("Invalid value for \"signingId\""
                    " in \"publish\" section");
      }
    }
    else if (item->first == "security") {
      // when use, the validator must specify the callback func to handle the validated data
      // it should be called when the Data packet that contains the published file names is received
      m_publishValidator.reset(new ndn::ValidatorConfig(m_face.get()));
      m_publishValidator->load(item->second, filename);
    }
    else if (item->first == "database") {
      const util::ConfigSection& databaseSection = item->second;
      for (auto subItem = databaseSection.begin();
           subItem != databaseSection.end();
           ++subItem) {
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

      // Items below must not be empty
      if (dbServer.empty()){
        throw Error("Invalid value for \"dbServer\""
                    " in \"publish\" section");
      }
      if (dbName.empty()){
        throw Error("Invalid value for \"dbName\""
                    " in \"publish\" section");
      }
      if (dbUser.empty()){
        throw Error("Invalid value for \"dbUser\""
                    " in \"publish\" section");
      }
      if (dbPasswd.empty()){
        throw Error("Invalid value for \"dbPasswd\""
                    " in \"publish\" section");
      }
    }
    else if (item->first == "sync") {
      const util::ConfigSection& synSection = item->second;
      for (auto subItem = synSection.begin();
           subItem != synSection.end();
           ++subItem) {
        if (subItem->first == "prefix") {
           syncPrefix = subItem->second.get_value<std::string>();
          if (syncPrefix.empty()){
            throw Error("Invalid value for \"prefix\""
                        " in \"publish\\sync\" section");
          }
        }
        // todo: parse the sync_security section
      }
    }
  }

  m_prefix = prefix;
  m_signingId = ndn::Name(signingId);
  setCatalogId();

  m_syncPrefix = syncPrefix;
  util::ConnectionDetails mysqlId(dbServer, dbUser, dbPasswd, dbName);

  initializeDatabase(mysqlId);
  setFilters();
}

template <typename DatabaseHandler>
void
PublishAdapter<DatabaseHandler>::initializeDatabase(const util::ConnectionDetails& databaseId)
{
  //empty
}

template <>
void
PublishAdapter<MYSQL>::initializeDatabase(const util::ConnectionDetails& databaseId)
{
  std::shared_ptr<MYSQL> conn = atmos::util::MySQLConnectionSetup(databaseId);

  m_databaseHandler = conn;

  if (m_databaseHandler != nullptr) {
    std::string errMsg;
    bool success = false;
    // Ignore errors (when database already exists, errors are expected)
    std::string createSyncTable =
      "CREATE TABLE `chronosync_update_info` (\
       `id` int(11) NOT NULL AUTO_INCREMENT,  \
       `session_name` varchar(1000) NOT NULL, \
       `seq_num` int(11) NOT NULL,            \
       PRIMARY KEY (`id`),                    \
       UNIQUE KEY `id_UNIQUE` (`id`)          \
       ) ENGINE=InnoDB DEFAULT CHARSET=utf8;";

    MySQLPerformQuery(m_databaseHandler, createSyncTable, util::CREATE,
                      success, errMsg);
#ifndef NDEBUG
    if (!success)
      _LOG_DEBUG(errMsg);
#endif

    // create SQL string for table creation, id, sha256, and name are columns that we need
    std::stringstream ss;
    ss << "CREATE TABLE `" << m_databaseTable << "` (\
       `id` int(100) NOT NULL AUTO_INCREMENT,        \
       `sha256` varchar(64) NOT NULL,                \
       `name` varchar(1000) NOT NULL,";
    for (size_t i = 0; i < m_nameFields.size(); i++) {
      ss << "`" << m_nameFields[i] << "` varchar(100) NOT NULL, ";
    }
    ss << "PRIMARY KEY (`id`), UNIQUE KEY `sha256` (`sha256`)\
       ) ENGINE=InnoDB DEFAULT CHARSET=utf8;";

    success = false;
    MySQLPerformQuery(m_databaseHandler, ss.str(), util::CREATE, success, errMsg);

#ifndef NDEBUG
    if (!success)
      _LOG_DEBUG(errMsg);
#endif
  }
  else {
    throw Error("cannot connect to the Database");
  }
}

template <typename DatabaseHandler>
void
PublishAdapter<DatabaseHandler>::onPublishInterest(const ndn::InterestFilter& filter,
                                                   const ndn::Interest& interest)
{
  _LOG_DEBUG(">> PublishAdapter::onPublishInterest");

  // Example Interest : /cmip5/publish/<uri>/<nonce>
  _LOG_DEBUG(interest.getName().toUri());

  //send back ACK
  char buf[] = "ACK";
  std::shared_ptr<ndn::Data> data = std::make_shared<ndn::Data>(interest.getName());
  data->setFreshnessPeriod(ndn::time::milliseconds(10)); // 10 msec
  data->setContent(reinterpret_cast<const uint8_t*>(buf), strlen(buf));
  m_keyChain->sign(*data);
  m_face->put(*data);

  _LOG_DEBUG("Ack interest : " << interest.getName().toUri());


  //TODO: if already in catalog, what do we do?
  //ask for content
  ndn::Name interestStr = interest.getName().getSubName(m_prefix.size()+1);
  size_t m_nextSegment = 0;
  std::shared_ptr<ndn::Interest> retrieveInterest =
    std::make_shared<ndn::Interest>(interestStr.appendSegment(m_nextSegment));
  retrieveInterest->setInterestLifetime(ndn::time::milliseconds(4000));
  retrieveInterest->setMustBeFresh(m_mustBeFresh);
  m_face->expressInterest(*retrieveInterest,
                          bind(&PublishAdapter<DatabaseHandler>::onPublishedData,
                               this,_1, _2),
                          bind(&publish::PublishAdapter<DatabaseHandler>::onTimeout, this, _1));

  _LOG_DEBUG("Expressing Interest " << retrieveInterest->toUri());
  _LOG_DEBUG("<< PublishAdapter::onPublishInterest");
}

template <typename DatabaseHandler>
void
PublishAdapter<DatabaseHandler>::onTimeout(const ndn::Interest& interest)
{
  _LOG_ERROR(interest.getName() << "timed out");
}

template <typename DatabaseHandler>
void
PublishAdapter<DatabaseHandler>::onValidationFailed(const std::shared_ptr<const ndn::Data>& data,
                                                    const std::string& failureInfo)
{
  _LOG_ERROR(data->getName() << " validation failed: " << failureInfo);
}

template <typename DatabaseHandler>
void
PublishAdapter<DatabaseHandler>::onPublishedData(const ndn::Interest& interest,
                                                 const ndn::Data& data)
{
  _LOG_DEBUG(">> PublishAdapter::onPublishedData");
  _LOG_DEBUG("Recv data : " << data.getName());
  if (data.getContent().empty()) {
    return;
  }
  if (m_publishValidator != nullptr) {
    m_publishValidator->validate(data,
                                 bind(&PublishAdapter<DatabaseHandler>::validatePublishedDataPaylod, this, _1),
                                 bind(&PublishAdapter<DatabaseHandler>::onValidationFailed, this, _1, _2));
  }
  else {
    std::shared_ptr<ndn::Data> dataPtr = std::make_shared<ndn::Data>(data);
    validatePublishedDataPaylod(dataPtr);
  }
}

template <typename DatabaseHandler>
void
PublishAdapter<DatabaseHandler>::validatePublishedDataPaylod(const std::shared_ptr<const ndn::Data>& data)
{
  _LOG_DEBUG(">> PublishAdapter::onValidatePublishedDataPayload");

  // validate published data payload, if failed, return
  if (!validatePublicationChanges(data)) {
    _LOG_ERROR("Data validation failed : " << data->getName());
    const std::string payload(reinterpret_cast<const char*>(data->getContent().value()),
                              data->getContent().value_size());
    _LOG_DEBUG(payload);
    return;
  }

  // todo: return value to indicate if the insertion succeeds
  processUpdateData(data);

  // ideally, data should not be stale?
  m_socket->publishData(data->getContent(), ndn::time::seconds(3600));

  // if this is not the final block, continue to fetch the next one
  const ndn::name::Component& finalBlockId = data->getMetaInfo().getFinalBlockId();
  if (finalBlockId == data->getName()[-1]) {
    m_isFinished = true;
  }
  //else, get the next segment
  if (!m_isFinished) {
    ndn::Name nextInterestName = data->getName().getPrefix(-1);
    uint64_t incomingSegment = data->getName()[-1].toSegment();
    incomingSegment++;

    _LOG_DEBUG("Next Interest Name " << nextInterestName << " Segment " << incomingSegment);

    std::shared_ptr<ndn::Interest> nextInterest =
      std::make_shared<ndn::Interest>(nextInterestName.appendSegment(incomingSegment));
    nextInterest->setInterestLifetime(ndn::time::milliseconds(4000));
    nextInterest->setMustBeFresh(m_mustBeFresh);
    m_face->expressInterest(*nextInterest,
                            bind(&publish::PublishAdapter<DatabaseHandler>::onPublishedData,
                                 this,_1, _2),
                            bind(&publish::PublishAdapter<DatabaseHandler>::onTimeout,
                                 this, _1));
  }
}

template <typename DatabaseHandler>
void
PublishAdapter<DatabaseHandler>::processUpdateData(const std::shared_ptr<const ndn::Data>& data)
{
  _LOG_DEBUG(">> PublishAdapter::processUpdateData");

  const std::string payload(reinterpret_cast<const char*>(data->getContent().value()),
                            data->getContent().value_size());

  if (payload.length() <= 0) {
    return;
  }

  // the data payload must be JSON format
  //    http://redmine.named-data.net/projects/ndn-atmos/wiki/Sync
  Json::Value parsedFromPayload;
  Json::Reader jsonReader;
  if (!jsonReader.parse(payload, parsedFromPayload)) {
    // todo: logging events
    _LOG_DEBUG("Fail to parse the update data");
    return;
  }

  std::stringstream ss;
  if (json2Sql(ss, parsedFromPayload, util::ADD)) {
    // todo: before use, check if the connection is not NULL
    // we may need to use lock here to ensure thread safe
    operateDatabase(ss.str(), util::ADD);
  }

  ss.str("");
  ss.clear();
  if (json2Sql(ss, parsedFromPayload, util::REMOVE)) {
    operateDatabase(ss.str(), util::REMOVE);
  }
}

template <typename DatabaseHandler>
chronosync::SeqNo
PublishAdapter<DatabaseHandler>::getLatestSeqNo(const chronosync::MissingDataInfo& update)
{
  // empty
  return 0;
}

template <>
chronosync::SeqNo
PublishAdapter<MYSQL>::getLatestSeqNo(const chronosync::MissingDataInfo& update)
{
  _LOG_DEBUG(">> PublishAdapter::getLatestSeqNo");

  std::string sql = "SELECT seq_num FROM chronosync_update_info WHERE session_name = '"
    + update.session.toUri() + "';";

  std::string errMsg;
  bool success;
  std::shared_ptr<MYSQL_RES> results
    = atmos::util::MySQLPerformQuery(m_databaseHandler, sql, util::QUERY, success, errMsg);
  if (!success) {
    _LOG_DEBUG(errMsg);
    return 0; //database connection error?
  }
  else if (results != nullptr){
    MYSQL_ROW row;
    if (mysql_num_rows(results.get()) == 0)
      return 0;

    while ((row = mysql_fetch_row(results.get())))
    {
      chronosync::SeqNo seqNo = std::stoull(row[0]);
      return seqNo;
    }
  }
  return 0;
}

template <typename DatabaseHandler>
void
PublishAdapter<DatabaseHandler>::renewUpdateInformation(const chronosync::MissingDataInfo& update)
{
  //empty
}

template <>
void
PublishAdapter<MYSQL>::renewUpdateInformation(const chronosync::MissingDataInfo& update)
{
  std::string sql = "UPDATE chronosync_update_info SET seq_num = "
    + boost::lexical_cast<std::string>(update.high)
    + " WHERE session_name = '" + update.session.toUri() + "';";

  std::string errMsg;
  bool success = false;
  m_mutex.lock();
  util::MySQLPerformQuery(m_databaseHandler, sql, util::UPDATE, success, errMsg);
  m_mutex.unlock();
  if (!success)
    _LOG_ERROR(errMsg);
}

template <typename DatabaseHandler>
void
PublishAdapter<DatabaseHandler>::addUpdateInformation(const chronosync::MissingDataInfo& update)
{
  //empty
}

template <>
void
PublishAdapter<MYSQL>::addUpdateInformation(const chronosync::MissingDataInfo& update)
{
  std::string sql = "INSERT INTO chronosync_update_info (session_name, seq_num) VALUES ('"
    + update.session.toUri() + "', " + boost::lexical_cast<std::string>(update.high)
    + ");";

  std::string errMsg;
  bool success = false;
  m_mutex.lock();
  util::MySQLPerformQuery(m_databaseHandler, sql, util::ADD, success, errMsg);
  m_mutex.unlock();
  if (!success)
    _LOG_ERROR(errMsg);
}

template <typename DatabaseHandler>
void
PublishAdapter<DatabaseHandler>::onFetchUpdateDataTimeout(const ndn::Interest& interest)
{
  // todo: record event, and use recovery Interest to fetch the whole table
  _LOG_ERROR("UpdateData retrieval timed out: " << interest.getName());
}

template <typename DatabaseHandler>
void
PublishAdapter<DatabaseHandler>::processSyncUpdate(const std::vector<chronosync::MissingDataInfo>&
                                                   updates)
{
  _LOG_DEBUG(">> PublishAdapter::processSyncUpdate");

  if (updates.empty()) {
    return;
  }

  // multiple updates from different catalog are possible
  for (size_t i = 0; i < updates.size(); ++i) {
    // check if the session is in local DB
    // if yes, only fetch packets whose seq number is bigger than the one in the DB
    // if no, directly fetch Data
    chronosync::SeqNo localSeqNo = getLatestSeqNo(updates[i]);
    bool update = false;

    for (chronosync::SeqNo seq = updates[i].low; seq <= updates[i].high; ++seq) {
      if (seq > localSeqNo) {
        m_socket->fetchData(updates[i].session, seq,
                            bind(&PublishAdapter<DatabaseHandler>::processUpdateData,this, _1),
                            bind(&PublishAdapter<DatabaseHandler>::onValidationFailed,
                                 this, _1, _2),
                            bind(&PublishAdapter<DatabaseHandler>::onFetchUpdateDataTimeout,
                                 this, _1),
                            RETRY_WHEN_TIMEOUT);

        _LOG_DEBUG("Interest for [" << updates[i].session << ":" << seq << "]");

        update = true;
      }
    }
    // update the seq session name and seq number in local DB
    // indicating they are processed. So latter when this node reboots again, won't redo it
    if (update) {
      if (localSeqNo > 0)
        renewUpdateInformation(updates[i]);
      else
        addUpdateInformation(updates[i]);
    }
  }
}

template <typename DatabaseHandler>
void
PublishAdapter<DatabaseHandler>::operateDatabase(const std::string& sql, util::DatabaseOperation op)
{
  // empty
}

template <>
void
PublishAdapter<MYSQL>::operateDatabase(const std::string& sql, util::DatabaseOperation op)
{
  std::string errMsg;
  bool success = false;
  m_mutex.lock();
  atmos::util::MySQLPerformQuery(m_databaseHandler, sql, op, success, errMsg);
  m_mutex.unlock();
  if (!success)
    _LOG_ERROR(errMsg);
}

template<typename DatabaseHandler>
bool
PublishAdapter<DatabaseHandler>::json2Sql(std::stringstream& sqlString,
                                          Json::Value& jsonValue,
                                          util::DatabaseOperation op)
{
  if (jsonValue.type() != Json::objectValue) {
    return false;
  }
  if (op == util::ADD) {
    size_t updateNumber = jsonValue["add"].size();
    if (updateNumber <= 0)
      return false;

    sqlString << "INSERT INTO " << m_databaseTable << " (";
    for (size_t i = 0; i < m_tableColumns.size(); ++i) {
      if (i != 0)
        sqlString << ", ";
      sqlString << m_tableColumns[i];
    }
    sqlString << ") VALUES";

    for (size_t i = 0; i < updateNumber; ++i) { //parse each file name
      if (i > 0)
        sqlString << ",";
      // cast might be overflowed
      Json::Value item = jsonValue["add"][static_cast<int>(i)];
      if (!item.isConvertibleTo(Json::stringValue)) {
        _LOG_ERROR("Malformed JsonQuery string");
        return false;
      }
      std::string fileName(item.asString());
      // use digest sha256 for now, may be removed
      ndn::util::Digest<CryptoPP::SHA256> digest;
      digest.update(reinterpret_cast<const uint8_t*>(fileName.data()), fileName.length());

      sqlString << "('" << digest.toString() << "','" << fileName << "'";

      // parse the ndn name to get each value for each field
      if (!name2Fields(sqlString, fileName))
        return false;
      sqlString << ")";
    }
    sqlString << ";";
  }
  else if (op == util::REMOVE) {
    // remove files from db
    size_t updateNumber = jsonValue["remove"].size();
    if (updateNumber <= 0)
      return false;

    sqlString << "DELETE FROM " << m_databaseTable << " WHERE name IN (";
    for (size_t i = 0; i < updateNumber; ++i) {
      if (i > 0)
        sqlString << ",";
      // cast might be overflowed
      Json::Value item = jsonValue["remove"][static_cast<int>(i)];
      if (!item.isConvertibleTo(Json::stringValue)) {
        _LOG_ERROR("Malformed JsonQuery");
        return false;
      }
      std::string fileName(item.asString());

      sqlString << "'" << fileName << "'";
    }
    sqlString << ");";
  }
  return true;
}

template<typename DatabaseHandler>
bool
PublishAdapter<DatabaseHandler>::name2Fields(std::stringstream& sqlString,
                                             std::string& fileName)
{
  size_t start = 0;
  size_t pos = 0;
  size_t count = 0;
  std::string token;
  std::string delimiter = "/";
  // fileName must starts with either ndn:/ or /
  std::string nameWithNdn("ndn:/");
  std::string nameWithSlash("/");
  if (fileName.find(nameWithNdn) == 0) {
    start = nameWithNdn.size();
  }
  else if (fileName.find(nameWithSlash) == 0) {
    start = nameWithSlash.size();
  }
  else
    return false;

  while ((pos = fileName.find(delimiter, start)) != std::string::npos) {
    count++;
    token = fileName.substr(start, pos - start);
    // exclude the sha256 and name (already processed)
    if (count >= m_tableColumns.size() - 2) {
      return false;
    }
    sqlString << ",'" << token << "'";
    start = pos + 1;
  }

  // sha256 and name have been processed, and the last token will be processed later
  if (count != m_tableColumns.size() - 3  || std::string::npos == start)
    return false;
  token = fileName.substr(start, std::string::npos - start);
  sqlString << ",'" << token << "'";
  return true;
}

template<typename DatabaseHandler>
bool
PublishAdapter<DatabaseHandler>::validatePublicationChanges(const
                                                            std::shared_ptr<const ndn::Data>& data)
{
  _LOG_DEBUG(">> PublishAdapter::validatePublicationChanges");

  // The data name must be "/<publisher-prefix>/<nonce>"
  // the prefix is the data name removes the last component
  ndn::Name publisherPrefix = data->getName().getPrefix(-1);

  const std::string payload(reinterpret_cast<const char*>(data->getContent().value()),
                            data->getContent().value_size());
  Json::Value parsedFromString;
  Json::Reader reader;
  if (!reader.parse(payload, parsedFromString)) {
    // parse error, log events
    _LOG_DEBUG("Fail to parse the published Data" << data->getName());
    return false;
  }

  // validate added files...
  for (size_t i = 0; i < parsedFromString["add"].size(); i++) {
    if (!publisherPrefix.isPrefixOf(
          ndn::Name(parsedFromString["add"][static_cast<int>(i)].asString())))
      return false;
  }

  // validate removed files ...
  for (size_t i = 0; i < parsedFromString["remove"].size(); i++) {
    if (!publisherPrefix.isPrefixOf(
          ndn::Name(parsedFromString["remove"][static_cast<int>(i)].asString())))
      return false;
  }
  return true;
}

} // namespace publish
} // namespace atmos
#endif //ATMOS_PUBLISH_PUBLISH_ADAPTER_HPP
