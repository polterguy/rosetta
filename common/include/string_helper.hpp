
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

#ifndef ROSETTA_COMMON_STRING_HELPER_HPP
#define ROSETTA_COMMON_STRING_HELPER_HPP

#include <string>
#include <boost/asio.hpp>

using std::string;
using boost::asio::streambuf;

namespace rosetta {
namespace common {


/// Helper class to manipulate strings coming in our out from HTTP requests.
class string_helper final
{
public:
  
  /// Helper to retrieve a line from an asio stream buffer.
  /// Will verify that no "garbage data" is transmitted, which means anything not being either between [32-127),
  /// or CR, LF or TAB. Will also verify that CR/LF is only found at the end, and that there is a valid CR/LF sequence.
  /// If the given streambuf's content does not match these criteria, the method will throw an exception.
  static string get_line (streambuf & buffer);
  
  /// Decodes the given URI from its URI encoded notation.
  static string decode_uri (const string & uri);

private:
  
  /// Private constructor, to avoid instantiation, and force usage of only static methods.
  string_helper () { }
};


} // namespace common
} // namespace rosetta

#endif // ROSETTA_COMMON_STRING_HELPER_HPP
