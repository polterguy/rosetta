
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
#include <boost/uuid/sha1.hpp>
#include "common/include/sha1.hpp"

using std::string;
using namespace boost::uuids;

namespace rosetta {
namespace common {
namespace sha1 {


array<unsigned char, 20> compute (const vector<unsigned char> & data)
{
  array<unsigned char, 20> return_value;
  detail::sha1 sha;
  sha.process_bytes (&data.front(), data.size());
  unsigned int digest [5];
  sha.get_digest (digest);

  for (int idx = 0; idx < 5; idx++)
  {
    const char * tmp = reinterpret_cast<char *> (&digest[idx]);
    return_value [idx * 4] = tmp [3];
    return_value [idx * 4 + 1] = tmp [2];
    return_value [idx * 4 + 2] = tmp [1];
    return_value [idx * 4 + 3] = tmp [0];
  }
  return return_value;
}


} // namespace sha1
} // namespace common
} // namespace rosetta
