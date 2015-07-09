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

#include "util/catalog-adapter.hpp"
#include "util/config-file.hpp"

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/security/key-chain.hpp>

#include <memory>
#include <string>

namespace atmos {
namespace catalog {

/**
 * The Catalog acts as a façade around the Database.
 */
class Catalog {
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
   *
   * @param face:             Face that will be used for NDN communications
   * @param keyChain:         KeyChain that will be used for data signing
   * @param configFileName:   Configuration file that specifies the catalog configuration details
   */
  Catalog(const std::shared_ptr<ndn::Face>& face,
          const std::shared_ptr<ndn::KeyChain>& keyChain,
          const std::string& configFileName);

  virtual
  ~Catalog();

  /**
   * Function that performs the initialization of catalog instance and the adapters added in the
   * catalog. After initialization, face can be started by processEvents()
   */
  void
  initialize();

  /**
   * Helper function that adds adapters in catalog so that all adapters can be initialized when
   * the initialize() is called
   *
   * @param adapter: Adapter that will be added. Any adapter instances must be declared as the
   *                 base Class "util::CatalogAdapter"
   */
  void
  addAdapter(std::unique_ptr<util::CatalogAdapter>& adapter);

protected:

  /**
   * Helper function that configures the catalog according to the general section
   */
  void
  onConfig(const util::ConfigSection& configSection,
           bool isDryRun,
           const std::string& fileName);

  /**
   * Helper function that subscribes to the general section for the config file
   */
  void
  initializeCatalog();

  /**
   * Helper function that launches the adapters configuration processing functions
   */
  void
  initializeAdapters();

private:
  const std::shared_ptr<ndn::Face> m_face;
  const std::shared_ptr<ndn::KeyChain> m_keyChain;
  const std::string m_configFile;
  ndn::Name m_prefix;

  // Adapters that added by users
  std::vector<std::unique_ptr<util::CatalogAdapter>> m_adapters;
  std::vector<std::string> m_nameFields;
}; // class Catalog


} // namespace catalog
} // namespace atmos

#endif //ATMOS_CATALOG_CATALOG_HPP
