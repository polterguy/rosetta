
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
#include "http_server/include/helpers/uri_encode.hpp"
#include "http_server/include/exceptions/request_exception.hpp"

using std::string;
using std::vector;

namespace rosetta {
namespace http_server {
namespace uri_encode {


unsigned char from_hex (unsigned char ch) 
{
  if (ch >= '0' && ch <= '9')
    return ch - '0';
  else if (ch >= 'a' && ch <= 'f')
    return ch - 'a' + 10;
  else if (ch >= 'A' && ch <= 'F')
    return ch - 'A' + 10;
  else 
    throw request_exception ("Unknown escape % HEX HEX character sequence value found in encoded URI.");
}


string decode (const string & uri)
{
  // Will hold the decoded return URI value temporarily.
  std::vector<unsigned char> return_value;
  
  // Iterating through entire string, looking for either '+' or '%', which is specially handled.
  for (size_t idx = 0; idx < uri.length (); ++idx) {
    
    // Checking if this character should have special handling.
    if (uri [idx] == '+') {

      // '+' equals space " ".
      return_value.push_back (' ');
    } else if (uri [idx] == '%') {

      // '%' notation of character, followed by two characters. Sanity checking input first.
      if (idx + 2 >= uri.size ())
        throw request_exception ("Syntax error in URI encoded string, no values after '%' notation.");

      // The first character is bit shifted 4 places, and OR'ed with the value of the second character.
      // Then we make sure we skip the next 2 characters, since they're already handled.
      return_value.push_back ((from_hex (uri [idx + 1]) << 4) | from_hex (uri [idx + 2]));
      idx += 2;
    } else {

      // Normal plain character.
      return_value.push_back (uri [idx]);
    }
  }

  // Returning decoded URI to caller as string.
  return string (return_value.begin (), return_value.end ());
}


} // namespace uri_encode
} // namespace http_server
} // namespace rosetta
