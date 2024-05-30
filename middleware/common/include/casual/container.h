//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/platform.h"
#include "casual/assert.h"

#include "common/strong/type.h"
#include "common/algorithm.h"

#include "common/serialize/macro.h"

#include <vector>
#include <optional>
#include <unordered_map>

namespace casual
{
   namespace container
   {

      template< typename T, typename I>
      struct Index
      {
         using value_type = T;
         using index_type = I; // should be a strong type

         index_type insert( T value)
         {
            if( auto found = common::algorithm::find( m_data, std::nullopt))
            {
               *found = std::move( value);
               return index_type{ std::distance( std::begin( m_data), std::begin( found))};
            }
            m_data.push_back( std::move( value));
            return index_type{ m_data.size() -1};
         }

         template< typename... Ts>
         index_type emplace( Ts&&... ts)
         {
            if( auto found = common::algorithm::find( m_data, std::nullopt))
            {
               found->emplace( std::forward< Ts>( ts)...);
               return index_type{ std::distance( std::begin( m_data), std::begin( found))};
            }
            m_data.emplace_back( std::forward< Ts>( ts)...);
            return index_type{ m_data.size() -1};
         }

         void erase( index_type key)
         {
            CASUAL_ASSERT( key.value() < std::ssize( m_data));
            m_data[ key.value()] = std::nullopt;
         }

         const std::optional< value_type>& find( index_type key) const &
         {
            CASUAL_ASSERT( key.value() < std::ssize( m_data));
            return m_data[ key.value()];
         }

         std::optional< value_type>& find( index_type key) &
         {
            CASUAL_ASSERT( key.value() < std::ssize( m_data));
            return m_data[ key.value()];
         }


         value_type& operator [] ( index_type key) &
         {
            auto& result = find( key);
            CASUAL_ASSERT( result);
            return *result;
         }

         const value_type& operator [] ( index_type key) const &
         {
            auto& result = find( key);
            CASUAL_ASSERT( result);
            return *result;
         }

         std::optional< value_type> extract( index_type key) &
         {
            CASUAL_ASSERT( key.value() < std::ssize( m_data));
            return std::exchange( m_data[ key.value()], {});
         }


         auto size() const noexcept { return m_data.size();}
         auto empty() const noexcept { return m_data.empty();}

         auto begin() noexcept { return std::begin( m_data);}
         auto end() noexcept { return std::begin( m_data);}
         auto begin() const noexcept { return std::begin( m_data);}
         auto end() const noexcept { return std::begin( m_data);}
         

      private:
         std::vector< std::optional< value_type>> m_data;
      };

      template< typename T, typename index_type, typename lookup_type>
      struct lookup_index
      {
         using value_type = T;

         index_type insert( lookup_type key, T value)
         {
            auto id = m_index.insert( std::move( value));
            m_lookup.emplace( std::move( key), id);
            return id;
         }

         template< typename... Ts>
         index_type emplace( lookup_type key, Ts&&... ts)
         {
            auto id = m_index.emplace( std::forward< Ts>( ts)...);
            m_lookup.emplace( std::move( key), id);
            return id;
         }

         index_type lookup( const lookup_type& key) const
         {
            if( auto found = common::algorithm::find( m_lookup, key))
               return found->second;
            return {};
         }
         
         value_type& operator [] ( index_type key) &
         {
            return m_index[ key];
         }

         const value_type& operator [] ( index_type key) const &
         {
            return m_index[ key];
         }
         std::optional< value_type> extract( index_type key) &
         {
            if( auto found = common::algorithm::find_if( m_lookup, [ key]( auto& pair){ return pair.second == key;}))
               m_lookup.erase( std::begin( found)); 

            return m_index.extract( key);
         }

         void erase_if( auto callback)
         {
            for( auto current = std::begin( m_lookup); current != std::end( m_lookup); ++current)
            {
               if( callback( current->second, current->first, m_index[ current->second]))
               {
                  m_index.erase( current->second);
                  m_lookup.erase( current);
               }
            }
         }

         void for_each( auto callback)
         {
            for( auto& pair : m_lookup)
               callback( pair.second, pair.first, m_index[ pair.second]);
         }

         void for_each( auto callback) const
         {
            for( auto& pair : m_lookup)
               callback( pair.second, pair.first, m_index[ pair.second]);
         }

         void erase( index_type key)
         {
            m_index.erase( key);
            
            if( auto found = common::algorithm::find_if( m_lookup, [ key]( auto& pair){ return pair.second == key;}))
               m_lookup.erase( std::begin( found)); 
         }

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( m_index);
            CASUAL_SERIALIZE( m_lookup);
         )

      private:
         container::Index< T, index_type> m_index;
         std::unordered_map< lookup_type, index_type> m_lookup;
      };
      
   } // container
} // casual