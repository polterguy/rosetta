
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
#include "server/include/exceptions/request_exception.hpp"

namespace rosetta {
namespace common {

using std::string;

/// Match condition plugs into boost asio's "async_read_until" method, and will read from socket, until either
/// "max_length" number of characters have been read, or a CRLF sequence has been encountered.
/// If it stops due to "max_length" characters have been read, it will return true for has_error().
class match_condition final
{
public:

  /// Constructor taking a "max_length" number of bytes to read.
  explicit match_condition (size_t max_length)
    : _error (std::make_shared <bool> (false)),
      _left (max_length)
  { }

  /// Returns true if there was an error due to too many characters before delimiter was seen.
  bool has_error () const { return *_error; };

  /// Match method for using async_read_until from boost asio, for reading a line of data in an HTTP envelope.
  /// Notice that this method does not validate the validity of the characters read in any ways. It simply reads
  /// until either "max_length" has been seen, or LF has been seen.
  /// If it stops due to "max_length" characters have been read, it will return true for has_error().
  template<typename iterator> std::pair<iterator, bool> operator() (iterator begin, iterator end)
  {
    iterator idx = begin;
    for (; idx != end; ++idx) {

      // Checking the type of character.
      if (*idx == '\n')
        return std::make_pair (idx, true);

      // Checking if length exceeds max length.
      if (--_left == 0) {

        // End of "max characters to read", setting object into "error mode".
        *_error = true;
        return std::make_pair (idx, true);
      }
    }

    // No match, keep on reading.
    return std::make_pair (idx, false);
  }

private:

  /// Since boost asio's async_read_until will copy our match_condition instance, and use our copy,
  /// we need some mechanism of communicating errors into our callback, which are holding a copy of the
  /// originally created match condition. This is done by having a shared pointer to a boolean value,
  /// that indicates if an error occurred.
  /// This allows us to retrieve errors from our originally created match condition, which we passed
  /// into async_read_until, since the one asio is copying, and the one we created, share the same boolean
  /// error condition reference.
  std::shared_ptr<bool> _error;

  /// How many bytes we've got left to read from socket before we have a malformed request.
  size_t _left;
};


} // namespace common
} // namespace rosetta

// We have to declare our partial template specialization within the asio namespace.
namespace boost {
namespace asio {
template <> struct is_match_condition<rosetta::common::match_condition> : public true_type {};
}
}

#endif // ROSETTA_SERVER_MATCH_CONDITION_HPP
