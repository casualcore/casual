//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/communication/ipc/send.h"
#include "common/communication/log.h"

namespace casual
{
   namespace common::communication::ipc::send
   {
      namespace coordinator
      {

         void Message::error( const strong::ipc::id& destination) const noexcept
         {
            Trace trace{ "communication::ipc::send::Coordinator::Message::error"};
            
            if( ! callback)
            {
               log::line( log, "no callback for 'message': ", *this);
               return;
            }

            try
            {
               callback( destination, complete.complete());
            }
            catch( ...)
            {
               log::line( log::category::error, exception::capture(), " callback failed for 'message': ", *this, " - destination: ", destination);
            }
         }

         bool Remote::send( select::Directive& directive) noexcept
         {
            Trace trace{ "communication::ipc::send::Coordinator::Remote::send"};

            try
            {
               while( ! m_queue.empty())
               {
                  if( ipc::partial::send( m_destination, m_queue.front().complete))
                     m_queue.pop_front();
                  else
                     return false;
               }

               // we're done with this "remote"
               directive.write.remove( descriptor());
               return true;
            }
            catch( ...)
            {
               log::line( log, exception::capture(), " failed to send to destination: ", m_destination, " - action: invoke callback (if any) and discard for all pending messages to the destination");
               failed( directive);
               return true;
            }
         }

         void Remote::failed( select::Directive& directive)
         {
            for( auto& message : m_queue)
               message.error( m_destination.ipc());

            directive.write.remove( descriptor());
         }
         
      } // coordinator

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

      void Coordinator::failed( const strong::ipc::id& ipc)
      {
         if( auto found = algorithm::find( m_destinations, ipc))
         {
            found->failed( *m_directive);
            algorithm::container::erase( m_destinations, std::begin( found));
         }
      }

      strong::correlation::id Coordinator::send( const strong::ipc::id& ipc, coordinator::Message&& message)
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
               ipc::partial::Destination destination{ ipc};
               if( ! ipc::partial::send( destination, message.complete))
                  m_destinations.emplace_back( *m_directive, std::move( destination), std::move( message));
            }
            catch( ...)
            {
               log::line( log, exception::capture(), " failed to send to destination: ", ipc, " - action: invoke callback (if any) and discard for all pending messages to the destination");
               message.error( ipc);

               return {};
            }
         }
         return result;
      }



   } // common::communication::ipc::send
} // casual