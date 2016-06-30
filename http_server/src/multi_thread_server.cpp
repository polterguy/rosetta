
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

#include <vector>
#include <memory>
#include <boost/thread/thread.hpp>
#include "http_server/include/multi_thread_server.hpp"
#include "http_server/include/connection/connection.hpp"


namespace rosetta {
namespace http_server {

typedef std::shared_ptr<boost::thread> thread_ptr;
typedef std::vector<thread_ptr> thread_pool;


thread_pool_server::thread_pool_server (const class configuration & configuration)
  : server (configuration),
    _strand (service ())
{ }


void thread_pool_server::run ()
{
  // Creating thread pool according to size from configuration.
  size_t thread_pool_size = configuration ().get<size_t> ("threads", 128);
  thread_pool threads;

  // Creating thread pool.
  for (size_t i = 0; i < thread_pool_size; ++i) {

    // Creating thread, and binding it to server::run().
    thread_ptr thread (std::make_shared<boost::thread> ([this] () {

      // Invoking base class run() for every single thread in thread pool.
      server::run ();
    }));

    // Storing thread in thread pool, such that we can join() all threads together later.
    threads.push_back (thread);
  }

  // Wait for all threads in the pool to finish.
  for (auto t: threads) {
    t->join();
  }
}


void thread_pool_server::create_connection (socket_ptr socket, std::function<void(connection_ptr c)> on_success)
{
  // Dispatching base class invocation on the strand,
  // to make sure we don't have race conditions,
  // accessing the same list of connections.
  _strand.dispatch ([this, socket, on_success] () {

    // Creating our connection, inside of strand.
    server::create_connection (socket, on_success);
  });
}


void thread_pool_server::remove_connection (connection_ptr connection)
{
  // Dispatching base class invocation on the strand,
  // to make sure we don't have race conditions,
  // accessing the same list of connections.
  _strand.dispatch ([this, connection] () {

    // Creating our connection, inside of strand.
    return server::remove_connection (connection);
  });
}


} // namespace http_server
} // namespace rosetta
