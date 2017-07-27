//!
//! casual 
//!

#include "gateway/message.h"

#include "common/marshal/binary.h"
#include "common/marshal/network.h"
#include "common/network/byteorder.h"
#include "common/communication/tcp.h"

#include "common/message/transaction.h"
#include "common/message/service.h"

#include "common/terminal.h"


#include <iostream>
#include <typeindex>


namespace casual
{
   namespace common
   {
      namespace communication
      {
         namespace message
         {
            namespace complete
            {
               namespace network
               {
                  //
                  // just a local marshaler to help format Header...
                  //
                  template< typename A>
                  void casual_marshal_value( const Header& value, A& archive)
                  {
                     archive << static_cast<Header::host_type_type>(value.type);
                     archive << value.correlation;
                     archive << static_cast<Header::host_size_type>(value.size);
                  }
               }
            }
         }
      }
   }

   namespace gateway
   {

      namespace protocol
      {
         namespace local
         {
            namespace
            {
               namespace type
               {
                  struct Name
                  {
                     std::string role;
                     std::string description;
                  };

                  struct Type
                  {
                     std::string type;
                     common::network::byteorder::size::host::type size;
                  };

                  struct Info
                  {
                     Name name;
                     Type network;
                  };

                  template< typename T>
                  const char* name( T&& value)
                  {
                     static std::unordered_map< std::type_index, const char*> names
                     {
                        { typeid( std::uint8_t), "uint8"},
                        { typeid( std::uint16_t), "uint16"},
                        { typeid( std::uint32_t), "uint32"},
                        { typeid( std::uint64_t), "uint64"},
                     };


                     const auto found = common::range::find( names, typeid( common::marshal::binary::network::detail::cast( value)));

                     if( found)
                     {
                        return found->second;
                     }

                     return typeid( T).name();
                  }


                  template< typename T>
                  auto network( T&& value) -> typename std::enable_if< ! common::marshal::binary::network::detail::is_network_array< T>::value, Type>::type
                  {
                     auto network = common::network::byteorder::encode( common::marshal::binary::network::detail::cast( value));
                     return Type{ name( network), common::memory::size( network)};
                  }

                  template< typename T>
                  auto network( T&& value) -> typename std::enable_if< common::marshal::binary::network::detail::is_network_array< T>::value, Type>::type
                  {
                     return Type{ "fixed array", static_cast< common::network::byteorder::size::host::type>( common::memory::size( value))};
                  }


                  template< typename T>
                  Info info( T&& value, std::string role, std::string description)
                  {
                     return Info{ { std::move( role), std::move( description)}, network( std::forward< T>( value))};
                  }


                  template< typename T>
                  void print( std::ostream& out, T&& value, std::string role, std::string description)
                  {
                     print( out, info( std::forward< T>( value), std::move( role), std::move( description)));
                  }

                  template< typename T>
                  void print( std::ostream& out, std::string role, std::string description)
                  {
                     print( out, T{}, std::move( role), std::move( description));
                  }

               } // type



               struct Printer
               {
                  Printer( std::vector< type::Info>& types) : m_types( types) {}

                  Printer( std::vector< type::Info>& types, std::initializer_list< type::Name> roles) : m_types( types), m_roles( std::move( roles)) {}


                  template< typename T>
                  Printer& operator & ( T& value)
                  {
                     return *this << value;
                  }

                  template< typename T>
                  Printer& operator << ( const T& value)
                  {
                     write( value);
                     return *this;
                  }

                  template< typename Iter>
                  void append( Iter first, Iter last)
                  {
                     auto size = std::distance( first, last);

                     type::Info info;

                     if( ! m_roles.empty())
                     {
                        info.name.role = std::move( m_roles.front().role);
                        info.name.description = std::move( m_roles.front().description);
                        m_roles.pop();
                     }

                     info.network.size = size;
                     info.network.type = "dynamic array";

                     m_types.push_back( std::move( info));
                  }

                  template< typename C>
                  void append( C&& range)
                  {
                     append( std::begin( range), std::end( range));
                  }

               private:

                  template< typename T>
                  typename std::enable_if< ! common::marshal::binary::detail::is_native_marshable< T>::value>::type
                  write( T& value)
                  {
                     casual_marshal_value( value, *this);
                  }

                  template< typename T>
                  typename std::enable_if< common::marshal::binary::detail::is_native_marshable< T>::value>::type
                  write( T& value)
                  {
                     write_pod( value);
                  }


