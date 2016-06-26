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
* Configurable.

## No logging

Rosetta does not, have never, and will never, implement any type of logging of traffic.

The reasons for this, is because an adversary might gain control over your box. If he does,
then your log files will give away not only you, but also any users of your website. This
creates a dangerous situation, where the log files of a web server, can unintentionally
become your nemesis and Judas.

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

