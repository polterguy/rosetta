
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
#include "common/include/date.hpp"
#include "server/include/server.hpp"
#include "server/include/connection/request.hpp"
#include "server/include/connection/connection.hpp"
#include "server/include/exceptions/request_exception.hpp"
#include "server/include/connection/handlers/static_file_handler.hpp"

namespace rosetta {
namespace server {

using std::tuple;
using std::vector;
using std::string;
using std::ifstream;
using std::shared_ptr;
using std::istreambuf_iterator;
using boost::system::error_code;
using boost::algorithm::split;
using boost::asio::async_write;
using namespace rosetta::common;


static_file_handler::static_file_handler (server * server, socket_ptr socket, request * request, const string & extension)
  : request_handler (server, socket, request),
    _extension (extension)
{ }


void static_file_handler::handle (exceptional_executor x, function<void (exceptional_executor x)> callback)
{
  // Retrieving URI from request, removing initial "/" from URI.
  string uri = _request->uri ().substr (1);

  // Breaking up URI into components, and sanity checking each component, to verify client is not requesting an illegal URI.
  vector<string> entities;
  split (entities, uri, boost::is_any_of ("/"));
  for (string & idx : entities) {

    if (idx == "")
      throw request_exception ("Illegal URI; '" + _request->uri () + "'."); // Two consecutive "/" after each other.

    if (idx.find ("..") != string::npos) // Request is probably trying to access files outside of the main www-root folder.
      throw request_exception ("Illegal URI; '" + _request->uri () + "'.");

    if (idx.find ("~") == 0) // Linux backup file.
      throw request_exception ("Illegal URI; '" + _request->uri () + "'.");

    if (idx.find (".") == 0) // Linux hidden file, or a file without a name, and only extension.
      throw request_exception ("Illegal URI; '" + _request->uri () + "'.");
  }

  // Figuring out which file was requested.
  string path = _server->configuration().get<string> ("www-root", "www-root/") + uri;

  // Making sure file exists.
  if (!boost::filesystem::exists (path)) {

      // Writing error status response, and returning early.
      _request->write_error_response (404, x);
      return;
  }

  // Checking if client passed in an "If-Modified-Since" header, and if so, handle it accordingly.
  string if_modified_since = (*_request)["if-modified-since"];
  if (if_modified_since != "") {

    // We have an "If-Modified-Since" HTTP header, checking if file was tampered with since that date.
    date if_modified_date = date::parse (if_modified_since);
    date file_modify_date = date::from_file_change (path);

    // Comparing dates.
    if (file_modify_date > if_modified_date) {

      // File has been tampered with since the "If-Modified-Since" HTTP header, returning file in response.
      write_file (path, x, callback);
    } else {

      // File has not been tampered with since the "If-Modified-Since" HTTP header, returning 304 response, without file content.
      write_status (304, x, [this, callback] (exceptional_executor x) {

        // Building our request headers.
        vector<tuple<string, string> > headers { {"Date", date::now ().to_string ()} };

        // Writing HTTP headers to connection.
        write_headers (headers, x, nullptr);
      });
    }
  } else {

    // Writing file to client on socket.
    write_file (path, x, callback);
  }
}


// Returns the requested file back to client.
void static_file_handler::write_file (const string & filepath, exceptional_executor x, function<void (exceptional_executor x)> callback)
{
  // Figuring out size of file, and making sure it's not larger than what we are allowed to handle according to configuration of server.
  size_t size = boost::filesystem::file_size (filepath);
  if (size > _server->configuration().get<size_t> ("max-response-content-length", 4194304)) {

    // File was too long for server to serve according to configuration.
    _request->write_error_response (500, x);
    return;
  }

  // Retrieving MIME type, and verifying this is a type of file we actually serve.
  string mime_type = get_mime ();
  if (mime_type == "") {

    // File type is not served according to configuration of server.
    _request->write_error_response (403, x);
    return;
  }

  // Writing out status line on socket.
  write_status (200, x, [this, filepath, callback, size, mime_type] (exceptional_executor x) {

    // Building our request headers.
    vector<tuple<string, string> > headers {
      {"Content-Type", mime_type },
      {"Date", date::now ().to_string ()},
      {"Last-Modified", date::from_file_change (filepath).to_string ()},
      {"Content-Length", lexical_cast<string> (size)}};

    // Writing HTTP headers to connection.
    write_headers (headers, x, [this, filepath, callback] (exceptional_executor x) {

      // Writing additional CR/LF sequence, to signal to client that we're beginning to send content.
      async_write (*_socket, buffer (string("\r\n")), [this, filepath, callback, x] (const error_code & error, size_t bytes_written) {

        // Sanity check.
        if (error)
          throw request_exception ("Socket error while writing file; '" + filepath + "'.");

        // Reading file's content, and putting it into a vector.
        ifstream fs (filepath, std::ios_base::binary);
        vector<char> file_content ((istreambuf_iterator<char> (fs)), istreambuf_iterator<char>());

        // Writing content to connection's socket.
        async_write (*_socket, buffer (file_content), [filepath, callback, x] (const error_code & error, size_t bytes_written) {

          // Sanity check.
          if (error)
            throw request_exception ("Socket error while writing file; '" + filepath + "'.");

          // Finished serving static file, invoking callback supplied when invoking method.
          callback (x);
        });
      });
    });
  });
}


string static_file_handler::get_mime ()
{
  // Returning MIME type for file extension, defaulting to "application/octet-stream"
  return _server->configuration().get<string> ("mime-" + _extension, "");
}


} // namespace server
} // namespace rosetta
