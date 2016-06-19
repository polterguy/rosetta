
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

#include <string>
#include <istream>
#include <boost/algorithm/string.hpp>
#include "common/include/string_helper.hpp"

using std::string;
using std::istream;
using std::getline;
using namespace boost;

namespace rosetta {
namespace common {


string string_helper::get_line (streambuf & buffer, int size)
{
  string return_value;
  istream stream (&buffer);

  // If size is not given, we simply return the first line, otherwise we need to check for HTTP header
  // value spanning multiple lines.
  if (size == -1) {

      // Reading single line from stream.
      getline (stream, return_value);
      trim_right_if (return_value, is_any_of ("\r"));

  } else {

    // Looping until we've read size no characters from stream.
    do {

      // Reading next line from stream.
      string tmp;
      getline (stream, tmp);

      // Decrement size.
      size -= (tmp.size() + 2);

      // We do not left trim the first line we read for any occurrences of TAB or SP, only every consecutive lines, after the first line.
      // After left trimming, we prepend a single SP character.
      if (return_value.size () > 0) {

        // This is not the first line we read, making sure we replace any occurrences of TAB or SP with a single SP.
        trim_left_if (tmp, is_any_of ("\t "));
        tmp = " " + tmp;
      }

      // Then we remove the trailing CR, before appending to return_value.
      trim_right_if (tmp, is_any_of ("\r"));
      return_value += tmp;

    } while (size > 0);
  }

  // Returning result to caller.
  return return_value;
}


inline unsigned char from_hex (unsigned char ch) 
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


string string_helper::decode_uri (const string & uri)
{
  // Will hold the decoded return URI value.
  string return_value;
  
  // Iterating through entire string, looking for either '+' or '%', which is specially handled.
  for (size_t i = 0; i < uri.length (); ++i) {
    
    // Checking if this character should have special handling.
    if (uri[i] == '+') {

      // '+' equals space " ".
      return_value += ' ';

    } else if (uri[i] == '%' && uri.size() > i + 2) {

      // '%' notation of character.
      // The first character is bit shifted 4 places, and OR'ed with the value of the second character.
      return_value += (from_hex (uri [i+1]) << 4) | from_hex (uri [i+2]);
      i += 2;

    } else {

      // Normal plain character.
      return_value += uri[i];

    }
  }

  // Returning decoded URI to caller.
  return return_value;
}


} // namespace common
} // namespace rosetta
