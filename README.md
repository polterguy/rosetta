Rosetta Web Server
===============

Rosetta Web Server, reclaiming the web.

## Getting started with Rosetta

Coming soon

## No Logging

Rosetta does not, have never, and will never, implement any type of logging
of traffic, in any layers. Application developers are also highly discouraged
to create any sort of logging, besides simple anonymous hit counters.

The reasons for this, is because an adversary might gain control over your
box. If he does, then your log files will give away not only you, but also
any users of your services. This creates a dangerous situation, where the
log files of a web server, can unintentionally snitch, on an entire network
of users.

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

