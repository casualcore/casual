//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include <vector>

#include "http/common.h"
#include "http/inbound/caller.h"
#include "buffer/field.h"
#include "common/service/call/context.h"
#include "common/transcode.h"
#include "common/exception/system.h"
#include "common/string.h"


namespace std
{
   std::ostream& operator<<( std::ostream& stream, const std::vector< std::pair< std::string, std::string >>& input)
   {
      for (const auto& data : input)
      {
         stream << "key: [" << data.first << "], value: [" << data.second << "]" << '\n';
      }
      return stream;
   }
}
namespace casual
{
   using namespace common;

   namespace http
   {
      namespace inbound
      {
         namespace
         {
            namespace header
            {
               common::service::header::Fields copy( const header_type& headers)
               {
                  const http::Trace trace("casual::http::inbound::header::copy");

                  common::service::header::Fields result;

                  for( auto& header : common::range::make( headers.data, headers.size))
                  {
                     result.emplace_back( header.key, header.value);
                  }

                  return result;
               }
               header_type copy( const common::service::header::Fields& headers)
               {
                  const http::Trace trace("casual::http::inbound::header::copy");

                  header_type result;

                  result.size = headers.size();
                  result.data = reinterpret_cast< header_data_type*>( malloc( result.size * sizeof (header_data_type)));

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
                  void add( long code, long usercode)
                  {
                     const http::Trace trace("casual::http::inbound::header::codes::add");

                     common::service::header::fields().emplace_back( 
                        common::service::header::Field( http::header::name::result::code, std::to_string( code)));
                     common::service::header::fields().emplace_back( 
                        common::service::header::Field( http::header::name::result::user::code, std::to_string( usercode)));
                  }
               }
            }

            namespace parameter
            {
               using container = std::vector< std::pair< std::string, std::string> >;
               container copy( const buffer_type& buffer)
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

               common::platform::binary::type make_json( const parameter::container& parameters)
               {
                  common::platform::binary::type buffer;
                  buffer.push_back('{');
                  for ( const auto& parameter : parameters)
                  {
                     buffer.push_back('"');
                     algorithm::append( parameter.first, buffer);
                     buffer.push_back('"');
                     buffer.push_back(':');
                     buffer.push_back('"');
                     algorithm::append( parameter.second, buffer);
                     buffer.push_back('"');
                     buffer.push_back(',');
                  }
                  if ( buffer.back() == ',') buffer.pop_back();
                  buffer.push_back('}');

                  log::line( verbose::log, "parameters: ", std::string( buffer.begin(), buffer.end()));
                  return buffer;
               }

               common::platform::binary::type make_xml( const parameter::container& parameters)
               {
                  common::platform::binary::type buffer;
                  static const std::string root("root");

                  buffer.push_back('<');
                  std::copy( root.begin(), root.end(), std::back_inserter( buffer));
                  buffer.push_back('>');
                  for ( const auto& parameter : parameters)
                  {
                     buffer.push_back('<');
                     algorithm::append( parameter.first, buffer);
                     buffer.push_back('>');
                     algorithm::append( parameter.second, buffer);
                     buffer.push_back('<');
                     buffer.push_back('/');
                     algorithm::append( parameter.first, buffer);
                     buffer.push_back('>');
                  }
                  buffer.push_back('<');
                  buffer.push_back('/');
                  std::copy( root.begin(), root.end(), std::back_inserter( buffer));
                  buffer.push_back('>');

                  log::line( verbose::log, "parameters: ", std::string( buffer.begin(), buffer.end()));
                  return buffer;
               }
            }

