Rosetta Web Server
===============

Rosetta Web Server is a small web server.

## Getting started with Rosetta

Coming soon

## HTTP Cloaking

Rosetta supports something called **HTTP cloaking**. This simply means that
it can understand a *"hidden"* an HTTP request, in something that wouldn't
normally look like an HTTP request, for a man in the middle. The way it does
this, is by being able to accept HTTP envelopes that other servers wouldn't
normally even recognize as HTTP traffic at all.

HTTP Cloaking literally translates into; *"Send me garbage data, that nobody
else will be able to understand, and I will be able to make sense of it, somehow."*

The advantages is that it eliminates a man in the middle to even understand
what type of traffic you're sending and receiving, if your client supports
Cloaking. It therefor helps you to anonymize your type of traffic.

### Technicalities

The HTTP envelope parser accepts any characters between [0-11> as a LF,
any character between [11-21> as CR, and any character between [21-33>
as SP. In addition it ignores any characters between [191-256>,
and bit shifts down one position any character between [128-191>, before
it interprets what characters it is being sent in the HTTP request envelope.

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
contain anything, since both the method, URI and the version, are optional
fields. If missing, Rosetta will default the request method to GET, and
URI to / and the HTTP version to HTTP/1.1.

The simplest possible HTTP GET request, does not need to contain anything
but CR/LF, which again might be interchanged with any of the characters
between [0-11> for LF, and [11-21> for CR, which would be interpreted as
a perfectly valid GET request, for the default page, whatever that is
in your system.

This means you could request the default HTML page in a Rosetta server, with
for instance 0x05 and 0xF5, for instance, which would be interpreted as
a CR/LF sequence by the HTTP envelope parser. Or 0x00 and 0x0b, etc. And
you could put as many characters between the CR and LF in the range of
[191-256> as you wish.

In fact, even without considering adding "garbage characters", which are
characters from 190 and up to 255, there exists 10x10 different ways
of creating a simple CR/LF sequence in Rosetta. Once you start adding
garbage characters to the stream, to further increase the strength of your
cloaking, then the amount of possible ways to create a valid CR/LF sequence
to a Rosetta server, literally becomes infinite.

Any HTTP header fields, use the first character not in the range of
[a-z, A-Z, 0-9 or '-'] as the delimiter between the HTTP header name, and
its value. In addition, when parsing HTTP header fields, the name of them
is turned into lowercase, and stored, and referred to, internally in Rosetta,
as lowercase. Any SP/TAB characters are trimmed away, from both the name
and the value. This process is done after the initial bit-shifting process,
which means that a perfectly valid HTTP header can look something like this
for instance;

  coNneCtion &   keep-alive

Which of course means;

Connection:keep-alive

In addition, the header, name and value, can contain any ignored bytes, in
the range of [191-256>, at any position, to create "garbage characters", making
the header field, more difficult to interpret for an adversary man in the middle.
And the CR/LF sequence, can be any characters between [0-11> && [11-21>. And SP
here means any character i the range of [21-33>.

The URI can use any of these letters as delimiter between the actual URI,
and the HTTP GET parameters [?*$~^€§]. Again, this is done after the initial
bit-shifting process, and discarding of invalid characters, etc.

This means, that if you create a Cloaked HTTP request, it would be impossible
for an adversary man in the middle, to even know what type of traffic you
are sending. And it would appear outwards like any *"random binary bytes
sent from any random game you are currently playing"*.

## Why Cloak?

By using HTTP Cloaking, you can *"hide"* your HTTP requests, as something
that others, proxies and similar men in the middle, would not normally
recognize as HTTP. This makes it near impossible for a man in the middle
to understand what type of traffic you are sending over a network, since
a Cloaket HTTP request would be perceived as "random binary bytes" sent
from a client to a server. And any attempts at trying to understand what
type of traffic is being sent, would imply analyzing the message, or
implementing pattern recognition, which would highly likely create false
positives, making any man in the middle interpret other types of traffic
as HTTP traffic.

In addition it makes it literally impossible for a man in the middle to
tamper with, and/or change your HTTP message, since it wouldn't even
know with certainty, if this is an HTTP message, or any other types
of random binary data sent between two computers.

In addition, when you combine Cloaking with additional layers of encryption,
in addition to HTTPS, such as OpenPGP or AES, inbetween the HTTPS and the
message transmission, then unless an adversary knows which key you used
to encrypt your message with, and has access to it, a brute force decryption
of the HTTP message, would be significantly more CPU resource demanding.

This is because anyone trying to brute force decrypt a message, would not
even know what to look for. And even if they were able to actually decrypt
the message, using a database of pre-fabricated cryptography keys for instance,
they would not be able to discriminate a valid HTTP message from "random garbage".

Meaning, even if they were able to successfully decrypt your message, they
would not be able to recognize the decrypted message from any *"random crap"*
their decryption algorithm would produce for a miss.

In addition, you can create smaller HTTP requests, reducing the bandwidth
consumption of your HTTP traffic, since you can strip away almost the
entire HTTP envelope, and Rosetta will still be able to understand your request.

### Disclaimer, this is **not** cryptography

Please notice that HTTP Cloaking is not an alternative to encryption. You
should still use encryption on your HTTP traffic. Also notice that no known
client, as of today, are able to create Cloaked HTTP requests. So this is a
feature, which at the moment, is only for Rosetta to Rosetta web service
communication.

### Performance penalties of Cloaking your HTTP requests

The performance penalty of implementing support for understanding
Cloaked HTTP requests, is almost insignificant. This is because any
serious HTTP envelope parser, still needs to look for *"garbage data"*,
to protect itself from injecting malicious data into the server. The
only difference between Rosetta and any other web server in this regard,
is that Rosetta instead of discarding the *"garbage data"*, tries to
transform it, into something it can understand.

So there is little, if any, performance penalty of implementing HTTP Cloaking.

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

