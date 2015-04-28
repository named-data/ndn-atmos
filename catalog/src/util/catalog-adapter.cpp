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

#include "catalog-adapter.hpp"

namespace atmos {
namespace util {

CatalogAdapter::CatalogAdapter(const std::shared_ptr<ndn::Face>& face,
                               const std::shared_ptr<ndn::KeyChain>& keyChain)
  : m_face(face)
  , m_keyChain(keyChain)
{
  // empty
}

CatalogAdapter::~CatalogAdapter()
{
  // empty
}

void
CatalogAdapter::onRegisterSuccess(const ndn::Name& prefix)
{
  // std::cout << "Successfully registered " << prefix << std::endl;
}

void
CatalogAdapter::onRegisterFailure(const ndn::Name& prefix, const std::string& reason)
{
  throw Error("Failed to register prefix " + prefix.toUri() + " : " + reason);
}

void
CatalogAdapter::onTimeout(const ndn::Interest& interest)
{
  // At this point, probably should do a retry
}

} // namespace util
} // namespace atmos

