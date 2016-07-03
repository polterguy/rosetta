
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
#include <fstream>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <common/include/sha1.hpp>
#include <common/include/base64.hpp>
#include "http_server/include/auth/authentication.hpp"
#include "http_server/include/exceptions/server_exception.hpp"
#include "http_server/include/exceptions/security_exception.hpp"

using std::vector;
using std::getline;
using boost::trim;
using boost::split;
using boost::shared_lock;
using boost::unique_lock;
using boost::shared_mutex;

namespace rosetta {
namespace http_server {


authentication::authentication (io_service & service)
  : _service (service),
    _file_save_in_progress (false)
{
  // Opening authentication file given.
  ifstream auth_file (".users");
  if (!auth_file.good())
    throw server_exception ("Couldn't open authentication file for server.");

  // Reading all users from authentication file.
  while (!auth_file.eof()) {

    // Fetching next user from authentication file.
    string line;
    getline (auth_file, line);
    trim (line);
    if (line.size() == 0)
      continue; // Empty line

    // Splitting line into username:password:role.
    vector<string> entities;
    split (entities, line, boost::is_any_of (":"));

    // Basic sanity check.
    if (entities.size() != 3)
      throw server_exception ("Authentication file is corrupted.");
    if (_users.find (entities[0]) != _users.end())
      throw server_exception ("Authentication file is corrupted, same user is listed multiple times.");

    // Inserting username as tuple with password and role user belongs to.
    _users [entities[0]] = user {entities[0], entities[1], entities[2]};
  }
}


authentication::ticket authentication::authenticate (const string & username, const string & password, const string & server_salt) const
{
  // Creating a sha1 out of password + server_salt, and base64 encoding the results, before doing comparison against password in auth file.
  auto to_hash = password + server_salt;
  auto sha1 = sha1::compute ( {to_hash.begin(), to_hash.end ()} );
  string base64_password;
  base64::encode ( {sha1.begin(), sha1.end()}, base64_password);

  // Making sure we synchronize access to users, in a shared_lock, allowing multiple readers, but none while write operation is executed.
  shared_lock<shared_mutex> lock (_lock);

  // Finding user with username and password combination
  auto user_iter = _users.find (username);
  if (user_iter == _users.end() || user_iter->second.password != base64_password) {

    // Couldn't authenticate client.
    throw security_exception ("No such user.");
  } else {

    // Authenticated!
    return ticket {user_iter->second.username, user_iter->second.role};
  }
}


void authentication::change_password (const string & username, const string & password, const string & server_salt)
{
  // Creating a sha1 out of password + server_salt, and base64 encoding the results, which becomes the password to put into auth file.
  auto to_hash = password + server_salt;
  auto sha1 = sha1::compute ( {to_hash.begin(), to_hash.end ()} );
  string base64_password;
  base64::encode ( {sha1.begin(), sha1.end()}, base64_password);

  // Making sure we synchronize access to users, in a unique_lock, to prevent any other read/write operations to execute concurrently.
  unique_lock<shared_mutex> lock (_lock);

  // Finding user with username and password combination
  auto user_iter = _users.find (username);
  if (user_iter != _users.end()) {

    // Username match, updating password before saving auth file.
    user_iter->second.password = base64_password;
    save ();
  } else {

    // No such user.
    throw security_exception ("No such user.");
  }
}


void authentication::change_role (const string & username, const string & role)
{
  // Making sure we synchronize access to users, in a unique_lock, to prevent any other read/write operations while this one is executed.
  unique_lock<shared_mutex> lock (_lock);

  // Finding user with username and password combination
  auto user_iter = _users.find (username);
  if (user_iter != _users.end()) {

    // Username match, updating role before saving auth file.
    user_iter->second.role = role;
    save ();
  } else {

    // No such user.
    throw security_exception ("No such user.");
  }
}


void authentication::create_user (const string & username, const string & password, const string & role, const string & server_salt)
{
  // Creating a sha1 out of password + server_salt, and base64 encoding the results, before creating our new user.
  auto to_hash = password + server_salt;
  auto sha1 = sha1::compute ( {to_hash.begin(), to_hash.end ()} );
  string base64_password;
  base64::encode ( {sha1.begin(), sha1.end()}, base64_password);

  // Making sure we synchronize access to users, in a unique_lock, to prevent any other read/write operations while this one is executed.
  unique_lock<shared_mutex> lock (_lock);

  // Checking if user with the same name exists from before.
  auto user_iter = _users.find (username);
  if (user_iter == _users.end()) {

    // Username is not taken, creating user before saving auth file.
    _users [username] = {username, base64_password, role};
    save ();
  } else {

    // User already exists.
    throw security_exception ("User already exists.");
  }
}


void authentication::delete_user (const string & username)
{
  // Making sure we synchronize access to users, in a unique_lock, to prevent any other read/write operations while this one is executed.
  unique_lock<shared_mutex> lock (_lock);

  // Checking if user exists.
  auto user_iter = _users.find (username);
  if (user_iter != _users.end()) {

    // Deleting user, before saving updated auth file.
    _users.erase (username);
    save ();
  } else {

    // User did not exist.
    throw security_exception ("No such user.");
  }
}


// Explanation;
// When we enter this method, we're inside a unique_lock, which we need while updating the users database in memory.
// However, we want to release this lock ASAP.
// Hence, what we do, is to set a boolean, signaling to other threads, that file is already on its way to being saved.
// If another thread then invokes this method, before file actually has been physically saved to disc, then it will
// see that the boolean value is true, hence it will not post its own save() function as a future, making us get away
// with one save(), possibly for multiple updates to the database.
// In addition, since the actual save() implementation is actually only reading the memory, and never actually modifying
// the database, which is kept in memory, the save() method only requires in fact a shared_lock, allowing other readers
// to gain access to the database, as long as they are only reading from it.
// This allows us to get away with having a unique_lock that *ONLY* locks the database, while it is modifying the data in
// memory, and not while it is saving whatever it has in memory to disc.
// This again, results in a *TINY* lock, which is extremely short, and hence does not lock down access to the users database,
// for a long time, and in fact only while memory is being changed.
// So basically, a thread that tries to invoke save(), before another thread had its save() executed, will simply return
// immediately, without initiating a save() operation of its own.
//
// Or to explain it really, really simple;
// Writing the users database to disc is not a "lock operation", "locking" only occurs when changing its values in memory!
//
// Simply put; Fucking Brilliant!! ;)
void authentication::save ()
{
  // This might make us get away with one save operation, for multiples changes, since there is no reasons to post work,
  // saving the file, if another thread did the same thing, and its save method was still not executed.
  // Notice, we're still inside a "unique_lock" at this point, so changing the boolean is safe!
  if (_file_save_in_progress)
    return;
  _file_save_in_progress = true;

  // Notice, to release lock ASAP, we post this as "future work" to io_service.
  _service.post ([this] () {

    // Making sure we synchronize access to users, in a shared_lock, allowing multiple readers,
    // but none while any change operations are executed.
    // This prevents another "change" operation to enter, while this one is being performed, while
    // still allowing other "read" operations to enter.
    // Making our locks *TINY*!!
    shared_lock<shared_mutex> lock (_lock);

    // Saving file.
    ofstream fs (".users", std::ios::trunc | std::ios::out);
    if (!fs.good ())
      throw server_exception ("Couldn't open authentication file for writing.");
    for (auto & idx : _users) {
      fs << idx.first << ":" << idx.second.password << ":" << idx.second.role << std::endl;
    }

    // Signaling to other threads that they must post their own save() functions to have file saved.
    // Notice, due to the shared_lock above in this function, it's really quite irrelevant exactly where in
    // this method we are changing this boolean value.
    _file_save_in_progress = false;
  });
}


} // namespace http_server
} // namespace rosetta
