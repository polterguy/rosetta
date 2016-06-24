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

It has no dependencies when built, and should easily start, simply by executing
the main process image, automatically creating its own configuration file.

Rosetta is also highly *"forgiving"* in regards to the HTTP standard. This means
that whenever it can, it will try to interpret what the client is actually
trying to communicate, even when the client is not able to do this, according
to the strictness of the HTTP standard. For instance, it ignores CR when parsing
an HTTP envelope, with the HTTP-Request line, and its headers. This means that
sending a LF is enough to make Rosetta understand where the line breaks.
Another example is that it will *"auto correct"* both the HTTP headers and
the HTTP-Request line, capitalize the things that should be capitalized, and
still understand things such as *"get"*, instead of *"GET"*, and *"connection"*,
instead of *"Connection"*.

This makes it a perfect *"first web server"*, since it allows you to more
easily get started, without knowing all the intrinsic parts of the HTTP
standard.

### Thread models

Rosetta features two different thread models, *"single-thread"* and *"thread-pool"*.

*"single-thread"* means it runs in a single thread, which might be useful for
extremely small devices, with small amounts of traffic. *"thread-pool"* means
it runs on a pool of threads, which you can configure yourself, and that every
single connection to the server, runs in its own separate thread, taken from
this thread pool. A third thread model is planned, which will create one thread,
and one io_service, for each processor on your device.

This allows you to experiment with whatever thread model gives you the best results,
according to which device you run your web server on.

### HTTP/1.1

Rosetta aims at supporting the ghist of HTTP/1.1. Some parts are still under
development, but the bricks that should for the most parts already be in place,
are mentioned below.

* Keep-Alive connections.
* Timeouts, both on request envelope, request content, and keep-alive connections.
* Pipelining of requests.
* If-Modified-Since. The server will return a 304 response to the client without
  content, if a static file is requested, and it has not been modified since the
  requested If-Modified-Since date.
* Max connections. You can choose to refuse a connection from a client, if
  the client already has reached the *"max-connections-per-client"* limit, to
  make sure a single erroneous or malicious client, does not exhaust your server.
* Intelligent 4xx and 5xx error responses.
* Decide which content to serve, and how to serve it, through your configuration file.
* Configurable. Rosetta will create its own default configuration file the first time
  you run it, but this file can be modified by you afterwards.

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

