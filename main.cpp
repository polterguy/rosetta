
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
#include <cstdlib>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include "common/include/sha1.hpp"
#include "common/include/base64.hpp"
#include "common/include/configuration.hpp"
#include "http_server/include/server.hpp"
#include "http_server/include/helpers/date.hpp"
#include "http_server/include/exceptions/argument_exception.hpp"

using std::endl;
using std::string;
using namespace rosetta::common;
using namespace rosetta::http_server;

// Name of default configuration file
char const * const DEFAULT_CONFIGURATION_FILE = "rosetta.config";

// Forward declaration of functions used in file
string get_configuration_file (int argc, char* argv[]);
void create_default_configuration_file();
void show_copyright_server_info (const configuration & config);


/// Main entry point for Rosetta web server
int main (int argc, char * argv[])
{
  // Making sure we trap everything that can go wrong, to give some sane feedback to
  // std::cerr if an exception occurs.
  try {

    // Parsing arguments, retrieving configuration file to use.
    string config_file = get_configuration_file (argc, argv);
    
    // Retrieving configuration object, and instantiating server.
    configuration config (config_file);

    // Checking if "users.dat" file exists, and if not, create a default user with username "Aladdin" and password "OpenSesame".
    if (!boost::filesystem::exists (".users")) {

      // Making sure we create our default user.
      ofstream users_file (".users");
      string username = "Aladdin";
      string password = "OpenSesame";
      string password_salt = password + config.get<string> ("server-salt");
      auto sha1_password = sha1::compute ( {password_salt.begin(), password_salt.end()} );
      string base64_sha1_encoded_password;
      base64::encode ( {sha1_password.begin(), sha1_password.end()}, base64_sha1_encoded_password);
      users_file << username + ":" + base64_sha1_encoded_password + ":root";
    }

    // Creating server object.
    server server_instance (config);

    // Showing copyright and server info to std out.
    show_copyright_server_info (config);

    // Starting server.
    // This method won't return before server is somehow stopped, or a severe exception,
    // that we do not know how to handle occurs.
    server_instance.run();
  } catch (const std::exception & err) {
    
    // Giving feedback to std error with exception message.
    std::cerr << "Unhandled exception; '" << err.what() << "'" << endl;
  }
}


/// Shows copyright and server info to std out.
void show_copyright_server_info (const configuration & config)
{
  // Showing copyright notice, providing callback to inject information about how to access website.
  configuration::serialize_copyright (std::cout, [config] (std::ostream & stream) {

    // Serializing information about how to view website, making sure we've accommodated for 80 characters per line.
    string s = "# Go to; 'http://localhost:"
             + config.get<string>("port")
             + "' to see your website.";

    // Making sure we've got 80 characters in string, padded by "#", before we serialize it to stream.
    while (s.size() < 79) { s += " "; }
    stream << s << "#" << endl;

    // Serializing boost version.
    s = "# Boost version; '" + string (BOOST_LIB_VERSION) + "'";
    while (s.size() < 79) { s += " "; }
    stream << s << "#" << endl;
  });
}


/// Parses arguments, and returns file path to configuration file used
/// Returns true if server should be started, otherwise false
string get_configuration_file (int argc, char * argv[])
{
  // Sanity check
  if (argc > 2) {
    
    // Oops, too many arguments
    throw argument_exception ("Supply only one argument, to configuration file");
  } else if (argc == 1) {
    
    // No arguments supplied, checking if default configuration file exists,
    // and if not, we create it
    if (!boost::filesystem::exists (DEFAULT_CONFIGURATION_FILE)) {

      // No arguments supplied, and no default configuration file does not exist,
      // creating default configuration file, before returning true to caller
      create_default_configuration_file();
    }

    // No arguments, default configuration is used
    return DEFAULT_CONFIGURATION_FILE;
  } else {
  
    // Caller supplied exactly *one* argument, now checking the type of argument he supplied
    string arg = argv[1];
      
    // Assuming this is a path to a configuration file
    // Checking that this is a valid path, to an existing configuration file
    if (boost::filesystem::exists (arg)) {
      
      // Configuration file supplied actually exists, returning this file to caller
      return arg;
    } else {
      
      // Oops, configuration file caller supplied does not exist
      throw argument_exception ("Configuration file '" + arg + "' does not exist!");
    }
  }
}


