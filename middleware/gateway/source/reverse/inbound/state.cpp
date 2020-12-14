//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/reverse/inbound/state.h"


namespace casual
{
   namespace gateway::reverse::inbound
   {
      using namespace common;

      namespace state
      {

         Messages::complete_type Messages::consume( const common::Uuid& correlation, platform::time::unit pending)
         {
            if( auto found = common::algorithm::find( m_calls, correlation))
            {
               auto message = algorithm::extract( m_calls, std::begin( found));
               m_size -= Messages::size( message);
               message.pending = pending;

               return serialize::native::complete( std::move( message));
            }

            common::code::raise::log( common::code::casual::invalid_argument, "failed to find correlation: ", correlation);
         }

         Messages::complete_type Messages::consume( const common::Uuid& correlation)
         {
            if( auto found = algorithm::find( m_complete, correlation))
            {
               auto result = algorithm::extract( m_complete, std::begin( found));
               m_size -= result.payload.size();

               return result;
            }

            code::raise::log( code::casual::invalid_argument, "failed to find correlation: ", correlation);
         }

      } // state

      state::external::Connection* State::consume( const common::Uuid& correlation)
      {
         if( auto found = algorithm::find( correlations, correlation))
         {
            auto descriptor = algorithm::extract( correlations, std::begin( found)).descriptor;
            
            if( auto connector = algorithm::find( external.connections, descriptor))
               return connector.data();
         }

         return nullptr;
      }

   } // gateway::reverse::inbound

} // casual