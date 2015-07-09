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
#include <ndn-cxx/encoding/block.hpp>

#include <memory>
#include <string>

#include <iostream>
#include "util/config-file.hpp"


namespace atmos {
namespace util {

/**
 * Catalog Adapter acts as a common interface for Classes that need to register as Interest Filters
 *
 * Both QueryAdapter and PublisherAdapter use this as a template to allow consistancy between
 * their designs and flow-control
 */
class CatalogAdapter {
public:
  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

  /**
   * Constructor
   * @param face:      Face that will be used for NDN communications
   * @param keyChain:  KeyChain that will be used for data signing
   */
  CatalogAdapter(const std::shared_ptr<ndn::Face>& face,
                 const std::shared_ptr<ndn::KeyChain>& keyChain);

  virtual
  ~CatalogAdapter();

  /**
   * Helper function that sets the configuration section handler
   * @param config: ConfigFile object to set the handler
   * @param prefix: Catalog prefix
   */
  virtual void
  setConfigFile(util::ConfigFile& config,
                const ndn::Name& prefix,
                const std::vector<std::string>& nameFields) = 0;

protected:

  /**
   * Callback function that handles the section parsing jobs
   */
  virtual void
  onConfig(const util::ConfigSection& section,
           bool isDryDun,
           const std::string& fileName,
           const ndn::Name& prefix) = 0;

  // @{ (onData and onTimeout) and/or onInterest should be overwritten at a minimum

  /**
   * Timeout from a Data request
   *
   * @param interest: Interest that failed to be satisfied (within the timelimit)
   */
  virtual void
  onTimeout(const ndn::Interest& interest);

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

protected:
  // Face to communicate with
  const std::shared_ptr<ndn::Face> m_face;
  // KeyChain used for data signing
  const std::shared_ptr<ndn::KeyChain> m_keyChain;
  ndn::Name m_prefix;
  // Name for the signing key
  ndn::Name m_signingId;
  std::vector<std::string> m_nameFields;
}; // class CatalogAdapter


} // namespace util
} // namespace atmos

#endif //ATMOS_UTIL_CATALOG_ADAPTER_HPP
