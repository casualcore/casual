
#include "http/common.h"
#include "http/inbound/caller.h"
#include "buffer/field.h"
#include "common/service/call/context.h"
#include "common/transcode.h"
#include "common/exception/system.h"
#include "common/string.h"

#include <map>
#include <vector>

namespace std
{
   std::ostream& operator<<( std::ostream& stream, std::map< std::string, std::string> input)
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
               Buffer copy( const Type& input)
               {
                  Buffer output;
                  output.data = reinterpret_cast<char*>( malloc( input.size()));
                  output.size = input.size();
                  common::range::copy( input, output.data);
                  return output;
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

            namespace parameter
            {
               std::map< std::string, std::string> copy( const Buffer& buffer)
               {
                  std::map< std::string, std::string> result;
                  auto paramters = common::string::split( buffer.data, '&');
                  for (const auto parameter : paramters)
                  {
                     const auto parts = common::string::split( parameter, '=');
                     if (parts.size() == 2)
                     {
                        result[parts[0]] = parts[1];
                     }
                  }
                  return result;
               }

               common::platform::binary::type make_json( const std::map< std::string, std::string> parameters)
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
                  common::platform::binary::type buffer;
                  if ( protocol == http::protocol::json() && transport->payload.size < 1)
                  {
                     buffer = parameter::make_json( parameters);
                  }
                  else
                  {
                     buffer = common::platform::binary::type( transport->payload.data, transport->payload.data + transport->payload.size);
                  }

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

