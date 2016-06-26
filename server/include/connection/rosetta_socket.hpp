
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

#ifndef ROSETTA_SERVER_ROSETTA_SOCKET_HPP
#define ROSETTA_SERVER_ROSETTA_SOCKET_HPP

#include <memory>
#include <functional>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include "common/include/match_condition.hpp"

using namespace boost::asio;
using boost::system::error_code;
using namespace rosetta::common;

namespace rosetta {
namespace server {


/// A common base class, wrapping both SSL sockets and normal sockets.
/// Simply makes it possible to wrap the standard global functions from boost asio, into two
/// different implementations. One for SSL and another for plain sockets.
class rosetta_socket : public std::enable_shared_from_this<rosetta_socket>, public boost::noncopyable
{
public:

  /// Reads from socket until match condition is reached.
  virtual void async_read_until (streambuf & buffer, match_condition & match, std::function<void (const error_code & error, size_t bytes_read)> callback) = 0;

  /// Reads from socket until match condition is reached.
  virtual void async_read (streambuf & buffer, detail::transfer_exactly_t no, std::function<void (const error_code & error, size_t bytes_read)> callback) = 0;

  /// Writes to socket.
  virtual void async_write (const_buffers_1 buffer, std::function<void (const error_code & error, size_t bytes_read)> callback) = 0;

  /// Writes to socket.
  virtual void async_write (mutable_buffers_1 buffer, std::function<void (const error_code & error, size_t bytes_read)> callback) = 0;

  /// Returns remote endpoint for socket.
  virtual ip::tcp::endpoint remote_endpoint () = 0;

  /// Shuts down socket.
  virtual void shutdown (socket_base::shutdown_type what, error_code & error) = 0;

  /// Close socket.
  virtual void close () = 0;

  /// Returns true if socket is open.
  virtual bool is_open() = 0;

  /// Returns true is socket is SSL type.
  virtual bool is_secure() const = 0;
};


/// A plain socket.
class rosetta_socket_plain final : public rosetta_socket
{
public:
  
  /// Constructor initializing socket with io_service.
  rosetta_socket_plain (io_service & service)
    : _socket (service)
  { }

  /// Reads from socket until match condition is reached.
  void async_read_until (streambuf & buffer, match_condition & match, std::function<void (const error_code & error, size_t bytes_read)> callback) override
  {
    boost::asio::async_read_until (_socket, buffer, match, callback);
  }

  /// Reads from the socket until requirements are met.
  void async_read (streambuf & buffer, detail::transfer_exactly_t no, std::function<void (const error_code & error, size_t bytes_read)> callback) override
  {
    boost::asio::async_read (_socket, buffer, no, callback);
  }

  /// Writes to socket.
  void async_write (const_buffers_1 buffer, std::function<void (const error_code & error, size_t bytes_read)> callback) override
  {
    boost::asio::async_write (_socket, buffer, callback);
  }

  /// Writes to socket.
  void async_write (mutable_buffers_1 buffer, std::function<void (const error_code & error, size_t bytes_read)> callback) override
  {
    boost::asio::async_write (_socket, buffer, callback);
  }

  /// Returns remote endpoint for socket.
  ip::tcp::endpoint remote_endpoint () override { return _socket.remote_endpoint(); }

  /// Returns socket to caller.
  ip::tcp::socket & socket() { return _socket; }

  /// Shuts down socket.
  void shutdown (socket_base::shutdown_type what, error_code & error) override { _socket.shutdown (what, error); }

  /// Close socket.
  void close () override { _socket.close(); }

  /// Returns true if socket is open.
  bool is_open() override { return _socket.is_open(); }

  /// Returns false.
  virtual bool is_secure() const override { return false; }

private:

  /// Actual boost asio socket for instance
  ip::tcp::socket _socket;
};


/// An SSL socket.
class rosetta_socket_ssl final : public rosetta_socket
{
public:

  /// Constructor initializing ssl stream with io_service and context.
  rosetta_socket_ssl (io_service & service, ssl::context & context)
    : _socket (service, context)
  { }

  /// Reads from socket until match condition is reached.
  void async_read_until (streambuf & buffer, match_condition & match, std::function<void (const error_code & error, size_t bytes_read)> callback) override
  {
    boost::asio::async_read_until (_socket, buffer, match, callback);
  }

  /// Reads from the socket until requirements are met.
  void async_read (streambuf & buffer, detail::transfer_exactly_t no, std::function<void (const error_code & error, size_t bytes_read)> callback) override
  {
    boost::asio::async_read (_socket, buffer, no, callback);
  }

  /// Writes to socket.
  void async_write (const_buffers_1 buffer, std::function<void (const error_code & error, size_t bytes_read)> callback) override
  {
    boost::asio::async_write (_socket, buffer, callback);
  }

  /// Writes to socket.
  void async_write (mutable_buffers_1 buffer, std::function<void (const error_code & error, size_t bytes_read)> callback) override
  {
    boost::asio::async_write (_socket, buffer, callback);
  }

  /// Returns SSL stream wrapping socket to caller.
  ssl::stream<ip::tcp::socket> & ssl_stream() { return _socket; }

  /// Returns remote endpoint for socket.
  ip::tcp::endpoint remote_endpoint () override { return _socket.lowest_layer().remote_endpoint(); }

  /// Shuts down socket.
  void shutdown (socket_base::shutdown_type what, error_code & error) override { _socket.lowest_layer().shutdown (what, error); }

  /// Close socket.
  void close () override { _socket.lowest_layer().close(); }

  /// Returns true if socket is open.
  bool is_open() override { return _socket.lowest_layer().is_open(); }

  /// Returns true.
  virtual bool is_secure() const override { return true; }

private:

  /// Actual boost asio socket for instance
  ssl::stream<ip::tcp::socket> _socket;
};


} // namespace server
} // namespace rosetta

#endif // ROSETTA_SERVER_ROSETTA_SOCKET_HPP
