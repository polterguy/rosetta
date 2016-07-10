Rosetta Web Server
==================

Rosetta is a small, portable, and simple web server.

## Features

* SSL
* GET/PUT/DELETE/POST support, on files and folders
* Basic Authentication
* Persistent connections
* Timeouts
* Max connections per client
* IP version 4 and 6
* Pipelining
* If-Modified-Since
* Upgrade-Insecure-Requests
* User-Agent whitelist and blacklist
* No logging
* Highly configurable
* Paranoid by default
* Ultra-fast
* Ultra-small

## Getting started with Rosetta

Coming soon

## Features

Rosetta is a tiny web server, created to do one thing well, which is supporting REST HTTP.
This allows you to use your server almost the same way you would use a database. Using GET,
DELETE and PUT to retrieve, create, modify and delete files. In addition, you can retrieve
the content of folders, as JSON, by adding ?list as an HTTP parameter to your GET requests.

Combined with WWW-Authenticate support, this allows you to securely access your files, any
way you see fit, treating your server almost like a *"database"*, having all your logic in
JavaScript, and/or other types of clients.

### Single threaded, yet still asynchronous

Since Rosetta is created to run on extreme hardware constraints, it does not feature any
multi threading support. It is still able to server multiple requests, concurrently though,
thanks to its asynchronous architecture.

This means that it will use much less power, and can run on platforms, that are not normally
thought to be able to run web servers.

## HTTP REST support

Rosetta is actually exclusively built around the HTTP GET/PUT/POST/DELETE verbs, and does
not have any integration towards any server side programming languages at all. This makes
it first of all much safer and secure, since it reduces the attack surface. In addition, it
creates a situation, where your server becomes logically, more similar to a *"database"*,
than a traditional web server.

Your file system, on your server, basically becomes your database, and Rosetta allows you
to query into your server, using GET, PUT and DELETE requests. POST is only for modifying
your user objects, and your authorization objects, and nothing else.

### Common for all HTTP verbs

To make sure you retrieve the authenticated version of a resource, append "?authenticate"
to your requests. This will ensure that your client receives a 307 respond, forcing the client
to authenticate (log in), using Basic authentication, unless it is already authenticated.

### GET verb

With a GET request, you can either retrieve a single file, or the contents of a folder. To
retrieve a folder's content, simply add the parameter "?list", and make sure what you try
to request, is a folder, by appending a "/" at the end of your URL.

To get a file, you could create a request resembling something like this;

´´´
GET /foo.html HTTP/1.1
´´´

To get a folder, you could do something like this;

´´´html
GET /bar/?list HTTP/1.1
´´

### PUT verb

With a PUT request, you can either create a single file, by adding content to your request,
or you can create a folder, at which point you must make sure your URL ends with a slash "/".

### DELETE verb

A DELETE request, can either delete a folder, or a single file. Make sure you append a "/"
at the end of your URL, if you wish to delete a folder.

### POST verb

Since Rosetta features no server side programming language, but is a simple REST web server,
the only usage for a POST request, is to either modify your authentication object, by POSTing
to the URL "/.users". If you do, you must be logged in as a *"root"* user. Any user can still
POST to this file, to change their passwords. But only a root account can do anything else.

Or, you can modify the authorization objects, which are ASCII files in your server's directories,
called ".auth". This operation is only legal for a *"root"* account.

## The Paranoid web server

Rosetta is a highly paranoid web server. Among other things, it does not do any logging. You
can also configure it to only accept requests from specific User-Agents, such as the Tor
browser. It support SSL and Upgrade-Insecure-Requests, and can be configured to be both
a *"normal web server"*, in addition to being configured into *"paranoia land"*.

### Sha1 protected passwords

Rosetta supports Basic Authentication, which allows a client to authenticate itself through
use of the WWW-Authenticate and Authorization HTTP headers. On the server side, the passwords
used to authenticate the client, is stored as salted Sha1 hashed values.

This means, that even if an adversary gains physical access to the box, or some how manages
to hack your server, and retrieve the user data, with usernames and passwords, then he can
still not retrieve the passwords of your users.

The reason why this is important, is because often users will use the same passwords, for
multiple different sites and/or servers. Although this is **not recommended**, many users
still do this, to make it easier to remember their passwords.

During initial startup of your server, then Rosetta will create a *"server salt"*, which
is based upon the current time, and add some pseudo-random bytes to this mix. Then it will
create a Sha1 value out of this string, before it bas64 encodes it. This string is then
appended to your passwords, both during creation of new users, and when a username/password
is sent from the client, to authenticate the client. Then this combined string will create
a Sha1 value, which is then base64 encoded. This base64 encoded resulting value, is then
compared towards your values in your password file, to check for a match for a username/
password combination.

