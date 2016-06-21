
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

#include <vector>
#include <string>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include "common/include/configuration.hpp"
#include "common/include/configuration_exception.hpp"

using std::endl;
using std::string;

namespace rosetta {
namespace common {


configuration::configuration()
{ }


configuration::configuration (const string & file_path)
{
  load (file_path);
}


void configuration::load (const string & file_path)
{
  // Creating a file input stream with the given path
  std::ifstream stream (file_path);
  
  // Reading lines from file
  while (stream.good ()) {
    
    // Reading a single line, and trimming it for whitespace characters
    string line;
    getline (stream, line);
    boost::trim (line);
    
    // Checking if this is an empty value, or a comment
    if (line == "" || line[0] == '#')
      continue;
    
    // Sanity check
    if (line.find_first_of ("=") == string::npos)
      throw configuration_exception ("Configuration file corrupt, missing 'value' for key close to '" + line + "'");
    
    // Finding "key" and "value"
    std::vector<string> entities;
    bool first = true;
    boost::algorithm::split (entities, line, [& first] (auto c) {
      if (first && c == '=') {
        first = false;
        return true;
      }
      return false;
    });
    
    // Trimming both key and value
    boost::algorithm::trim_right (entities[0]);
    boost::algorithm::trim_left (entities[1]);

    // Next sanity check
    if (entities[0].length () == 0)
      throw configuration_exception ("Key was empty close to; '" + line + "'");

    if (entities[1].length () == 0)
      throw configuration_exception ("Value was empty close to; '" + line + "'");

    // Stuffing into settings map
    _settings [entities[0]] = entities[1];
  }
}


void configuration::save (const string & file_path)
{
  // Creating a file output stream with the given path
  std::ofstream stream (file_path);
  
  // Writing initial copyright notice, and header information
  serialize_copyright (stream, [] (std::ostream & s){
    s << "# This file provides the settings for your system in a 'key=value' fashion.    #" << endl;
  });
  
  // Writing all keys to file
  for (auto i: _settings) {
    
    // Writing currently iterated setting to file
    stream << i.first << "=" << i.second << endl;
  }
}


void configuration::serialize_copyright (std::ostream & stream, std::function<void(std::ostream & s)> functor)
{
  stream << endl;
  stream << "################################################################################" << endl;
  stream << "#                                                                              #" << endl;
  stream << "# Rosetta server, copyright(c) 2016, Thomas Hansen, phosphorusfive@gmail.com.  #" << endl;
  stream << "# This program is free software: you can redistribute it and/or modify         #" << endl;
  stream << "# it under the terms of the GNU Affero General Public License, as published by #" << endl;
  stream << "# the Free Software Foundation, version 3.                                     #" << endl;
  stream << "#                                                                              #" << endl;
  stream << "# This program is distributed in the hope that it will be useful,              #" << endl;
  stream << "# but WITHOUT ANY WARRANTY; without even the implied warranty of               #" << endl;
  stream << "# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 #" << endl;
  stream << "# GNU Affero General Public License for more details.                          #" << endl;
  stream << "#                                                                              #" << endl;
  stream << "# You should have received a copy of the GNU Affero General Public License     #" << endl;
  stream << "# along with this program.  If not, see <http://www.gnu.org/licenses/>.        #" << endl;
  stream << "#                                                                              #" << endl;
  
  // Invoking callback for providing additional information within copyright segment of file
  if (functor != nullptr) {
    
    // User provided a callback to inject additional information into copyright section
    functor (stream);
    stream << "#                                                                              #" << endl;
  }
  
  stream << "################################################################################" << endl;
  stream << endl;
}


} // namespace common
} // namespace rosetta
