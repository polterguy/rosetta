
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

#ifndef ROSETTA_COMMON_CONFIGURATION_EXCEPTION_HPP
#define ROSETTA_COMMON_CONFIGURATION_EXCEPTION_HPP

#include <string>
#include "common/include/rosetta_exception.hpp"

using std::string;

namespace rosetta {
namespace common {
   

/// Exception being thrown when there are configuration errors.
class configuration_exception : public rosetta_exception
{
public:
   
  /// Constructor taking an error message
  configuration_exception (const string & msg) : rosetta_exception (msg) { }
};


} // namespace common
} // namespace rosetta

#endif // ROSETTA_COMMON_CONFIGURATION_EXCEPTION_HPP
