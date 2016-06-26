Rosetta Web Server
==================

Rosetta is a small, portable, and simple web server, aiming at being able to make
anything with a network card, into a potential web server. You can run Rosetta in two
different modes, *"thread-pool"* and *"single-thread"*. The latter could probably
turn your wrist watch into a web server, if you tried a little bit. The former,
scales upwards, to supercomputers, with thousands of CPUs, if you want it to.

## Features

* SSL.
* Thread pooling.
* Single thread.
* Persistent connections.
* Timeouts.
* Max connections per client.
* Pipelining.
* If-Modified-Since.
* Upgrade-Insecure-Requests
* GET, HEAD and TRACE support.
* User-Agent whitelist
* Configurable.

## The Paranoid web server

Rosetta is a highly paranoid web server. Among other things, it does not do any logging. You
can also configure it to only accept requests from specific User-Agents, such as the Tor
browser. It support SSL and Upgrade-Insecure-Requests, and can be configured to be both
a *"normal web server"*, in addition to being configured into *"paranoia land"*.

### No logging

Rosetta does not, have never, and will never, implement any type of logging of traffic.

The reasons for this, is because an adversary might gain control over your box. If he does,
then your log files will give away not only you, but also any users of your website. This
creates a dangerous situation, where the log files of a web server, can unintentionally
become your nemesis and Judas.

### User-Agent whitelist

Rosetta can be configured to only accept requests from a specific range of user agents.
This allows you to for instance only accept requests from visitors using the Tor browser,
which prevents users from visiting your website, unless they're within the safety of
the Onion protocol.

This feature can be configured through the *"user-agents-whitelist"* configuration
property. It works by creating a list of pipe separated (|) strings, which of the
visiting user agent, must match at least one of, before the request is accepted.
If it can not find a match, then the request is refused with a *"403 Forbidden"*
response.

Visiting user-agents must not obey by the user agent rules, which means that this
is only for preventing users, accidentally visiting your website, with a user agent
that is not trusted.

This is turned off by default, but can be turned on by changing the *"user-agents-whitelist"*
property in your configuration file, to for instance *"Chrome|FireFox"* to only accept
requests from Google Chrome or FireFox.

### Upgrade-Insecure-Requests

Although it is not a part of the standard yet, Rosetta support automatic upgrading of
insecure requests, for user agents that supports this feature. This means, that if a user
accidentally visits your website, without prepending the request with *"https"*, the
request will be redirected to a secure connection automatically, and no data will be
served over a non-secure channel.

This feature is on by default, but can be modified through the configuration property
called *"upgrade-insecure-requests"*. 0 means "off", 1 means "on".

## Future plans

Implement support for all HTTP verbs, PUT, DELETE, POST and OPTIONS, in addition to Basic
Authentication. A custom verb is also being considered; LIST. The idea, is to allow you
to control your files, on your server, through standardized HTTP verbs, turning your
box, into a *"publishing box"* or a *"content management system"*, based upon the
standard verbs from HTTP, allowing you to publish new content, and manage your content,
from anything you wish.

## Getting started with Rosetta

Coming soon

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

