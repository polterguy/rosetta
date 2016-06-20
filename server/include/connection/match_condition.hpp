
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

#ifndef ROSETTA_SERVER_MATCH_CONDITION_HPP
#define ROSETTA_SERVER_MATCH_CONDITION_HPP

#include <string>
#include <memory>
#include <utility>
#include <stdexcept>
#include <boost/asio.hpp>

namespace rosetta {
namespace server {

using std::pair;
using std::string;
using std::make_pair;
using std::shared_ptr;
using std::make_shared;

/// Match condition plugs into boost asio's "async_read_until" method, and will read from socket until either
/// "max_length" number of characters have been read, or "match" has been read as a sequence from socket,
/// not followed by any of the characters in "unless".
/// Both "match" and "unless" are optional, and if neither is supplied, then "max_length" number of
/// characters will be read.
/// If "max_length" was exceeded, without finding the "match" string, not followed by any characters in
/// the "unless" argument, then has_error() will return true.
class match_condition final
{
public:

  /// Constructor taking a "max_length" number of bytes to read, a "match" string, and an "unless" argument.
  /// Match condition plugs into boost asio's "async_read_until" method, and will read from socket until either
  /// "max_length" number of characters have been read, or "match" has been read as a sequence from socket,
  /// not followed by any of the characters in "unless".
  /// Both "match" and "unless" are optional, and if neither is supplied, then "max_length" number of
  /// characters will be read.
  explicit match_condition (size_t max_length, const string & match = "")
    : _error (make_shared <bool> ()),
      _match (match),
      _left (max_length)
  { }

  /// Returns true if there was an error due to too many characters before delimiter was seen
  bool has_error () const { return *_error; };

  /// Match method invoked from asio for socket read operations when async_read_until is used
  template<typename iterator> pair<iterator, bool> operator() (iterator begin, iterator end)
  {
    iterator idx = begin;
    for (; idx != end; ++idx) {

      // Checking if length exceeds max length.
      if (--_left <= 0) {

        // End of requested "maximum characters" to read.
        // Error is true, only if a delimiter was given, otherwise this was a simple "give me x characters" match.
        *_error = _match.size () > 0;
        return make_pair (idx, true);
      }

      // Checking if this is a "length only" match condition.
      if (_match.size () > 0) {

        // Checking if currently iterated character equals the next character we expect.
        if (_match [_so_far.size()] == *idx) {

          // Currently iterated character was equal to what we were expecting to come up next.
          _so_far.push_back (*idx);

          // Checking if we have an entire match, for all characters.
          if (_so_far.size() == _match.size())
            return make_pair (idx, true); // We have a match, returning true.
        } else if (_so_far.length() > 0) {

          // Throwing away "matched so far" buffer.
          _so_far.clear();

          // Currently iterated character can still equal the first expected char.
          if (_match [0] == *idx)
            _so_far.push_back (*idx); // Currently iterated character was equal to the first match character.
        }
      }
    }

    // No match, keep on reading.
    return make_pair (idx, false);
  }

private:

  /// Since boost asio's async_read_until will copy our match_condition instance, and use our copy,
  /// we need some mechanism of communicating errors into our callback, which are holding a copy of the
  /// originally created match condition. This is done by having a shared pointer to a boolean value,
  /// that indicates if an error occurred.
  /// This allows us to retrieve errors from our originally created match condition, which we passed
  /// into async_read_until, since the one asio is copying, and the one we created, share the same boolean
  /// error condition reference.
  shared_ptr<bool> _error;

  /// The "match" string to look for.
  string _match;

  /// Used to keep track of how many of our match characters we have seen so far.
  string _so_far;

  /// How many bytes we've got left to read from socket before we have a malformed request.
  size_t _left;
};


} // namespace server
} // namespace rosetta

// We have to declare our partial template specialization within the asio namespace.
namespace boost {
namespace asio {
template <> struct is_match_condition<rosetta::server::match_condition> : public true_type {};
}
}

#endif // ROSETTA_SERVER_MATCH_CONDITION_HPP
