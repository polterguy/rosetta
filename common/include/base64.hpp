
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

#ifndef ROSETTA_SERVER_BASE64_HELPER_HPP
#define ROSETTA_SERVER_BASE64_HELPER_HPP

#include <string>
#include <vector>

using std::string;
using std::vector;

namespace rosetta {
namespace common {
namespace base64 {

/// Encodes the given bytes into a base64 string, and returns to caller.
void encode (const vector<unsigned char> & bytes, string & result);

/// Decodes a base64 encoded string, and returns as a vector of unsigned char.
void decode (const string & base64_string, vector<unsigned char> & result);


} // namespace base64
} // namespace common
} // namespace rosetta

#endif // ROSETTA_SERVER_BASE64_HELPER_HPP
