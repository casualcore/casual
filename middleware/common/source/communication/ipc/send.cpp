//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/communication/ipc/send.h"

namespace casual
{
   namespace common::communication::ipc::send
   {
      bool Coordinator::operator () ( strong::file::descriptor::id descriptor, communication::select::tag::write) &
      {
         Trace trace{ "communication::ipc::send::Coordinator::operator <select>"};
         log::line( verbose::log, "descriptor: ", descriptor);

         if( auto found = algorithm::find( m_destinations, descriptor))
         {
            log::line( verbose::log, "found: ", *found);
            if( found->send( *m_directive))
               m_destinations.erase( std::begin( found));

            return true;
         }
         return false;
      }

      bool Coordinator::send() noexcept
      {
         Trace trace{ "communication::ipc::send::Coordinator::send <batch>"};

         auto erase = std::get< 1>( algorithm::partition( m_destinations, [ directive = m_directive]( auto& destination)
         { 
            // we negate to get the ones we'll keep first in the "queue"
            return ! destination.send( *directive);
         }));

         algorithm::container::erase( m_destinations, erase);

         return empty();
      }

      platform::size::type Coordinator::pending() const noexcept
      {
         return algorithm::accumulate( m_destinations, platform::size::type{}, []( auto count, auto& destination)
         {
            return count + destination.size();
         });
      }

      strong::correlation::id Coordinator::send( const strong::ipc::id& ipc, Coordinator::Message&& message)
      {
         Trace trace{ "communication::ipc::send::Coordinator::send"};
         
         auto result = message.complete.correlation();

         if( auto found = algorithm::find( m_destinations, ipc))
         {
            if( found->push( std::move( message)).send( *m_directive))
               m_destinations.erase( std::begin( found));
         }
         else
         {
            try
            {
               ipc::outbound::partial::Destination destination{ ipc};
               if( ! ipc::outbound::partial::send( destination, message.complete))
                  m_destinations.emplace_back( *m_directive, std::move( destination), std::move( message));
            }
            catch( ...)
            {
               log::line( log::category::error, exception::capture(), " failed to send to ipc: ", ipc, " - action: invoke error callback (if any) on the message");
               message.error( ipc);

               return {};
            }
         }
         return result;
      }

      bool Coordinator::Remote::send( select::Directive& directive) noexcept
      {
         try
         {
            while( ! m_queue.empty())
            {
               if( ipc::outbound::partial::send( m_destination, m_queue.front().complete))
                  m_queue.pop_front();
               else
                  return false;
            }
         }
         catch( ...)
         {
            log::line( log::category::error, exception::capture(), " failed to send to destination: ", m_destination, " - action: invoke callback (if any) and discard for all pending messages to the destination");

            auto invoke_callback = [ &destination = m_destination.ipc()]( auto& message) noexcept
            {
               try
               {
                  message.error( destination);
               }
               catch( ...)
               {
                  log::line( log::category::error, exception::capture(), " failed to invoke callback for message: ", message, " - action: discard");
               }
            };

            algorithm::for_each( m_queue, invoke_callback);
         }
         
         directive.write.remove( descriptor());
         return true;
      }

   } // common::communication::ipc::send
} // casual