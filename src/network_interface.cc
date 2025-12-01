#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"
#include "exception.hh"
#include "helpers.hh" 

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  const uint32_t next_hop_ip = next_hop.ipv4_numeric();

  // Case 1: Destination Ethernet address is known in our cache
  if ( arp_table.count( next_hop_ip ) ) {
    EthernetFrame frame;
    frame.header.src = ethernet_address_;
    frame.header.dst = arp_table[next_hop_ip].eth_addr;
    frame.header.type = EthernetHeader::TYPE_IPv4;
    frame.payload = serialize( dgram );
    transmit( frame );
    return;
  }

  // Case 2: Destination Ethernet address is unknown
  if ( waiting_arp_requests.count( next_hop_ip ) ) {
    if ( current_time - waiting_arp_requests[next_hop_ip] <= ARP_REQUEST_TIMEOUT_MS ) {
      waiting_dgrams[next_hop_ip].push_back( dgram );
      return; 
    } else {
        waiting_dgrams[next_hop_ip].clear();
    }
  }
  waiting_dgrams[next_hop_ip].push_back( dgram );

  ARPMessage arp_request;
  arp_request.opcode = ARPMessage::OPCODE_REQUEST;
  arp_request.sender_ethernet_address = ethernet_address_;
  arp_request.sender_ip_address = ip_address_.ipv4_numeric();
  arp_request.target_ethernet_address = {};
  arp_request.target_ip_address = next_hop_ip;

  EthernetFrame eth_frame;
  eth_frame.header.src = ethernet_address_;
  eth_frame.header.dst = ETHERNET_BROADCAST;
  eth_frame.header.type = EthernetHeader::TYPE_ARP;
  eth_frame.payload = serialize( arp_request );
  transmit( eth_frame );

  waiting_arp_requests[next_hop_ip] = current_time;
}
void NetworkInterface::recv_frame( EthernetFrame frame )
{
  if ( frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST ) {
    return;
  }

  // Case 1: IPv4 Frame
  if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
    InternetDatagram dgram;
    if ( parse( dgram, frame.payload ) ) {
      datagrams_received_.push( dgram );
    }
    return;
  }

  // Case 2: ARP Frame
  if ( frame.header.type == EthernetHeader::TYPE_ARP ) {
    ARPMessage arp_msg;
    if ( parse( arp_msg, frame.payload ) ) {
      const uint32_t sender_ip = arp_msg.sender_ip_address;
      const EthernetAddress sender_eth = arp_msg.sender_ethernet_address;
      arp_table[sender_ip] = { sender_eth, current_time + ARP_TTL_MS };
      if ( waiting_dgrams.count( sender_ip ) ) {
        for ( const auto& dgram : waiting_dgrams[sender_ip] ) {
          send_datagram( dgram, Address::from_ipv4_numeric( sender_ip ) );
        }
        waiting_dgrams.erase( sender_ip );
      }
      if ( arp_msg.opcode == ARPMessage::OPCODE_REQUEST && 
           arp_msg.target_ip_address == ip_address_.ipv4_numeric() ) {
        
        ARPMessage arp_reply;
        arp_reply.opcode = ARPMessage::OPCODE_REPLY;
        arp_reply.sender_ethernet_address = ethernet_address_;
        arp_reply.sender_ip_address = ip_address_.ipv4_numeric();
        arp_reply.target_ethernet_address = sender_eth;
        arp_reply.target_ip_address = sender_ip;

        EthernetFrame reply_frame;
        reply_frame.header.src = ethernet_address_;
        reply_frame.header.dst = sender_eth;
        reply_frame.header.type = EthernetHeader::TYPE_ARP;
        reply_frame.payload = serialize( arp_reply );

        transmit( reply_frame );
      }
    }
  }
}

void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  current_time += ms_since_last_tick;
  for ( auto it = arp_table.begin(); it != arp_table.end(); ) {
    if ( current_time >= it->second.expiration ) {
      it = arp_table.erase( it );
    } else {
      ++it;
    }
  }
}