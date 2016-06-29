
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

#ifndef ROSETTA_COMMON_CONFIGURATION_HPP
#define ROSETTA_COMMON_CONFIGURATION_HPP

#include <map>
#include <string>
#include <ostream>
#include <functional>
#include <boost/lexical_cast.hpp>
#include "common/include/configuration_exception.hpp"

using std::string;

namespace rosetta {
namespace common {


/// Class encapsulating configuration files.
/// Reads standard configuration files in "key=value" format, with possibility of adding
/// comments by starting a line with "#".
class configuration
{
public:
  
  /// Constructor, creates empty configuration.
  configuration ();

  /// Constructor, pass in path to configuration file.
  /// Uses the system's current path to look for the configuration file supplied, unless a
  /// fully qualified path is supplied.
  explicit configuration (const string & file_path);

  /// Returns value of specified key as type T, or default_value, if key is not found.
  /// Performs lexical_cast to change type of value, which is internally stored as string.
  template<typename T> T get (const string & key, const T & default_value) const
  {
    // Checking if we can find the specified key.
    auto iter = _settings.find (key);
    if (iter == _settings.end()) {

      // Key not found, returning default value.
      return default_value;
    }
    
    // Key was found, performing lexical_cast to type requested by caller.
    return boost::lexical_cast<T> (iter->second);
  }

  /// Returns value of specified key as type T.
  template<typename T> T get (const string & key) const
  {
    // Checking if we can find the specified key.
    auto iter = _settings.find (key);
    if (iter == _settings.end()) {
  
      // Key not found, throwing configuration error.
      throw configuration_exception ("Key; '" + key + "' not found in configuration file, and no default value provided.");
    }
    
    // Key was found, performing lexical_cast to type requested by caller.
    return boost::lexical_cast<T> (iter->second);
  }

  /// Sets configuration key to specified value.
  template<typename T> void set (const string & key, const T & value)
  {
    _settings[key] = boost::lexical_cast<string> (value);
  }

  /// Loads configuration settings from given file.
  /// Notice, you can load multiple configuration files into the same configuration object.
  /// Keys existing in the file you load, will have precedence, overwriting values for the
  /// same keys already in your configuration object.
  void load (const string & file_path);

  /// Saves configuration to specified file.
  /// Unless a fully qualified path is supplied, this method will use your system's current
  /// path as its directory.
  void save (const string & file_path);
  
  /// Prints out the copyright notice on the given stream.
  /// If you wish to inject additional information, inside the "copyright box", you can provide
  /// a callback functor, where you write your own additional information to the stream.
  static void serialize_copyright (std::ostream & stream, std::function<void(std::ostream & stream)> functor = nullptr);

private:

  // Contains our actual settings, as parsed from configuration file
  std::map<string, string> _settings;
};


} // namespace common
} // namespace rosetta

#endif // ROSETTA_COMMON_CONFIGURATION_HPP
