
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

#ifndef ROSETTA_SERVER_SINGLE_THREAD_SERVER_HPP
#define ROSETTA_SERVER_SINGLE_THREAD_SERVER_HPP

#include <memory>
#include <boost/asio.hpp>
#include "server/include/server.hpp"

using std::shared_ptr;
using boost::asio::ip::tcp;
using boost::asio::io_service;
using boost::asio::signal_set;
using namespace rosetta::common;

namespace rosetta {
namespace server {


/// A server object running on a single thread
class single_thread_server final : public server
{
public:

  /// Private constructor, to allow for factory method to create correct type of server
  single_thread_server (const class configuration & configuration);

  /// Overriding the run() method for a single thread implementation
  void run() override;
};


} // namespace server
} // namespace rosetta

#endif // ROSETTA_SERVER_SINGLE_THREAD_SERVER_HPP
