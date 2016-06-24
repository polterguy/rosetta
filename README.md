Rosetta Web Server
==================

Rosetta Web Server, reclaim the Web.

## Getting started with Rosetta

Coming soon

## Features

Rosetta is a web server built for small devices. This means it is highly
optimized and small in size. It does not feature *"all the bells and whistles
of Apache"*, because it has a completely different purpose than a big web
server, such as Apache.

Rosetta is highly portable, written in optimized C++, and utilizes boost ASIO.

Its intention is to be a web server for small devices, with network constraints,
and less amount of CPU and/or memory, than what you'd expect from a *"fully fledged
mountain hall web server installation"*. It is also created to be as easy to
use as possible, and will among other things automatically create a default
configuration file, the first time you use it.

It has no dependencies when built, and should easily start simply by executing
the main process image, automatically creating and reading its own configuration
file.

It features two different thread models, *"single-thread"* and *"thread-pool"*.

*"single-thread"* means it runs in a single thread, which might be useful for
extremely small devices, with small amounts of traffic.

*"thread-pool"* means it runs on a pool of threads, which you can configure
yourself, and that every single connection to the server, runs in its own separate
thread, taken from this thread pool.

### HTTP/1.1

Rosetta aims at supporting the ghist of HTTP/1.1. Some parts are still under
development, but the bricks that should for the most parts already be in place,
are mentioned below.

* Keep-Alive connections.
* Pipelining of requests.
* If-Modified-Since, making it possible to return a 304 to client, if the file
  has not been modified since the specified date.

### No Logging

Rosetta does not, have never, and will never, implement any type of logging
of traffic.

The reasons for this, is because an adversary might gain control over your
box. If he does, then your log files will give away not only you, but also
any users of your website. This creates a dangerous situation, where the
log files of a web server, can unintentionally become your enemy and nemesis.

One benefit from this fact, is that logging is actually quite expensive for
a server, since it requires locking access to the log files, while doing the
logging. This requires expensive locks and/or mutex objects, to write to
the log files, that are indeed shared resources from the web server's point
of view. This means that by turning OFF logging, Rosetta should perform
a lot faster, than if it had logging enabled.

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

