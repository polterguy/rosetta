
/*
 * Rosetta web server, copyright(c) 2016, Thomas Hansen, phosphorusfive@gmail.com.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License, as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ROSETTA_SERVER_REQUEST_HANDLER_FACTORY_HPP
#define ROSETTA_SERVER_REQUEST_HANDLER_FACTORY_HPP

#include <memory>
#include <functional>

namespace rosetta {
namespace http_server {

class request;
class connection;
class request_handler_base;
typedef std::shared_ptr<request_handler_base> request_handler_ptr;


/// Creates the specified type of handler, according to file extension given, and configuration of server.
request_handler_ptr create_request_handler (connection_ptr connection, class request * request, int status_code = -1);


} // namespace http_server
} // namespace rosetta

#endif // ROSETTA_SERVER_REQUEST_HANDLER_FACTORY_HPP
