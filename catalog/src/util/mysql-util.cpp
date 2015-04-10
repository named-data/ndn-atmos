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

#include "util/mysql-util.hpp"

#include "mysql/errmsg.h"

namespace atmos {
namespace util {

ConnectionDetails::ConnectionDetails(const std::string& serverInput, const std::string& userInput,
                                     const std::string& passwordInput, const std::string& databaseInput)
  : server(serverInput), user(userInput), password(passwordInput), database(databaseInput)
{
  // empty
}


std::shared_ptr<MYSQL>
MySQLConnectionSetup(ConnectionDetails& details) {
  MYSQL* conn = mysql_init(NULL);
  mysql_real_connect(conn, details.server.c_str(), details.user.c_str(),
                     details.password.c_str(), details.database.c_str(), 0, NULL, 0);
  std::shared_ptr<MYSQL> connection(conn);
  return connection;
}

std::shared_ptr<MYSQL_RES>
PerformQuery(std::shared_ptr<MYSQL> connection, const std::string& sql_query) {
  std::shared_ptr<MYSQL_RES> result(NULL);

  switch (mysql_query(connection.get(), sql_query.c_str()))
  {
    case 0:
    {
      MYSQL_RES* resultPtr = mysql_store_result(connection.get());
      if (resultPtr != NULL)
      {
        result.reset(resultPtr);
      }
      break;
    }
    // Various error cases
    case CR_COMMANDS_OUT_OF_SYNC:
    case CR_SERVER_GONE_ERROR:
    case CR_SERVER_LOST:
    case CR_UNKNOWN_ERROR:
    default:
      break;
  }
  return result;
}

} // namespace util
} // namespace atmos
