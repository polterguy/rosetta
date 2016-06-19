
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
  explicit match_condition (size_t max_length, const string & match = "", const string & unless = "")
    : _error (make_shared <bool> ()),
      _match (match),
      _left (max_length),
      _max_length (max_length),
      _unless (unless),
      _found_match (false)
  { }

  /// Returns true if there was an error due to too many characters before delimiter was seen
  bool has_error () const { return *_error; };

  /// Match method invoked from asio for socket read operations when async_read_until is used
  template<typename iterator> pair<iterator, bool> operator() (iterator begin, iterator end)
  {
    iterator idx = begin;
    for (; idx != end; ++idx) {

      // Checking if this is a "length only" match condition.
      if (_match.size () != 0) {

        // Checking if we saw a match in our last iteration.
        if (_found_match) {

          // Making sure we unsignal the match object, in case we don't get a full match,
          // due to our "unless" condition kicking in, giving us a false positive.
          _found_match = false;

          // Checking if next character is not among our "unless" characters, and if so, returning a match.
          if (_unless.find (*idx) == string::npos) {

            // Unless condition did not kick in, stopping iteration, making sure we keep the currently iterated
            // character still available in the buffer, for any following async_read_until operations.
            return make_pair (--idx, true);
          } else {

            // Unless character kicked in, restarting process of finding a match.
            _so_far.clear ();
          }
        }

        // Checking if current char equals next char in delimiter range.
        if (_match [_so_far.length()] == *idx) {

          // Currently iterated char was equal to what we were expected up next.
          _so_far.push_back (*idx);

          // Checking if we have an entire match, for all characters.
          if (_so_far == _match) {

            // We have a match, now we signal this for next round, which checks the "unless" characters.
            // But only if we have read anything besides CRLF from the socket, to make sure we don't wait
            // indefinitely for something that never turns up.
            if (_left + 1 == _max_length)
              return make_pair (idx, true);
            _found_match = true;
          }
        } else if (_so_far.length() > 0) {

          // Currently iterated char was not equal to the char we expected coming up next,
          // hence we throw away entire _so_far buffer.
          _so_far.clear();

          // Currently iterate char can still equal the first expected char.
          if (_match [0] == *idx) {

            // Content of idx was the first character in our expectations.
            _so_far.push_back (*idx);
          }
        }
      }

      // Checking if length exceeds max length.
      if (_left-- == 0) {

        // End of requested "maximum characters" to read.
        // Error is true, only if a delimiter was given.
        *_error = _match.size () > 0;
        return make_pair (idx, true);
      }
    }

    // No match, keep on reading
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

  /// Contains all characters matched so far from delimiter given to match method.
  string _so_far;

  /// How many bytes we've got left to read from socket before we have a malformed request.
  size_t _left;

  /// Maximum number of characters to read.
  size_t _max_length;

  /// Match condition does not kick in, if first character after match condition normally should kick in,
  /// is one of the characters in _unless.
  string _unless;

  /// Used to signal we have a match for next iteration.
  bool _found_match;
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
