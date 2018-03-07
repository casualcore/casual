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
   std::ostream& operator<<( std::ostream& stream, std::vector< std::pair< std::string, std::string >> input)
   {
      for (const auto data : input)
      {
         stream << "key: [" << data.first << "], value: [" << data.second << "]" << '\n';
      }
      return stream;
   }
}

namespace casual
{
   namespace http
   {
      namespace inbound
      {
         namespace
         {
            namespace header
            {
               common::service::header::Fields copy( CasualHeader* headers, long length)
               {
                  const http::Trace trace("casual::http::inbound::header::copy");

                  common::service::header::Fields result;

                  for( auto& header : common::range::make( headers, length))
                  {
                     result.emplace_back( header.key, header.value);
                  }

                  return result;
               }
            }

            namespace parameter
            {
               using container = std::vector< std::pair< std::string, std::string> >;
               container copy( const Buffer& buffer)
               {
                  container result;
                  auto paramters = common::string::split( buffer.data, '&');
                  for (const auto parameter : paramters)
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
                     std::copy( parameter.first.begin(), parameter.first.end(), std::back_inserter( buffer));
                     buffer.push_back('"');
                     buffer.push_back(':');
                     buffer.push_back('"');
                     std::copy( parameter.second.begin(), parameter.second.end(), std::back_inserter( buffer));
                     buffer.push_back('"');
                     buffer.push_back(',');
                  }
                  if ( buffer.back() == ',') buffer.pop_back();
                  buffer.push_back('}');

                  http::verbose::log << "parameters: " << std::string( buffer.begin(), buffer.end()) << '\n';
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
                     std::copy( parameter.first.begin(), parameter.first.end(), std::back_inserter( buffer));
                     buffer.push_back('>');
                     std::copy( parameter.second.begin(), parameter.second.end(), std::back_inserter( buffer));
                     buffer.push_back('<');
                     buffer.push_back('/');
                     std::copy( parameter.first.begin(), parameter.first.end(), std::back_inserter( buffer));
                     buffer.push_back('>');
                  }
                  buffer.push_back('<');
                  buffer.push_back('/');
                  std::copy( root.begin(), root.end(), std::back_inserter( buffer));
                  buffer.push_back('>');

                  http::verbose::log << "parameters: " << std::string( buffer.begin(), buffer.end()) << '\n';
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
                        //
                        // Do the actual transforming
                        //
                        return found->second( std::move( buffer));
                     }
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
               Buffer copy( const Type& input)
               {
                  Buffer output;
                  output.data = reinterpret_cast<char*>( malloc( input.size()));
                  output.size = input.size();
                  common::algorithm::copy( input, output.data);
                  return output;
               }

               common::platform::binary::type assemble( CasualBuffer* transport, const parameter::container& parameters, const std::string& protocol)
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
               int handle( CasualBuffer* transport)
               {
                  try
                  {
                     throw;
                  }
                  catch ( const common::exception::xatmi::exception& exception)
                   {
                      transport->errorcode = exception.code().value();
                      transport->payload = buffer::copy( exception.code().message());
                   }
                   catch ( const common::exception::system::invalid::Argument& exception)
                   {
                      transport->errorcode = exception.code().value();
                      transport->payload = buffer::copy( std::string( exception.what()));
                   }
                   catch (...)
                   {
                      transport->errorcode = common::code::make_error_code( common::code::xatmi::os).value();
                      transport->payload = buffer::copy( std::string( "unknown error"));
                   }

                   return ERROR;
               }
            }


            long send( CasualBuffer* transport)
            {
               const http::Trace trace("casual::http::inbound::send");
               const auto& protocol = transport->protocol;
               const auto& service = transport->service;

               http::verbose::log << "protocol: " << protocol << '\n';
               http::verbose::log << "service: " << service << '\n';

               transport->context = cTPACALL;

               try
               {
                  //
                  // Handle header
                  //
                  const auto& header = header::copy( transport->header, transport->headersize);
                  common::service::header::fields( header);

                  //
                  // Handle parameter
                  //
                  const auto& parameters = parameter::copy( transport->parameter);
                  http::verbose::log << "parameters: " << parameters;

                  //
                  // Handle buffer
                  //
                  auto buffer = buffer::assemble( transport, parameters, protocol);
                  common::buffer::Payload payload( protocol::convert::to::buffer( protocol), buffer);
                  payload = buffer::input::transform( std::move( payload), protocol);

                  //
                  // Call service
                  //
                  namespace call = common::service::call;
                  transport->calldescriptor = call::context().async( service, common::buffer::payload::Send( payload), call::async::Flag::no_block);
               }
               catch (...)
               {
                  return exception::handle( transport);
               }

               transport->errorcode = 0;

               return OK;
            }

            long receive( CasualBuffer* transport)
            {
               const http::Trace trace("casual::http::inbound::receive");
               const auto& protocol = transport->protocol;
               const auto descriptor = transport->calldescriptor;
               const auto& service = transport->service;

               http::verbose::log << "protocol: " << protocol << '\n';
               http::verbose::log << "service: " << service << '\n';
               http::verbose::log << "descriptor: " << descriptor << '\n';

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

            long cancel( CasualBuffer* transport)
            {
               const http::Trace trace("casual::http::inbound::cancel");
               const auto& protocol = transport->protocol;
               const auto descriptor = transport->calldescriptor;
               const auto& service = transport->service;

               http::verbose::log << "protocol: " << protocol << '\n';
               http::verbose::log << "service: " << service << '\n';
               http::verbose::log << "descriptor: " << descriptor << '\n';

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

long casual_xatmi_send( CasualBuffer* data)
{
   return casual::http::inbound::send(data);
}

long casual_xatmi_receive( CasualBuffer* data)
{
   return casual::http::inbound::receive(data);
}

long casual_xatmi_cancel( CasualBuffer* data)
{
   return casual::http::inbound::cancel(data);
}

