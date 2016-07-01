
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

#ifndef ROSETTA_SERVER_SHA1_HELPER_HPP
#define ROSETTA_SERVER_SHA1_HELPER_HPP

#include <array>
#include <vector>

using std::array;
using std::vector;

namespace rosetta {
namespace common {
namespace sha1 {


array<unsigned char, 20> compute (const vector<unsigned char> & data);


} // namespace sha1
} // namespace common
} // namespace rosetta

#endif // ROSETTA_SERVER_SHA1_HELPER_HPP
