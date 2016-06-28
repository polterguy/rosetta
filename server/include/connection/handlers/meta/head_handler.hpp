
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

#include "common/include/exceptional_executor.hpp"
#include "server/include/connection/request_handler.hpp"

namespace rosetta {
namespace server {

using std::string;
using namespace rosetta::common;

class request;
class connection;


/// Handles an HTTP request.
class head_handler final : public request_handler
{
public:

  /// Creates a static file handler.
  head_handler (class connection * connection, class request * request, const string & extension);

  /// Handles the given request.
  virtual void handle (exceptional_executor x, functor on_success) override;

private:

  /// Writes head back to client.
  void write_head (const string & full_path, exceptional_executor x, functor on_success);


  /// The file extension of the current request.
  const string _extension;
};


} // namespace server
} // namespace rosetta

#endif // ROSETTA_SERVER_HEAD_HANDLER_HPP
