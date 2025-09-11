#include "byte_stream.hh"
#include <string>
using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) , buffer{},state(false), bytes_Pushed(0),bytes_Poped(0) {}
uint64_t getsize(string data){
  size_t bytes = data.size();
  return bytes;
}
void Writer::push( std::string data )
{
  if (is_closed()){
    return;
  }
  if (available_capacity()>=getsize(data)){
    buffer.append(data);
    bytes_Pushed+= data.size();
    capacity_ =  capacity_ - data.size();
  }
  else
  {
    buffer.append(data.substr(0, available_capacity()));
    bytes_Pushed += available_capacity();
    capacity_ = capacity_- capacity_;   
  }
}

void Writer::close()
{ 
  state = true;
}

bool Writer::is_closed() const
{
  return state;
}

uint64_t Writer::available_capacity() const
{
  return capacity_;
}

uint64_t Writer::bytes_pushed() const
{
  return bytes_Pushed;
}

string_view Reader::peek() const
{
  return buffer;
}

void Reader::pop( uint64_t len )
{
  if (buffer.size() == 0){
    return;
  }
  if ( len > buffer.size() ) {
    len = buffer.size();
  }
  buffer.erase( 0, len );
  bytes_Poped += len;
  capacity_+= len;
}

bool Reader::is_finished() const
{
  if (buffer.size() == 0)
  {
    return state;
  }
  return false;
  
}

uint64_t Reader::bytes_buffered() const
{
  return buffer.size();
}

uint64_t Reader::bytes_popped() const
{
  return bytes_Poped;
}

