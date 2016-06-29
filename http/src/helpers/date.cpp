
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
#include <sstream>
#include <boost/filesystem.hpp>
#include "http/include/helpers/date.hpp"

using std::string;

namespace rosetta {
namespace common {


date::date ()
  : _date (boost::local_time::local_sec_clock::local_time (boost::local_time::time_zone_ptr ()))
{ }


date::date (const boost::local_time::local_date_time & date)
  : _date (date)
{ }


date date::now()
{
  return date ();
}


date date::from_file_change(const string & filepath)
{
  boost::posix_time::ptime pt = boost::posix_time::from_time_t (boost::filesystem::last_write_time (filepath));
  return date (boost::local_time::local_date_time (pt, boost::local_time::time_zone_ptr ()));
}


date date::parse (const string & value)
{
  // Figuring out which date format this is, to support HTTP/1.0
  size_t pos_of_comma = value.find (",");
  boost::local_time::local_time_input_facet * lt = nullptr;
  if (pos_of_comma == 3) {

    // Standard HTTP/1.1 date format, RFC 1123.
    lt = new boost::local_time::local_time_input_facet("%a, %d %b %Y %H:%M:%S GMT");

  } else if (pos_of_comma != string::npos) {

    // RFC 850
    lt = new boost::local_time::local_time_input_facet("%A, %d-%b-%y %H:%M:%S GMT");

  } else {
    
    // ANSI C's asctime() format.
    // TODO: Doesn't work on my Mac.
    // According to boost date_time, the %e formatting flag is "missing on some platforms" (quote)
    lt = new boost::local_time::local_time_input_facet("%a %b %e %H:%M:%S %Y");
  }

  // Turning given string into a date using stringstream.
  std::stringstream ss (value);
  ss.imbue (std::locale(std::locale::classic (), lt));
  date return_value;
  ss >> return_value._date;
  return return_value;
}


string date::to_string () const
{
  boost::local_time::local_time_facet * lf (new boost::local_time::local_time_facet("%a, %d %b %Y %H:%M:%S GMT"));
  std::stringstream ss;
  ss.imbue (std::locale (ss.getloc(), lf));
  ss << _date;
  return ss.str();
}


bool operator < (const date & lhs, const date & rhs)
{
  return lhs._date < rhs._date;
}


bool operator > (const date & lhs, const date & rhs)
{
  return lhs._date > rhs._date;
}


} // namespace common
} // namespace rosetta
