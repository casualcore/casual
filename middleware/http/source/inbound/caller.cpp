//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "http/common.h"
#include "http/inbound/caller.h"
#include "casual/buffer/field.h"
#include "common/service/call/context.h"
#include "common/transcode.h"
#include "common/string.h"
#include "common/string/compose.h"
#include "common/log.h"

#include "common/code/raise.h"
#include "common/code/casual.h"
#include "common/code/category.h"
#include "common/code/xatmi.h"
#include "common/exception/capture.h"
#include "common/environment.h"
#include "common/execute.h"

#include <vector>
#include <unordered_map>
#include <string>

namespace casual
{
   namespace http
   {
      namespace inbound
      {
         namespace local
         {
            namespace
            {
               auto& configuration()
               {
                  static struct Configuration
                  {
                     const bool force_binary_base64 = common::environment::variable::get( "CASUAL_HTTP_FORCE_BINARY_BASE64", false);
                  } result;

                  return result;
               }

               namespace lookup
               {
                  using LookupStore = std::unordered_map< long, common::service::non::blocking::Lookup>;

                  // global store for Lookup-objects
                  LookupStore store;

                  namespace key
                  {
                     // Must be able to tell if key is set or not
                     // No practical risc to overflow
                     long next{ 1};
                  }

                  long add( const std::string& service)
                  {
                     store.emplace( key::next, common::service::non::blocking::Lookup{ service});
                     return key::next++;
                  }

                  LookupStore::iterator find( long key)
                  {
                     auto found = store.find( key);
                     
                     if( found != store.end())
                        return found;

                     common::code::raise::error( common::code::casual::invalid_argument, "could not find lookup object for key: ", key);
                  }

                  common::service::non::blocking::Lookup consume( long key)
                  {
                     auto found = find( key);
                     auto result = std::move( found->second);
                     store.erase( found);
                     return result;
                  }

                  common::service::non::blocking::Lookup& get( long key) 
                  {
                     return find( key)->second;
                  }

                  void remove( long key) { store.erase( key); }
               }

               namespace header
               {
                  common::service::header::Fields copy( const http_header_type& headers)
                  {
                     const http::Trace trace("casual::http::inbound::local::header::copy");

                     common::service::header::Fields result;

                     for( auto& header : common::range::make( headers.data, headers.size))
                        result.emplace_back( header.key, header.value);

                     return result;
                  }
                  http_header_type copy( const common::service::header::Fields& headers)
                  {
                     const http::Trace trace("casual::http::inbound::local::header::copy");

                     http_header_type result;

                     result.size = headers.size();
                     result.data = reinterpret_cast< header_data_type*>( malloc( result.size * sizeof( header_data_type)));

                     auto index = 0;
                     for( const auto& header : headers)
                     {
                        // key
                        std::copy( header.key.begin(), header.key.end(), result.data[ index].key);
                        result.data[ index].key[ std::min( header.key.size(), sizeof( result.data[index].key) - 1)] = '\0';

                        // value
                        std::copy( header.value.begin(), header.value.end(), result.data[ index].value);
                        result.data[ index].value[ std::min( header.value.size(), sizeof( result.data[index].value) - 1)] = '\0';

                        index++;
                     }

                     return result;
                  }
                  namespace codes
                  {
                     common::service::header::Fields add( long code, long usercode)
                     {
                        const http::Trace trace("casual::http::inbound::local::header::codes::add");

                        common::service::header::Fields fields;
                        fields.emplace_back( 
                           http::header::name::result::code, 
                           http::header::value::result::code( static_cast< common::code::xatmi>( code))
                           );
                        fields.emplace_back( 
                           http::header::name::result::user::code, 
                           http::header::value::result::user::code( usercode)
                           );                        
                        return fields;
                     }
                  }
               }

               namespace parameter
               {
                  using container = std::vector< std::pair< std::string, std::string> >;
                  container copy( const http_buffer_type& buffer)
                  {
                     container result;
                     const auto parameters = common::string::split( buffer.data, '&');
                     for (const auto& parameter : parameters)
                     {
                        const auto parts = common::string::split( parameter, '=');
                        if (parts.size() == 2)
                        {
                           result.emplace_back( std::make_pair( parts[0], parts[1]));
                        }
                     }
                     return result;
                  }

