
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

#ifndef ROSETTA_SERVER_REQUEST_HANDLER_HPP
#define ROSETTA_SERVER_REQUEST_HANDLER_HPP

#include <tuple>
#include <memory>
#include <vector>
#include <boost/asio.hpp>
#include "common/include/exceptional_executor.hpp"

namespace rosetta {
namespace server {

using std::string;
using namespace rosetta::common;

class request;
class connection;
class request_handler;
typedef std::shared_ptr<request_handler> request_handler_ptr;
typedef std::vector<std::tuple<string, string> > header_list;


/// Handles an HTTP request.
class request_handler : public boost::noncopyable
{
public:

  /// Creates the specified type of handler, according to file extension given, and configuration of server.
  static request_handler_ptr create (connection * connection, request * request, int status_code = -1);

  /// Handles the given request.
  virtual void handle (exceptional_executor x, functor callback) = 0;

protected:

  /// Protected constructor, to make sure only factory method can create instances.
  request_handler (connection * connection, request * request);

  /// Writing given HTTP headetr with given value back to client.
  void write_status (unsigned int status_code, exceptional_executor x, functor callback);

  /// Writing given HTTP header with given value back to client.
  void write_header (const string & key, const string & value, exceptional_executor x, functor callback, bool is_last = false);

  /// Writing given HTTP headers with given value back to client.
  void write_headers (header_list headers, exceptional_executor x, functor callback, bool is_last = false);

  /// Writing the given file on socket back to client.
  // Notice, the file we serve, is not necessarily the file requested. Hence, we cannot use request_envelope::extension() here.
  void write_file (const string & file_path, exceptional_executor x, functor callback);

  /// Returns connection for this instance.
  connection * connection() { return _connection; }

  /// Returns request for this instance.
  request * request() { return _request; }

private:

  /// Returns the MIME type according to file extension.
  string get_mime (const string & filepath);


  /// The connection this instance belongs to.
  class connection * _connection;

  /// The request that owns this instance.
  class request * _request;
};


} // namespace server
} // namespace rosetta

#endif // ROSETTA_SERVER_REQUEST_HANDLER_HPP
