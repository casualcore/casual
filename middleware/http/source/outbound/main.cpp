//!
//! casual
//!

#include "http/outbound/configuration.h"

#include "http/outbound/request.h"

#include "common/exception/handle.h"
#include "common/arguments.h"
#include "common/communication/ipc.h"

#include "common/message/service.h"
#include "common/message/handle.h"

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
               std::shared_ptr< std::vector< std::string>> headers;
            };



            std::unordered_map< std::string, Node> lookup;
         };

         namespace transform
         {
            std::shared_ptr< std::vector< std::string>> header( const std::vector< configuration::Header>& model)
            {
               auto headers = common::range::transform( model, []( auto& h){
                  return h.name + ':' + h.value;
               });

               return std::make_shared< std::vector< std::string>>( std::move( headers));
            }

         } // transform

         void advertise( const State& state)
         {
            common::message::service::Advertise message;
            message.process = common::process::handle();

            common::range::transform( state.lookup, message.services, []( auto& l){
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
            auto header = transform::header( model.casual_default.headers);

            common::range::for_each( model.services, [&]( auto& s){
               State::Node node;
               {
                  node.url = s.url;
                  if( s.headers.empty())
                     node.headers = header;
                  else
                     node.headers = transform::header( s.headers);
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

                  // make sure we always send reply
                  auto send_reply = common::scope::execute( [&](){
                     common::communication::ipc::blocking::send( message.process.queue, reply);
                  });

                  auto& node = state.lookup.at( message.service.name);

                  auto respons = http::request::post( node.url, message.buffer.memory, *node.headers.get());

                  reply.buffer.type = message.buffer.type;
                  reply.buffer.memory = std::move( respons.payload);
                  reply.status = common::code::xatmi::ok;

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



