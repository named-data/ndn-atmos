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

#include "catalog.hpp"

namespace atmos {
namespace catalog {

Catalog::Catalog(const std::shared_ptr<ndn::Face>& face,
                 const std::shared_ptr<ndn::KeyChain>& keyChain,
                 const std::string& configFileName)
  : m_face(face)
  , m_keyChain(keyChain)
  , m_configFile(configFileName)
{
  // empty
}

Catalog::~Catalog()
{
  // empty
}

void
Catalog::onConfig(const util::ConfigSection& configSection,
                  bool isDryRun,
                  const std::string& fileName)
{
  if (isDryRun) {
    return;
  }
  for (auto i = configSection.begin();
       i != configSection.end();
       ++ i)
  {
    if (i->first == "prefix") {
      m_prefix.clear();
      m_prefix.append(i->second.get_value<std::string>());
      if (m_prefix.empty()) {
        throw Error("Empty value for \"prefix\""
                                 " in \"general\" section");
      }
    }
  }
}

void
Catalog::addAdapter(std::unique_ptr<util::CatalogAdapter>& adapter)
{
  m_adapters.push_back(std::move(adapter));
}

void
Catalog::initializeCatalog()
{
  util::ConfigFile config(&util::ConfigFile::ignoreUnknownSection);

  config.addSectionHandler("general", bind(&Catalog::onConfig, this, _1, _2, _3));

  config.parse(m_configFile, true);
  config.parse(m_configFile, false);
}

void
Catalog::initializeAdapters()
{
  util::ConfigFile config(&util::ConfigFile::ignoreUnknownSection);
  for (auto i = m_adapters.begin();
       i != m_adapters.end();
       ++ i)
  {
    (*i)->setConfigFile(config, m_prefix);
  }

  config.parse(m_configFile, true);
  config.parse(m_configFile, false);
}

void
Catalog::initialize()
{
  initializeCatalog();
  initializeAdapters();
}

} // namespace catalog
} // namespace atmos
