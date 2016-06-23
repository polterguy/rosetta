
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

#include <functional>
#include "common/include/rosetta_exception.hpp"
#include "common/include/exceptional_executor.hpp"

namespace rosetta {
namespace common {

using std::string;


exceptional_executor::exceptional_executor (std::function<void ()> functor)
  : _functor (functor)
{ }


exceptional_executor::~exceptional_executor ()
{
  if (_functor != nullptr)
    _functor();
}


exceptional_executor::exceptional_executor (const exceptional_executor & rhs)
{
  _functor = rhs._functor;
  rhs._functor = nullptr;
}


exceptional_executor & exceptional_executor::operator = (const exceptional_executor & rhs)
{
  _functor = rhs._functor;
  rhs._functor = nullptr;
  return *this;
}


void exceptional_executor::release () const
{
  _functor = nullptr;
}


} // namespace common
} // namespace rosetta
