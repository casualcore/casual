//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/documentation/protocol/example.h"
#include "gateway/message.h"
#include "gateway/message/protocol.h"

#include "common/serialize/native/binary.h"
#include "common/serialize/native/network.h"
#include "common/serialize/yaml.h"
#include "common/network/byteorder.h"
#include "common/communication/tcp.h"

#include "common/message/transaction.h"
#include "common/message/service.h"

#include "common/binary/span.h"
#include "common/terminal.h"
#include "common/execute.h"
#include "common/algorithm/sorted.h"
#include "common/string/compose.h"

#include <iostream>
#include <typeindex>

namespace casual
{
   namespace common::serialize::customize::composite
   {
      //! just a local marshaler to help format Header...
      template< typename A>
      struct Value< communication::tcp::message::Header, A>
      {
         template< typename V>  
         static void serialize( A& archive, V&& value)
         {
            CASUAL_SERIALIZE_NAME( static_cast< communication::tcp::message::Header::host_type_type>( value.type), "type");
            CASUAL_SERIALIZE_NAME( common::binary::span::fixed::make( value.correlation), "correlation");
            CASUAL_SERIALIZE_NAME( static_cast< communication::tcp::message::Header::host_size_type>( value.size), "size");
         }
      };

   } // common::serialize::customize::composite

   namespace gateway::documentation::protocol
   {
      using namespace common;

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

                  struct
                  {
                     platform::size::type min{};
                     platform::size::type max{};
                  } size;
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

                  const auto found = common::algorithm::find( names, typeid( common::serialize::native::binary::network::detail::cast( value)));

                  if( found)
                     return found->second;

                  return typeid( T).name();
               }

               template< common::serialize::native::binary::network::detail::network_value T>
               auto network( T&& value)
               {
                  auto network = common::network::byteorder::encode( common::serialize::native::binary::network::detail::cast( value));
                  const auto size = common::memory::size( network);
                  return Type{ name( network), { size, size}};
               }

               template< common::serialize::native::binary::network::detail::network_array T>
               auto network( T&& value)
               {
                  const auto size = common::memory::size( value);
                  return Type{ "fixed array", { size, size}};
               }


               template< typename T>
               Info info( T&& value, type::Name name)
               {
                  return Info{ std::move( name), network( std::forward< T>( value))};
               }

               template< typename T>
               Info info( T&& value, std::string role, std::string description)
               {
                  return info( std::forward< T>( value), { std::move( role), std::move( description)});
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
               // we need to have the names and serialize in a "order" way. And of course we're network normalizing...
               constexpr static auto archive_properties() 
               {
                  using Property = common::serialize::archive::Property;
                  return Property::named | Property::order | Property::network;
               }

               Printer() = default;

               Printer( std::initializer_list< type::Name> roles)
               {
                  common::algorithm::for_each( roles, [&]( auto& name)
                  {
                     m_descriptions.emplace( std::move( name.role), std::move( name.description));
                  });
               }

               template< typename T>
               Printer& operator << ( T&& value)
               {
                  common::serialize::value::write( *this, std::forward< T>( value), nullptr);
                  return *this;
               }

               inline void container_start( platform::size::type size, const char* name) 
               { 
                  canonical.push( name);
                  write_size( size);
                  canonical.push( "element");
               }
               inline void container_end( const char*) 
               { 
                  canonical.pop(); // element
                  canonical.pop(); // name
               }

               inline void composite_start( const char* name) { canonical.push( name);}
               inline void composite_end(  const char*) { canonical.pop();}


               template< concepts::serialize::archive::native::write T> 
               auto write( T value, const char* name)
               { 
                  canonical.push( name);
                  write( value);
                  canonical.pop();
               }

               std::vector< type::Info> release() 
               {
                  return std::exchange( m_types, {});
               }

            private:


               template< typename T>
               void type( T&& value)
               {
                  m_types.push_back( type::info( std::forward< T>( value), get_name( canonical.name())));
               }

               void write_size( platform::size::type value)
               {
                  canonical.push( "size");
                  type( value);
                  canonical.pop();
               }

               void dynamic( platform::size::type min, platform::size::type max, const char* type)
               {
                  type::Info info;
                  info.name = get_name( canonical.name());
                  info.network.size.min = min;
                  info.network.size.max = max;
                  info.network.type = type;
                  m_types.push_back( std::move( info));               
               }
               template< concepts::arithmetic T>
               void write( const T& value)
               {
                  type( value);
               }

               void write( std::string_view value)
               {
                  write_size( value.size());
                  canonical.push( "data");
                  dynamic( 0, std::numeric_limits< platform::size::type>::max(), "dynamic string");
                  canonical.pop();
               }

               void write( std::u8string_view value)
               {
                  write_size( value.size());
                  canonical.push( "data");
                  dynamic( 0, std::numeric_limits< platform::size::type>::max(), "dynamic (unicode) string");
                  canonical.pop();
               }

               void write( std::span< const std::byte> value)
               {
                  write_size( value.size());
                  canonical.push( "data");
                  dynamic( 0,  std::numeric_limits< platform::size::type>::max(), "dynamic binary");
                  canonical.pop();
               }

               void write( common::binary::span::Fixed< const std::byte> value)
               {
                  //write_size( value.size());
                  dynamic( value.size(), value.size(), "(fixed) binary");
               }

               struct 
               {
                  inline void push( const char* name)
                  {
                     m_parts.push_back( name);
                  }

                  inline void pop()
                  {
                     m_parts.pop_back();
                  }

                  std::string name() const  
                  {
                     auto parts = common::algorithm::find_if( m_parts, []( auto part) { return part != nullptr;});

                     if( ! parts)
                        return {};
                     
                     std::ostringstream out;
                     out << *parts;
                     ++parts;
                     
                     common::algorithm::for_each( parts, [&out]( auto part)
                     {
                        if( part != nullptr)
                           out << '.' << part;
                     });

                     return std::move( out).str();
                  } 

               private:
                  std::deque< const char*> m_parts;

               } canonical;



               type::Name get_name( std::string role)
               {
                  type::Name name{ std::move( role), {}};

                  auto found = common::algorithm::find( m_descriptions, name.role);

                  if( found)
                     name.description = found->second;

                  return name;
               } 

               
               std::map< std::string, std::string> m_descriptions;
               std::vector< type::Info> m_types;

            };


