
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

#ifndef ROSETTA_SERVER_URL_ENCODE_HPP
#define ROSETTA_SERVER_URL_ENCODE_HPP

#include <string>

using std::string;

namespace rosetta {
namespace http_server {
namespace uri_encode {


/// Decodes a URI encoded string.
string decode (const string & uri);


/// URI Encodes a string.
string encode (const string & val);


} // namespace uri_encode
} // namespace http_server
} // namespace rosetta

#endif // ROSETTA_SERVER_URL_ENCODE_HPP
