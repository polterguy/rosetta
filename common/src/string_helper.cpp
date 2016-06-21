
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
#include "common/include/rosetta_exception.hpp"

using std::string;
using std::vector;
using std::istream;

namespace rosetta {
namespace common {


string string_helper::get_line (streambuf & buffer)
{
  // Reading next line from stream, and putting into vector buffer, for efficiency.
  vector<unsigned char> vec;
  istream stream (&buffer);

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

  // Checking that the last two characters in vector are CR/LF sequence.
  if (vec.size() < 2 || *(vec.end () - 1) != '\n' || *(vec.end () - 2) != '\r')
    throw rosetta_exception ("Garbage data found in HTTP envelope, no valid CR/LF sequence found before end of stream.");

  // Now removing the last two characters, before checking for anymore CR characters in the middle of the stream, which is an error.
  vec.erase (vec.end() - 2, vec.end ());
  if (std::find (vec.begin(), vec.end(), '\r') != vec.end ())
    throw rosetta_exception ("Garbage data found in HTTP envelope, CR character found in the middle of a line sent from client.");

  // Returning result to caller, by transforming vector to string, which should now not contain any CR or LF anywhere.
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


string string_helper::decode_uri (const string & uri)
{
  // Will hold the decoded return URI value.
  string return_value;
  
  // Iterating through entire string, looking for either '+' or '%', which is specially handled.
  for (size_t idx = 0; idx < uri.length (); ++idx) {
    
    // Checking if this character should have special handling.
    if (uri [idx] == '+') {

      // '+' equals space " ".
      return_value += ' ';

    } else if (uri [idx] == '%' && uri.size() > idx + 2) {

      // '%' notation of character, followed by two characters.
      // The first character is bit shifted 4 places, and OR'ed with the value of the second character.
      return_value += (from_hex (uri [idx + 1]) << 4) | from_hex (uri [idx + 2]);
      idx += 2;

    } else {

      // Normal plain character.
      return_value += uri [idx];
    }
  }

  // Returning decoded URI to caller.
  return return_value;
}


} // namespace common
} // namespace rosetta
