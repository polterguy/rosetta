
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

#include <memory>
#include <boost/asio.hpp>
#include "server/include/server.hpp"

using namespace rosetta::common;

namespace rosetta {
namespace server {


/// A server object running multiple threads according to configuration
class thread_pool_server final : public server
{
public:

  /// Private constructor, to allow for factory method to create correct type of server
  thread_pool_server (const class configuration & configuration);

  /// Overriding the run() method for a single thread implementation
  void run() override;

  /// Creates a new connection on the given socket.
  connection_ptr create_connection (socket_ptr socket) override;

  /// Removes the specified connection from server.
  void remove_connection (connection_ptr connection) override;

private:

  /// Starts a new thread by invoking run() on the io_service
  void thread_run ();


  /// Strand object, to synchronize access to resources that are common for multiple threads.
  boost::asio::strand _strand;
};


} // namespace server
} // namespace rosetta

#endif // ROSETTA_SERVER_THREAD_POOL_SERVER_HPP
