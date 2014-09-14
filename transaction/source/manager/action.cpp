//!
//! action.cpp
//!
//! Created on: Oct 20, 2013
//!     Author: Lazan
//!

#include "transaction/manager/action.h"
#include "transaction/manager/handle.h"

#include "common/ipc.h"
#include "common/process.h"
#include "common/internal/log.h"
#include "common/environment.h"
#include "common/internal/trace.h"


#include "sf/log.h"

#include <string>


namespace casual
{
   using namespace common;

   namespace transaction
   {
      namespace action
      {
         void configure( State& state)
         {
            {
               common::trace::internal::Scope trace( "connect to broker");

               //
               // Do the initialization dance with the broker
               //
               common::message::transaction::manager::Connect connect;

               connect.path = common::process::path();
               connect.server.queue_id = common::ipc::receive::id();

               queue::blocking::Writer broker( common::ipc::broker::id(), state);
               broker( connect);
            }

            {
               common::trace::internal::Scope trace( "configure");

               //
               // Wait for configuration
               //
               common::message::transaction::manager::Configuration configuration;

               queue::blocking::Reader read( common::ipc::receive::queue(), state);
               read( configuration);

               //
               // configure state
               //
               state::configure( state, configuration);
            }

         }


         namespace boot
         {
            void Proxie::operator () ( state::resource::Proxy& proxy)
            {
               for( auto index = proxy.concurency; index > 0; --index)
               {
                  auto& info = m_state.xaConfig.at( proxy.key);

                  state::resource::Proxy::Instance instance;//( proxy.id);
                  instance.id = proxy.id;

                  instance.server.pid = process::spawn(
                        info.server,
                        {
                              "--tm-queue", std::to_string( ipc::receive::id()),
                              "--rm-key", info.key,
                              "--rm-openinfo", proxy.openinfo,
                              "--rm-closeinfo", proxy.closeinfo,
                              "--rm-id", std::to_string( proxy.id),
                              "--domain", common::environment::domain::name()
                        }
                     );

                  instance.state = state::resource::Proxy::Instance::State::started;

                  proxy.instances.push_back( std::move( instance));
               }
            }

         } // boot


         namespace persistent
         {

            bool Send::operator () ( state::pending::Reply& message) const
            {
               queue::non_blocking::Writer write{ message.target, m_state};
               if( ! write.send( message.message))
               {
                  common::log::internal::transaction << "failed to send reply - type: " << message.message.type << " to: " << message.target << "\n";
                  return false;
               }
               return true;
            }

            bool Send::operator () ( state::pending::Request& message) const
            {
               decltype( message.resources) resources;
               std::swap( message.resources, resources);

               for( auto&& id : resources)
               {

                  auto found = state::find::idle::instance( m_state.resources, id);

                  if( found)
                  {
                     queue::non_blocking::Writer write{ found->server.queue_id, m_state};

                     if( ! write.send( message.message))
                     {
                        common::log::internal::transaction << "failed to send resource request - type: " << message.message.type << " to: " << found->server << "\n";
                        message.resources.push_back( id);
                     }
                  }
                  else
                  {
                     message.resources.push_back( id);
                  }
               }

               return message.resources.empty();
            }

         } // pending

      } // action
   } //transaction
} // casual
