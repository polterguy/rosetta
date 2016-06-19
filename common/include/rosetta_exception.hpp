
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

#ifndef ROSETTA_COMMON_ROSETTA_EXCEPTION_HPP
#define ROSETTA_COMMON_ROSETTA_EXCEPTION_HPP

#include <string>
#include <exception>

using std::string;
using std::exception;

namespace rosetta {
namespace common {


/// Base exception class for Rosetta.
/// All exceptions thrown by Rosetta will inherit, either directly, or indirectly from this class.
class rosetta_exception : public exception
{
public:
   
  /// Constructor taking the error message.
  rosetta_exception (const string & msg) : _message(msg) { }
  
  /// Returns the error message supplied when throwing exception.
  virtual char const * what() const throw () { return _message.c_str(); }

private:
  
  /// Contains the message supplied when exception was thrown.
  string _message;
};


} // namespace common
} // namespace rosetta

#endif // ROSETTA_COMMON_ROSETTA_EXCEPTION_HPP