                  template< typename T>
                  void write_pod( const T& value)
                  {
                     if( m_roles.empty())
                     {
                        m_types.push_back( type::info( value, "unknown", ""));
                     }
                     else
                     {
                        m_types.push_back( type::info( value, std::move( m_roles.front().role), std::move( m_roles.front().description)));
                        m_roles.pop();
                     }
                  }

                  template< typename T>
                  void write( const std::vector< T>& value)
                  {
                     // TODO:
                     write_pod( static_cast<common::network::byteorder::size::host::type>(value.size()));

                     for( auto& current : value)
                     {
                        *this << current;
                     }
                  }

                  void write( const std::string& value)
                  {
                     // TODO:
                     write_pod( static_cast<common::network::byteorder::size::host::type>(value.size()));

                     append(
                        std::begin( value),
                        std::end( value));
                  }

                  void write( const common::platform::binary::type& value)
                  {
                     // TODO:
                     write_pod( static_cast<common::network::byteorder::size::host::type>(value.size()));

                     append(
                        std::begin( value),
                        std::end( value));
                  }


                  std::vector< type::Info>& m_types;
                  std::queue< type::Name> m_roles;

               };

               namespace extract
               {
                  template< typename T>
                  std::vector< type::Info> types( T&& type, std::initializer_list< type::Name> roles)
                  {
                     std::vector< type::Info> result;
                     local::Printer typer{ result, std::move( roles)};

                     typer << std::forward< T>( type);

                     return result;
                  }


               } // extract

               namespace format
               {

                  common::terminal::format::formatter< type::Info> type_info()
                  {
                     return {
                        { false, false, true, " | "},
                        common::terminal::format::column( "role name", []( const type::Info& i) { return i.name.role;}, common::terminal::color::no_color),
                        common::terminal::format::column( "network type", []( const type::Info& i) { return i.network.type;}, common::terminal::color::no_color),
                        common::terminal::format::column( "network size", []( const type::Info& i) { return i.network.size;}, common::terminal::color::no_color, common::terminal::format::Align::right),
                        common::terminal::format::column( "description", []( const type::Info& i) { return i.name.description;}, common::terminal::color::no_color),
                     };
                  }

                  template< typename T>
                  void type( std::ostream& out, T&& value, std::initializer_list< type::Name> roles)
                  {
                     auto formatter = type_info();

                     formatter.print( out, extract::types( std::forward< T>( value), std::move( roles)));
                  }


               } // format


            } // <unnamed>
         } // local

         namespace print
         {
            void header( std::ostream& out)
            {
               out << R"(
| role name     | network type | network size  | comments
|---------------|--------------|---------------|---------
)";
            }

            void message_header( std::ostream& out)
            {
               common::communication::message::complete::network::Header header;

               out << R"(
# casual domain protocol

Defines what messages is sent between domains and exactly what they contain. 

Some definitions:

* `fixed array`: an array of fixed size where every element is an 8 bit byte.
* `dynamic array`: an array with dynamic size, where every element is an 8 bit byte.

If an attribute name has `element` in it, for example: `services.element.timeout`, the
data is part of an element in a container. You should read it as `container.element.attribute`


## common::communication::message::complete::network::Header 

This header will be the first part of every message below, hence it's name, _Header_

message.type is used to dispatch to handler for that particular message, and knows how to (un)marshal and act on the message.

It's probably a good idea (probably the only way) to read the header only, to see how much more one has to read to get
the rest of the message.


)";

               local::format::type( out, header, {
                  { "header.type", "type of the message that the payload contains"},
                  { "header.correlation", "correlation id of the message"},
                  { "header.size", "the size of the payload that follows"},
               });
            }

            template< typename M>
            void transaction_request( std::ostream& out, M&& message)
            {
               message.trid = common::transaction::ID::create();

               out << "message type: **" << M::type() << "**\n\n";

               local::format::type( out, message, {
                        { "execution", "uuid of the current execution path"},
                        { "xid.format", "xid format type. if 0 no more information of the xid is transported"},
                        { "xid.gtrid_length", "length of the transaction gtrid part"},
                        { "xid.bqual_length", "length of the transaction branch part"},
                        { "xid.payload", "byte array with the size of gtrid_length + bqual_length (max 128)"},
                        { "resource.id", "RM id of the resource - has to correlate with the reply"},
                        { "flags", "XA flags to be forward to the resource"},
                     });
            }