                  platform::binary::type make_json( const parameter::container& parameters)
                  {
                     platform::binary::type buffer;
                     buffer.push_back('{');
                     for ( const auto& parameter : parameters)
                     {
                        buffer.push_back('"');
                        common::algorithm::append( parameter.first, buffer);
                        buffer.push_back('"');
                        buffer.push_back(':');
                        buffer.push_back('"');
                        common::algorithm::append( parameter.second, buffer);
                        buffer.push_back('"');
                        buffer.push_back(',');
                     }
                     if ( buffer.back() == ',') buffer.pop_back();
                     buffer.push_back('}');

                     common::log::line( common::verbose::log, "parameters: ", std::string( buffer.begin(), buffer.end()));
                     return buffer;
                  }

                  platform::binary::type make_xml( const parameter::container& parameters)
                  {
                     platform::binary::type buffer;
                     static const std::string root("root");

                     buffer.push_back('<');
                     std::copy( root.begin(), root.end(), std::back_inserter( buffer));
                     buffer.push_back('>');
                     for ( const auto& parameter : parameters)
                     {
                        buffer.push_back('<');
                        common::algorithm::append( parameter.first, buffer);
                        buffer.push_back('>');
                        common::algorithm::append( parameter.second, buffer);
                        buffer.push_back('<');
                        buffer.push_back('/');
                        common::algorithm::append( parameter.first, buffer);
                        buffer.push_back('>');
                     }
                     buffer.push_back('<');
                     buffer.push_back('/');
                     std::copy( root.begin(), root.end(), std::back_inserter( buffer));
                     buffer.push_back('>');

                     common::log::line( common::verbose::log, "parameters: ", std::string( buffer.begin(), buffer.end()));
                     return buffer;
                  }
               }

               namespace buffer
               {
                  template< typename Type>
                  http_buffer_type copy( const Type& input)
                  {
                     http_buffer_type output;
                     output.data = reinterpret_cast<char*>( malloc( input.size()));
                     output.size = input.size();
                     common::algorithm::copy( input, output.data);
                     return output;
                  }

                  platform::binary::type assemble( casual_http_buffer_type* transport, const parameter::container& parameters, const std::string& protocol)
                  {
                     if ( protocol == http::protocol::json && transport->payload.size < 1)
                        return parameter::make_json( parameters);
                     else if ( protocol == http::protocol::xml && transport->payload.size < 1)
                        return parameter::make_xml( parameters);
                        
                     return platform::binary::type( transport->payload.data, transport->payload.data + transport->payload.size);
                  }
               } // buffer

               namespace exception
               {
                  long handle( casual_http_buffer_type* transport)
                  {
                     auto usercode{ 0};
                     try
                     {
                        throw;
                     }
                     catch( common::service::call::Fail& exception)
                     {
                        transport->code = common::cast::underlying( common::code::xatmi::service_fail);

                        if( local::configuration().force_binary_base64)
                           http::buffer::transcode::to::wire( exception.result.buffer);
                        
                        transport->payload = buffer::copy( exception.result.buffer.memory);
                        usercode = exception.result.user;
                     }
                     catch( ...)
                     {
                        auto error = common::exception::capture();

                        if( error.code() == common::code::xatmi::no_message)
                           return AGAIN; // No reply yet, try again later

                        if( common::code::is::category< common::code::xatmi>( error.code()))
                           transport->code = error.code().value();
                        else 
                           transport->code = common::cast::underlying( common::code::xatmi::os);

                        transport->payload = buffer::copy( common::string::compose( error));
                     }

                     // Handle reply headers
                     auto header = header::codes::add( transport->code, usercode);
                     transport->header_out = header::copy( header);

                     return ERROR;
                  }
               }

               namespace service
               {

                  // Contacts service-manager and gets a lookup object.
                  // When lookup object returns true, the service-manager is
                  // ready to handle the actual call
                  long lookup( casual_http_buffer_type* transport)
                  {
                     const http::Trace trace("casual::http::inbound::local::service::lookup");
                     const auto& service = transport->service;

                     common::log::line( common::verbose::log, "service: ", service);

                     try
                     { 
                        if( transport->lookup_key == 0) 
                           transport->lookup_key = lookup::add( service);

                        const auto key = transport->lookup_key;
                        common::log::line( common::verbose::log, "key: ", key);

                        return lookup::get( key) ? OK : AGAIN;
                     }
                     catch( ...)
                     {
                        lookup::remove( transport->lookup_key);
                        return exception::handle( transport);
                     }
                  }