            namespace extract
            {
               template< typename T>
               std::vector< type::Info> types( T&& type, std::initializer_list< type::Name> roles)
               {
                  local::Printer printer{ std::move( roles)};
                  printer << type;

                  return printer.release();
               }


            } // extract

            namespace format
            {

               auto type_info()
               {
                  auto format_size = []( const type::Info& info)
                  {
                     if( info.network.size.min == info.network.size.max)
                        return std::to_string( info.network.size.min);
                     if( info.network.size.max == std::numeric_limits< platform::size::type>::max())
                        return common::string::compose( '[', info.network.size.min, "..*]");
                     return common::string::compose( '[', info.network.size.min, "..", info.network.size.max, ']');
                  };
                  return common::terminal::format::formatter< type::Info>::construct( 
                     std::string{ " | "},
                     common::terminal::format::column( "role name", []( const type::Info& i) { return i.name.role;}, common::terminal::color::no_color),
                     common::terminal::format::column( "network type", []( const type::Info& i) { return i.network.type;}, common::terminal::color::no_color),
                     common::terminal::format::column( "network size", format_size, common::terminal::color::no_color, common::terminal::format::Align::right),
                     common::terminal::format::column( "description", []( const type::Info& i) { return i.name.description;}, common::terminal::color::no_color)
                  );
               }

               template< typename T>
               void type( std::ostream& out, T&& value, std::initializer_list< type::Name> roles)
               {
                  auto formatter = type_info();

                  formatter.print( out, extract::types( std::forward< T>( value), std::move( roles)));
               }


            } // format

            namespace message
            {
               template< typename Message>
               decltype( auto) section( std::ostream& out, std::string_view bangs)
               {
                  auto versions = []()
                  {
                     auto versions = range::reverse( gateway::message::protocol::versions);
                     auto range = gateway::message::protocol::version< Message>();
                     auto lowest = algorithm::find( versions, range.min);
                     return std::get< 0>( algorithm::sorted::upper_bound( lowest, range.max));
                  };

                  return common::stream::write( out, "\n\n", bangs, ' ', Message::type(), " - **#", std::to_underlying( Message::type()), "** - _", versions(), '_');
               }
            } // message

            namespace binary
            {
               auto value( platform::size::type size)
               {
                  constexpr auto min = std::numeric_limits< char>::min();
                  constexpr auto max = std::numeric_limits< char>::max();

                  platform::binary::type result;
                  result.resize( size);

                  common::algorithm::for_each( result, [ current = min]( auto& byte) mutable
                  {
                     byte = static_cast< std::byte>( current);

                     if( current == max)
                        current = min;
                     else
                        ++current;
                  });

                  return result;
               }
            } // binary

            namespace string
            {
               auto value( platform::size::type size)
               {
                  static constexpr std::string_view letters = "abcdefabcdef";

                  std::string result;
                  result.resize( size);

                  common::algorithm::for_each( result, [ current = std::begin( letters)]( auto& value) mutable
                  {
                     if( current == std::end( letters))
                        current = std::begin( letters);

                     value = *(current++);
                  });

                  return result;
               }
            } // string

            namespace span
            {
               auto value()
               {
                  auto binary = binary::value( 8);
                  strong::execution::span::id result;
                  algorithm::copy( binary, result.underlying());
                  return result;
               };
            } // span

            namespace yaml
            {
               // local yaml writer that has network property -> we use the 
               // gateway network specialization for every message.
               struct Writer : common::serialize::Writer
               {
                  constexpr static auto archive_properties() { return common::serialize::archive::Property::named | common::serialize::archive::Property::network;}

                  Writer()
                     : common::serialize::Writer{ common::serialize::yaml::writer()}
                  {}

                  template< typename V>
                  [[maybe_unused]] Writer& operator << ( V&& value)
                  {
                     common::serialize::value::write( *this, std::forward< V>( value), nullptr);
                     return *this;
                  }
               };

               static_assert( common::serialize::archive::is::network::normalizing< Writer>);
               static_assert( ! common::serialize::archive::is::dynamic< Writer>);
               
            } // yaml


            template< typename M> 
            void example_and_base64( std::ostream& out)
            {
               out << "\n#### example \n```yaml\n"; 

               auto message = protocol::example::message< M>();

               auto writer = yaml::Writer{};
               writer << message;
               writer.consume( out);


               out << "```\n\n";
               out << "Binary representation of the example (network byte ordering in base64):\n`" << protocol::example::representation::base64< M>() << "`\n\n";
            }


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
            common::communication::tcp::message::Header header;

