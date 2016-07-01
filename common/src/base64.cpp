
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
#include <vector>
#include <sstream>
#include "common/include/base64.hpp"

using std::string;

namespace rosetta {
namespace common {
namespace base64 {


/*
 * Thanks to René Nyffenegger for providing the basis for this method at;
 * http://www.adp-gmbh.ch/cpp/common/base64.html
 */
static const string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";


static inline bool is_base64 (unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}


void encode (const vector<unsigned char> & bytes, string & result)
{
  auto in_len = bytes.size ();
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];
  auto iter = bytes.begin ();

  while (in_len--) {
    char_array_3[i++] = *iter++;
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(i = 0; (i <4) ; i++)
        result += base64_chars [char_array_4 [i]];
      i = 0;
    }
  }

  if (i)
  {
    for(j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++)
      result += base64_chars[char_array_4[j]];

    while (i++ < 3)
      result += '=';
  }
}


void decode (const string & base64_string, vector<unsigned char> & result)
{
  int in_len = base64_string.size();
  int i = 0;
  int j = 0;
  int in_ = 0;
  unsigned char char_array_4 [4], char_array_3 [3];

  while (in_len-- != 0 && (base64_string [in_] != '=') && is_base64 (base64_string [in_])) {
    char_array_4 [i++] = base64_string [in_];
    in_++;
    if (i == 4) {
      for (i = 0; i < 4; i++)
        char_array_4 [i] = base64_chars.find (char_array_4 [i]);

      char_array_3 [0] = (char_array_4 [0] << 2) + ((char_array_4 [1] & 0x30) >> 4);
      char_array_3 [1] = ((char_array_4 [1] & 0xf) << 4) + ((char_array_4 [2] & 0x3c) >> 2);
      char_array_3 [2] = ((char_array_4 [2] & 0x3) << 6) + char_array_4 [3];

      for (i = 0; (i < 3); i++)
        result.push_back (char_array_3 [i]);
      i = 0;
    }
  }

  if (i) {
    for (j = i; j <4; j++)
      char_array_4 [j] = 0;

    for (j = 0; j < 4; j++)
      char_array_4 [j] = base64_chars.find (char_array_4 [j]);

    char_array_3 [0] = (char_array_4 [0] << 2) + ((char_array_4 [1] & 0x30) >> 4);
    char_array_3 [1] = ((char_array_4 [1] & 0xf) << 4) + ((char_array_4 [2] & 0x3c) >> 2);
    char_array_3 [2] = ((char_array_4 [2] & 0x3) << 6) + char_array_4 [3];

    for (j = 0; (j < i - 1); j++)
      result.push_back (char_array_3 [j]);
  }
}


} // namespace base64
} // namespace common
} // namespace rosetta
