
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

#ifndef ROSETTA_SERVER_REQUEST_ENVELOPE_HPP
#define ROSETTA_SERVER_REQUEST_ENVELOPE_HPP

#include <map>
#include <tuple>
#include <string>
#include <vector>
#include <boost/filesystem.hpp>
#include "common/include/exceptional_executor.hpp"

namespace rosetta {
namespace http {

using std::string;
using namespace boost::filesystem;
using namespace rosetta::common;

class connection;
class request;

// Helpers for HTTP headers and GET parameters collections types.
typedef std::tuple<string, string> collection_type;
typedef std::vector<collection_type> collection;
typedef const std::vector<collection_type> const_collection;


/// Helper for reading the request envelope; HTTP-Request line, and HTTP headers.
class request_envelope
{
public:

  /// Creates an instance of class.
  request_envelope (connection * connection, request * request);

  /// Reads the request envelope from the connection, and invokes given callback afterwards.
  void read (exceptional_executor x, functor on_success);

  /// Returns the URI of the request.
  inline class path uri() const { return _uri; }

  /// Returns the server side path of the document/folder the uri is referring to.
  inline class path path() const { return _path; }

  /// Returns the type of the request.
  inline string method() const { return _method; }

  /// Returns the HTTP version of the request.
  inline string http_version() const { return _http_version; }

  /// Retrieves the value of the header with the specified name, or empty string if no such header exists.
  string header (const string & name) const;

  /// Returns the headers collection for the current request.
  const const_collection & headers () const { return _headers; }

  /// Returns the parameters collection for the current request.
  const const_collection & parameters () const { return _parameters; }

private:

  /// Parses the HTTP-Request line.
  void parse_request_line (const string & request_line);

  /// Parses and verifies correctness of the URI from the HTTP-Request line.
  void parse_uri (string uri);

  /// Reads the next HTTP headers from socket.
  void read_headers (exceptional_executor x, functor on_success);

  /// Parses and verifies sanity of the given HTTP header line.
  void parse_http_header_line (const string & line);

  /// Parses the HTTP GET parameters.
  void parse_parameters (const string & params);


  /// Connection this instance belongs to.
  connection * _connection;

  /// Request this instance belongs to.
  request * _request;

  /// Type of request, GET/POST/DELETE/PUT etc.
  string _method;

  /// Internal path to resource request is referring to.
  class path _path;

  /// URI of request.
  class path _uri;

  /// HTTP version of request.
  string _http_version;

  /// Headers.
  collection _headers;

  /// GET parameters.
  collection _parameters;
};


} // namespace http
} // namespace rosetta

#endif // ROSETTA_SERVER_REQUEST_ENVELOPE_HPP