            namespace buffer
            {
               namespace input
               {
                  common::buffer::Payload transform( common::buffer::Payload&& buffer, const std::string& protocol)
                  {
                     const http::Trace trace("casual::http::inbound::buffer::input::transform");
                     auto transcode_base64 = [&]( common::buffer::Payload&& buffer)
                     {
                        common::buffer::Payload result;
                        result.type = std::move( buffer.type);
                        result.memory = common::transcode::base64::decode( std::string( buffer.memory.begin(), buffer.memory.end()));

                        return result;
                    };

                     auto transcode_none = [&]( common::buffer::Payload&& buffer)
                     {
                        common::buffer::Payload result;
                        result.type = std::move( buffer.type);
                        result.memory = std::move( buffer.memory);

                        return result;
                     };

                     const static auto mapping = std::map< std::string, std::function<common::buffer::Payload(common::buffer::Payload&&)> >
                     {
                        { http::protocol::binary(), transcode_base64},
                        { http::protocol::x_octet(), transcode_base64},
                        { http::protocol::field(), transcode_base64},
                        { http::protocol::json(), transcode_none},
                        { http::protocol::xml(), transcode_none}
                     };

                     auto found = common::algorithm::find( mapping, protocol);
                     if( found)
                     {
                        log::line( verbose::log, "found protocol transcoder: ", found->first);

                        // Do the actual transforming
                        return found->second( std::move( buffer));
                     }

                     log::line( log::category::error, "failed to find transcoder for protocol: ", protocol);
                     return {};
                  }
               }
               namespace output
               {
                  common::buffer::Payload transform( common::buffer::Payload&& buffer, const std::string& protocol)
                  {
                     const http::Trace trace("casual::http::inbound::buffer::output::transform");
                     auto transcode_base64 = [&]( common::buffer::Payload&& buffer)
                     {
                        common::buffer::Payload result;
                        result.type = std::move( buffer.type);
                        auto encoded = common::transcode::base64::encode( buffer.memory);
                        result.memory = common::platform::binary::type( encoded.begin(), encoded.end());

                        return result;
                     };

                     auto transcode_none = [&]( common::buffer::Payload&& buffer)
                     {
                        common::buffer::Payload result;
                        result.type = std::move( buffer.type);
                        result.memory = std::move( buffer.memory);

                        return result;
                     };

                     const static auto mapping = std::map< std::string, std::function<common::buffer::Payload(common::buffer::Payload&&)> >
                     {
                        { http::protocol::binary(), transcode_base64},
                        { http::protocol::x_octet(), transcode_base64},
                        { http::protocol::field(), transcode_base64},
                        { http::protocol::json(), transcode_none},
                        { http::protocol::xml(), transcode_none}
                     };

                     auto found = common::algorithm::find( mapping, protocol);
                     if (found)
                     {
                        //
                        // Do the actual transforming
                        //
                        return found->second( std::move(buffer));
                     }

                     return {};
                  }
               }

               template< typename Type>
               buffer_type copy( const Type& input)
               {
                  buffer_type output;
                  output.data = reinterpret_cast<char*>( malloc( input.size()));
                  output.size = input.size();
                  common::algorithm::copy( input, output.data);
                  return output;
               }

               common::platform::binary::type assemble( casual_buffer_type* transport, const parameter::container& parameters, const std::string& protocol)
               {
                  common::platform::binary::type buffer;
                  if ( protocol == http::protocol::json() && transport->payload.size < 1)
                  {
                     buffer = parameter::make_json( parameters);
                  }
                  else if ( protocol == http::protocol::xml() && transport->payload.size < 1)
                  {
                     buffer = parameter::make_xml( parameters);
                  }
                  else
                  {
                     buffer = common::platform::binary::type( transport->payload.data, transport->payload.data + transport->payload.size);
                  }
                  return buffer;
               }
            }

            namespace exception
            {
               int handle( casual_buffer_type* transport)
               {
                  auto usercode{0};
                  try
                  {
                     throw;
                  }
                  catch ( const common::service::call::Fail& exception)
                  {
                     transport->code = cast::underlying( code::xatmi::service_fail);
                     transport->payload = buffer::copy( exception.code().message());
                     usercode = exception.result.user;
                  }
                  catch ( const common::exception::xatmi::exception& exception)
                  {
                     transport->code = exception.code().value();
                     transport->payload = buffer::copy( exception.code().message());
                  }
                  catch ( const common::exception::system::invalid::Argument& exception)
                  {
                     transport->code = exception.code().value();
                     transport->payload = buffer::copy( std::string( exception.what()));
                  }
                  catch (...)
                  {
                     transport->code = common::code::make_error_code( common::code::xatmi::os).value();
                     transport->payload = buffer::copy( std::string( "unknown error"));
                  }

                  //
                  // Handle reply headers
                  //
                  header::codes::add( transport->code, usercode);
                  transport->header_out = header::copy( common::service::header::fields());

                  return ERROR;
               }
            }


