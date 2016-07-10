
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

#ifndef ROSETTA_SERVER_CONTENT_REQUEST_HANDLER_HPP
#define ROSETTA_SERVER_CONTENT_REQUEST_HANDLER_HPP

#include <array>
#include <fstream>
#include <iostream>
#include "common/include/exceptional_executor.hpp"
#include "http_server/include/connection/handlers/request_handler_base.hpp"

using std::array;
using std::string;
using std::istream;
using std::shared_ptr;
using namespace rosetta::common;

namespace rosetta {
namespace http_server {

class request;


/// PUT handler for static files.
class content_request_handler : public request_handler_base
{
public:

  /// Creates a PUT handler.
  content_request_handler (class request * request);

protected:

  /// Returns Content-Length of request, and verifies there is any content, and that request is not malformed.
  size_t get_content_length (connection_ptr connection);
};


} // namespace http_server
} // namespace rosetta

#endif // ROSETTA_SERVER_CONTENT_REQUEST_HANDLER_HPP