            out << R"(
# casual domain protocol

[//]: # (Attention! this is a generated markdown from casual-gateway-markdown-protocol - do not edit this file!)

Defines what messages are sent between domains and exactly what they contain.

## version

Every message protocol definition will have a _protocol version range_ that defines witch versions 
of the protocol the message is part of.

The protocol version value, that is sent over the wire, is just an integer that starts at `1000` for `1.0`. 

The full table:

version | protocol value
--------|-------------------
)";
            for( auto version : common::range::reverse( message::protocol::versions))
               common::stream::write( out, version, "     | ", std::to_underlying( version), '\n');


            out << R"(
## definitions

* `fixed array`: an array of fixed size where every element is an 8 bit byte.
* `dynamic array`: an array with dynamic size, where every element is an 8 bit byte.

If an attribute name has `element` in it, for example: `services.element.timeout`, the
data is part of an element in a container. You should read it as `container.element.attribute`

## sizes 

`casual` it self does not impose any size restriction on anything. Whatever the platform supports,`casual` is fine with.
There are however restrictions from the `xatmi` specification, regarding service names and such.

`casual` will not apply restriction on sizes unless some specification dictates it, or we got some
sort of proof that a size limitation is needed.

Hence, the maximum sizes below are huge, and it is unlikely that maximum sizes will 
actually work on systems known to `casual` today. But it might in the future... 


## message header 

This header will be the first part of every message below, hence it's name, _header_

message.type is used to dispatch to handler for that particular message, and knows how to (un)marshal and act on the message.

It's probably a good idea (probably the only way) to read the header only, to see how much more you have to read to get
the rest of the message.


)";

            local::format::type( out, CASUAL_NAMED_VALUE( header), {
               { "header.type", "type of the message that the payload contains"},
               { "header.correlation", "correlation id of the message"},
               { "header.size", "the size of the payload that follows"},
            });
         }


         template< typename M>
         void transaction_request( std::ostream& out, M&& message)
         {
            message.trid = common::transaction::id::create();

            local::format::type( out, message, {
                     { "execution", "uuid of the current execution context (breadcrumb)"},
                     { "xid.formatID", "xid format type. if 0 no more information of the xid is transported"},
                     { "xid.gtrid_length", "length of the transaction gtrid part"},
                     { "xid.bqual_length", "length of the transaction branch part"},
                     { "xid.data", "byte array with the size of gtrid_length + bqual_length (max 128)"},
                     { "resource", "RM id of the resource - has to correlate with the reply"},
                     { "flags", "XA flags to be forward to the resource"},
                  });
         }

         template< typename M>
         void transaction_reply( std::ostream& out, M&& message)
         {
            message.trid = common::transaction::id::create();

            local::format::type( out, message, {
                     { "execution", "uuid of the current execution context (breadcrumb)"},
                     { "xid.formatID", "xid format type. if 0 no more information of the xid is transported"},
                     { "xid.gtrid_length", "length of the transaction gtrid part"},
                     { "xid.bqual_length", "length of the transaction branch part"},
                     { "xid.data", "byte array with the size of gtrid_length + bqual_length (max 128)"},
                     { "resource", "RM id of the resource - has to correlate with the request"},
                     { "state", "The state of the operation - If successful XA_OK ( 0)"},
                  });
         }


         void transaction( std::ostream& out)
         {
            {

               using message_type = common::message::transaction::resource::prepare::Request;

               local::message::section< message_type>( out, "##") << R"(

Sent to and received from other domains when one domain wants to prepare a transaction. 

)";
               transaction_request( out, message_type{});
               local::example_and_base64< message_type>( out);

            }

            {
               using message_type = common::message::transaction::resource::prepare::Reply;

               local::message::section< message_type>( out, "##") << R"(

Sent to and received from other domains when one domain is done preparing a transaction. 

)";
               transaction_reply( out, message_type{});
               local::example_and_base64< message_type>( out);
            }

            out << R"(
