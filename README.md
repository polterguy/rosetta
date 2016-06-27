Rosetta Web Server
==================

Rosetta is a small, portable, and simple web server, aiming at being able to make
anything with a network card, into a potential web server. You can run Rosetta in two
different modes, *"thread-pool"* and *"single-thread"*. The latter could probably
turn your wrist watch into a web server, if you tried. The former,
scales upwards, to supercomputers, with thousands of CPUs.

## Features

* SSL
* Multi-threading/thread-pool
* Persistent connections
* Timeouts
* Max connections per client
* Pipelining
* If-Modified-Since
* Upgrade-Insecure-Requests
* User-Agent whitelist
* Highly configurable

## The Paranoid web server

Rosetta is a highly paranoid web server. Among other things, it does not do any logging. You
can also configure it to only accept requests from specific User-Agents, such as the Tor
browser. It support SSL and Upgrade-Insecure-Requests, and can be configured to be both
a *"normal web server"*, in addition to being configured into *"paranoia land"*.

### No logging

Rosetta does not, have never, and will never, implement any type of logging of traffic.

The reasons for this, is because an adversary might gain control over your box. If he does,
then your log files will give away not only you, but also all users of your website. This
creates a dangerous situation, where the log files of your web server, can unintentionally
become you and your friends Nemesis and Judas.

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

### Built for ultra-secure platforms.

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

