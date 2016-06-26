
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
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include "common/include/configuration.hpp"
#include "server/include/server.hpp"
#include "server/include/exceptions/argument_exception.hpp"

using std::endl;
using std::string;
using namespace rosetta::common;
using namespace rosetta::server;

// Name of default configuration file
char const * const DEFAULT_CONFIGURATION_FILE = "rosetta.config";

// Forward declaration of functions used in file
string get_configuration_file (int argc, char* argv[]);
void create_default_configuration_file();
void show_copyright_server_info (const configuration & config);


/// Main entry point for Rosetta web server
int main (int argc, char* argv[])
{
  // Making sure we trap everything that can go wrong, to give some sane feedback to
  // std::cerr if an exception occurs.
  try {

    // Parsing arguments, retrieving configuration file to use.
    string config_file = get_configuration_file (argc, argv);
    
    // Retrieving configuration object, and instantiating server.
    configuration config (config_file);
    server_ptr server (server::create (config));

    // Showing copyright and server info to std out.
    show_copyright_server_info (config);
    
    // Starting server.
    // This method won't return before server is somehow stopped, or a severe exception,
    // that we do not know how to handle occurs.
    server->run();
  } catch (const std::exception & err) {
    
    // Giving feedback to std error with exception message.
    std::cerr << "Unhandled Rosetta exception; '" << err.what() << "'" << endl;
  }
}


/// Shows copyright and server info to std out.
void show_copyright_server_info (const configuration & config)
{
  // Showing copyright notice, providing callback to inject information about how to access website.
  configuration::serialize_copyright (std::cout, [config] (std::ostream & stream) {
    
    // Serializing information about how to view website, making sure we've accommodated for 80 characters per line.
    string s = "# Open your browser and go to; 'http://localhost:"
             + config.get<string>("port")
             + "' to see your website.";
      
    // Making sure we've got 80 characters in string, padded by "#", before we serialize it to stream.
    while (s.size() < 79) { s += " "; }
    stream << s << "#" << endl;
    
    // Serializing thread model.
    string thread_model = config.get<string> ("thread-model");
    s = "# Thread model is; '" + thread_model + "'";
    while (s.size() < 79) { s += " "; }
    stream << s << "#" << endl;
    
    // If thread-model is "multi-thread", we serialize the number of threads for server.
    if (thread_model == "multi-thread") {
    
      // Serializing number of threads.
      s = "# Thread pool size; " + boost::lexical_cast<string> (config.get<size_t> ("threads"));
      while (s.size() < 79) { s += " "; }
      stream << s << "#" << endl;
    }
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
  config.set ("thread-model", "thread-pool");
  config.set ("threads", 128);
  config.set ("www-root", "www-root/");
  config.set ("default-page", "/index.html");
  config.set ("trace-allowed", false);
  config.set ("head-allowed", false);
  config.set ("ssl-certificate", "rosetta.crt");
  config.set ("ssl-private-key", "rosetta.key");
  config.set ("user-agents-whitelist", "*");
  config.set ("provide-server-info", false);
  config.set ("static-response-headers", "");

  // Request settings.
  config.set ("max-uri-length", 2048);
  config.set ("max-header-length", 8192);
  config.set ("max-request-content-length", 4194304); // 4 MB
  config.set ("request-content-read-timeout", 300); // 5 minutes
  config.set ("upgrade-insecure-requests", false);

  // Connection settings.
  config.set ("connection-ssl-handshake-timeout", 20);
  config.set ("connection-keep-alive-timeout", 20);
  config.set ("max-connections-per-client", 2);

  // Request Handlers, according to file extensions.
  config.set ("html-handler", "static-file-handler");
  config.set ("js-handler", "static-file-handler");
  config.set ("css-handler", "static-file-handler");
  config.set ("png-handler", "static-file-handler");
  config.set ("gif-handler", "static-file-handler");
  config.set ("jpeg-handler", "static-file-handler");
  config.set ("jpg-handler", "static-file-handler");
  config.set ("ico-handler", "static-file-handler");
  config.set ("xml-handler", "static-file-handler");
  config.set ("zip-handler", "static-file-handler");
  config.set ("json-handler", "static-file-handler");
  config.set ("default-handler", "append-html-static-file-handler");

  // Common MIME types.
  config.set ("mime-html", "text/html; charset=utf-8");
  config.set ("mime-css", "text/css; charset=utf-8");
  config.set ("mime-js", "application/javascript; charset=utf-8");
  config.set ("mime-json", "application/json; charset=utf-8");
  config.set ("mime-png", "image/png");
  config.set ("mime-jpg", "image/jpeg");
  config.set ("mime-jpeg", "image/jpeg");
  config.set ("mime-ico", "image/x-icon");
  config.set ("mime-bz", "application/x-bzip");
  config.set ("mime-zip", "application/zip");
  config.set ("mime-xml", "application/rss+xml");

  // Saving configuration to file
  config.save (DEFAULT_CONFIGURATION_FILE);
}

