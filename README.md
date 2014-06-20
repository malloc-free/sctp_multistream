sctp_multistream is distributed under the GNU Lesser General Public License
as published by the Free Software Foundation; either version 3 of the
Licence, or (at your option) any later version.

This is a multistreaming library for lksctp-tools. The idea is to provide
separate file descriptors for sending and receiving data over streams in 
SCTP. The current method requires specifying the stream to send on in the 
function sctp_sendmsg, and passing of sctp_sndrcvinfo into sctp_recvmsg to 
obtain information on the receiving stream. In sctp_multistream, each
stream gets a descriptor, and this descriptor can be used to connect, bind
and listen on a specific stream on an association. New connections to an
an endpoint with an existing association open use a new stream rather than
create a new association.

Internally, communication over the association is handled asynchronously,
using epoll, and pipes and events are used to notify when new incoming 
connections have arrived on the association, and when data is ready to be
read from a particular stream. Listening applications on the server side
have to be assigned streams, and when a client connects, this stream has to
be indicated.

The plan so far is to have associations automatically close when there are no
open streams or any applications listening on streams. It would be possilbe
to introduce a timeout for this shutdown process.

To build, you will need lksctp_tools installed on your Linux machine. More
information: lksctp.sourceforge.net
