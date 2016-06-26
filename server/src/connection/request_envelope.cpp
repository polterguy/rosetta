
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
#include <boost/algorithm/string.hpp>
#include "common/include/match_condition.hpp"
#include "server/include/server.hpp"
#include "server/include/connection/request.hpp"
#include "server/include/connection/connection.hpp"
#include "server/include/connection/request_envelope.hpp"
#include "server/include/exceptions/request_exception.hpp"

namespace rosetta {
namespace server {

using std::string;
using namespace boost::asio;
using namespace rosetta::common;

string get_line (streambuf & buffer);
string decode_uri (const string & uri);
string capitalize_header_name (const string & name);


request_envelope::request_envelope (connection * connection, request * request)
  : _connection (connection),
    _request (request)
{ }


// This method first reads the HTTP-Request line and parses it. Afterwards, it handles over control to the method
// that is responsible for reading the HTTP headers.
// It passes the X and on_success function on inwards, to the next layers, allowing them to decide whether or
// not the request has been successfully parsed or not, and on_success() can be invoked, or x should be allowed to
// go out of scope, cleaning thins up for us.
// See the request::handle() for more details about this logic.
void request_envelope::read (exceptional_executor x, functor on_success)
{
  // Figuring out max length of URI.
  const size_t MAX_URI_LENGTH = _connection->server()->configuration().get<size_t> ("max-uri-length", 4096);
  match_condition match (MAX_URI_LENGTH);

  // Reading until "max_length" or CR/LF has been found.
  _connection->socket().async_read_until (_connection->buffer(), match, [this, match, x, on_success] (auto error, auto bytes_read) {

    // Checking if socket has an error, or HTTP-Request line was too long.
    if (error == error::operation_aborted)
      return;
    if (error)
      throw request_exception ("Socket error while reading HTTP-Request line.");

    if (match.has_error()) {

      // Too long URI.
      _request->write_error_response (x, 414);
      return;
    }

    // Parsing request line, and verifying it's OK.
    parse_request_line (get_line (_connection->buffer()));

    // Reading headers.
    read_headers (x, on_success);
  });
}


void request_envelope::read_headers (exceptional_executor x, functor on_success)
{
  // Retrieving max header size.
  const size_t max_header_length = _connection->server()->configuration().get<size_t> ("max-header-length", 8192);
  match_condition match (max_header_length);

  // Reading first header.
  _connection->socket().async_read_until (_connection->buffer(), match, [this, x, match, on_success] (auto error, auto bytes_read) {

    // Checking if socket has an error, or header exceeded maximum length.
    if (error == error::operation_aborted)
      return;
    if (error)
      throw request_exception ("Socket error while reading HTTP-Request line.");

    if (match.has_error() || _headers.size() > _connection->server()->configuration().get<size_t> ("max-header-count", 25)) {

      // HTTP header was too much for us to handle, or there were too many HTTP headers sent from client. Returning 413 to client.
      _request->write_error_response (x, 413);
      return;
    }

    // Now we can start parsing HTTP headers.
    string line = get_line (_connection->buffer());

    // Checking if there are any more headers being sent from client.
    // When we are done reading headers, and there are no more headers, then there should be an additional empty string sent from client.
    if (line.size () == 0) {

      // No more headers, the previous header was the last.
      // Invoking given on_success() handler function.
      on_success (x);
    } else {

      // Parsing HTTP header, before repeating the process, and invoking "self".
      parse_http_header_line (line);
      read_headers (x, on_success);
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

      // Now adding actual header into headers collection.
      _headers.push_back (collection_type (name, value));
    } // else; Ignore header completely.
  }
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


void request_envelope::parse_request_line (const string & request_line)
{
  // Making things slightly more tidy and comfortable in here ...
  using namespace std;
  using namespace boost;
  using namespace boost::algorithm;

  // Splitting initial HTTP line into its three parts.
  vector<string> parts;
  split (parts, request_line, ::isspace);

  // Removing all empty parts of URI, meaning consecutive spaces.
  parts.erase (remove (parts.begin(), parts.end(), ""), parts.end ());

  // Now we can start deducting which type of request, and path, etc, this is.
  size_t no_parts = parts.size ();

  // At least the method and the URI needs to be supplied. The version is defaulted to HTTP/1.1, so it is optional.
  // This is in accordance to the HTTP/1.1 standard; 19.3.
  if (parts.size() < 2 || parts.size() > 3)
    throw request_exception ("Malformed HTTP-Request line.");

  // To be more fault tolerant, according to the HTTP/1.1 standard, point 19.3, we make sure the method is in UPPERCASE.
  // We also default the version to HTTP/1.1, unless it is explicitly given, and if given, we make sure it is UPPERCASE.
  _type           = to_upper_copy (parts [0]);
  _uri            = parts [1];
  _version        = no_parts > 2 ? to_upper_copy (parts [2]) : "HTTP/1.1";

  // Checking if path is a request for the default document.
  if (_uri == "/") {

    // Serving default document.
    _uri = _connection->server()->configuration().get<string> ("default-page", "/index.html");
  } else if (_uri [0] != '/') {

    // To make sure we're more fault tolerant, we prepend the URI with "/", if it is not given.
    _uri = "/" + _uri;
  }

  // Retrieving extension of request URI.
  string file_name = _uri.substr (_uri.find_last_of ("/") + 1);
  size_t pos_of_dot = file_name.find_last_of (".");
  _extension = pos_of_dot == string::npos ? "" : file_name.substr (pos_of_dot + 1);

  // Checking if URI contains HTTP GET parameters.
  auto index_of_pars = _uri.find ("?");
  if (index_of_pars == 1) {

    // Default page was requested, as "/", with HTTP GET parameters.
    parse_parameters (decode_uri (_uri.substr (2)));

    // Serving default document.
    _uri = _connection->server()->configuration().get<string> ("default-page", "/index.html");
  } else if (index_of_pars != string::npos) {

    // URI contains GET parameters.
    parse_parameters (decode_uri (_uri.substr (index_of_pars + 1)));

    // Decoding URI.
    _uri = decode_uri (_uri.substr (0, index_of_pars));
  } else {

    // No parameters, decoding URI.
    _uri = decode_uri (_uri);
  }
}


void request_envelope::parse_parameters (const string & params)
{
  // Splitting up into separate parameters, and looping through each parameter, removing empty parameters (two consecutive "&" immediately following each other).
  std::vector<string> pars;
  split (pars, params, boost::is_any_of ("&"));
  pars.erase (std::remove (pars.begin(), pars.end(), ""), pars.end ());
  for (string & idx : pars) {
    
    // Splitting up name/value of parameter, allowing for parameters without value.
    size_t index_of_equal = idx.find ("=");
    string name  = index_of_equal == string::npos ? idx : idx.substr (0, index_of_equal);
    string value = index_of_equal == string::npos ? "" : idx.substr (index_of_equal + 1);
    _parameters.push_back (collection_type (name, value));
  }
}


/// Helper method for parsing an envelope line.
string get_line (streambuf & buffer)
{
  // Reading next line from stream, and putting into vector buffer, for efficiency.
  std::vector<unsigned char> vec;
  std::istream stream (&buffer);

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


unsigned char from_hex (unsigned char ch) 
{
  if (ch <= '9' && ch >= '0')
    ch -= '0';
  else if (ch <= 'f' && ch >= 'a')
    ch -= 'a' - 10;
  else if (ch <= 'F' && ch >= 'A')
    ch -= 'A' - 10;
  else 
    ch = 0;
  return ch;
}


string decode_uri (const string & uri)
{
  // Will hold the decoded return URI value.
  std::vector<unsigned char> return_value;
  
  // Iterating through entire string, looking for either '+' or '%', which is specially handled.
  for (size_t idx = 0; idx < uri.length (); ++idx) {
    
    // Checking if this character should have special handling.
    if (uri [idx] == '+') {

      // '+' equals space " ".
      return_value.push_back (' ');

    } else if (uri [idx] == '%' && uri.size() > idx + 2) {

      // '%' notation of character, followed by two characters.
      // The first character is bit shifted 4 places, and OR'ed with the value of the second character.
      return_value.push_back ((from_hex (uri [idx + 1]) << 4) | from_hex (uri [idx + 2]));
      idx += 2;

    } else {

      // Normal plain character.
      return_value.push_back (uri [idx]);
    }
  }

  // Returning decoded URI to caller.
  return string (return_value.begin (), return_value.end ());
}


string capitalize_header_name (const string & name)
{
  std::vector<char> return_value;
  bool next_is_upper = true;
  for (auto idx : name) {
    if (next_is_upper) {
      return_value.push_back (toupper (idx));
    } else {
      return_value.push_back (idx);
    }
    next_is_upper = idx == '-';
  }
  return string (return_value.begin(), return_value.end());
}


} // namespace server
} // namespace rosetta
