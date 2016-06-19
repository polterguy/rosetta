
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

#include <memory>
#include <vector>
#include <utility>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include "common/include/string_helper.hpp"
#include "server/include/server.hpp"
#include "server/include/connection/request.hpp"
#include "server/include/connection/connection.hpp"
#include "server/include/connection/match_condition.hpp"
#include "server/include/exceptions/request_exception.hpp"
#include "server/include/connection/handlers/request_handler.hpp"

namespace rosetta {
namespace server {
  
using std::string;
using std::vector;
using boost::system::error_code;
using boost::algorithm::trim;


request_ptr request::create (connection * connection, const string & type, const string & path, const string & version)
{
  return request_ptr (new request (connection, type, path, version));
}


request::request (connection * connection, const string & type, const string & path, const string & version)
  : _connection (connection)
{
  // Sanity check.
  if (_connection == nullptr)
    throw request_exception ("Request was not given a valid connection."); // Oops, no connection!

  // More sanity checking.
  if (version != "http/1.1")
    throw request_exception ("Unknown HTTP-Version; '" + version + "'."); // We only support HTTP version 1.1 at the moment.

  // Checking path, simple case first.
  if (path.length () == 0 || path [0] != '/')
    throw request_exception ("Illegal path; '" + path + "'."); // This is not an OK path.

  // Checking if path is empty, at which case we default it to the default file, referenced in configuration.
  // In addition to temporarily removing the initial "/", to not mess up our split below, and return empty values.
  string substr_path = path.substr (1);
  if (substr_path.size () == 0)
    substr_path = _connection->_server->configuration().get<string> ("default-page", "index.html");

  // Checking if URI contains GET parameters.
  auto index_of_pars = substr_path.find ("?");
  if (string::npos != index_of_pars) {

    // URI contains GET parameters.
    // Making sure we decode the URI as we pass it into the parameters parsing method, in case parameter contains for instance "?" etc.
    parse_parameters (string_helper::decode_uri (substr_path.substr (index_of_pars + 1)));

    // Decoding actual URI.
    substr_path = string_helper::decode_uri (substr_path.substr (0, index_of_pars));
  } else {

    // No parameters in request URI, making sure we decode the string any way, in case request is for a document with a "funny name".
    substr_path = string_helper::decode_uri (substr_path);
  }

  // Breaking up path into components, and sanity checking each component.
  vector<string> entities;
  split (entities, substr_path, boost::is_any_of ("/"));
  for (string & idx : entities) {

    if (idx == "")
      throw request_exception ("Illegal path; '" + path + "'."); // Two consecutive "/" after each other.

    if (idx.find ("..") != string::npos) // Request is attempting at accessing files outside of the main www-root folder.
      throw request_exception ("Illegal path; '" + path + "'.");

    if (idx.find ("~") == 0) // Linux backup file.
      throw request_exception ("Illegal path; '" + path + "'.");

    if (idx.find (".") == 0) // Linux hidden file, or a file without a name, and only extension.
      throw request_exception ("Illegal path; '" + path + "'.");
  }

  // Now we know that this is something we can probably handle.
  _type         = type;
  _path         = "/" + substr_path; // Making sure we add back the initial "/" again.
  _filename     = entities [entities.size() - 1];
  _http_version = version;

  // Checking if there's a file extension in our filename.
  auto pos_of_dot = _filename.find (".");
  if (pos_of_dot != string::npos)
    _extension = _filename.substr (pos_of_dot + 1);
}


void request::parse_parameters (const string & params)
{
  // Splitting up into separate parameters, and looping through each parameter.
  vector<string> pars;
  split (pars, params, boost::is_any_of ("&"));
  for (string & idx : pars) {

    // Splitting up name/value of parameter.
    vector<string> name_value;
    split (name_value, idx, boost::is_any_of ("="));

    // Creating our HTTP GET parameter, with the given name/value combination, making sure we default its value to "" if
    // no value is supplied.
    string name = name_value [0];
    string value = name_value.size() > 1 ? name_value [1] : "";
    _parameters [name] = value;
  }
}


void request::read_headers (exceptional_executor x, function<void(exceptional_executor x)> functor)
{
  // Sanity check, we don't accept more than "max-header-count" HTTP headers from configuration.
  auto max_header_count = _connection->_server->configuration().get<size_t> ("max-header-count", 25);
  if (_headers.size() > max_header_count) {

    // Writing error status response, and returning early.
    _connection->write_error_response (413, x);
    return;
  }

  // Making sure each header don't exceed the maximum length defined in configuration.
  size_t max_header_length = _connection->_server->configuration().get<size_t> ("max-header-length", 8192);
  match_condition match (max_header_length, "\r\n", "\t ");

  // Now we can read the first header from socket, making sure it does not exceed max-header-length
  async_read_until (*_connection->_socket, _connection->_request_buffer, match, [this, x, match, functor] (const error_code & error, size_t bytes_read) {

    // Making sure there was no errors while reading socket
    if (error)
      throw request_exception ("Socket error while reading HTTP headers.");

    // Making sure there was no more than maximum number of bytes read according to configuration.
    if (match.has_error ()) {

      // Writing error status response, and returning early.
      _connection->write_error_response (413, x);
      return;
    }

    // Now we can start parsing HTTP header.
    string line = string_helper::get_line (_connection->_request_buffer, bytes_read);

    // Checking if there are no more headers.
    if (line == "") {

      // We're now done reading all HTTP headers, giving control back to connection through function callback.
      functor (x);
    } else {

      // There are possibly more HTTP headers, continue reading until we see an empty line.
      string key, value;
      const size_t equals_idx = line.find (':');
      if (string::npos == equals_idx) {

        // Missing colon (:) in HTTP header.
        throw request_exception ("Syntes error in HTTP header close to; '" + line + "'");
      } else {

        key = line.substr (0, equals_idx);
        value = line.substr (equals_idx + 1);
      }

      // Trimming header key and value before we put results into collection.
      trim (key);
      trim (value);
      _headers [key] = value;

      // Fetching next header by invoking self.
      read_headers (x, functor);
    }
  });
}


void request::read_content (exceptional_executor x, function<void(exceptional_executor x)> functor)
{
  // Checking if there is any content first.
  string content_length_str = (*this)["Content-Length"];

  // Checking if there is any Content-Length
  if (content_length_str == "") {

    // No content, invoking callback functor immediately.
    functor (x);
  } else {

    // Checking that content does not exceed max request content length, defaulting to 16 MB.
    auto content_length = lexical_cast<size_t> (content_length_str);
    auto max_content_length = _connection->_server->configuration().get<size_t> ("max-request-content-length", 4194304);
    if (content_length > max_content_length) {

      // Writing error status response, and returning early.
      _connection->write_error_response (500, x);
      return;
    }

    // Reading content into streambuf.
    match_condition match (content_length);
    async_read_until (*_connection->_socket, _connection->_request_buffer, match, [this, match, content_length, functor, x] (const error_code & error, size_t bytes_read) {

      // Checking for socket errors.
      if (error)
        throw request_exception ("Socket error while reading content of request.");

      // Verifying that number of bytes read was the same as the number of bytes specified in Content-Length header.
      if (bytes_read != content_length)
        throw request_exception ("Not enough bytes in request content to match the Content-Length header.");

      // Invoking functor callback supplied by caller.
      // Notice, at this point, we simply keep the content in our connection's streambuf for later references.
      functor (x);
    });
  }
}


void request::handle (exceptional_executor x, function<void(exceptional_executor x)> callback)
{
  // First we need to create our handler.
  _request_handler = request_handler::create (_connection->_server, _connection, this, _extension);

  // Then we let our handler take care of the rest of our request, making sure we pass in exceptional_executor,
  // such that if an exception occurs, then connection is closed.
  _request_handler->handle (x, [callback] (exceptional_executor x) {

    callback (x);
  });
}


const string & request::operator [] (const string & key) const
{
  // String we return if there is no HTTP header with the specified name.
  const static string empty_return_value = "";

  // Checking if we have the specified HTTP header.
  auto index = _headers.find (key);
  if (index == _headers.end ())
    return empty_return_value; // No such header.

  return index->second;
}


} // namespace server
} // namespace rosetta
