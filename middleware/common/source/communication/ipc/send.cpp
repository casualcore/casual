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

      strong::correlation::id Coordinator::send( const strong::ipc::id& ipc, ipc::message::complete::Send&& complete)
      {
         Trace trace{ "communication::ipc::send::Coordinator::send"};
         auto result = complete.correlation();

         if( auto found = algorithm::find( m_destinations, ipc))
         {
            if( found->push( std::move( complete)).send( *m_directive))
               m_destinations.erase( std::begin( found));
         }
         else
         {
            ipc::outbound::partial::Destination destination{ ipc};

            try
            {
            if( ! ipc::outbound::partial::send( destination, complete))
               m_destinations.emplace_back( *m_directive, std::move( destination), std::move( complete));
            }
            catch( ...)
            {
               auto error = exception::capture();
               if( error.code() == code::casual::communication_unavailable)
               {
                  log::line( log::category::error, error, " failed to send to destination: ", destination, " - action: discard");
                  return {};
               }
               throw;
            }
         }

         return result;
      }

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

      bool Coordinator::Remote::send( select::Directive& directive)
      {
         while( ! m_queue.empty())
         {
            try
            {
               if( ipc::outbound::partial::send( m_destination, m_queue.front()))
                  m_queue.pop_front();
               else 
                  return false;
            }
            catch( ...)
            {
               auto error = exception::capture();
               if( error.code() == code::casual::communication_unavailable)
               {
                  log::line( log::category::error, error, " failed to send to destination: ", m_destination, " - action: discard all pending messages to the destination");
                  return true;
               }
               throw;
            }
         }
         directive.write.remove( descriptor());
         return true;
      }

   } // common::communication::ipc::send
} // casual