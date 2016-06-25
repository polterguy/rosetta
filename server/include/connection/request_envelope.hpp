
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
#include "common/include/exceptional_executor.hpp"

namespace rosetta {
namespace server {

using std::string;
using namespace rosetta::common;

class request;
class connection;

// How our headers and arguments are stored within class.
typedef std::tuple<string, string> collection_type;
typedef std::vector<collection_type> collection;
typedef const std::vector<collection_type> const_collection;


/// Helper for reading the request envelope, HTTP-Request line and headers.
class request_envelope
{
public:

  /// Creates an instance of class.
  request_envelope (connection * connection, request * request);

  /// Reads the request envelope from the connection, and invokes given callback afterwards.
  void read (exceptional_executor x, functor callback);

  /// Returns the URI of the request.
  const inline string & uri() const { return _uri; }

  /// Returns the extension of the request.
  const inline string & extension() const { return _extension; }

  /// Returns the type of the request.
  const inline string & type() const { return _type; }

  /// Returns the HTTP version of the request.
  const inline string & version() const { return _version; }

  /// Retrieves the value of the header with the specified name, or empty string if no such header exists.
  const string & header (const string & name) const;

  /// Returns the headers collection for the current request.
  const const_collection & headers () const { return _headers; }

  /// Returns the parameters collection for the current request.
  const const_collection & parameters () const { return _parameters; }

private:

  /// Parses the HTTP-Request line.
  void parse_request_line (const string & request_line);

  /// Reads the HTTP headers.
  void read_headers (exceptional_executor x, functor callback);

  /// Parses the HTTP GET parameters.
  void parse_parameters (const string & params);


  /// Connection this instance belongs to.
  connection * _connection;

  /// Request this instance belongs to.
  request * _request;

  /// Type of request, GET/POST/DELETE/PUT etc.
  string _type;

  /// Path to resource requested.
  string _uri;

  /// Path to resource requested.
  string _extension;

  /// HTTP version of request.
  string _version;

  /// Headers.
  collection _headers;

  /// HTTP parameters.
  collection _parameters;
};


} // namespace server
} // namespace rosetta

#endif // ROSETTA_SERVER_REQUEST_ENVELOPE_HPP
