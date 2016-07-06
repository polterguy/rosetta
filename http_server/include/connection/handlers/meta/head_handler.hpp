
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

#ifndef ROSETTA_SERVER_HEAD_HANDLER_HPP
#define ROSETTA_SERVER_HEAD_HANDLER_HPP

#include <boost/filesystem.hpp>
#include "common/include/exceptional_executor.hpp"
#include "http_server/include/connection/handlers/request_file_handler.hpp"

using namespace boost::filesystem;
using namespace rosetta::common;

namespace rosetta {
namespace http_server {

class request;
class connection;


/// HEAD handler.
class head_handler final : public request_file_handler
{
public:

  /// Creates a HEAD handler.
  head_handler (connection_ptr connection, class request * request);

  /// Handles the given request.
  virtual void handle (std::function<void()> on_success) override;
};


} // namespace http_server
} // namespace rosetta

#endif // ROSETTA_SERVER_HEAD_HANDLER_HPP
