Rosetta Web Server
===============

Rosetta Web Server is a small web server.

## Getting started with Rosetta

Coming soon

## HTTP Cloaking

Rosetta supports something called **HTTP cloaking**. This simply means that
it can *"hide"* an HTTP request, in something that wouldn't normally look
like an HTTP request, for an adversary. The way it does this, is by being able
to accept HTTP envelopes that other server wouldn't normally even recognize
as HTTP traffic at all.

For instance, the HTTP envelope parser accepts any characters between [0-11>
as a LF, any character between [11-21> as CR and any character between
[21-33> as SP. In addition it ignores any characters between 190 and 255,
and bit shifts down one position any character above 127, before it interprets
what characters it is being sent in the HTTP envelope.

In addition, a GET request can be identified as either "GET", "get", "gEt",
"xqwG" or "-.,g". It simply looks for the first occurrence of "g", "h", "p",
"u", "d", case insensitive, and determines what type of request it is receiving,
according to which letter it found first, in the initial HTTP-Request line.

G meaning GET, H means HEAD, P means POST, U means PUT and D means DELETE.

In addition, if the HTTP-Version part of the initial HTTP-Request line,
contains the numbers [2 9], then it defaults the interpretation of the
version to use as HTTP/2. Otherwise, it default the version to HTTP/1.1.

Both the interpretation of the HTTP method, version, and URI, is done after
the original bitshift/byte change has been performed.

On top of this, a perfectly valid Rosetta HTTP request, does not need to
contain anything, besides the URL in the HTTP-Request line, since both
the method, and the version, are optional fields. If missing, Rosetta will
default the request method to GET, and the HTTP version to HTTP/1.1.

And in fact, the absolutely simplest possible HTTP GET request, does not
need to contain anything but CR/LF, which again might be interchanged with
any of the characters between [0-11> / [11-21>, which would be interpreted
as a GET request, for the default page, whatever that is in your system.

This means you could request the default HTML page in a Rosetta server, with
for instance 0x05 and 0xF5, for instance, which would be interpreted as
a CR/LF sequence by the HTTP envelope parser. Or 0x00 and 0x0b, etc.

In fact, even without considering adding "garbage characters", which are
characters from 190 and up to 255, there exists 10x10 different ways
of creating a simple CR/LF sequence in Rosetta. Once you start adding
garbage characters to the stream, to further increase the strength of your
cloaking, then the amount of possible ways to create a valid CR/LF sequence
to a Rosetta server, literally becomes infinite! This is because you can
put an infinite amount of characters between your CR and LF, that are in
the range of [190-256>

Any HTTP header fields, use the first character not in the range of
[a-z, A-Z, 0-9 or '-'] as the delimiter between the HTTP header name, and
its value. In addition, when parsing HTTP header fields, the name of them
is turned into lowercase, and stored, and referred to, internally in Rosetta,
as lowercase. In addition, the headers that the server itself is parsing,
is also turned into lowercases, before the server checks their values.
And any SP characters are trimmed away. This process is done after the
initial bit-shifting process, which means that a perfectly valid HTTP
header can look something like this for instance;

  coNneCtion &   keep-alive

Which of course means;

Connection:keep-alive

In addition, the header, name and value, can contain any ignored bytes, in
the range of [190-256>, at any position, to create "garbage character", making
the header field, more difficult to deduct. And the CR/LF sequence, can be
any characters between [0-11> && [11-21>. And SP here means any character in
the range of [21-33>.

The URI can use any of these letters as delimiter between the actual URI,
and the HTTP GET parameters [?*$~^€§]. Again, this is done after the initial
bit-shifting process, and discarding of invalid characters, etc.

In addition, a Rosetta box, does not store any logs, what so ever, does
not identify itself in any ways, and tries to keep the amount of HTTP
headers it needs for a valid HTTP request to a minimum. For instance,
Rosetta does not require you to send the *"Host"* HTTP header, since
by default, it simply swallows everything, and accepts everything.

Rosetta also does not implement by default the TRACE, CONNECT or OPTIONS
methods.

It also keeps the amount of headers it returns, back to the client,
to a minimum. This is so to reduce the "knowns what to look for", for
any adversary, trying to attack the server, and understand the messages
sent.

## Why Cloak?

To understand the reasoning for this, one must understand how a brute force
decryption attack on an HTTP message works. However, to explain it simple;
It is highly likely mathematically, at least 10 orders of magnitude more
CPU intensive, to brute force decrypt an HTTP message, if the adversary
does not know what it is looking for. In addition, the very act of brute
force decrypting an HTTP message that has been *"cloaked"*, would highly likely
create many more false positives, than a non-cloaked HTTP message. This
means that the CPU powers necessary to brute force decrypt a severely cloaked
HTTP message, is forced to run the same algorithm for each time it thinks it
finds a match, as the Rosetta web server must do to de-cloak the messages
it receives.

For Rosetta, doing this once, for each message, is quite cheap in regards to
CPU and other resources. For a brute force decryption attack, that needs to
do this possibly trillions of times, for each candidate for a match it finds,
this makes the task of brute force decryption becomes possibly several orders
of magnitudes more expensive in regards to processor power.

In addition, knowing for certainty, if it has a positively de-cloaked message,
or simply a false positive, would require analyzing the actual message,
which again might be impossible, if further decryption layers have been added,
on top of the normal SSL encryption.

Notice please though, that HTTP cloaking is an **addition** to encrypting
your HTTP traffic, and not a "substitute". In itself, without encryption,
it is close to pointless. But combined with traditional encryption, such as SSL
and/or OpenPGP, it makes it mathematically, several orders of magnitudes more
difficult to retrieve the HTTP request that was originally sent from the client.
This is so since an adversary would not even know what to look for, when doing
a brute force decryption attack, with for instance a pre-calculated decryption-key
databases, and similar techniques, without being forced into doing some sort of
pattern recognition, or other expensive analyzing of possible matches, which
would result in lots of false positives for the algorithm, and much, much,
much more CPU and resource requirements, than if it knows it is simply looking
for the capital letters *"GET"*.

To give you an example, the Enigma machine, was apparently, according to modern
dogma, broken because of the fact that every single message the Germans transmitted,
ended with the letters; *"Heil Hitler"* - This made it orders of magnitude
more easy for Alan Turing to create a code-breaker, since he knew how a positive
match looked like. Or at least the last 11 characters of a successful crack.

The strictness of the HTTP standard itself, is that same "red flag", which
allows an adversary to know what it is looking for, the same way Alan Turing
knew what he was looking for, since every single successful code-breaking
result, would with a 80% probability start out with the letters *"GET"*, 15%
probability start with the letters *"POST"* (which could be deducted from
its size, to further reduce the checklist), and 5% probability with the words;
*"PUT"*, *"DELETE"*, *"HEAD"* and so on.

By *"cloaking"* the message, a code-breaking machine, wouldn't even know how
a positive match looks like, and would be completely left in the wild, in regards
to any brute force attack to try to find the decryption key. And would have
to actually read and analyze the contents of the HTTP message, for every single
possible match, to analyze if it actually is a match or not. This process is
simply too expensive, for any computers today to be able to do it, even if
they possessed the CPU capability of actually decrypting the message in the
first place.

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

