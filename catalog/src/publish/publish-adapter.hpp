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

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace atmos {
namespace publish {

/**
 * PublishAdapter handles the Publish usecases for the catalog
 */
template <typename DatabaseHandler>
class PublishAdapter : public atmos::util::CatalogAdapter {
public:
  /**
   * Constructor
   *
   * @param face:      Face that will be used for NDN communications
   * @param keyChain:  KeyChain that will be used for data signing
   */
  PublishAdapter(const std::shared_ptr<ndn::Face>& face,
                 const std::shared_ptr<ndn::KeyChain>& keyChain);

  virtual
  ~PublishAdapter();

  /**
   * Helper function that subscribe to a publish section for the config file
   */
  void
  setConfigFile(util::ConfigFile& config,
                const ndn::Name& prefix,
                const std::vector<std::string>& nameFields);

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

  /**
   * Data containing the actual thing we need to publish
   *
   * @param interest: Interest that caused this Data to be routed
   * @param data:     Data that needs to be handled
   */
  virtual void
  onPublishedData(const ndn::Interest& interest, const ndn::Data& data);

  /**
   * Helper function to set the DatabaseHandler
   */
  void
  setDatabaseHandler(const util::ConnectionDetails&  databaseId);

  /**
   * Helper function that sets filters to make the adapter work
   */
  void
  setFilters();

   /**
   * Function to validate publication changes against the trust model, which is, all file
   * names must be under the publisher's prefix. This function should be called by a callback
   * function invoked by validator
   *
   * @param data: received data from the publisher
   */
  bool
  validatePublicationChanges(const std::shared_ptr<const ndn::Data>& data);

protected:
  typedef std::unordered_map<ndn::Name, const ndn::RegisteredPrefixId*> RegisteredPrefixList;
  // Prefix for ChronoSync
  ndn::Name m_syncPrefix;
  // Handle to the Catalog's database
  std::shared_ptr<DatabaseHandler> m_databaseHandler;
  std::unique_ptr<ndn::ValidatorConfig> m_publishValidator;
  RegisteredPrefixList m_registeredPrefixList;
};


template <typename DatabaseHandler>
PublishAdapter<DatabaseHandler>::PublishAdapter(const std::shared_ptr<ndn::Face>& face,
                                                const std::shared_ptr<ndn::KeyChain>& keyChain)
  : util::CatalogAdapter(face, keyChain)
{
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
                                               const std::vector<std::string>& nameFields)
{
  m_nameFields = nameFields;
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
       ++ item)
  {
    if (item->first == "signingId") {
      signingId.assign(item->second.get_value<std::string>());
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
           ++ subItem) {
        if (subItem->first == "dbServer") {
          dbServer.assign(subItem->second.get_value<std::string>());
          if (dbServer.empty()){
            throw Error("Invalid value for \"dbServer\""
                                    " in \"publish\" section");
          }
        }
        if (subItem->first == "dbName") {
          dbName.assign(subItem->second.get_value<std::string>());
          if (dbName.empty()){
            throw Error("Invalid value for \"dbName\""
                                    " in \"publish\" section");
          }
        }
        if (subItem->first == "dbUser") {
          dbUser.assign(subItem->second.get_value<std::string>());
          if (dbUser.empty()){
            throw Error("Invalid value for \"dbUser\""
                                    " in \"publish\" section");
          }
        }
        if (subItem->first == "dbPasswd") {
          dbPasswd.assign(subItem->second.get_value<std::string>());
          if (dbPasswd.empty()){
            throw Error("Invalid value for \"dbPasswd\""
                                    " in \"publish\" section");
          }
        }
      }
    }
    else if (item->first == "sync") {
      const util::ConfigSection& synSection = item->second;
      for (auto subItem = synSection.begin();
           subItem != synSection.end();
           ++ subItem) {
        if (subItem->first == "prefix") {
          syncPrefix.clear();
          syncPrefix.assign(subItem->second.get_value<std::string>());
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
  m_syncPrefix.clear();
  m_syncPrefix.append(syncPrefix);
  util::ConnectionDetails mysqlId(dbServer, dbUser, dbPasswd, dbName);

  setDatabaseHandler(mysqlId);
  setFilters();
}

template <typename DatabaseHandler>
void
PublishAdapter<DatabaseHandler>::setDatabaseHandler(const util::ConnectionDetails& databaseId)
{
  //empty
}

template <>
void
PublishAdapter<MYSQL>::setDatabaseHandler(const util::ConnectionDetails& databaseId)
{
  std::shared_ptr<MYSQL> conn = atmos::util::MySQLConnectionSetup(databaseId);

  m_databaseHandler = conn;
}

template <typename DatabaseHandler>
void
PublishAdapter<DatabaseHandler>::onPublishInterest(const ndn::InterestFilter& filter,
                                                   const ndn::Interest& interest)
{
  // @todo: Request the data for publish
}

template <typename DatabaseHandler>
void
PublishAdapter<DatabaseHandler>::onPublishedData(const ndn::Interest& interest,
                                                 const ndn::Data& data)
{
  // @todo handle data publication
}

template<typename DatabaseHandler>
bool
PublishAdapter<DatabaseHandler>::validatePublicationChanges(const std::shared_ptr<const ndn::Data>& data)
{
  // The data name must be "/<publisher-prefix>/<nonce>"
  // the prefix is the data name removes the last component
  ndn::Name publisherPrefix = data->getName().getPrefix(-1);

  const std::string payload(reinterpret_cast<const char*>(data->getContent().value()),
                            data->getContent().value_size());
  Json::Value parsedFromString;
  Json::Reader reader;
  if (!reader.parse(payload, parsedFromString)) {
    // parse error, log events
    std::cout << "Cannot parse the published data " << data->getName() << " into Json" << std::endl;
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
