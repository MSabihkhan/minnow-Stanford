#include "tcp_receiver.hh"
#include "debug.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{

  if ( message.RST == true ) {
    reassembler_.reader().set_error();
    return;
  }
  if ( isn.has_value() == false ) {
    if ( message.SYN ) {
      isn = message.seqno;
    } else {
      return;
    }
  }

  uint64_t checkpoint = reassembler_.writer().bytes_pushed() + 1;
  uint64_t abs_seqno = message.seqno.unwrap( *isn, checkpoint );
  uint64_t abs_payload = abs_seqno + ( message.SYN ? 1 : 0 );
  uint64_t stream_index = abs_payload - 1;
  reassembler_.insert( stream_index, message.payload, message.FIN );
}

TCPReceiverMessage TCPReceiver::send() const
{
  TCPReceiverMessage msg;
  uint64_t remaining_size = reassembler_.writer().available_capacity();
  msg.window_size = min<uint64_t>( remaining_size, UINT16_MAX );
  if ( isn.has_value() == true ) {
    uint64_t abs_seqno = reassembler_.writer().bytes_pushed() + 1;
    if ( reassembler_.writer().is_closed() == true ) {
      abs_seqno += 1;
    }
    msg.ackno = Wrap32::wrap( abs_seqno, *isn );
  } else {
    msg.ackno = nullopt;
  }
  if ( reassembler_.writer().has_error() == true ) {
    msg.RST = true;
  }

  return msg;
}
