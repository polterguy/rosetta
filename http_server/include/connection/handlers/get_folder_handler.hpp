
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

#ifndef ROSETTA_SERVER_STATIC_FOLDER_HANDLER_HPP
#define ROSETTA_SERVER_STATIC_FOLDER_HANDLER_HPP

#include <boost/filesystem.hpp>
#include "common/include/exceptional_executor.hpp"
#include "http_server/include/connection/handlers/request_handler_base.hpp"

using std::string;
using namespace boost::filesystem;
using namespace rosetta::common;

namespace rosetta {
namespace http_server {

class request;
class connection;


/// GET handler for static files.
class get_folder_handler final : public request_handler_base
{
public:

  /// Creates a static file handler.
  get_folder_handler (class request * request);

  /// Handles the given request.
  virtual void handle (connection_ptr connection, std::function<void()> on_success) override;

private:

  /// Checks if file should be rendered back to client, or if we should return a 304.
  bool should_write_folder (path folderpath);

  /// Writes folder content back to client as JSON.
  void write_folder (connection_ptr connection, path folderpath, std::function<void()> on_success);

  /// Writes objects of type either "files" or "folders" back to client as JSON.
  void write_objects (const string & type,
                      connection_ptr connection,
                      std::shared_ptr<std::vector<unsigned char>> buffer_ptr,
                      path folderpath);

  /// Writes 304 response back to client.
  void write_304_response (connection_ptr connection, std::function<void()> on_success);
};


} // namespace http_server
} // namespace rosetta

#endif // ROSETTA_SERVER_STATIC_FOLDER_HANDLER_HPP
