
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
using std::vector;
using std::istream;
using std::getline;
using std::istreambuf_iterator;
using namespace boost;

namespace rosetta {
namespace common {


string string_helper::get_line (streambuf & buffer, int size)
{
  // Reading next line from stream.
  vector<char> vec;
  istream stream (&buffer);

  // Iterating stream until CR/LF has been seen, ignoring everything but the normal US ASCII range of characters,
  // and CR/LF, to allow for cloaking HTTP requests.
  // Please notice, that with the current logic here, you can even create requests that have lines in them, where you can put
  // "garbage data" between the CR and the LF to create a maximum amount of cloaking for your HTTP requests.
  // In addition, everything from 1 to 10 is interpreted as a LF, and everything from 11 to 20 is interpreted as a CR.
  // Which means, it becomes literally impossible for an adversary to see the difference between a cloaked HTTP request,
  // and "random garbage", since it has nothing to look for, to even see the CR/LF sequence, to understand if there are any
  // CR/LF sequences in the data.
  while (stream.good ()) {
    char idx = stream.get ();
    switch (idx) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
      vec.push_back ('\n');
      break;
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
      vec.push_back ('\r');
      break;
    default:
      if (idx > 31 && idx < 127)
        vec.push_back (idx);
      break;
    }

    // Checking if we have seen an entire line.
    if (vec.size() > 1 && vec [vec.size() - 2] == '\r' && vec [vec.size() - 1] == '\n')
      break;
  }

  // Returning result to caller, ignoring CR/LF when creating return value.
  return string (vec.begin (), vec.end () - 2);;
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