This makes it mathematically and theoretically almost impossible to retrieve the actual
password used to actually log into your box. Even for an adversary with access to this
salted Sha1 string.

### WWW-Authenticate

Rosetta allows you to show different content to users, according to whether or not they
are authenticated with a username and password or not. This is a simple role based system,
where a user has a username and password combination, which ties him to a *"role"*.

All access to files and folders on your server, is tied up towards this role system, which
means you can setup a server, such that you have folders that are only possible to
access for users belonging to some specific role.

This authorization system, ties together with the different HTTP verbs Rosetta recognizes,
such that one role can have PUT, DELETE and GET access, while another role only has GET
access, etc.

The default role access for your *"www-root"* folder for instance, is that only a "root"
user can DELETE and PUT files into it, while "*" (all) visitors can view its content.

The access rights for a specific folder can be found in the hidden files inside a folder
which is called *".auth.dat"*.

The users for your server, can be found at the root folder of your server installation,
in a file called *".users.dat"* by default. None of these files are served by Rosetta,
and only used as an internal *"database"* for authorizing access to other resources.

To retrieve the public content of a folder, you can simply create a request, with the URI
of e.g. http://my-server.com/foo/ (Notice, you need to finish a folder request with a "/")

The above will retrieve the public content of a folder. If you add the parameter ?authorize
to the request, then it will force the client to authenticate the client, with a username
and password combination, before listing the files and folders in that folder. Which might
result in a different result, if there are sub-folders beneath the "foo" folder, that only
some users have access to.

Requesting a file inside a folder, which has restricted GET access, is not possible unless
the client authenticates itself, with a username/password combination, by using the
"Authorization" HTTP header, to pass in its basic Authentication username/password combination.

Authorization access rights are inherited on folders. This means that if you do not explicitly
override a folder's authorization rights, then the rights of the parent folder will be
inherited to its child folders.

### Authentication turned OFF on non-SSL traffic by default

The WWW-Authenticate header is never transmitted from the server unless the connection is
secure, by default.

This makes it impossible for a client to accidentally submit his username/password
combination over an insecure connection, which would allow an adversary man in the middle
to pick up his username/password combination.

### No logging

Rosetta does not, have never, and will never, implement any type of logging of traffic.
Except possibly for malicious requests, and errors, resulting from an attempted attack,
or compromised client.

The reasons for this, is because an adversary might gain control over your server. If he does,
then your log files will give away not only you, but also all users of your website. This
creates a dangerous situation, where the log files of your web server, can unintentionally
become your and your friends enemy.

This feature, combined with SSL, makes it impossible for an adversary to know which
documents, from your server, your users have accessed. Significantly reducing your and
your friends risk for using the web server.

### User-Agent whitelist and blacklist

Rosetta can be configured to only accept requests from a specific range of user agents.
This allows you to for instance only accept requests from visitors using a specific user
agent, which prevents users from visiting your website, if they are using a user agent,
that is insecure for some reasons. For instance, this feature, allows you to force your
users to upgrade insecure browsers, to help them protect themselves against themselves.

This feature can be configured through the *"user-agent-whitelist"* and the *"user-agent-
blacklist"* configuration properties. It works by creating a list of pipe separated (|)
strings, which if the visiting user agent, matches at least one of, then the user agent
becomes either blacklisted (refused) or whitelisted (accepted).

Visiting user-agents must not obey by the user agent rules, which means that this
is only for preventing users, accidentally visiting your website, with a user agent
that is not trusted. However, for the ultra paranoid among us, this is an extra
feature, protecting the privacy, of not only you, and your data, but also your visitors.

These features are turned **off** by default, but can easily be turned on, by changing
the *"user-agent-whitelist"* and/or *"user-agent-blacklist"* properties in your
configuration file, to for instance;

user-agent-whitelist = Mac|Linux
user-agent-blacklist = Chrome

The above configuration would enable only Mac OS X clients and Linux clients, running
any browsers, except Google Chrome to access your website.

### Turn OFF non-SSL traffic

With Rosetta, you can also completely turn of non-SSL traffic, by setting the *"port"*
property, in your configuration file, to **-1**. This prevents all traffic to your
web server, that is not secured through SSL.

### Upgrade-Insecure-Requests

Although it is not a part of the standard yet, Rosetta support automatic upgrading of
insecure requests, for user agents that supports this feature. This means, that if a user
accidentally visits your website, without prepending the request with *"https"*, the
request will be redirected to a secure connection automatically, and no data will be
served over a non-secure channel.

