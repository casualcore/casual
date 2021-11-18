//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "casual/platform.h"

#include "common/message/conversation.h"
#include "common/code/casual.h"

#include "common/serialize/macro.h"

namespace casual
{
   namespace common::service::conversation
   {
      namespace state
      {
         namespace detail::index
         {
            //! potentially a container for all _descriptors_
            template< typename I, typename V>
            struct container
            {
               using index_type = I;

               template< typename... Ts>
               auto reserve( Ts&&... ts)
               {
                  if( auto found = algorithm::find_if( m_values, []( auto& value){ return absent( value);}))
                  {
                     *found = V{ std::forward< Ts>( ts)...};
                     return index_type( std::distance( std::begin( m_values), std::begin( found)));
                  }
                  else
                  {
                     m_values.push_back( V{ std::forward< Ts>( ts)...});
                     return index_type( m_values.size() - 1);
                  }
               }

               void unreserve( index_type index) { lookup( *this, index) = V{};}

               auto& at( index_type index) { return lookup( *this, index);}
               auto& at( index_type index) const { return lookup( *this, index);}


               auto begin() const noexcept { return std::begin( m_values);}
               auto end() const noexcept { return std::end( m_values);}

               //! @return true if there are no _active_ values. 
               bool empty() const noexcept
               {
                  return algorithm::all_of( m_values, []( auto& value){ return absent( value);});
               }

               platform::size::type size() const noexcept
               {
                  return algorithm::accumulate( m_values, platform::size::type{}, []( auto count, auto& value)
                  { 
                     return absent( value) ? count : count + 1;
                  });
               }

               platform::size::type capacity() const noexcept { return m_values.size();}

            private:

               template< typename C>
               friend auto lookup( C& container, index_type index) -> decltype( container.m_values.front())
               {
                  const auto index_v = container.index( index);

                  if( index_v >= 0 && index_v < static_cast< platform::size::type>( container.m_values.size()))
                     if( ! absent(  container.m_values[ index_v]))
                        return container.m_values[ index_v];

                  code::raise::error( code::xatmi::descriptor, "invalid index: ", index);
               }

               platform::size::type index( index_type index)
               {
                  return index.value();
               };


               std::vector< V> m_values;
            };

         } // detail::index
         
         namespace descriptor
         {
            struct Value
            {
               using Duplex = message::conversation::duplex::Type;

               strong::correlation::id correlation;
               process::Handle process;

               Duplex duplex = Duplex::receive;
               bool initiator = false;

               inline friend bool absent( const Value& value) { return ! predicate::boolean( value.correlation);}
               
               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( correlation);
                  CASUAL_SERIALIZE( process);
                  CASUAL_SERIALIZE( duplex);
                  CASUAL_SERIALIZE( initiator);
               )
            };            
         } // descriptor


      } // state

      struct State
      {
         state::detail::index::container< strong::conversation::descriptor::id, state::descriptor::Value> descriptors;

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( descriptors);
         )
      };

   } // common::service::conversation

} // casual


