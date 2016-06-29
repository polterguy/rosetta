
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

#ifndef ROSETTA_SERVER_DATE_HELPER_HPP
#define ROSETTA_SERVER_DATE_HELPER_HPP

#include <string>
#include <boost/date_time/local_time/local_time.hpp>

using std::string;

namespace rosetta {
namespace http_server {


/// Helper class to create and parse HTTP standard date strings.
class date final
{
public:

  /// Returns the "now" date.
  static date now ();

  /// Returns a date according to when a file was last changed.
  static date from_file_change (const string & filepath);

  /// Parses a date from the given HTTP date format.
  static date parse (const string & value);

  /// Returns the date as a string, formatted according to RFC 1123.
  string to_string () const;

  /// Returns the date as a string, formatted according to ISO 8601.
  string to_iso_string () const;

  /// Comparison less-than operator.
  friend bool operator < (const date & lhs, const date & rhs);

  /// Comparison more-than operator.
  friend bool operator > (const date & lhs, const date & rhs);

private:

  /// Creates a new date, with its value being "now".
  date ();

  /// Creates a new date, with its value being the specified "date".
  date (const boost::local_time::local_date_time & date);

  /// Actual content of date object.
  boost::local_time::local_date_time _date;
};


} // namespace http_server
} // namespace rosetta

#endif // ROSETTA_COMMON_DATE_HELPER_HPP