            template< typename M>
            void transaction_reply( std::ostream& out, M&& message)
            {
               message.trid = common::transaction::ID::create();

               out << "message type: **" << M::type() << "**\n\n";

               local::format::type( out, message, {
                        { "execution", "uuid of the current execution path"},
                        { "xid.format", "xid format type. if 0 no more information of the xid is transported"},
                        { "xid.gtrid_length", "length of the transaction gtrid part"},
                        { "xid.bqual_length", "length of the transaction branch part"},
                        { "xid.payload", "byte array with the size of gtrid_length + bqual_length (max 128)"},
                        { "resource.id", "RM id of the resource - has to correlate with the request"},
                        { "resource.state", "The state of the operation - If successful XA_OK ( 0)"},
                     });
            }


            void transaction( std::ostream& out)
            {
               out << R"(
## Transaction messages

### Resource prepare

)";
               {

                  using message_type = message::interdomain::transaction::resource::receive::prepare::Request;

                  out << R"(
#### message::interdomain::transaction::resource::receive::prepare::Request

Sent to and received from other domains when one domain wants to prepare a transaction. 

)";
                  transaction_request( out, message_type{});
               }

               {
                  using message_type = message::interdomain::transaction::resource::receive::prepare::Reply;

                  out << R"(
#### message::interdomain::transaction::resource::receive::prepare::Reply

Sent to and received from other domains when one domain wants to prepare a transaction. 

)";
                  transaction_reply( out, message_type{});
               }

               out << R"(
### Resource commit

)";

               {
                  using message_type = message::interdomain::transaction::resource::receive::commit::Request;

                  out << R"(
#### message::interdomain::transaction::resource::receive::commit::Request

Sent to and received from other domains when one domain wants to commit an already prepared transaction.

)";
                  transaction_request( out, message_type{});
               }

               {
                  using message_type = message::interdomain::transaction::resource::receive::commit::Reply;

                  out << R"(
#### message::interdomain::transaction::resource::receive::commit::Reply

Reply to a commit request. 

)";
                  transaction_reply( out, message_type{});
               }



               out << R"(
