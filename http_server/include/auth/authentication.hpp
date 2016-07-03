
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

#ifndef ROSETTA_SERVER_AUTHENTICATION_HPP
#define ROSETTA_SERVER_AUTHENTICATION_HPP

#include <map>
#include <tuple>
#include <functional>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/thread.hpp>

using std::string;
using namespace boost::asio;
using namespace boost::filesystem;

namespace rosetta {
namespace http_server {

class server;

typedef std::function<void(bool)> success_handler;


/// Responsible for authenticate a client.
class authentication final : boost::noncopyable
{
public:

  /// Ticket class, wraps an authenticated user.
  struct ticket final
  {
    inline bool authenticated() const { return username.size() > 0; }
    string username;
    string role;
  };

  /// Authenticates a user, and returns a ticket.
  ticket authenticate (const string & username, const string & password, const string & server_salt) const;

  /// Changes password of specified account.
  void change_password (const string & username, const string & password, const string & server_salt);

  /// Changes role of specified account.
  void change_role (const string & username, const string & role);

  /// Creates a new user in system.
  void create_user (const string & username, const string & password, const string & role, const string & server_salt);

  /// Creates a new user in system.
  void delete_user (const string & username);

private:

  /// Making sure only server class can created instances.
  friend class server;

  /// Creates an authentication instance.
  authentication (io_service & service);

  /// Wraps a single user in system
  struct user final
  {
    string username;
    string password;
    string role;
  };

  /// Saves authentication file.
  void save ();


  /// Users, with their usernames and roles.
  std::map<string, user> _users;

  /// Synchronization object for making sure write operations are atomic.
  mutable boost::shared_mutex _lock;

  /// io_service this instance belongs to.
  io_service & _service;

  /// True if authentication file is already on its way to being saved.
  bool _file_save_in_progress;
};


} // namespace http_server
} // namespace rosetta

#endif // ROSETTA_SERVER_AUTHENTICATION_HPP
