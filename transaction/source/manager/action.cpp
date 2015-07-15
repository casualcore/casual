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
               common::Trace trace( "connect to broker", log::internal::transaction);

               //
               // Do the initialization dance with the broker
               //
               common::message::transaction::manager::Connect connect;

               connect.path = common::process::path();
               connect.process = common::process::handle();

               queue::blocking::Writer broker( common::ipc::broker::id(), state);
               broker( connect);

            }

            {
               common::Trace trace( "configure", log::internal::transaction);

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

            {
               common::Trace trace( "event registration", log::internal::transaction);

               common::message::dead::process::Registration message;
               message.process = common::process::handle();

               queue::blocking::Writer broker( common::ipc::broker::id(), state);
               broker( message);

            }

         }

         namespace local
         {
            namespace
            {
               struct Timout : state::Base
               {
                  using state::Base::Base;

                  inline void operator () ( const common::transaction::ID& trid)
                  {
                     //
                     // Find the transaction
                     //
                     auto found = common::range::find_if(
                           common::range::make( m_state.transactions), find::Transaction{ trid});

                     if( found)
                     {
                        if( found->state() == Transaction::Resource::State::cInvolved)
                        {

                           handle::Rollback::message_type message;
                           message.trid = trid;

                           //
                           // We make our self the sender, and we'll discard the response  (from our self)
                           //
                           message.process = process::handle();

                           handle::Rollback{ m_state}( message);
                        }
                        else
                        {
                           log::internal::transaction << "transaction: " << *found << " already in transition\n";
                        }
                     }
                     else
                     {
                        log::error << "persistent transaction is not active - action: discard\n";
                     }


                     //
                     // Set the transaction in "timeout-mode"
                     //
                     m_state.log.timeout( trid);

                  }
               };

            } // <unnamed>
         } // local


         void timeout( State& state)
         {
            trace::internal::Scope trace( "action::timeout", common::log::internal::transaction);

            auto transactions = state.log.passed( common::platform::clock_type::now());

            range::for_each( transactions, local::Timout{ state});
         }

         namespace boot
         {
            void Proxie::operator () ( state::resource::Proxy& proxy)
            {
               trace::internal::Scope trace( "boot::Proxie::operator()", common::log::internal::transaction);

               for( auto index = proxy.concurency; index > 0; --index)
               {
                  auto& info = m_state.xaConfig.at( proxy.key);

                  state::resource::Proxy::Instance instance;//( proxy.id);
                  instance.id = proxy.id;

                  instance.process.pid = process::spawn(
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
               try
               {
                  queue::non_blocking::Writer write{ message.target, m_state};

                  if( ! write.send( message.message))
                  {
                     common::log::internal::transaction << "failed to send reply - type: " << message.message.type << " to: " << message.target << "\n";
                     return false;
                  }
               }
               catch( const exception::queue::Unavailable&)
               {
                  common::log::error << "failed to send reply to " << message.target << " TODO: rollback transaction?\n";
                  //
                  // ipc-queue has been removed...
                  // TODO: deduce from message.message.type what we should do
                  //  We should rollback if we are in a prepare stage?
                  //
               }
               return true;
            }

            bool Send::operator () ( state::pending::Request& message) const
            {
               decltype( message.resources) resources;
               std::swap( message.resources, resources);

               for( auto&& id : resources)
               {

                  auto found = m_state.idle_instance( id);

                  if( found)
                  {
                     queue::non_blocking::Writer write{ found->process.queue, m_state};

                     if( ! write.send( message.message))
                     {
                        common::log::internal::transaction << "failed to send resource request - type: " << message.message.type << " to: " << found->process << "\n";
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
