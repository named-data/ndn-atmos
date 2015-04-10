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

#include <json/reader.h>
#include <json/value.h>
#include <json/writer.h>

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/interest-filter.hpp>
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/security/key-chain.hpp>

#include "mysql/mysql.h"

#include <memory>
#include <string>

namespace atmos {
namespace publish {

/**
 * PublishAdapter handles the Publish usecases for the catalog
 */
template <typename DatabaseHandler>
class PublishAdapter : public atmos::util::CatalogAdapter<DatabaseHandler> {
public:
  /**
   * Constructor
   *
   * @param face:            Face that will be used for NDN communications
   * @param keyChain:        KeyChain to sign query responses and evaluate the incoming publish
   *                          and ChronoSync requests against
   * @param databaseHandler: <typename DatabaseHandler> to the database that stores our catalog
   * @oaram prefix:          Name that will define the prefix to all publish requests
   *                          that will be routed to this specific Catalog Instance
   */
  PublishAdapter(std::shared_ptr<ndn::Face> face, std::shared_ptr<ndn::KeyChain> keyChain,
                 std::shared_ptr<DatabaseHandler> databaseHandler, const ndn::Name& prefix);


  /**
   * Destructor
   */
  virtual
  ~PublishAdapter();

protected:
  /**
   * Initial "please publish this" Interests
   *
   * @param filter:   InterestFilter that caused this Interest to be routed
   * @param interest: Interest that needs to be handled
   */
  virtual void
  onInterest(const ndn::InterestFilter& filter, const ndn::Interest& interest);

  /**
   * Data containing the actual thing we need to publish
   *
   * @param interest: Interest that caused this Data to be routed
   * @param data:     Data that needs to be handled
   */
  virtual void
  onData(const ndn::Interest& interest, const ndn::Data& data);

  // @todo: Should we do anything special with the timeouts for the publish requests?
  //        If so, overwrite onTimeout()

};

template <typename DatabaseHandler>
PublishAdapter<DatabaseHandler>::PublishAdapter(std::shared_ptr<ndn::Face> face,
                           std::shared_ptr<ndn::KeyChain> keyChain,
                           std::shared_ptr<DatabaseHandler> databaseHandler,
                           const ndn::Name& prefix)
  : atmos::util::CatalogAdapter<DatabaseHandler>(face, keyChain, databaseHandler,
  ndn::Name(prefix).append("/publish"))
{
  face->setInterestFilter(ndn::InterestFilter(ndn::Name(prefix).append("/publish")),
                                               bind(&atmos::publish::PublishAdapter<DatabaseHandler>::onInterest,
                                                    this, _1, _2),
                                               bind(&atmos::publish::PublishAdapter<DatabaseHandler>::onRegisterSuccess,
                                                    this, _1),
                                               bind(&atmos::publish::PublishAdapter<DatabaseHandler>::onRegisterFailure,
                                                    this, _1, _2));

  std::shared_ptr<ndn::Interest> request(std::make_shared<ndn::Interest>(ndn::Name(prefix).append("/publish")));
  atmos::util::CatalogAdapter<DatabaseHandler>::m_face->expressInterest(*request,
                                                                        bind(&atmos::publish::PublishAdapter<DatabaseHandler>::onData,
                                                                             this, _1, _2),
                                                                        bind(&atmos::publish::PublishAdapter<DatabaseHandler>::onTimeout,
                                                                             this, _1));
}

template <typename DatabaseHandler>
PublishAdapter<DatabaseHandler>::~PublishAdapter()
{
  // empty
}

template <typename DatabaseHandler>
void
PublishAdapter<DatabaseHandler>::onInterest(const ndn::InterestFilter& filter, const ndn::Interest& interest)
{
  // @todo: Request the data for publish
}

template <typename DatabaseHandler>
void
PublishAdapter<DatabaseHandler>::onData(const ndn::Interest& interest, const ndn::Data& data)
{
  // @todo handle publishing the data
}

} // namespace publish
} // namespace atmos
#endif //ATMOS_PUBLISH_PUBLISH_ADAPTER_HPP