/// Creates default configuration file
void create_default_configuration_file()
{
  // Creating default configuration settings.
  configuration config;

  // Main server settings.
  config.set ("address", "localhost");
  config.set ("port", 8080);
  config.set ("ssl-port", 8081);
  config.set ("www-root", "www-root");
  config.set ("ssl-certificate", "server.crt");
  config.set ("ssl-private-key", "server.key");
  config.set ("user-agent-whitelist", "*");
  config.set ("user-agent-blacklist", "");
  config.set ("provide-server-info", false);
  config.set ("static-response-headers", "");
  config.set ("authenticate-over-non-ssl", false);
  config.set ("default-document", "index.html");
  config.set ("head-allowed", false);
  config.set ("trace-allowed", false);
  config.set ("options-allowed", true);

  // Request settings.
  config.set ("max-uri-length", 4096);
  config.set ("max-header-length", 8192);
  config.set ("max-header-count", 25);
  config.set ("max-request-content-length", 4194304); // 4 MB
  config.set ("max-post-request-content-length", 4096); // 4KB, post is only used for changing passwords and such
  config.set ("request-content-read-timeout", 300); // 5 minutes
  config.set ("request-post-content-read-timeout", 30); // 30 seconds
  config.set ("upgrade-insecure-requests", true);

  // Connection settings.
  config.set ("connection-ssl-handshake-timeout", 20);
  config.set ("connection-keep-alive-timeout", 20);
  config.set ("max-connections-per-client", 8);

  // Request Handlers, according to file extensions.
  config.set ("handler.html", "get-file-handler");
  config.set ("handler", "get-file-handler");
  config.set ("handler.js", "get-file-handler");
  config.set ("handler.css", "get-file-handler");
  config.set ("handler.png", "get-file-handler");
  config.set ("handler.gif", "get-file-handler");
  config.set ("handler.jpeg", "get-file-handler");
  config.set ("handler.jpg", "get-file-handler");
  config.set ("handler.ico", "get-file-handler");
  config.set ("handler.xml", "get-file-handler");
  config.set ("handler.zip", "get-file-handler");
  config.set ("handler.json", "get-file-handler");

  // Common MIME types.
  config.set ("mime.html", "text/html; charset=utf-8");
  config.set ("mime", "text/html; charset=utf-8");
  config.set ("mime.css", "text/css; charset=utf-8");
  config.set ("mime.js", "application/javascript; charset=utf-8");
  config.set ("mime.json", "application/json; charset=utf-8");
  config.set ("mime.png", "image/png");
  config.set ("mime.jpg", "image/jpeg");
  config.set ("mime.jpeg", "image/jpeg");
  config.set ("mime.ico", "image/x-icon");
  config.set ("mime.bz", "application/x-bzip");
  config.set ("mime.zip", "application/zip");
  config.set ("mime.xml", "application/rss+xml");
  
  // Creating a server salt. The salt doesn't need to be "cryptographically secure", only "pseudo random".
  // Hence, we create it from a couple of random bytes from rand(), and append the "now date", before we sha1 hash it, and create a base64 encoded
  // value from it, using this base64 encoded string as our "server salt".
  srand (time (0));
  string salt;
  salt.push_back ((rand() % 26) + 'a');
  salt.push_back ((rand() % 26) + 'a');
  salt.push_back ((rand() % 26) + 'a');
  salt.push_back ((rand() % 26) + 'a');
  salt.push_back ((rand() % 26) + 'a');
  salt.push_back ((rand() % 26) + 'a');
  salt.push_back ((rand() % 26) + 'a');
  salt += date::now().to_iso_string ();
  auto sha1_value = sha1::compute ( {salt.begin(), salt.end()} );
  string sha1_base64_encode_salt;
  base64::encode ( {sha1_value.begin(), sha1_value.end()}, sha1_base64_encode_salt);
  config.set ("server-salt", sha1_base64_encode_salt);
  
  // Saving configuration to file
  config.save (DEFAULT_CONFIGURATION_FILE);
}

