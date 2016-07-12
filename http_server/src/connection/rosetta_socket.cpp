
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

#include "common/include/exceptional_executor.hpp"
#include "http_server/include/connection/rosetta_socket.hpp"
#include "http_server/include/connection/connection.hpp"

using namespace boost::asio;
using boost::system::error_code;
using namespace rosetta::common;

namespace rosetta {
namespace http_server {


void rosetta_socket_plain::async_read_until (streambuf & buffer, match_condition & match, socket_callback callback)
{
  exceptional_executor x ([this] () {
    _connection->close ();
  });
  boost::asio::async_read_until (_socket, buffer, match, [this, callback, x] (const error_code & error, size_t no_bytes) {
    x.release();
    callback (error, no_bytes);
  });
}

void rosetta_socket_plain::async_read (streambuf & buffer, boost::asio::detail::transfer_exactly_t no, socket_callback callback)
{
  exceptional_executor x ([this] () {
    _connection->close ();
  });
  boost::asio::async_read (_socket, buffer, no, [this, callback, x] (const error_code & error, size_t no_bytes) {
    x.release();
    callback (error, no_bytes);
  });
}

void rosetta_socket_plain::async_write (const_buffers_1 buffer, socket_callback callback)
{
  exceptional_executor x ([this] () {
    _connection->close ();
  });
  boost::asio::async_write (_socket, buffer, [this, callback, x] (const error_code & error, size_t no_bytes) {
    x.release();
    callback (error, no_bytes);
  });
}

void rosetta_socket_plain::async_write (mutable_buffers_1 buffer, socket_callback callback)
{
  exceptional_executor x ([this] () {
    _connection->close ();
  });
  boost::asio::async_write (_socket, buffer, [this, callback, x] (const error_code & error, size_t no_bytes) {
    x.release();
    callback (error, no_bytes);
  });
}



void rosetta_socket_ssl::async_read_until (streambuf & buffer, match_condition & match, socket_callback callback)
{
  exceptional_executor x ([this] () {
    _connection->close ();
  });
  boost::asio::async_read_until (_socket, buffer, match, [this, callback, x] (const error_code & error, size_t no_bytes) {
    x.release();
    callback (error, no_bytes);
  });
}

void rosetta_socket_ssl::async_read (streambuf & buffer, boost::asio::detail::transfer_exactly_t no, socket_callback callback)
{
  exceptional_executor x ([this] () {
    _connection->close ();
  });
  boost::asio::async_read (_socket, buffer, no, [this, callback, x] (const error_code & error, size_t no_bytes) {
    x.release();
    callback (error, no_bytes);
  });
}

void rosetta_socket_ssl::async_write (const_buffers_1 buffer, socket_callback callback)
{
  exceptional_executor x ([this] () {
    _connection->close ();
  });
  boost::asio::async_write (_socket, buffer, [this, callback, x] (const error_code & error, size_t no_bytes) {
    x.release();
    callback (error, no_bytes);
  });
}

void rosetta_socket_ssl::async_write (mutable_buffers_1 buffer, socket_callback callback)
{
  exceptional_executor x ([this] () {
    _connection->close ();
  });
  boost::asio::async_write (_socket, buffer, [this, callback, x] (const error_code & error, size_t no_bytes) {
    x.release();
    callback (error, no_bytes);
  });
}


} // namespace http_server
} // namespace rosetta

