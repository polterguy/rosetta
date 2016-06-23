
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

#ifndef ROSETTA_SERVER_STATIC_FILE_HANDLER_HPP
#define ROSETTA_SERVER_STATIC_FILE_HANDLER_HPP

#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include "common/include/exceptional_executor.hpp"
#include "server/include/connection/handlers/request_handler.hpp"

namespace rosetta {
namespace server {

using std::string;
using namespace rosetta::common;

class request;
class connection;

typedef std::shared_ptr<connection> connection_ptr;


/// Handles an HTTP request.
class static_file_handler final : public request_handler
{
public:

  /// Creates a static file handler.
  static_file_handler (connection_ptr connection, request * request, const string & extension);

  /// Handles the given request.
  virtual void handle (exceptional_executor x, std::function<void (exceptional_executor x)> functor) override;

private:

  /// The file extension of the current request.
  const string _extension;
};


} // namespace server
} // namespace rosetta

#endif // ROSETTA_SERVER_STATIC_FILE_HANDLER_HPP