This feature is on by default, but can be modified through the configuration property
called *"upgrade-insecure-requests"*. 0 means "off", 1 means "on".

### Non-identifiable

Rosetta will not identify itself by default. This means that a malicious hacker, will
not be able to see which web server he or she is visiting. If you choose to turn this
feature off, by changing the configuration property *"provide-server-info"* into "1",
then Rosetta will still not provide any version information back to the client.

This makes Rosetta harder to hack, since it provides less information back to a malicious
hacker, about which type of server he or she is encountering.

### Toxic header and envelope injection protection

Rosetta will not accept an HTTP request that tries to inject malicious code through the
HTTP headers or envelope in general. This is implemented by making sure Rosetta only
accepts a sub-range of the plain ASCII set as legal characters in the HTTP envelope.

This helps protect against an adversary that tries to inject malicious code through the
request envelope, by for instance create an HTTP header that contains executable code,
as its name and/or value.

### Buffer and stack overflow protection

Rosetta will not accept more than 25 HTTP headers from a client by default, and each
header cannot be larger than 8192 bytes in total. This helps protect against *"buffer
and stack overflow attacks"*, which would otherwise allow a malicious client to inject
malicious code into the server, by overflowing the stack and/or buffers for reading
the HTTP envelope.

As an additional protection measure in this regard, Rosetta will also reject a request
that has more than 4096 bytes in its initial HTTP-Request line.

### No negotiation with malicious clients

If Rosetta discovers what it thinks is a *"malicious request"*, then it will immediately
close the TCP connection, and not return anything at all to the client.

This helps protect against a malicious client, acquiring meta data information about your
server, such as a 404 instead of a 401 response, verifying the existence of a path, etc.

### No meta data retrieval for restricted content

If a client tries to access a resource, then Rosetta will first check if client is
authorized to access the resource, before it checks for its existence. This makes
it impossible for a client to gain knowledge about a document's existence, and such
gather meta data information about your server, by creating a huge number of requests,
trying to traverse your server, for *"random document/folder names"*, to see if it
returns a 404 (Not-Found) or a 401 (Not-Authorized)

For instance, if you have a folder in your system that requires that the client belongs
to the *"root"* role, then regardless of what the client tries to access within this folder,
Rosetta will return a 401 (Not-Authorized) response, and never a 404 (Not-Found) response.

### Built for ultra-secure platforms

Rosetta is built in C++, using boost ASIO, and is ultra-portable. This means you can run
Rosetta on top of the best and most secure platforms, using proven technology, with a low
risk. Rosetta have few dependencies, which makes its attack surface significantly smaller
than other server systems.

Rosetta is also built on top of the existing HTTP standard, and other web technologies,
which have been *"debugged by the world"* for several decades, translating to a highly
scalable and secure platform, for delivering content securely, to a multitude of different
devices.

Is there's a bug in PHP? No problem, Rosetta have no PHP bindings. Is there a bug in CGI?
No problem, Rosetta have no CGI bindings. Is there a bug in database *"x"*? No problem,
Rosetta have no bindings to it! Guaranteed!

Rosetta is actually condensed, a simple file system, allowing you to modify the files on
your server, through normal HTTP(S) traffic. This makes its *"attack surface"* much
smaller than a big web server, such as for instance Apache. It also makes Rosetta
ultra-fast, eliminating the needs for additional cache software, sitting in front of
Rosetta, serving cached content to clients.

### Paranoid by default

Rosetta by default, unless configured otherwise, starts out as paranoid, in ultra-secure
mode, by turning on and off all the switches, such that it starts out in a highly secure
state, even before you start configuring it.

This means that you don't have to study its documentation, or spend a lot of time
making it secure, since by default, it already is secure when you start using it.

The only thing you need to do, to make sure it becomes bullet proof in fact, is to put
an SSL certificate and private key into the root folder of your server, with the name of
*"server.crt"* and *"server.key"*.

By default, Rosetta accepts insecure connections, but will attempt to upgrade automatically
to secure connections, if the client supports this feature. It uses port 80 for normal HTTP
traffic, and port 443 for SSL traffic. It does not accept HEAD and TRACE requests, and is
configured to run in *"thread-pool"* mode, with 128 threads. It has sane values for timeouts,
preventing malicious and/or badly written clients to lock up resources on your server, by
creating requests that never finishes.

## License

Rosetta web server, copyright(c) 2016, Thomas Hansen, phosphorusfive@gmail.com.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License, as published by
the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

