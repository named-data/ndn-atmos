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
#include <mysql/errmsg.h>
#include <stdexcept>
#include <iostream>

namespace atmos {
namespace util {

ConnectionDetails::ConnectionDetails(const std::string& serverInput, const std::string& userInput,
                                     const std::string& passwordInput, const std::string& databaseInput)
  : server(serverInput), user(userInput), password(passwordInput), database(databaseInput)
{
  // empty
}

std::shared_ptr<ConnectionPool_T>
zdbConnectionSetup(const ConnectionDetails& details)
{
  std::string dbConnStr("mysql://");
  dbConnStr += details.user;
  dbConnStr += ":";
  dbConnStr += details.password;
  dbConnStr += "@";
  dbConnStr += details.server;
  dbConnStr += ":3306/";
  dbConnStr += details.database;

  URL_T url = URL_new(dbConnStr.c_str());

  ConnectionPool_T dbConnPool = ConnectionPool_new(url);
  ConnectionPool_setMaxConnections(dbConnPool, MAX_DB_CONNECTIONS);
  ConnectionPool_setReaper(dbConnPool, 1);
  ConnectionPool_start(dbConnPool);
  auto sharedPool = std::make_shared<ConnectionPool_T>(dbConnPool);
  return sharedPool;
}

} // namespace util
} // namespace atmos
