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

#include "config.hpp"
#include "catalog/catalog.hpp"
#include "query/query-adapter.hpp"
#include "publish/publish-adapter.hpp"

#include <memory>
#include <getopt.h>
#include <ndn-cxx/face.hpp>


void
usage()
{
  std::cout << "\n Usage:\n atmos-catalog "
    "[-h] [-f config file] "
    "   [-f config file]    - set the configuration file\n"
    "   [-h]                - print help and exit\n"
    "\n";
}

int
main(int argc, char** argv)
{
  int option;
  std::string configFile(DEFAULT_CONFIG_FILE);

  while ((option = getopt(argc, argv, "f:h")) != -1) {
    switch (option) {
      case 'f':
        configFile.assign(optarg);
        break;
      case 'h':
      default:
        usage();
        return 0;
    }
  }

  argc -= optind;
  argv += optind;
  if (argc != 0) {
    usage();
    return 1;
  }

  std::shared_ptr<ndn::Face> face(new ndn::Face());
  std::shared_ptr<ndn::KeyChain> keyChain(new ndn::KeyChain());

  std::unique_ptr<atmos::util::CatalogAdapter>
    queryAdapter(new atmos::query::QueryAdapter<MYSQL>(face, keyChain));
  std::unique_ptr<atmos::util::CatalogAdapter>
    publishAdapter(new atmos::publish::PublishAdapter<MYSQL>(face, keyChain));

  atmos::catalog::Catalog catalogInstance(face, keyChain, configFile);
  catalogInstance.addAdapter(queryAdapter);
  catalogInstance.addAdapter(publishAdapter);

  catalogInstance.initialize();
  face->processEvents();

  return 0;
}
