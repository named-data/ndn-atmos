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

#ifndef ATMOS_UTIL_CONNECTION_DETAILS_HPP
#define ATMOS_UTIL_CONNECTION_DETAILS_HPP

#include "mysql/mysql.h"

#include <memory>
#include <string>

namespace atmos {
namespace util {
struct ConnectionDetails {
public:
  std::string server;
  std::string user;
  std::string password;
  std::string database;

  ConnectionDetails(const std::string& serverInput, const std::string& userInput,
                    const std::string& passwordInput, const std::string& databaseInput);
};

std::shared_ptr<MYSQL>
MySQLConnectionSetup(ConnectionDetails& details);

std::shared_ptr<MYSQL_RES>
PerformQuery(std::shared_ptr<MYSQL> connection, const std::string& sql_query);

} // namespace util
} // namespace atmos
#endif //ATMOS_UTIL_CONNECTION_DETAILS_HPP
