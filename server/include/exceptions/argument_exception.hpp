
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

#ifndef ROSETTA_SERVER_ARGUMENT_EXCEPTION_HPP
#define ROSETTA_SERVER_ARGUMENT_EXCEPTION_HPP

#include <string>
#include <exception>
#include "common/include/rosetta_exception.hpp"

using std::string;
using rosetta::common::rosetta_exception;

namespace rosetta {
namespace server {


/// Exception being thrown when arguments sent into rosetta is somehow not valid
class argument_exception : public rosetta_exception
{
public:

   /// Constructor
  argument_exception (const string & msg) : rosetta_exception (msg) { }
};


} // namespace server
} // namespace rosetta

#endif // ROSETTA_SERVER_ARGUMENT_EXCEPTION_HPP
