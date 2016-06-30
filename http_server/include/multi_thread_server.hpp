
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

#ifndef ROSETTA_SERVER_THREAD_POOL_SERVER_HPP
#define ROSETTA_SERVER_THREAD_POOL_SERVER_HPP

#include <functional>
#include "http_server/include/server.hpp"

using namespace rosetta::common;

namespace rosetta {
namespace http_server {


/// A server object running multiple threads according to configuration
class thread_pool_server final : public server
{
public:

  /// Creates a thread-pool server instance.
  thread_pool_server (const class configuration & configuration);

  /// Overriding the run() method for a thread pool implementation.
  void run() override;

  /// Creates a new connection on the given socket. Overridden to make sure we wrap creation inside of a strand.
  void create_connection (socket_ptr socket, std::function<void(connection_ptr c)> on_success) override;

  /// Removes the specified connection from server. Overridden to make sure we wrap removal inside of a strand.
  void remove_connection (connection_ptr connection) override;

private:


  /// Strand object, to synchronize access to resources that are shared between multiple threads.
  strand _strand;
};


} // namespace http_server
} // namespace rosetta

#endif // ROSETTA_SERVER_THREAD_POOL_SERVER_HPP