            long send( casual_buffer_type* transport)
            {
               const http::Trace trace("casual::http::inbound::send");
               const auto& protocol = transport->protocol;
               const auto& service = transport->service;

               log::line( verbose::log, "protocol: ", protocol);
               log::line( verbose::log, "service: ", service);

               transport->context = cTPACALL;

               try
               {
                  // use this in transport, and check if it's ready?
                  // if( transport->lookup)
                  //   ... do this stuff and make the call

                  common::service::non::blocking::Lookup lookup{ service};

                  // Handle header
                  const auto& header = header::copy( transport->header_in);
                  common::service::header::fields( header);

                  // Handle parameter
                  const auto& parameters = parameter::copy( transport->parameter);
                  log::line( verbose::log, "parameters: ", parameters);

                  // Handle buffer
                  auto buffer = buffer::assemble( transport, parameters, protocol);
                  common::buffer::Payload payload( protocol::convert::to::buffer( protocol), buffer);
                  payload = buffer::input::transform( std::move( payload), protocol);

                  // Call service
                  namespace call = common::service::call;
                  transport->descriptor = call::context().async( std::move( lookup), common::buffer::payload::Send( payload), call::async::Flag::no_block);
               }
               catch (...)
               {
                  //
                  // Header cleanup
                  //
                  common::service::header::fields().clear();

                  return exception::handle( transport);
               }

               transport->code = cast::underlying( common::code::xatmi::ok);

               //
               // Header cleanup
               //
               common::service::header::fields().clear();

               return OK;
            }

            long receive( casual_buffer_type* transport)
            {
               const http::Trace trace("casual::http::inbound::receive");
               const auto& protocol = transport->protocol;
               const auto& descriptor = transport->descriptor;
               const auto& service = transport->service;

               log::line( verbose::log, "protocol: ", protocol);
               log::line( verbose::log, "service: ", service);
               log::line( verbose::log, "descriptor: ", descriptor);

               transport->context = cTPGETRPLY;

               try
               {
                  //
                  // Handle reply
                  //
                  namespace call = common::service::call;
                  auto reply = call::context().reply( descriptor, call::reply::Flag::no_block);

                  //
                  // Handle buffer
                  //
                  auto output = buffer::output::transform( std::move( reply.buffer), protocol);
                  transport->payload = buffer::copy( output.memory);

                  //
                  // Handle reply headers
                  //
                  header::codes::add( cast::underlying( common::code::xatmi::ok), reply.user);
                  transport->header_out = inbound::header::copy( common::service::header::fields());

                  //
                  // Header cleanup
                  //
                  common::service::header::fields().clear();

                  return OK;
               }
               catch ( const common::exception::xatmi::no::Message&)
               {
                  //
                  // No reply yet, try again later
                  //
                  return AGAIN;
               }
               catch (...)
               {
                  return exception::handle( transport);
               }
            }

            long cancel( casual_buffer_type* transport)
            {
               const http::Trace trace("casual::http::inbound::cancel");
               const auto& protocol = transport->protocol;
               const auto& descriptor = transport->descriptor;
               const auto& service = transport->service;

               log::line( verbose::log, "protocol: ", protocol);
               log::line( verbose::log, "service: ", service);
               log::line( verbose::log, "descriptor: ", descriptor);

               transport->context = cTPGETRPLY;

               try
               {
                  //
                  // Handle reply
                  //
                  namespace call = common::service::call;
                  call::context().cancel( descriptor);

                  return OK;
               }
               catch (...)
               {
                  return exception::handle( transport);
               }
            }
         }
      }
   }
}

long casual_xatmi_send( casual_buffer_type* data)
{
   return casual::http::inbound::send(data);
}

long casual_xatmi_receive( casual_buffer_type* data)
{
   return casual::http::inbound::receive(data);
}

long casual_xatmi_cancel( casual_buffer_type* data)
{
   return casual::http::inbound::cancel(data);
}

