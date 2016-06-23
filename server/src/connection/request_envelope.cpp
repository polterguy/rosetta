
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

#include <vector>
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include "common/include/date.hpp"
#include "common/include/match_condition.hpp"
#include "server/include/server.hpp"
#include "server/include/connection/request.hpp"
#include "server/include/connection/connection.hpp"
#include "server/include/connection/request_envelope.hpp"
#include "server/include/exceptions/request_exception.hpp"

namespace rosetta {
namespace server {

using std::string;
using boost::system::error_code;
using namespace rosetta::common;

string get_line (boost::asio::streambuf & buffer);
string decode_uri (const string & uri);


void request_envelope::read (connection_ptr connection, request * request, exceptional_executor x, std::function<void (exceptional_executor x)> functor)
{
  // Figuring out max length of URI.
  const static size_t MAX_URI_LENGTH = connection->server()->configuration().get<size_t> ("max-uri-length", 4096);
  match_condition match (MAX_URI_LENGTH);

  // Reading until "max_length" or CR/LF has been found.
  async_read_until (connection->socket(), connection->buffer(), match, [this, connection, request, match, x, functor] (const error_code & error, size_t bytes_read) {

    // Default document.
    const static string DEFAULT_DOCUMENT = connection->server()->configuration().get<string> ("default-page", "/index.html");

    // Checking if socket has an error, or HTTP-Request line was too long.
    if (error)
      return; // Simply letting x go out of scope, cleans things up for us.

    if (match.has_error()) {

      // Too long URI.
      request->write_error_response (connection, x, 414);
      return;
    }

    // Parsing request line, and verifying it's OK.
    if (!parse_request_line (get_line (connection->buffer()), DEFAULT_DOCUMENT))
      return; // Simply letting x go out of scope, cleans things up for us.


    // Reading headers.
    read_headers (connection, request, x, [functor] (exceptional_executor x) {

      // Success so far.
      functor (x);
    });
  });
}


void request_envelope::read_headers (connection_ptr connection, request * request, exceptional_executor x, std::function<void (exceptional_executor x)> functor)
{
  // Retrieving max header size.
  const static size_t max_header_length = connection->server()->configuration().get<size_t> ("max-header-length", 8192);
  match_condition match (max_header_length);

  // Reading first header.
  async_read_until (connection->socket(), connection->buffer(), match, [this, connection, request, x, match, functor] (const error_code & error, size_t bytes_read) {

    // Making sure there was no errors while reading socket
    if (error)
      return; // Simply letting x go out of scope, cleans things up.

    if (match.has_error ()) {

      // Returning 413 to client.
      request->write_error_response (connection, x, 413);
      return;
    }

    // Now we can start parsing HTTP headers.
    string line = get_line (connection->buffer());
    if (line.size () == 0)
      functor (x);

    // There are possibly more HTTP headers, continue reading until we see an empty line.
    const auto equals_idx = line.find (':');

    // Retrieving header name, ignoring headers without value, simply ignoring headers without a value.
    if (equals_idx != string::npos) {

      // Retrieving actual header name and value.
      string header_name = boost::algorithm::trim_copy (line.substr (0, equals_idx));
      string header_value = boost::algorithm::trim_copy (line.substr (equals_idx + 1));

      // Now adding actual header into headers collection.
      _headers [header_name] = header_value;
    }

    // Reading next header from socket.
    read_headers (connection, request, x, functor);
  });
}


const string & request_envelope::get_header (const string & name) const
{
  // Empty return value, used when there are no such header.
  const static string EMPTY_HEADER_VALUE = "";

  // Looking for a header with the specified name.
  for (auto & idx : _headers) {
    if (idx.first == name)
      return idx.second; // Found it!
  }

  // No such header.
  return EMPTY_HEADER_VALUE;
}


bool request_envelope::parse_request_line (const string & request_line, const string & default_document)
{
  // Splitting initial HTTP line into its three parts.
  std::vector<string> parts;
  boost::algorithm::split (parts, request_line, ::isspace);

  // Removing all empty parts of URI, meaning consecutive spaces.
  parts.erase (std::remove (parts.begin(), parts.end(), ""), parts.end ());

  // Now we can start deducting which type of request, and path, etc, this is.
  size_t no_parts = parts.size ();

  // At least the method and the URI needs to be supplied. The version is defaulted to HTTP/1.1, so it is optional.
  if (parts.size() < 2)
    return false; // Failure.

  // To be more fault tolerant, according to the HTTP/1.1 standard, point 19.3, we make sure the method is in UPPERCASE.
  // We also default the version to HTTP/1.1, unless it is explicitly given, and if given, we make sure it is UPPERCASE.
  _type           = boost::algorithm::to_upper_copy (parts [0]);
  _uri            = parts [1];
  _version        = no_parts > 2 ? boost::algorithm::to_upper_copy (parts [2]) : "HTTP/1.1";

  // Checking if path is a request for the default document.
  if (_uri == "/") {

    // Serving default document.
    _uri = default_document;
  } else if (_uri [0] != '/') {

    // To make sure we're more fault tolerant, we prepend the URI with "/" if it is not given.
    _uri = "/" + _uri;
  }

  // Retrieving extension of request URI.
  string file_name = _uri.substr (_uri.find_last_of ("/") + 1);
  size_t pos_of_dot = file_name.find_last_of (".");
  _extension = pos_of_dot == string::npos ? "" : file_name.substr (pos_of_dot + 1);

  // Checking if URI contains HTTP GET parameters, allowing for multiple different GET parameter delimiters, to support
  // maximum amount of HTTP cloaking.
  auto index_of_pars = _uri.find ("?");
  if (index_of_pars == 1) {

    // Default page was requested, with HTTP GET parameters.
    parse_parameters (decode_uri (_uri.substr (1)));

    // Serving default document.
    _uri = default_document;
  } else if (index_of_pars != string::npos) {

    // URI contains GET parameters.
    parse_parameters (decode_uri (_uri.substr (index_of_pars + 1)));

    // Decoding URI.
    _uri = decode_uri (_uri.substr (0, index_of_pars));
  } else {

    // Decoding URI.
    _uri = decode_uri (_uri);
  }
  return true; // Success
}


void request_envelope::parse_parameters (const string & params)
{
  // Splitting up into separate parameters, and looping through each parameter.
  std::vector<string> pars;
  split (pars, params, boost::is_any_of ("&"));
  for (string & idx : pars) {
    
    // Splitting up name/value of parameter, allowing for parameters without value.
    size_t index_of_equal = idx.find ("=");
    string name = index_of_equal == string::npos ? idx : idx.substr (0, index_of_equal);
    string value = index_of_equal == string::npos ? "" : idx.substr (index_of_equal + 1);
    _parameters [name] = value;
  }
}


/// Helper method for parsing an envelope line.
string get_line (boost::asio::streambuf & buffer)
{
  // Reading next line from stream, and putting into vector buffer, for efficiency.
  std::vector<unsigned char> vec;
  std::istream stream (&buffer);

  // Iterating stream until CR/LF has been seen, and returning the line to caller.
  bool seen_lf = false;
  while (stream.good () && !seen_lf) {

    // Get next character from stream, and checking which type of character it is.
    unsigned char idx = stream.get ();
    switch (idx) {
    case '\t':
    case '\r':
      break; // Good character.
    case '\n':
      seen_lf = true;
      break; // Good character, reading stops here.
    default:
      if (idx < 32 || idx == 127) // Malicious/bad character.
        throw rosetta_exception ("Garbage data found in HTTP envelope, control character found in envelope.");
    }

    // Appending character into vector.
    vec.push_back (idx);
  }

  // Checking that the last character in vector is a LF sequence, before erasing it.
  if (vec.size() < 1 || *(vec.end () - 1) != '\n')
    throw rosetta_exception ("Garbage data found in HTTP envelope, no LF found before end of stream.");
  vec.erase (--vec.end());

  // Then erasing all CR characters, regardless of where they are in the string. Ref; 19.3 of the HTTP/1.1 standard.
  vec.erase (std::remove (vec.begin (), vec.end (), '\r'), vec.end());

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


} // namespace server
} // namespace rosetta
