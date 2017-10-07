/*
 * platuxedo_nginx_utility_caller.cpp
 *
 *  Created on: 2 okt. 2017
 *      Author: 40043017
 */

#include "http/common.h"
#include "http/inbound/caller.h"
#include "buffer/field.h"
#include "common/service/call/context.h"
#include "common/transcode.h"
#include "common/exception/system.h"

#include <map>
#include <vector>

namespace
{
// Global representation av senaste tuxedo errorkod
// Typ för att representera olika "context" där tuxedo anropas
enum tuxedo_context
{
   cTPINIT, cTPALLOC, cTPACALL, cTPGETRPLY
};

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
               std::vector< common::service::header::Field> copy( CasualHeader* header, long length)
               {
                  const http::Trace trace("casual::http::inbound::header::copy");
                  if (!header) return {};

                  std::vector< common::service::header::Field> result;
                  for (auto i = 0; i < length; i++)
                  {
                     result.push_back( common::service::header::Field( header->key, header->value));
                     header++;
                  }
                  return result;
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
                        http::verbose::log << "pre - transform" << '\n';

                        result.memory = common::transcode::base64::decode( std::string( buffer.memory.begin(), buffer.memory.end()));
                        http::verbose::log << "post - transform" << '\n';

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
                        { http::protocol::json(), transcode_none},
                        { http::protocol::xml(), transcode_none}
                     };

                     auto found = common::range::find( mapping, protocol);
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
                        { http::protocol::json(), transcode_none},
                        { http::protocol::xml(), transcode_none}
                     };

                     auto found = common::range::find( mapping, protocol);
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

               std::string type( const std::string& protocol)
               {
                  const static auto mapping = std::map< std::string, std::string>{
                   { http::protocol::binary(), common::buffer::type::binary()},
                   { http::protocol::x_octet(), common::buffer::type::x_octet()},
                   { http::protocol::json(), common::buffer::type::json()},
                   { http::protocol::xml(), common::buffer::type::xml()}
                  };

                  auto found = common::range::find( mapping, protocol);
                  if( found) return found->second;

                  throw common::exception::xatmi::buffer::type::Input();
               }

               template< typename Type>
               Payload copy( const Type& buffer)
               {
                  Payload payload;
                  payload.data = reinterpret_cast<char*>( malloc( buffer.size()));
                  payload.size = buffer.size();
                  common::range::copy( buffer, payload.data);
                  return payload;
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
                      return ERROR;
                   }
                   catch ( const common::exception::system::invalid::Argument& exception)
                   {
                      transport->errorcode = exception.code().value();
                      transport->payload = buffer::copy( std::string( exception.what()));
                      return ERROR;
                   }
                   catch (...)
                   {
                      transport->errorcode = common::code::make_error_code( common::code::xatmi::os).value();
                      transport->payload = buffer::copy( std::string( "unknown error"));
                      return ERROR;
                   }
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
                  // Handle buffer
                  //
                  common::platform::binary::type buffer( transport->payload.data, transport->payload.data + transport->payload.size);
                  common::buffer::Payload payload( buffer::type( protocol), buffer);
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

