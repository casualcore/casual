//!
//! casual
//!

#include "http/outbound/configuration.h"
#include "http/outbound/request.h"
#include "http/common.h"

#include "common/exception/handle.h"
#include "common/arguments.h"
#include "common/communication/ipc.h"

#include "common/message/service.h"
#include "common/message/handle.h"

#include "common/execute.h"

#include <memory>

namespace casual
{
   namespace http
   {
      namespace outbound
      {
         struct Settings
         {
            std::vector< std::string> configurations;
         };

         struct State
         {
            struct Node
            {
               std::string url;
               std::shared_ptr< common::service::header::Fields> headers;
               bool discard_transaction = false;
            };

            inline friend std::ostream& operator << ( std::ostream& out, const Node& value)
            {
               return out << "{ url: " << value.url
                  << ", headers: " << common::range::make( *value.headers)
                  << "}";
            }

            std::unordered_map< std::string, Node> lookup;
         };

         namespace transform
         {
            auto header( const std::vector< configuration::Header>& model)
            {
               common::service::header::Fields headers;

               common::algorithm::transform( model, headers.container(), []( auto& h){
                  return common::service::header::Field{ h.name, h.value};
               });

               return std::make_shared< common::service::header::Fields>( std::move( headers));
            }

         } // transform

         void advertise( const State& state)
         {
            common::message::service::Advertise message;
            message.process = common::process::handle();

            common::algorithm::transform( state.lookup, message.services, []( auto& l){
               common::message::service::advertise::Service service;
               service.category = "http";
               service.name = l.first;
               service.transaction = common::service::transaction::Type::none;
               return service;
            });

            common::communication::ipc::blocking::send( common::communication::ipc::service::manager::device(), message);

         }

         State configure( configuration::Model model)
         {
            State state;

            // default headers will be used if service have 0 headers
            auto header = transform::header( model.casual_default.service.headers);

            common::algorithm::for_each( model.services, [&]( auto& s){
               State::Node node;
               {
                  node.url = s.url;
                  if( s.headers.empty())
                     node.headers = header;
                  else
                     node.headers = transform::header( s.headers);

                  node.discard_transaction = s.discard_transaction.value_or( 
                     model.casual_default.service.discard_transaction);

               }
               state.lookup[ s.name] = std::move( node);
            });


            return state;
         }

         namespace handle
         {
            struct Base
            {
               Base( State& state) : state( state) {}

               State& state;
            };

            struct Service : Base
            {
               using Base::Base;

               void operator() ( common::message::service::call::callee::Request& message)
               {
                  auto reply = common::message::reverse::type( message);
                  reply.status = common::code::xatmi::no_entry;

                  // make sure we always send reply (if user wants one).
                  auto send_reply = common::execute::scope( [&](){
                     if( ! message.flags.exist( common::message::service::call::request::Flag::no_reply))
                     {
                        common::communication::ipc::blocking::send( message.process.queue, reply);
                     }
                  });

                  // make sure we always send ACK
                  auto send_ack = common::execute::scope( [&](){
                     common::message::service::call::ACK ack;
                     ack.correlation = message.correlation;
                     ack.process = common::process::handle();
                     common::communication::ipc::blocking::send( common::communication::ipc::service::manager::device(), ack);
                  });


                  auto& node = state.lookup.at( message.service.name);

                  // we can't allow this forward to be in a transaction

                  if( message.trid)
                  {
                     if( ! node.discard_transaction)
                     {
                        common::log::line( common::log::category::error, common::code::xatmi::protocol, " - http-outbound can't be used in transaction for service: ",  message.service.name);
                        common::log::line( common::log::category::verbose::error, common::code::xatmi::protocol, " - message: ", message);
                        common::log::line( common::log::category::verbose::error, common::code::xatmi::protocol, " - node: ", node);
                        reply.status = common::code::xatmi::protocol;
                        return;
                      }
                      else
                      {
                         common::log::line( log, "transaction discarded for '", message.service.name, "'");
                      }
                  }

                  try
                  {
                     auto respons = http::request::post( node.url, message.buffer, *node.headers.get());

                     reply.buffer = std::move( respons.payload);
                     reply.status = common::code::xatmi::ok;
                  }
                  catch( const common::exception::system::communication::Error& exception)
                  {
                     common::log::line( common::log::category::error, exception);
                  }
               }
            };
         } // handle



         void start( State state)
         {
            advertise( state);

            auto handler = common::communication::ipc::inbound::device().handler(
               common::message::handle::Shutdown{},
               common::message::handle::ping(),
               handle::Service( state)
            );


            common::message::dispatch::blocking::pump( handler, common::communication::ipc::inbound::device());
         }

         void main( int argc, char **argv)
         {
            common::process::instance::connect();

            Settings settings;
            {
               common::Arguments arguments({
                  common::argument::directive( { "--configuration-files"}, "configuration files", settings.configurations)
               });

               arguments.parse( argc, argv);
            }

            start( configure( configuration::get( settings.configurations)));

         }
      } // outbound
   } // http

} // casual

int main( int argc, char **argv)
{
   try
   {
      casual::http::outbound::main( argc, argv);
   }
   catch( ...)
   {
      return casual::common::exception::handle();
   }
   return 0;
}