### Resource rollback

)";


               {
                  using message_type = message::interdomain::transaction::resource::receive::rollback::Request;

                  out << R"(
#### message::interdomain::transaction::resource::receive::rollback::Request

Sent to and received from other domains when one domain wants to rollback an already prepared transaction.
That is, when one or more resources has failed to prepare.

)";
                  transaction_request( out, message_type{});
               }

               {
                  using message_type =  message::interdomain::transaction::resource::receive::rollback::Reply;

                  out << R"(
#### message::interdomain::transaction::resource::receive::rollback::Reply

Reply to a rollback request. 

)";
                  transaction_reply( out, message_type{});
               }

            }



            void service_call( std::ostream& out)
            {
               out << R"(
## Service messages

### Service call 

)";
               {
                  using message_type = message::interdomain::service::call::receive::Request;

                  out << R"(
#### message::interdomain::service::call::receive::Request

Sent to and received from other domains when one domain wants call a service in the other domain

)";
                  out << "message type: **" << message_type::type() << "**\n\n";

                  message_type request;
                  request.trid = common::transaction::ID::create();
                  request.service.name.resize( 128);
                  request.parent.resize( 128);
                  request.buffer.type.resize( 8 + 1 + 16);
                  request.buffer.memory.resize( 128);

                  local::format::type( out, request, {
                           { "execution", "uuid of the current execution path"},
                           { "service.name.size", "service name size"},
                           { "service.name.data", "byte array with service name"},
                           { "service.timeout", "timeout of the service in use (in microseconds)"},
                           { "parent.name.size", "parent service name size"},
                           { "parent.name.data", "byte array with parent service name"},

                           { "xid.format", "xid format type. if 0 no more information of the xid is transported"},
                           { "xid.gtrid_length", "length of the transaction gtrid part"},
                           { "xid.bqual_length", "length of the transaction branch part"},
                           { "xid.payload", "byte array with the size of gtrid_length + bqual_length (max 128)"},

                           { "flags", "XATMI flags sent to the service"},

                           { "buffer.type.size", "buffer type name size"},
                           { "buffer.type.data", "byte array with buffer type in the form 'type/subtype'"},
                           { "buffer.payload.size", "buffer payload size (could be very big)"},
                           { "buffer.payload.data", "buffer payload data (with the size of buffer.payload.size)"},
                        });
               }


               {
                  using message_type = message::interdomain::service::call::receive::Reply;

                  out << R"(
#### message::interdomain::service::call::receive::Reply

Reply to call request

)";
                  out << "message type: **" << message_type::type() << "**\n\n";

                  message_type message;

                  message.transaction.trid = common::transaction::ID::create();
                  message.buffer.type.resize( 8 + 1 + 16);
                  message.buffer.memory.resize( 128);

                  local::format::type( out, message, {
                           { "execution", "uuid of the current execution path"},

                           { "call.error", "XATMI error code, if any."},
                           { "call.code", "XATMI user supplied code"},

                           { "transaction.trid.xid.format", "xid format type. if 0 no more information of the xid is transported"},
                           { "transaction.trid.xid.gtrid_length", "length of the transactino gtrid part"},
                           { "transaction.trid.xid.bqual_length", "length of the transaction branch part"},
                           { "transaction.trid.xid.payload", "byte array with the size of gtrid_length + bqual_length (max 128)"},
                           { "transaction.state", "state of the transaction TX_ACTIVE, TX_TIMEOUT_ROLLBACK_ONLY, TX_ROLLBACK_ONLY"},

                           { "buffer.type.size", "buffer type name size"},
                           { "buffer.type.data", "byte array with buffer type in the form 'type/subtype'"},
                           { "buffer.payload.size", "buffer payload size (could be very big)"},
                           { "buffer.payload.data", "buffer payload data (with the size of buffer.payload.size)"},


                        });
               }

            }

            void domain_discovery( std::ostream& out)
            {
               out << R"(
## Discovery messages

### domain discovery 

)";

               {
                  using message_type = message::interdomain::domain::discovery::receive::Request;

                  out << R"(
#### message::interdomain::domain::discovery::Request

Sent to and received from other domains when one domain wants discover information abut the other.

)";

                  out << "message type: **" << message_type::type() << "**\n\n";

                  message_type message;

                  message.services.push_back( std::string( 128, 0));
                  message.queues.push_back( std::string( 128, 0));

                  local::format::type( out, message, {
                           { "execution", "uuid of the current execution path"},
                           { "domain.id", "uuid of the caller domain"},
                           { "domain.name.size", "size of the caller domain name"},
                           { "domain.name.data", "dynamic byte array with the caller domain name"},
                           { "services.size", "number of requested services to follow (an array of services)"},
                           { "services.element.size", "size of the current service name"},
                           { "services.element.data", "dynamic byte array of the current service name"},
                           { "queues.size", "number of requested queues to follow (an array of queues)"},
                           { "queues.element.size", "size of the current queue name"},
                           { "queues.element.data", "dynamic byte array of the current queue name"},
                        });


               }


               {
                  using message_type = message::interdomain::domain::discovery::receive::Reply;

                  out << R"(
#### message::interdomain::domain::discovery::Reply

Sent to and received from other domains when one domain wants discover information abut the other.

)";

                  out << "message type: **" << message_type::type() << "**\n\n";

                  message_type message;

                  message.services.push_back( std::string( 128, 0));
                  message.queues.push_back( std::string( 128, 0));

                  local::format::type( out, message, {
                           { "execution", "uuid of the current execution path"},
                           { "domain.id", "uuid of the caller domain"},
                           { "domain.name.size", "size of the caller domain name"},
                           { "domain.name.data", "dynamic byte array with the caller domain name"},
                           { "services.size", "number of services to follow (an array of services)"},
                           { "services.element.name.size", "size of the current service name"},
                           { "services.element.name.data", "dynamic byte array of the current service name"},
                           { "services.element.category.size", "size of the current service category"},
                           { "services.element.category.data", "dynamic byte array of the current service category"},
                           { "services.element.transaction", "service transaction mode (auto, atomic, join, none)"},
                           { "services.element.timeout", "service timeout"},
                           { "services.element.hops", "number of domain hops to the service (local services has 0 hops)"},
                           { "queues.size", "number of requested queues to follow (an array of queues)"},
                           { "queues.element.size", "size of the current queue name"},
                           { "queues.element.data", "dynamic byte array of the current queue name"},
                           { "queues.element.retries", "how many 'retries' the queue has"},
                        });
               }
            }

            void protocol()
            {
               message_header( std::cout);
               domain_discovery( std::cout);
               service_call( std::cout);
               transaction( std::cout);
            }
         } // print
      } // protocol
   } // gateway
} // casual

int main( int argc, char **argv)
{
   casual::gateway::protocol::print::protocol();

   return 0;
}





