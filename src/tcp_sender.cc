#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"

using namespace std;

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::sequence_numbers_in_flight() const
{

  return inflight;
}

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::consecutive_retransmissions() const
{

  return consecutive_retransmitions;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  uint64_t effective_window = 0;
  if ( rcwsize == 0 ) {
    effective_window = 1;
  } else {
    effective_window = rcwsize;
  }

  while ( inflight < effective_window ) {
    TCPSenderMessage msg;
    if ( input_.has_error() ) {
      msg.RST = true;
    }
    if ( sync_check == false ) {
      sync_check = true;
      msg.SYN = true;
    }
    uint64_t available_window = effective_window - inflight;
    msg.seqno = isn_ + nxt_seq_no;
    uint64_t payload_size = min( TCPConfig::MAX_PAYLOAD_SIZE, available_window - msg.SYN );

    string payload_data;
    read( reader(), payload_size, payload_data );
    msg.payload = move( payload_data );
    if ( !fin_check && reader().is_finished() && ( available_window > msg.sequence_length() ) ) {
      msg.FIN = true;
      fin_check = true;
    }
    if ( msg.sequence_length() == 0 ) {
      break;
    }

    transmit( msg );
    outstanding_msgs.push( msg );
    inflight += msg.sequence_length();
    nxt_seq_no += msg.sequence_length();

    if ( !timer_on ) {
      timer_on = true;
      time_elapsed = 0;
    }
    if ( msg.FIN ) {
      break;
    }
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{

  TCPSenderMessage msg;
  msg.seqno = isn_ + nxt_seq_no;
  if ( input_.has_error() ) {
    msg.RST = true;
  }
  return msg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if ( msg.RST ) {
    input_.set_error();
    return;
  }

  rcwsize = msg.window_size;
  if ( msg.ackno.has_value() ) {
    uint64_t ackno = msg.ackno->unwrap( isn_, rc_ackno );

    if ( ackno > nxt_seq_no ) {
      return;
    }
    if ( ackno > rc_ackno ) {
      rc_ackno = ackno;

      while ( !outstanding_msgs.empty() ) {
        const auto& segment = outstanding_msgs.front();
        uint64_t seg_end = segment.seqno.unwrap( isn_, rc_ackno ) + segment.sequence_length();

        if ( seg_end <= ackno ) {
          inflight -= segment.sequence_length();
          outstanding_msgs.pop();
        } else {
          break;
        }
      }
      current_RTO_ms = initial_RTO_ms_;
      consecutive_retransmitions = 0;
      if ( !outstanding_msgs.empty() ) {
        time_elapsed = 0;
      } else {
        timer_on = false;
      }
    }
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  if ( !timer_on ) {
    return;
  }

  time_elapsed = time_elapsed + ms_since_last_tick;
  if ( time_elapsed >= current_RTO_ms ) {
    transmit( outstanding_msgs.front() );
    if ( rcwsize > 0 ) {
      current_RTO_ms *= 2;
    }

    consecutive_retransmitions++;
    time_elapsed = 0;
  }
}
