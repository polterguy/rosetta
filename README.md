Rosetta Web Server
==================

Rosetta is a small, portable, and simple web server, aiming at being able to make
anything with a network card, into a potential web server. You can run Rosetta in two
different modes, *"thread-pool"* and *"single-thread"*. The latter could probably
turn your wrist watch into a web server, if you tried a little bit. The former,
scales upwards, to supercomputers, with thousands of CPUs, if you want it to.

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

### User-Agent whitelist

Rosetta can be configured to only accept requests from a specific range of user agents.
This allows you to for instance only accept requests from visitors using the Tor browser,
which prevents users from visiting your website, unless they're within the safety of
the Onion protocol, or using a user agent, that is insecure for other reasons. For
instance, this feature, allows you to force your users to upgrade insecure browsers,
to help them protect themselves against themselves.

This feature can be configured through the *"user-agents-whitelist"* configuration
property. It works by creating a list of pipe separated (|) strings, which of the
visiting user agent, must match at least one of, before the request is accepted.
If it can not find a match, then the request is refused with a *"403 Forbidden"*
response, without any additional information provided to the client.

Visiting user-agents must not obey by the user agent rules, which means that this
is only for preventing users, accidentally visiting your website, with a user agent
that is not trusted. However, for the ultra paranoid among us, this is an extra
feature, protecting the privacy, of not only you, and your data, but also your visitors.
This way, the web server, denies to become the Judas and Nemesis of not only you, but
also your visitors and users.

This is turned **off** by default, but can easily be turned on by changing the
*"user-agents-whitelist"* property in your configuration file, to for instance
**Chrome|Linux**, to only accept requests from Google Chrome, or some sort of Linux
installation.

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

### Configurable HTTP forgiveness mode

Rosetta is highly forgiving in regards to the HTTP standard by default. Among other
things, it will accept a missing HTTP-Request version information, by default. In
addition, it will Auto-Capitalize HTTP headers, if they are submitted in lowercase
letters. In such a regard, Rosetta is a perfect noob web server, since it will try
to interpret an HTTP request, even when the client is not able to create it 100%
accurately according to the strictness of the HTTP standard.

If we stopped there though, then a malicious hacker could easily do a *"feature detection"*
on the server, to identify which type of server he was encountering. You can therefor
configure Rosetta, in 5 different modes, which defines how forgiving it will be, in
regards to the HTTP standard. The modes are created such that a 3 value, forgives
everything a 2 and a 1 value forgives. 5 forgives everything 4 and 3 forgives, and so
on. So it is incremental in its amount of forgiveness. The different modes, and their
features are the following;

1. Forgive nothing!
   * Choke on missing CR in CR/LF sequence.
   * Choke on additional CR inside of HTTP envelope lines.
   * Choke on missing "/" in front of URL.
   * Choke on missing HTTP-Version information.
   * Choke if version is neither HTTP/1.1 nor HTTP/2
2. Forgive a missing "/" in front of the URL, and forgive a missing CR in envelope lines.
   * In addition, ignore all additional CR characters in all HTTP headers.
3. Automatically Capitalize HTTP Header-Names, if they are submitted WRONG or wRonG.
   * Trim header names before storing them internally.
   * Right trim values of HTTP headers, in addition to left trimming the value.
4. Forgive, and ignore, an HTTP header, with no value (missing ":")
   * Also, ignore additional SP " " in HTTP-Request line.
5. Forgive a missing HTTP-Version part in HTTP-Request line, and default it to "HTTP/1.1".
   * Accept "foo-bar" as HTTP version information if given, without choking.

If you set Rosetta into an *"http-forgiveness-mode"* of 5, it will basically try to
interpret anything, and understand almost whatever you throw at it of "bugs". If you
set the value to "1", it will be extremely strict in regards to its interpretation
of the HTTP standard. This allows you to make the web server behave in different ways,
according to your configuration, making it more difficult to run a *"feature detection"*
attack on your server, to identify it, as a part of a malicious hacking attempt.

For the paranoid among us, the best value for *"http-forgiveness-mode"* is probably 1,
since it gives away no details about the server. For the naive among us, trying to
learn the HTTP standard, setting up their first web server, a value of 5 is probably
easiest to start with.

The default value is 5.

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