### Resource commit

)";

            {
               using message_type = common::message::transaction::resource::commit::Request;

               local::message::section< message_type>( out, "##") << R"(

Sent to and received from other domains when one domain wants to commit an already prepared transaction.

)";
               transaction_request( out, message_type{});
               local::example_and_base64< message_type>( out);
            }

            {
               using message_type = common::message::transaction::resource::commit::Reply;

               local::message::section< message_type>( out, "##") << R"(

Reply to a commit request. 

)";
               transaction_reply( out, message_type{});
               local::example_and_base64< message_type>( out);
            }

            {
               using message_type = common::message::transaction::resource::rollback::Request;

               local::message::section< message_type>( out, "##") << R"(

Sent to and received from other domains when one domain wants to rollback an already prepared transaction.
That is, when one or more resources has failed to prepare.

)";
               transaction_request( out, message_type{});
               local::example_and_base64< message_type>( out);
            }

            {
               using message_type =  common::message::transaction::resource::rollback::Reply;

               local::message::section< message_type>( out, "##") << R"(

Reply to a rollback request. 

)";
               transaction_reply( out, message_type{});
               local::example_and_base64< message_type>( out);
            }
         }

         void service_call( std::ostream& out)
         {
            {
               using message_type = common::message::service::call::callee::Request;

               local::message::section< message_type>( out, "##") << R"(

Sent to and received from other domains when one domain wants call a service in the other domain

)";

               message_type request;
               request.trid = common::transaction::id::create();
               request.deadline.remaining = std::chrono::seconds{ 42};
               request.service.name = local::string::value( 128);
               request.parent.service = local::string::value( 128);
               request.buffer.type = local::string::value( 8) + '/' + local::string::value( 16);
               request.buffer.data = local::binary::value( 1024);

               local::format::type( out, request, {
                        { "execution", "uuid of the current execution context (breadcrumb)"},
                        { "service.name.size", "service name size"},
                        { "service.name.data", "byte array with service name"},
                        { "has_value", "if 1, deadline.remaining is propagated"},
                        { "deadline.remaining", "if has_value, the remaining time before deadline (ns)"},
                        { "parent.span", "parent execution span"},
                        { "parent.service.size", "parent service name size"},
                        { "parent.service.data", "byte array with parent service name"},

                        { "xid.formatID", "xid format type. if 0 no more information of the xid is transported"},
                        { "xid.gtrid_length", "length of the transaction gtrid part"},
                        { "xid.bqual_length", "length of the transaction branch part"},
                        { "xid.data", "byte array with the size of gtrid_length + bqual_length (max 128)"},

                        { "flags", "XATMI flags sent to the service"},

                        { "buffer.type.size", "buffer type name size"},
                        { "buffer.type.data", "byte array with buffer type in the form 'type/subtype'"},
                        { "buffer.data.size", "buffer payload size (could be very big)"},
                        { "buffer.data.data", "buffer payload data (with the size of buffer.payload.size)"},
                     });

               local::example_and_base64< message_type>( out);
            }

            {
               using message_type = common::message::service::call::v1_2::callee::Request;

               local::message::section< message_type>( out, "##") << R"(

Sent to and received from other domains when one domain wants call a service in the other domain

)";

               message_type request;
               request.trid = common::transaction::id::create();
               request.service.name = local::string::value( 128);
               request.parent = local::string::value( 128);
               request.buffer.type = local::string::value( 8) + '/' + local::string::value( 16);
               request.buffer.data = local::binary::value( 1024);

               local::format::type( out, request, {
                        { "execution", "uuid of the current execution context (breadcrumb)"},
                        { "service.name.size", "service name size"},
                        { "service.name.data", "byte array with service name"},
                        { "service.timeout.duration", "timeout of the service in use (ns)"},
                        { "parent.size", "parent service name size"},
                        { "parent.data", "byte array with parent service name"},

                        { "xid.formatID", "xid format type. if 0 no more information of the xid is transported"},
                        { "xid.gtrid_length", "length of the transaction gtrid part"},
                        { "xid.bqual_length", "length of the transaction branch part"},
                        { "xid.data", "byte array with the size of gtrid_length + bqual_length (max 128)"},

                        { "flags", "XATMI flags sent to the service"},

                        { "buffer.type.size", "buffer type name size"},
                        { "buffer.type.data", "byte array with buffer type in the form 'type/subtype'"},
                        { "buffer.data.size", "buffer payload size (could be very big)"},
                        { "buffer.data.data", "buffer payload data (with the size of buffer.payload.size)"},
                     });

               local::example_and_base64< message_type>( out);
            }


            {
               using message_type = common::message::service::call::Reply;

               local::message::section< message_type>( out, "##") << R"(

Reply to call request

)";
               message_type message;

               message.transaction_state = decltype( message.transaction_state)::ok;
               message.buffer.type = local::string::value( 8) + '/' + local::string::value( 16);
               message.buffer.data = local::binary::value( 1024);

               local::format::type( out, message, {
                        { "execution", "uuid of the current execution context (breadcrumb)"},
                        { "code.result", "XATMI result/error code, 0 represent OK"},
                        { "code.user", "XATMI user supplied code"},
                        { "transaction_state", "0:ok/absent, 1:rollback, 2:timeout, 3:error"},
                        { "buffer.type.size", "buffer type name size"},
                        { "buffer.type.data", "byte array with buffer type in the form 'type/subtype'"},
                        { "buffer.data.size", "buffer payload size (could be very big)"},
                        { "buffer.data.data", "buffer payload data (with the size of buffer.payload.size)"},
                     });

               local::example_and_base64< message_type>( out);
            }

            {
               using message_type = common::message::service::call::v1_2::Reply;

               local::message::section< message_type>( out, "##") << R"(

Reply to call request

)";
               message_type message;

               message.transaction.trid = common::transaction::id::create();
               message.buffer.type = local::string::value( 8) + '/' + local::string::value( 16);
               message.buffer.data = local::binary::value( 1024);

               local::format::type( out, message, {
                        { "execution", "uuid of the current execution context (breadcrumb)"},

                        { "code.result", "XATMI result/error code, 0 represent OK"},
                        { "code.user", "XATMI user supplied code"},

                        { "transaction.xid.formatID", "xid format type. if 0 no more information of the xid is transported"},
                        { "transaction.xid.gtrid_length", "length of the transaction gtrid part"},
                        { "transaction.xid.bqual_length", "length of the transaction branch part"},
                        { "transaction.xid.data", "byte array with the size of gtrid_length + bqual_length (max 128)"},
                        { "transaction.state", "0:ok, 1:rollback, 2:timeout, 3:error"},

                        { "buffer.type.size", "buffer type name size"},
                        { "buffer.type.data", "byte array with buffer type in the form 'type/subtype'"},
                        { "buffer.data.size", "buffer payload size (could be very big)"},
                        { "buffer.data.data", "buffer payload data (with the size of buffer.payload.size)"},


                     });

               local::example_and_base64< message_type>( out);
            }

         }


         void domain_lifetime( std::ostream& out)
         {    

            {
               using message_type = gateway::message::domain::connect::Request;
               
               local::message::section< message_type>( out, "##") << R"(

Connection requests from another domain that wants to connect

)";
                  message_type message;
                  message.versions = { gateway::message::protocol::Version::v1_2};
                  message.domain.name = "domain-A";

                  local::format::type( out, message, {
                     { "execution", "uuid of the current execution context (breadcrumb)"},
                     { "domain.id", "uuid of the outbound domain"},
                     { "domain.name.size", "size of the outbound domain name"},
                     { "domain.name.data", "dynamic byte array with the outbound domain name"},
                     { "protocol.versions.size", "number of protocol versions outbound domain can 'speak'"},
                     { "protocol.versions.element", "a protocol version "},
                  });

                  local::example_and_base64< message_type>( out);
            }

            {
               using message_type = gateway::message::domain::connect::Reply;
               
               local::message::section< message_type>( out, "##") << R"(

Connection reply with the chosen _protocol version_

)";

               message_type message;
               message.version = gateway::message::protocol::Version::v1_2;
               message.domain.name = "domain-A";

               local::format::type( out, message, {
                  { "execution", "uuid of the current execution context (breadcrumb)"},
                  { "domain.id", "uuid of the inbound domain"},
                  { "domain.name.size", "size of the inbound domain name"},
                  { "domain.name.data", "dynamic byte array with the inbound domain name"},
                  { "protocol.version", "the chosen protocol version to use, or invalid (0) if incompatible"},
               });

               local::example_and_base64< message_type>( out);
            }

            {
               using message_type = gateway::message::domain::disconnect::Request;
               
               local::message::section< message_type>( out, "##") << R"(

Sent from inbound to connected outbound to notify that the connection is about to close, and outbound 
will stop sending new requests. Hence, the inbound can gracefully disconnect.

)";
               message_type message;

               local::format::type( out, message, {
                  { "execution", "uuid of the current execution context (breadcrumb)"},
               });
               
               local::example_and_base64< message_type>( out);
            }

            {
               using message_type = gateway::message::domain::disconnect::Reply;
               
               local::message::section< message_type>( out, "##") << R"(

Confirmation that the outbound has got the disconnect request.

)";
               message_type message;

               local::format::type( out, message, {
                  { "execution", "uuid of the current execution context (breadcrumb)"},
               });

               local::example_and_base64< message_type>( out);
            }
            
         }


         void domain_discovery( std::ostream& out)
         {
            {
               using message_type = casual::domain::message::discovery::Request;

               local::message::section< message_type>( out, "##") << R"(

Sent to and received from other domains when one domain wants to discover information abut the other.

)";
               message_type message;
               message.domain.name = "domain-A";

               message.content.services = { local::string::value( 128)};
               message.content.queues = { local::string::value( 128)};

               local::format::type( out, message, {
                        { "execution", "uuid of the current execution context (breadcrumb)"},
                        { "domain.id", "uuid of the caller domain"},
                        { "domain.name.size", "size of the caller domain name"},
                        { "domain.name.data", "dynamic byte array with the caller domain name"},
                        { "content.services.size", "number of requested services to follow (an array of services)"},
                        { "content.services.element.size", "size of the current service name"},
                        { "content.services.element.data", "dynamic byte array of the current service name"},
                        { "content.queues.size", "number of requested queues to follow (an array of queues)"},
                        { "content.queues.element.size", "size of the current queue name"},
                        { "content.queues.element.data", "dynamic byte array of the current queue name"},
                     });

               local::example_and_base64< message_type>( out);
            }


            {
               using message_type = casual::domain::message::discovery::Reply;

               local::message::section< message_type>( out, "##") << R"(

Sent to and received from other domains when one domain wants to discover information abut the other.

)";            
               auto message = protocol::example::message< message_type>();

               local::format::type( out, message, {
                        { "execution", "uuid of the current execution context (breadcrumb)"},
                        // { "version", "the chosen version - 0 if no compatible version was possible"},
                        { "domain.id", "uuid of the caller domain"},
                        { "domain.name.size", "size of the caller domain name"},
                        { "domain.name.data", "dynamic byte array with the caller domain name"},
                        { "content.services.size", "number of services to follow (an array of services)"},
                        { "content.services.element.name.size", "size of the current service name"},
                        { "content.services.element.name.data", "dynamic byte array of the current service name"},
                        { "content.services.element.category.size", "size of the current service category"},
                        { "content.services.element.category.data", "dynamic byte array of the current service category"},
                        { "content.services.element.transaction", "service transaction mode (auto, atomic, join, none)"},
                        { "content.services.element.timeout.duration", "service timeout (ns)"},
                        { "content.services.element.hops", "number of domain hops to the service (local services has 0 hops)"},
                        { "content.queues.size", "number of requested queues to follow (an array of queues)"},
                        { "content.queues.element.name.size", "size of the current queue name"},
                        { "content.queues.element.name.data", "dynamic byte array of the current queue name"},
                        { "content.queues.element.retry.count", "how many 'retries' the queue has"},
                        { "content.queues.element.retry.delay", "when retried, the amount of us until available"},
                        { "content.queues.element.enable.enqueue", "if queue is enabled for enqueue (1/0)(true/false)"},
                        { "content.queues.element.enable.dequeue", "if queue is enabled for dequeue (1/0)(true/false)"},
                     });

               local::example_and_base64< message_type>( out);
            }

            {
               using message_type = casual::domain::message::discovery::topology::implicit::Update;

               local::message::section< message_type>( out, "##") << R"(

Sent to all inbound connections from a domain when when it gets a new connection or gets this message from an outbound.
When the message is passed "upstream" domains will add its id to the domains array, hence it's possible to mitigate endless 
loops of this message, depending on the topology of the deployment.

)";            
               auto message = protocol::example::message< message_type>();

               local::format::type( out, message, {
                        { "execution", "uuid of the current execution context (breadcrumb)"},
                        { "origin.id", "uuid of the origin domain"},
                        { "origin.name.size", "size of the origin domain name"},
                        { "origin.name.data", "dynamic byte array with the origin domain name"},
                        { "domains.size", "number of domains (an array of domains that has seen this message)"},
                        { "domains.element.id", "uuid of the domain"},
                        { "domains.element.name.size", "size of the domain name"},
                        { "domains.element.name.data", "dynamic byte array with the domain name"},
                     });

               local::example_and_base64< message_type>( out);
            }
         }

         void queue( std::ostream& out)
         {

            {
               using message_type = queue::ipc::message::group::enqueue::Request;

               local::message::section< message_type>( out, "##") << R"(

Represent enqueue request.

)";
               message_type message;

               message.trid = common::transaction::id::create();

               message.name = local::string::value( 128);
               message.message.payload.data = local::binary::value( 1024);


               local::format::type( out, message, {
                        { "execution", "uuid of the current execution context (breadcrumb)"},
                        { "name.size", "size of queue name"},
                        { "name.data", "data of queue name"},
                        { "xid.formatID", "xid format type. if 0 no more information of the xid is transported"},
                        { "xid.gtrid_length", "length of the transaction gtrid part"},
                        { "xid.bqual_length", "length of the transaction branch part"},
                        { "xid.data", "byte array with the size of gtrid_length + bqual_length (max 128)"},
                        { "message.id", "id of the message"},
                        { "message.attributes.properties.size", "length of message properties"},
                        { "message.attributes.properties.data", "data of message properties"},
                        { "message.attributes.reply.size", "length of the reply queue"},
                        { "message.attributes.reply.data", "data of reply queue"},
                        { "message.attributes.available", "when the message is available for dequeue (us since epoch)"},
                        { "message.payload.type.size", "length of the type string"},
                        { "message.payload.type.data", "data of the type string"},
                        { "message.payload.data.size", "size of the payload"},
                        { "message.payload.data.data", "data of the payload"},
                     });

               local::example_and_base64< message_type>( out);   
            }


            // v1.3
            {
               using message_type = queue::ipc::message::group::enqueue::Reply;

               local::message::section< message_type>( out, "##") << R"(

Represent enqueue reply.

)";
               message_type message;

               local::format::type( out, message, {
                        { "execution", "uuid of the current execution context (breadcrumb)"},
                        { "id", "id of the enqueued message"},
                        { "code", "error/result code"},
                     });

               local::example_and_base64< message_type>( out);
            }

            // <= v1.2
            {
               using message_type = queue::ipc::message::group::enqueue::v1_2::Reply;

               local::message::section< message_type>( out, "##") << R"(

Represent enqueue reply.

)";

               message_type message;

               local::format::type( out, message, {
                        { "execution", "uuid of the current execution context (breadcrumb)"},
                        { "id", "id of the enqueued message"},
                     });

               local::example_and_base64< message_type>( out);
            }


            {
               using message_type = queue::ipc::message::group::dequeue::Request;

               local::message::section< message_type>( out, "##") << R"(

Represent dequeue request.

)";

               message_type message;

               message.trid = common::transaction::id::create();
               message.name = local::string::value( 128);


               local::format::type( out, message, {
                        { "execution", "uuid of the current execution context (breadcrumb)"},
                        { "name.size", "size of the queue name"},
                        { "name.data", "data of the queue name"},
                        { "xid.formatID", "xid format type. if 0 no more information of the xid is transported"},
                        { "xid.gtrid_length", "length of the transaction gtrid part"},
                        { "xid.bqual_length", "length of the transaction branch part"},
                        { "xid.data", "byte array with the size of gtrid_length + bqual_length (max 128)"},
                        { "selector.properties.size", "size of the selector properties (ignored if empty)"},
                        { "selector.properties.data", "data of the selector properties (ignored if empty)"},
                        { "selector.id", "selector uuid (ignored if 'empty'"},
                        { "block", "dictates if this is a blocking call or not"},
                     });

               local::example_and_base64< message_type>( out);
            }

            {
               using message_type = queue::ipc::message::group::dequeue::Reply;

               local::message::section< message_type>( out, "##") << R"(

Represent dequeue reply.

)";
               message_type message;

               message.message.emplace();
               message.message->attributes.properties = local::string::value( 128);
               message.message->attributes.reply = local::string::value( 128);
               message.message->payload.type = local::string::value( 128);
               message.message->payload.data = local::binary::value( 1024); 
               message.code = decltype( message.code)::system;

               local::format::type( out, message, {
                  { "execution", "uuid of the current execution context (breadcrumb)"},
                  { "has_value", "if 1 message has content, if 0 no more information of the message is transported"},
                  { "message.id", "id of the message"},
                  { "message.attributes.properties.size", "length of message properties"},
                  { "message.attributes.properties.data", "data of message properties"},
                  { "message.attributes.reply.size", "length of the reply queue"},
                  { "message.attributes.reply.data", "data of reply queue"},
                  { "message.attributes.available", "when the message was available for dequeue (us since epoch)"},
                  { "message.payload.type.size", "length of the type string"},
                  { "message.payload.type.data", "data of the type string"},
                  { "message.payload.data.size", "size of the payload"},
                  { "message.payload.data.data", "data of the payload"},
                  { "message.redelivered", "how many times the message has been redelivered"},
                  { "message.timestamp", "when the message was enqueued (us since epoch)"},
                  { "code", "result/error code"},
               });

               local::example_and_base64< message_type>( out);
            }

            // <= v1.2
            {
               using message_type = queue::ipc::message::group::dequeue::v1_2::Reply;

               local::message::section< message_type>( out, "##") << R"(

Represent dequeue reply.

)";
               message_type message;

               auto& actual_message = message.message.emplace_back();
               actual_message.attributes.properties = local::string::value( 128);
               actual_message.attributes.reply = local::string::value( 128);
               actual_message.payload.type = local::string::value( 128);
               actual_message.payload.data = local::binary::value( 1024); 

               local::format::type( out, message, {
                        { "execution", "uuid of the current execution context (breadcrumb)"},
                        { "message.size", "number of messages dequeued"},
                        { "message.element.id", "id of the message"},
                        { "message.element.attributes.properties.size", "length of message properties"},
                        { "message.element.attributes.properties.data", "data of message properties"},
                        { "message.element.attributes.reply.size", "length of the reply queue"},
                        { "message.element.attributes.reply.data", "data of reply queue"},
                        { "message.element.attributes.available", "when the message was available for dequeue (us since epoch)"},
                        { "message.element.payload.type.size", "length of the type string"},
                        { "message.element.payload.type.data", "data of the type string"},
                        { "message.element.payload.data.size", "size of the payload"},
                        { "message.element.payload.data.data", "data of the payload"},
                        { "message.element.redelivered", "how many times the message has been redelivered"},
                        { "message.element.timestamp", "when the message was enqueued (us since epoch)"},
                     });

               local::example_and_base64< message_type>( out);
            }
         }

         void conversation( std::ostream& out)
         {

            {
               using message_type = common::message::conversation::connect::callee::Request;

               local::message::section< message_type>( out, "##") << R"(

Sent to establish a conversation

)";
               message_type message;
               message.deadline.remaining = std::chrono::seconds{ 42};
               message.service.name = local::string::value( 128);
               message.parent.service = local::string::value( 128);
               message.parent.span = local::span::value();
               message.trid = common::transaction::id::create();
               message.buffer.type = local::string::value( 8) + '/' + local::string::value( 16);
               message.buffer.data = local::binary::value( 1024);
               using Duplex = decltype( message.duplex);
               message.duplex = Duplex::receive;

               local::format::type( out, message, {
                        { "execution", "uuid of the current execution context (breadcrumb)"},
                        { "service.name.size", "size of the service name"},
                        { "service.name.data", "data of the service name"},
                        { "has_value", "if 1, deadline.remaining is propagated"},
                        { "deadline.remaining", "if has_value, the remaining time before deadline (ns)"},
                        { "parent.span", "parent execution span"},
                        { "parent.service.size", "parent service name size"},
                        { "parent.service.data", "byte array with parent service name"},
                        { "xid.formatID", "xid format type. if 0 no more information of the xid is transported"},
                        { "xid.gtrid_length", "length of the transaction gtrid part"},
                        { "xid.bqual_length", "length of the transaction branch part"},
                        { "xid.data", "byte array with the size of gtrid_length + bqual_length (max 128)"},
                        { "duplex", string::compose( "in what duplex the callee shall enter (", Duplex::receive, ":", std::to_underlying( Duplex::receive), ", ", Duplex::send, ":", std::to_underlying( Duplex::send),')') },
                        { "buffer.type.size", "buffer type name size"},
                        { "buffer.type.data", "byte array with buffer type in the form 'type/subtype'"},
                        { "buffer.data.size", "buffer payload size (could be very big)"},
                        { "buffer.data.data", "buffer payload data (with the size of buffer.payload.size)"},
                     });

               local::example_and_base64< message_type>( out);
            }

            {
               using message_type = common::message::conversation::connect::v1_2::callee::Request;

               local::message::section< message_type>( out, "##") << R"(

Sent to establish a conversation

)";
               message_type message;

               message.service.name = local::string::value( 128);
               message.parent = local::string::value( 128);
               message.trid = common::transaction::id::create();
               message.buffer.type = local::string::value( 8) + '/' + local::string::value( 16);
               message.buffer.data = local::binary::value( 1024);
               using Duplex = decltype( message.duplex);
               message.duplex = Duplex::receive;

               local::format::type( out, message, {
                        { "execution", "uuid of the current execution context (breadcrumb)"},
                        { "service.name.size", "size of the service name"},
                        { "service.name.data", "data of the service name"},
                        { "service.timeout.duration", "timeout (in ns"},
                        { "parent.size", "parent service name size"},
                        { "parent.data", "byte array with parent service name"},
                        { "xid.formatID", "xid format type. if 0 no more information of the xid is transported"},
                        { "xid.gtrid_length", "length of the transaction gtrid part"},
                        { "xid.bqual_length", "length of the transaction branch part"},
                        { "xid.data", "byte array with the size of gtrid_length + bqual_length (max 128)"},
                        { "duplex", string::compose( "in what duplex the callee shall enter (", Duplex::receive, ":", std::to_underlying( Duplex::receive), ", ", Duplex::send, ":", std::to_underlying( Duplex::send),')') },
                        { "buffer.type.size", "buffer type name size"},
                        { "buffer.type.data", "byte array with buffer type in the form 'type/subtype'"},
                        { "buffer.data.size", "buffer payload size (could be very big)"},
                        { "buffer.data.data", "buffer payload data (with the size of buffer.payload.size)"},
                     });

               local::example_and_base64< message_type>( out);
            }

            {
               using message_type = common::message::conversation::connect::Reply;

               local::message::section< message_type>( out, "##") << R"(

Reply for a conversation

)";
               message_type message;

               local::format::type( out, message, {
                        { "execution", "uuid of the current execution context (breadcrumb)"},
                        { "code.result", "result code of the connection attempt"},
                     });

               local::example_and_base64< message_type>( out);
            }

            {
               using message_type = common::message::conversation::callee::Send;

               local::message::section< message_type>( out, "##") << R"(

Represent a message sent 'over' an established connection

)";
               message_type message;

               message.buffer.type = local::string::value( 8) + '/' + local::string::value( 16);
               message.buffer.data = local::binary::value( 1024); 
               using Duplex = decltype( message.duplex);
               message.duplex = Duplex::receive;

               local::format::type( out, message, {
                        { "execution", "uuid of the current execution context (breadcrumb)"},
                        { "duplex", string::compose( "in what duplex the callee shall enter (", Duplex::receive, ":", std::to_underlying( Duplex::receive), ", ", Duplex::send, ":", std::to_underlying( Duplex::send),')') },
                        { "events", "events"},
                        { "code.result", "status of the connection"},
                        { "code.user", "user code, if callee did a tpreturn and supplied user-code"},
                        { "buffer.type.size", "buffer type name size"},
                        { "buffer.type.data", "byte array with buffer type in the form 'type/subtype'"},
                        { "buffer.data.size", "buffer payload size (could be very big)"},
                        { "buffer.data.data", "buffer payload data (with the size of buffer.payload.size)"},
                     });

               local::example_and_base64< message_type>( out);
            }

            {
               using message_type = common::message::conversation::Disconnect;

               local::message::section< message_type>( out, "##") << R"(

Sent to abruptly disconnect the conversation

)";
               message_type message;

               local::format::type( out, message, {
                        { "execution", "uuid of the current execution context (breadcrumb)"},
                        { "events", "events"},
                     });

               local::example_and_base64< message_type>( out);
            }

            {
               using message_type = common::message::conversation::connect::v1_2::callee::Request;

               local::message::section< message_type>( out, "##") << R"(

Sent to establish a conversation

)";
               message_type message;

               message.service.name = local::string::value( 128);
               message.parent = local::string::value( 128);
               message.trid = common::transaction::id::create();
               message.buffer.type = local::string::value( 8) + '/' + local::string::value( 16);
               message.buffer.data = local::binary::value( 1024);
               using Duplex = decltype( message.duplex);
               message.duplex = Duplex::receive;

               local::format::type( out, message, {
                        { "execution", "uuid of the current execution context (breadcrumb)"},
                        { "service.name.size", "size of the service name"},
                        { "service.name.data", "data of the service name"},
                        { "service.timeout.duration", "timeout (in ns"},
                        { "parent.size", "size of the parent service name (the caller)"},
                        { "parent.data", "data of the parent service name (the caller)"},
                        { "xid.formatID", "xid format type. if 0 no more information of the xid is transported"},
                        { "xid.gtrid_length", "length of the transaction gtrid part"},
                        { "xid.bqual_length", "length of the transaction branch part"},
                        { "xid.data", "byte array with the size of gtrid_length + bqual_length (max 128)"},
                        { "duplex", string::compose( "in what duplex the callee shall enter (", Duplex::receive, ":", std::to_underlying( Duplex::receive), ", ", Duplex::send, ":", std::to_underlying( Duplex::send),')') },
                        { "buffer.type.size", "buffer type name size"},
                        { "buffer.type.data", "byte array with buffer type in the form 'type/subtype'"},
                        { "buffer.data.size", "buffer payload size (could be very big)"},
                        { "buffer.data.data", "buffer payload data (with the size of buffer.payload.size)"},
                     });
            }
            
         }
         

         void protocol()
         {
            common::terminal::output::directive().plain();
            
            message_header( std::cout);
            domain_lifetime( std::cout);
            domain_discovery( std::cout);
            service_call( std::cout);
            transaction( std::cout);
            queue( std::cout);
            conversation( std::cout);
         }
      } // print

   } // gateway::documentation::protocol
} // casual

int main( int argc, const char** argv)
{
   return casual::common::exception::main::log::guard( []()
   {
      casual::gateway::documentation::protocol::print::protocol();
   });
}





