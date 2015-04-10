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

#ifndef ATMOS_CATALOG_CATALOG_HPP
#define ATMOS_CATALOG_CATALOG_HPP

#include "query/query-adapter.hpp"
#include "publish/publish-adapter.hpp"

#include <ndn-cxx/data.hpp>
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/security/key-chain.hpp>

#include <memory>
#include <string>

namespace atmos {
namespace catalog {

/**
 * The Catalog acts as a façade around the Database.
 * It is Templated on a DatabaseHandler: the connection into the database that it will use to
 * communicate with the actual system
 */
template <typename DatabaseHandler>
class Catalog {
public:
  /**
   * Constructor
   *
   * @param aFace:            Face that will be used for NDN communications
   * @param aKeyChain:        KeyChain to sign query responses and evaluate the incoming publish
   *                          and ChronoSync requests against
   * @param aDatabaseHandler: <typename DatabaseHandler> to the database that stores our catalog
   * @oaram aPrefix:          Name that will define the prefix to all queries and publish requests
   *                          that will be routed to this specific Catalog Instance
   */
  Catalog(std::shared_ptr<ndn::Face> aFace, std::shared_ptr<ndn::KeyChain> aKeyChain,
          std::shared_ptr<DatabaseHandler> aDatabaseHandler, const ndn::Name& aPrefix);

  /**
   * Destructor
   */
  virtual
  ~Catalog();

protected:
  // Templated Adapter to handle Query requests
  atmos::query::QueryAdapter<DatabaseHandler> m_queryAdapter;
  // Templated Adapter to handle Publisher requests
  atmos::publish::PublishAdapter<DatabaseHandler> m_publishAdapter;
}; // class Catalog

template <typename DatabaseHandler>
Catalog<DatabaseHandler>::Catalog(std::shared_ptr<ndn::Face> aFace,
                                  std::shared_ptr<ndn::KeyChain> aKeyChain,
                                  std::shared_ptr<DatabaseHandler> aDatabaseHandler,
                                  const ndn::Name& aPrefix)

  : m_queryAdapter(aFace, aKeyChain, aDatabaseHandler, aPrefix)
  , m_publishAdapter(aFace, aKeyChain, aDatabaseHandler, aPrefix)
{
  // empty
}

template <typename DatabaseHandler>
Catalog<DatabaseHandler>::~Catalog()
{
  // empty
}

} // namespace catalog
} // namespace atmos

#endif //ATMOS_CATALOG_CATALOG_HPP
