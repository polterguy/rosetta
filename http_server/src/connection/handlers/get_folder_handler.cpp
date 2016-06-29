
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

#include <tuple>
#include <vector>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "http_server/include/server.hpp"
#include "http_server/include/helpers/date.hpp"
#include "http_server/include/connection/request.hpp"
#include "http_server/include/connection/connection.hpp"
#include "http_server/include/exceptions/request_exception.hpp"
#include "http_server/include/connection/handlers/get_folder_handler.hpp"

namespace rosetta {
namespace http_server {

using std::string;
using boost::system::error_code;
using namespace rosetta::common;


get_folder_handler::get_folder_handler (class connection * connection, class request * request)
  : request_handler_base (connection, request)
{ }


void get_folder_handler::handle (exceptional_executor x, functor on_success)
{
  // Retrieving root path, and checking if we should write it.
  path full_path = request()->envelope().path();
  if (should_write_folder (full_path)) {

    // Returning file to client.
    write_folder (full_path, x, on_success);
  } else {

    // File has not been tampered with since the "If-Modified-Since" HTTP header, returning 304 response, without file content.
    write_304_response (x, on_success);
  }
}


bool get_folder_handler::should_write_folder (path full_path)
{
  // Checking if client passed in an "If-Modified-Since" header.
  string if_modified_since = request()->envelope().header ("If-Modified-Since");
  if (if_modified_since != "") {

    // We have an "If-Modified-Since" HTTP header, checking if file was tampered with since that date.
    date if_modified_date   = date::parse (if_modified_since);
    date folder_modify_date = date::from_file_change (full_path.string ());

    // Comparing dates.
    if (folder_modify_date > if_modified_date) {

      // File was modified after "If-Modified-Since" header, hence we should write file to client.
      return true;
    } else {

      // File has not been tampered with since the "If-Modified-Since" HTTP header, hence we should not write file back to client.
      return false;
    }
  } else {

    // Client sent no "If-Modified-Since" header, hence we should write file back to client.
    return true;
  }
}


void get_folder_handler::write_304_response (exceptional_executor x, functor on_success)
{
  // Writing status code 304 (Not-Modified) back to client.
  write_status (304, x, [this, on_success] (auto x) {

    // Writing standard HTTP headers to connection.
    write_standard_headers (x, [this, on_success] (auto x) {

      // Making sure we close envelope.      
      ensure_envelope_finished (x, [on_success] (auto x) {

        // invoking callback, since we're done writing the response.
        on_success (x);
      });
    });
  });
}


void get_folder_handler::write_folder (path folderpath, exceptional_executor x, functor on_success)
{
  // Using shared_ptr of vector to hold folder information.
  auto buffer_ptr = std::make_shared<std::vector<unsigned char>> ();
  buffer_ptr->push_back ('{');
  string folder_content = "\"content\":";
  buffer_ptr->insert (buffer_ptr->end(), folder_content.begin(), folder_content.end());
  buffer_ptr->push_back ('[');

  // Iterating over all objects in folder.  
  directory_iterator idx {folderpath};
  bool first = true;
  while (idx != directory_iterator{}) {

    // Retrieving path, and making sure this is not an invisible file/folder for some reasons.
    string path = idx->path().string();
    path = path.substr (folderpath.size ());

    // Checking if we should show this bugger.
    if (path [0] == '.' || path [0] == '~') {

      // Invisible guy!
      ++idx;
      continue;
    }

    // Making sure we get a "," between each entry.
    if (first)
      first = false;
    else
      buffer_ptr->push_back (',');

    // Making sure every object becomes a JavaScript/JSON object.
    buffer_ptr->push_back ('{');

    // Name of object.
    string name = "\"name\":\"" + path + "\"";
    buffer_ptr->insert (buffer_ptr->end(), name.begin(), name.end());

    // Type of object.
    string type = ",\"type\":\"" + string(is_directory (*idx) ? "folder" : "file") + "\"";
    buffer_ptr->insert (buffer_ptr->end(), type.begin(), type.end());

    // Size of object.
    if (is_regular_file (*idx)) {

      // We report the size of this guy, since it is a file.
      string object_size = ",\"size\":\"" + boost::lexical_cast<string>(file_size (*idx)) + "\"";
      buffer_ptr->insert (buffer_ptr->end(), object_size.begin(), object_size.end());
    }

    // Last changed.
    string last_change = ",\"changed\":\"" + date::from_file_change (idx->path().string ()).to_iso_string () + "\"";
    buffer_ptr->insert (buffer_ptr->end(), last_change.begin(), last_change.end());

    // Closing JSON object.
    buffer_ptr->push_back ('}');
    ++idx;
  }

  // Closing JSON array
  buffer_ptr->push_back (']');
  buffer_ptr->push_back ('}');

  // Writing status code.
  write_status (200, x, [this, buffer_ptr, folderpath, on_success] (auto x) {

    // Writing standard headers to client.
    write_standard_headers (x, [this, buffer_ptr, folderpath, on_success] (auto x) {

      // Writing headers for folder information.
      size_t size = buffer_ptr->size();

      // Building our standard response headers for a folder information transfer.
      collection headers {
        {"Content-Type", "application/json; charset=utf-8"},
        {"Content-Length", boost::lexical_cast<string> (size)},
        {"Last-Modified", date::from_file_change (folderpath.string ()).to_string ()}};

      // Writing special handler headers to connection.
      write_headers (headers, x, [this, buffer_ptr, on_success] (auto x) {
        
        // Make sure we close envelope.
        ensure_envelope_finished (x, [this, buffer_ptr, on_success] (auto x) {

          // Now writing content of folder.
          connection()->socket().async_write (buffer (*buffer_ptr), [this, on_success, x, buffer_ptr] (auto error, auto bytes_written) {

            // Finished!
            on_success (x);
          });
        });
      });
    });
  });
}


} // namespace http_server
} // namespace rosetta