                  long send( casual_http_buffer_type* transport)
                  {
                     const http::Trace trace("casual::http::inbound::local::service::send");
                     const auto& protocol = transport->protocol;
                     const auto& service = transport->service;

                     common::log::line( common::verbose::log, "protocol: ", protocol);
                     common::log::line( common::verbose::log, "service: ", service);

                     transport->context = cTPACALL;

                     try
                     {
                        // Lookup done, ready to call
                        auto lookup = lookup::consume( transport->lookup_key);
                        common::log::line( common::verbose::log, "lookup: ", lookup);
                        
                        // make sure we discard the reservation if something fails...
                        auto discard_guard = common::execute::scope( [ correlation = lookup.correlation()]()
                        { 
                           common::service::lookup::discard( correlation);
                        });

                        // Handle header
                        const auto& header = header::copy( transport->header_in);

                        // Handle parameter
                        const auto& parameters = parameter::copy( transport->parameter);
                        common::log::line( common::verbose::log, "parameters: ", parameters);

                        common::buffer::Payload payload( protocol::convert::to::buffer( protocol), buffer::assemble( transport, parameters, protocol));

                        // possible transcode buffer to wire-encoding
                        if( local::configuration().force_binary_base64)
                           http::buffer::transcode::from::wire( payload);

                        // Call service
                        namespace call = common::service::call;
                        transport->descriptor = call::context().async( 
                           std::move( lookup), 
                           common::buffer::payload::Send( payload),
                           header,
                           call::async::Flag::no_block);

                        discard_guard.release();
                     }
                     catch( ...)
                     {
                        return exception::handle( transport);
                     }

                     transport->code = common::cast::underlying( common::code::xatmi::ok);

                     return OK;
                  }

                  long receive( casual_http_buffer_type* const transport)
                  {
                     const http::Trace trace("casual::http::inbound::service::local::receive");
                     const auto& protocol = transport->protocol;
                     const auto& descriptor = transport->descriptor;
                     const auto& service = transport->service;

                     common::log::line( common::verbose::log, "protocol: ", protocol);
                     common::log::line( common::verbose::log, "service: ", service);
                     common::log::line( common::verbose::log, "descriptor: ", descriptor);

                     transport->context = cTPGETRPLY;

                     try
                     {
                        const http::Trace trace("casual::http::inbound::local::service::receive reply");

                        // Handle reply
                        namespace call = common::service::call;
                        auto reply = call::context().reply( descriptor, call::reply::Flag::no_block);

                        if( reply.buffer.null())
                        {
                           ::strncpy( transport->protocol, http::protocol::null, sizeof( transport->protocol));
                           common::algorithm::copy( "NULL", reply.buffer.memory);
                           common::log::line( common::verbose::log, "protocol: ", transport->protocol);
                        }
                        else 
                        {
                           // possible transcode buffer to wire-encoding
                           if( local::configuration().force_binary_base64)
                              http::buffer::transcode::to::wire( reply.buffer);
                        }
                           
                        transport->payload = buffer::copy( reply.buffer.memory);

                        // Handle reply headers
                        auto header = header::codes::add( common::cast::underlying( common::code::xatmi::ok), reply.user);
                        transport->header_out = header::copy( header);


                        return OK;
                     }
                     catch( ...)
                     {
                        return exception::handle( transport);
                     }
                  }

                  long cancel( casual_http_buffer_type* transport)
                  {
                     const http::Trace trace("casual::http::inbound::local::service::cancel");
                     const auto& protocol = transport->protocol;
                     const auto& descriptor = transport->descriptor;
                     const auto& service = transport->service;

                     common::log::line( common::verbose::log, "protocol: ", protocol);
                     common::log::line( common::verbose::log, "service: ", service);
                     common::log::line( common::verbose::log, "descriptor: ", descriptor);

                     transport->context = cTPGETRPLY;

                     try
                     {
                        // Handle reply
                        namespace call = common::service::call;
                        call::context().cancel( descriptor);

                        return OK;
                     }
                     catch( ...)
                     {
                        return exception::handle( transport);
                     }
                  }
               } // service
            } // <unnamed>
         } // local
      } // inbound
   } // http
} // casual

long casual_xatmi_lookup( casual_http_buffer_type* data)
{
   return casual::http::inbound::local::service::lookup( data);
}

long casual_xatmi_send( casual_http_buffer_type* data)
{
   return casual::http::inbound::local::service::send( data);
}

long casual_xatmi_receive( casual_http_buffer_type* data)
{
   return casual::http::inbound::local::service::receive( data);
}

long casual_xatmi_cancel( casual_http_buffer_type* data)
{
   return casual::http::inbound::local::service::cancel( data);
}

