
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
#include <utility>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include "common/include/rosetta_exception.hpp"
#include "common/include/configuration_exception.hpp"
#include "server/include/multi_thread_server.hpp"
#include "server/include/connection/connection.hpp"
#include "server/include/exceptions/program_exception.hpp"


namespace rosetta {
namespace server {


thread_pool_server::thread_pool_server (const class configuration & configuration)
  : server (configuration),
    _strand (_service)
{ }


void thread_pool_server::run ()
{
  typedef std::shared_ptr<boost::thread> thread_ptr;
  
  // Creating thread pool according to size from configuration.
  size_t thread_pool_size = configuration ().get ("threads", 128);
  std::vector<thread_ptr> threads;

  for (size_t i = 0; i < thread_pool_size; ++i) {
    
    // Creating thread, and binding it to thread_pool_server::thread_run().
    thread_ptr thread (std::make_shared<boost::thread> (boost::bind (&thread_pool_server::thread_run, this)));
    threads.push_back (thread);
  }

  // Wait for all threads in the pool to finish.
  for (auto t: threads) {
    t->join();
  }
}


void thread_pool_server::thread_run ()
{
  // To deal with exceptions, on a per thread basis, we need to deal with them out here, since io:service.run()
  // is a blocking operation, and potential exceptions will be thrown from async handlers.
  // This means our exceptions will propagate all the way out here.
  // To deal with them, we implement logging here, catch everything, and simply restart our io_service.
  while (true) {
    
    try {
      
      // This will block the current thread until all jobs are finished.
      // Since there's always another job in our queue, it will never return in fact, until a stop signal is given,
      // such as SIGINT or SIGTERM etc, or an exception occurs.
      io_service ().run ();
      
      // This is an attempt to gracefully shutdown the server, simply break while loop
      break;
    } catch (const std::exception & error) {
      
      // Unhandled exception, logging to std error object, before restarting io_server object.
      std::cerr << "Unhandled exception occurred, message was; '" << error.what() << "'" << std::endl;
    }
  }
}


connection_ptr thread_pool_server::create_connection (socket_ptr socket)
{
  // Creating return value outside of strand.
  connection_ptr return_value;

  // Dispatching base class invocation on the strand,
  // to make sure we don't have race conditions,
  // accessing the same list of connections.
  _strand.dispatch ([this, & return_value, socket] () {

    // Creating our connection, inside of strand.
    return_value = server::create_connection (socket);
  });
  return return_value;
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


} // namespace server
} // namespace rosetta
