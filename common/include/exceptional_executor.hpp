
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

#ifndef ROSETTA_COMMON_EXCEPTIONAL_EXECUTOR_HPP
#define ROSETTA_COMMON_EXCEPTIONAL_EXECUTOR_HPP

#include <functional>

using std::function;

namespace rosetta {
namespace common {


/// Helper class to create deterministic execution of code, that only executes if release() is not invoked,
/// before instance is destroyed.
/// Useful for creating a guarantee of piece of code that will execute if an exception occurs.
/// Class has "autp_pointer" semantics, which means that when one instance is copied over to another instance,
/// then the original owner will release the function it owns, and leave ownership over to the instance
/// that was made a copy of the original exceptional-executor.
/// This allows for you to pass on instances of exceptional_executor to methods and functions, and let the
/// method you pass it on to, claim ownership of invoking release() on the original piece of code.
class exceptional_executor final
{
public:

  /// Constructor taking a function that will evaluate when object goes out of scope, unless release() is called before.
  exceptional_executor (function<void ()> functor);

  /// Destructor that invokes functor object unless release is invoked before destruction occurs.
  ~exceptional_executor ();

  /// Copy constructor, to make sure we move functor object.
  exceptional_executor (const exceptional_executor & rhs);

  /// Assignment operator, to make sure we move functor object.
  exceptional_executor & operator = (const exceptional_executor & rhs);

  /// Releases functor object, making sure it is never invoked.
  void release ();

private:

  /// Function object that will evaluate when instance is destroyed, unless released before destructor is invoked.
  mutable function<void ()> _functor;
};


} // namespace common
} // namespace rosetta

#endif // ROSETTA_COMMON_EXCEPTIONAL_EXECUTOR_HPP
