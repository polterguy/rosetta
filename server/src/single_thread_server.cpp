
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

#include <utility>
#include <iostream>
#include "common/include/rosetta_exception.hpp"
#include "common/include/configuration_exception.hpp"
#include "server/include/single_thread_server.hpp"
#include "server/include/connection/connection.hpp"

namespace rosetta {
namespace server {


single_thread_server::single_thread_server (const class configuration & configuration)
  : server (configuration)
{ }


void single_thread_server::run ()
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


} // namespace server
} // namespace rosetta
