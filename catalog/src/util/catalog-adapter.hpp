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

#ifndef ATMOS_UTIL_CATALOG_ADAPTER_HPP
#define ATMOS_UTIL_CATALOG_ADAPTER_HPP

#include <ndn-cxx/data.hpp>
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/security/key-chain.hpp>

#include <memory>
#include <string>


#include <ndn-cxx/encoding/block.hpp>

#include <iostream>

namespace atmos {
namespace util {

/**
 * Catalog Adapter acts as a common interface for Classes that need to register as Interest Filters
 *
 * Both QueryAdapter and PublisherAdapter use this as a template to allow consistancy between
 * their designs and flow-control
 */
template <typename DatabaseHandler>
class CatalogAdapter {
public:
  /**
   * Constructor
   * @param face:            Face that will be used for NDN communications
   * @param keyChain:        KeyChain to sign query responses and evaluate the incoming publish
   *                          and ChronoSync requests against
   * @param databaseHandler: <typename DatabaseHandler> to the database that stores our catalog
   * @oaram prefix:          Name that will define the prefix to all queries and publish requests
   *                          that will be routed to this specific Catalog Instance
   */
  CatalogAdapter(std::shared_ptr<ndn::Face> face, std::shared_ptr<ndn::KeyChain> keyChain,
          std::shared_ptr<DatabaseHandler> databaseHandler, const ndn::Name& prefix);

  /**
   * Destructor
   */
  virtual
  ~CatalogAdapter();

protected:
  // @{ (onData and onTimeout) and/or onInterest should be overwritten at a minimum


  /**
   * Data that is routed to this class based on the Interest
   *
   * @param interest: Interest that caused this Data to be routed
   * @param data:     Data that needs to be handled
   */
  virtual void
  onData(const ndn::Interest& interest, const ndn::Data& data);

  /**
   * Timeout from a Data request
   *
   * @param interest: Interest that failed to be satisfied (within the timelimit)
   */
  virtual void
  onTimeout(const ndn::Interest& interest);


  /**
   * Interest that is routed to this class based on the InterestFilter
   *
   * @param filter:   InterestFilter that caused this Interest to be routed
   * @param interest: Interest that needs to be handled
   */
  virtual void
  onInterest(const ndn::InterestFilter& filter, const ndn::Interest& interest);
  // @}

  /**
   * Callback that should/can be used to evaluate that the Interest Filter has been correctly set up
   *
   * @param prefix: Name that will be routed to this class
   */
  virtual void
  onRegisterSuccess(const ndn::Name& prefix);

  /**
   * Callback that should/can be used to evaluate that the Interest Filter has been correctly set up
   *
   * @param prefix: Name that failed to route
   * @param reason: String explanation as to why the failure occured
   */
  virtual void
  onRegisterFailure(const ndn::Name& prefix, const std::string& reason);


  // Face to communicate with
  std::shared_ptr<ndn::Face> m_face;
  // KeyChain used for security
  std::shared_ptr<ndn::KeyChain> m_keyChain;
  // Handle to the Catalog's database
  std::shared_ptr<DatabaseHandler> m_databaseHandler;
  // Prefix for our namespace
  ndn::Name m_prefix;
}; // class CatalogAdapter

template <typename DatabaseHandler>
CatalogAdapter<DatabaseHandler>::CatalogAdapter(std::shared_ptr<ndn::Face> face,
                                                std::shared_ptr<ndn::KeyChain> keyChain,
                                                std::shared_ptr<DatabaseHandler> databaseHandler,
                                                const ndn::Name& prefix)
  : m_face(face), m_keyChain(keyChain), m_databaseHandler(databaseHandler), m_prefix(prefix)
{
  // empty
}

template <typename DatabaseHandler>
CatalogAdapter<DatabaseHandler>::~CatalogAdapter()
{
  // empty
}

template <typename DatabaseHandler>
void
CatalogAdapter<DatabaseHandler>::onRegisterSuccess(const ndn::Name& prefix)
{
  // std::cout << "Successfully registered " << prefix << std::endl;
}

template <typename DatabaseHandler>
void
CatalogAdapter<DatabaseHandler>::onRegisterFailure(const ndn::Name& prefix, const std::string& reason)
{
  // std::cout << "Failed to register prefix " << prefix << ": " << reason << std::endl;
}


template <typename DatabaseHandler>
void
CatalogAdapter<DatabaseHandler>::onData(const ndn::Interest& interest, const ndn::Data& data)
{
  // At this point we need to get the ndn::Block out of data.getContent()
}

template <typename DatabaseHandler>
void
CatalogAdapter<DatabaseHandler>::onInterest(const ndn::InterestFilter& filter, const ndn::Interest& interest)
{
  // At this point we need to use the filter to either:
  // a) Request the Data for the Interest, or
  // b) Use the Filter to ID where in the Interest the Interest's "Content" is, and grab that out
}


template <typename DatabaseHandler>
void
CatalogAdapter<DatabaseHandler>::onTimeout(const ndn::Interest& interest)
{
  // At this point, probably should do a retry
}

} // namespace util
} // namespace atmos

#endif //ATMOS_UTIL_CATALOG_ADAPTER_HPP
