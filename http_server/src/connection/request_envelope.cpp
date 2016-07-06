
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

#include <cctype>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include "common/include/base64.hpp"
#include "http_server/include/server.hpp"
#include "http_server/include/helpers/uri_encode.hpp"
#include "http_server/include/helpers/match_condition.hpp"
#include "http_server/include/connection/request.hpp"
#include "http_server/include/connection/connection.hpp"
#include "http_server/include/connection/request_envelope.hpp"
#include "http_server/include/exceptions/request_exception.hpp"
#include "http_server/include/exceptions/security_exception.hpp"

namespace rosetta {
namespace http_server {

using std::string;
using namespace boost::asio;
using namespace rosetta::common;

/// Returns the next line from the given stream buffer.
string get_line (streambuf & buffer);

/// Auto-Capitalize HTTP header names.
string capitalize_header_name (const string & name);

/// Makes sure URI is "sane", and does not contain "/../", etc.
bool sanity_check_path (path uri);


request_envelope::request_envelope (connection * connection, request * request)
  : _connection (connection),
    _request (request),
    _folder_request (false)
{ }


void request_envelope::read (std::function<void()> on_success)
{
  // Figuring out max length of URI.
  const size_t MAX_URI_LENGTH = _connection->server()->configuration().get<size_t> ("max-uri-length", 4096);
  match_condition match (MAX_URI_LENGTH);

  // Reading until "max_length" or CR/LF has been found.
  _connection->socket().async_read_until (_connection->buffer(), match, [this, match, on_success] (auto error, auto bytes_read) {

    // Checking if socket has an error, or HTTP-Request line was too long.
    if (error) {

      // Something went wrong while reading from socket.
      _connection->close();
    } else if (match.has_error()) {

      // Too long URI.
      _request->write_error_response (414);
    } else {

      // Parsing request line, and verifying it's OK.
      parse_request_line (get_line (_connection->buffer()));

      // Reading headers.
      read_headers (on_success);
    }
  });
}


bool request_envelope::has_parameter (const string & name) const
{
  for (auto idx : _parameters) {
    if (std::get<0> (idx) == name)
      return true;
  }
  return false;
}


void request_envelope::parse_request_line (const string & request_line)
{
  // Making things slightly more tidy and comfortable in here ...
  using namespace std;
  using namespace boost;
  using namespace boost::algorithm;

  // Splitting initial HTTP line into its three parts.
  vector<string> parts;
  split (parts, request_line, ::isspace);

  // Removing all empty parts, which translates into consecutive spaces, to make logic more fault tolerant. Ref; HTTP/1.1 - 19.3.
  parts.erase (remove (parts.begin(), parts.end(), ""), parts.end ());

  // Now we can start deducting which type of request, and path, etc, this is.
  size_t no_parts = parts.size ();

  // At least the method and the URI needs to be supplied. The version is defaulted to HTTP/1.1, so it is actually optional.
  // This is in accordance to the HTTP/1.1 standard; 19.3.
  if (parts.size() < 2 || parts.size() > 3)
    throw request_exception ("Malformed HTTP-Request line.");

  // To be more fault tolerant, according to the HTTP/1.1 standard, point 19.3, we make sure the method is in UPPERCASE.
  // We also default the version to HTTP/1.1, unless it is explicitly given, and if given, we make sure it is UPPERCASE.
  _method           = to_upper_copy (parts [0]);
  _http_version     = no_parts > 2 ? to_upper_copy (parts [2]) : "HTTP/1.1";

  // Then, at last, we parse the URI.
  parse_uri (parts [1]);
}


void request_envelope::parse_uri (string uri)
{
  if (uri [0] != '/') {

    // To make sure we're more fault tolerant, we prepend the URI with "/", if it is not given. Ref; 19.3.
    uri = "/" + uri;
  }

  // Checking if URI contains HTTP GET parameters.
  auto index_of_pars = uri.find ("?");
  if (index_of_pars != string::npos) {

    // URI contains GET parameters.
    parse_parameters (uri.substr (index_of_pars + 1));

    // Decoding URI.
    uri = uri_encode::decode (uri.substr (0, index_of_pars));
  } else {

    // No parameters, decoding URI.
    uri = uri_encode::decode (uri);
  }

  // Verify URI does not contain any characters besides the non-control US ASCII characters.
  for (auto & idx : uri) {
    if (idx < 32 || idx > 126)
      throw request_exception ("Illegal characters found in path.");
  }

  // Then setting path, URI and folder/file-type of request.
  _uri = uri;

  // Checking if this is a folder request, or a GET request for a folder's default document.
  if (has_parameter ("list") && uri.back() == '/')
    _folder_request = true;
  else if (uri.back() == '/' && _method == "GET")
    uri += _connection->server()->configuration().get<string> ("default-document", "index.html");

  _path = _connection->server()->configuration().get<string> ("www-root", "www-root");
  _path += uri;
  if (_folder_request) {

    // Removing last "/" to have a "normalized" and uniform way of accessing folders inside of the file system.
    _path = _path.parent_path();
  }

  // Then, finally, we can sanity check the path.
  if (!sanity_check_path (_path))
    throw request_exception ("Illegal characters found in path.");
}


void request_envelope::read_headers (std::function<void()> on_success)
{
  // Retrieving max header size.
  const size_t max_header_length = _connection->server()->configuration().get<size_t> ("max-header-length", 8192);
  match_condition match (max_header_length);

  // Reading first header.
  _connection->socket().async_read_until (_connection->buffer(), match, [this, match, on_success] (auto error, auto bytes_read) {

    // Checking if socket has an error, or header exceeded maximum length.
    if (error) {

      // Something went wrong when reading from socket.
      _connection->close();
    } else if (match.has_error()) {

      // HTTP header was too much for us to handle.
      _request->write_error_response (413);
    } else {

      // Now we can start parsing HTTP headers.
      string line = get_line (_connection->buffer());

      // Checking if there are any more headers being sent from client.
      // When we are done reading headers, and there are no more headers, then there should be an additional empty string sent from client.
      if (line.size () == 0) {

        // No more headers, the previous header was the last.
        // Invoking given on_success() handler function.
        on_success ();
      } else if (_headers.size() >= _connection->server()->configuration().get<size_t> ("max-header-count", 25)) {

        // Too many HTTP headers in request.
        _request->write_error_response (413);
      } else {

        // Parsing HTTP header, before repeating the process, and invoking "self".
        parse_http_header_line (line);
        read_headers (on_success);
      }
    }
  });
}


void request_envelope::parse_http_header_line (const string & line)
{
  // Making things slightly more tidy and comfortable in here ...
  using namespace std;
  using namespace boost;
  using namespace boost::algorithm;

  // Checking if this is continuation header value of the previous line read from socket.
  if ((line [0] == ' ' || line [0] == '\t') && _headers.size() > 0) {

    // This is a continuation of the header value that was read in the previous line from client.
    // Appending content according to ruling of HTTP/1.1 standard.
    get<1> (_headers.back()) += " " + trim_copy_if (line, is_any_of (" \t"));
  } else {

    // Splitting header into name and value.
    const auto equals_idx = line.find (':');

    // Retrieving header name, simply ignoring headers without a value to be more fault tolerant. (ref; 19.3 of HTTP/1.1 std)
    if (equals_idx != string::npos) {

      // Retrieving actual header name and value, Auto-Capitalizing header name, and trimming name/value, to be more fault tolerant. (ref; 19.3)
      string name = capitalize_header_name (trim_copy (line.substr (0, equals_idx)));
      string value = trim_copy (line.substr (equals_idx + 1));

      // Checking if this is an "Authorization" header, at which point we try to create an authentication::ticket for request.
      if (name == "Authorization") {

        // Authenticate user.
        _headers.push_back (collection_type (name, value));
        authenticate_client (value);
      } else {

        // Now adding actual header into headers collection, before invoking callback.
        _headers.push_back (collection_type (name, value));
      }
    } // else; Simply ignoring HTTP headers without any value.
  }
}


void request_envelope::authenticate_client (const string & header_value)
{
  // Splitting value up into its two parts.
  std::vector<string> entities;
  split (entities, header_value, boost::is_any_of (" "));
  if (entities.size() != 2 || entities[0] != "Basic")
    throw security_exception ("Unknown authorization type found in 'Authorization' HTTP header.");

  // BASE64 decoding the username and password.
  std::vector<unsigned char> result;
  base64::decode (entities[1], result);

  // Splitting Authorization value into username and password, and verifying syntax.
  string username_password_string (result.begin(), result.end());
  std::vector<string> username_password;
  split (username_password, username_password_string, boost::is_any_of (":"));
  if (username_password.size() != 2)
    throw security_exception ("Syntax error in 'Authorization' HTTP header.");

  // Authorizing request, passing in server's salt to hash function.
  auto server_salt = _connection->server()->configuration().get<string> ("server-salt");
  _ticket = _connection->server()->authentication().authenticate (username_password [0], username_password [1], server_salt);
}


const string & request_envelope::header (const string & name) const
{
  // Empty return value, used when there are no such header.
  const static string EMPTY_HEADER_VALUE = "";

  // Looking for the header with the specified name.
  for (auto & idx : _headers) {
    if (std::get<0> (idx) == name)
      return std::get<1> (idx); // Match!
  }

  // No such header.
  return EMPTY_HEADER_VALUE;
}


void request_envelope::parse_parameters (const string & params)
{
  // Splitting up into separate parameters, and looping through each parameter,
  // removing empty parameters (two consecutive "&" immediately following each other).
  std::vector<string> pars;
  split (pars, params, boost::is_any_of ("&"));
  pars.erase (std::remove (pars.begin(), pars.end(), ""), pars.end ());

  // Looping through each parameter.
  for (string & idx : pars) {
    
    // Splitting up name/value of parameter, making sure we allow for parameters without value.
    size_t index_of_equal = idx.find ("=");
    string name  = uri_encode::decode (index_of_equal == string::npos ? idx : idx.substr (0, index_of_equal));
    string value = uri_encode::decode (index_of_equal == string::npos ? "" : idx.substr (index_of_equal + 1));

    // Making sure neither name nor value contains any control characters.
    for (auto & idx : name) {
      if (idx < 32 || idx > 126)
        throw request_exception ("Illegal characters found in parameter.");
    }
    for (auto & idx : value) {
      if (idx < 32 || idx > 126)
        throw request_exception ("Illegal characters found in parameter.");
    }

    _parameters.push_back (collection_type (name, value));
  }
}


/// Helper method for parsing an envelope line.
string get_line (streambuf & buffer)
{
  // Making things more tidy in here.
  using namespace std;

  // Reading next line from stream, and putting into vector buffer, for efficiency.
  vector<unsigned char> vec;
  istream stream (&buffer);

  // Iterating stream until CR/LF has been seen, and returning the line to caller.
  while (stream.good ()) {

    // Get next character from stream, and checking which type of character it is.
    unsigned char idx = stream.get ();
    if (idx == '\n')
      break; // Ignoring, and breaking while
    if (idx == '\r')
      continue; // Ignoring
    if (idx < 32 || idx == 127)
      throw request_exception ("Garbage data found in HTTP envelope, control character found in envelope.");
    vec.push_back (idx);
  }

  // Returning result to caller, by transforming vector to string, now without any CR or LF anywhere.
  return string (vec.begin (), vec.end ());
}


string capitalize_header_name (const string & name)
{
  // Will hold the return value temporarily.
  std::vector<char> return_value;

  // State machine value, used to determine if next character should be capitalized or not.
  // Starts out with being true, since the first character of an HTTP header always should be capitalized.
  bool next_is_upper = true;

  // Iterating through all characters in string.
  for (auto idx : name) {

    // Checking if we should make currently character UPPERCASE or not.
    if (next_is_upper) {

      // Making sure the currently character is UPPERCASE.
      return_value.push_back (toupper (idx));
    } else {

      // Making sure the currently iterated character is lowercase.
      return_value.push_back (tolower (idx));
    }

    // After every "-" character in an HTTP header, the next character should be UPPERCASE.
    next_is_upper = idx == '-';
  }

  // Returning to caller as string.
  return string (return_value.begin(), return_value.end());
}


bool sanity_check_path (path uri)
{
  // Breaking up URI into components, and sanity checking each component, to verify client is not requesting an illegal URI.
  for (auto & idx : uri) {

    if (idx.string().find ("..") != string::npos) // Request is probably trying to access files outside of the main "www-root" folder.
      return false;

    if (idx.string().find ("~") == 0) // Linux backup file or folder.
      return false;

    if (idx.string() == ".") // Two consecutive // after each other, or "." as full name of folder/file
      return false;
  }
  return true; // URI is sane.
}


} // namespace http_server
} // namespace rosetta
