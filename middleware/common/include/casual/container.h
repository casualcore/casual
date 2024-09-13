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
      namespace detail
      {
         namespace id
         {
            struct Tag{};
            using type = common::strong::detail::integral::Type< platform::size::type, Tag, -1>;
         } // id
      
      } // detail

      template< typename T, typename I = detail::id::type>
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
            validate_index( key);
            m_data[ key.value()] = std::nullopt;
         }


         [[nodiscard]] bool contains( index_type key) const
         {
            if( ! valid_index( key))
               return false;

            return m_data[ key.value()].has_value();
         }


         value_type& operator [] ( index_type key) &
         {
            if( ! contains( key))
               common::code::raise::error( common::code::casual::invalid_argument, "out of bounds: ", key);

            return *m_data[ key.value()];
         }

         const value_type& operator [] ( index_type key) const &
         {
            if( ! contains( key))
               common::code::raise::error( common::code::casual::invalid_argument, "out of bounds: ", key);

            return *m_data[ key.value()];
         }

         std::optional< value_type> extract( index_type key) &
         {
            validate_index( key);
            return std::exchange( m_data[ key.value()], {});
         }

         //! @returns all values
         auto values() const
         {
            auto has_value = []( auto& value){ return value.has_value();};
            auto transform = []( auto& value) -> decltype( *value) { return *value;};
            
            return m_data | std::ranges::views::filter( has_value) | std::ranges::views::transform( transform);
         }

         auto indexes() const
         {            
            return common::algorithm::accumulate( m_data, std::vector< index_type>{}, [ index = platform::size::type{ 0}]( auto result, auto& value) mutable
            {
               if( value)
                  result.push_back( index_type( index));
                  
               ++index;
               return result;
            });   

            // Missing enumerate
            // auto has_value = []( auto index, auto& value){ return value.has_value();};
            // auto transform = []( auto index, auto& value) -> index_type { return index_type( index);};
            // return m_data | std::views::enumerate | std::ranges::views::filter( has_value) | std::ranges::views::transform( transform);


         }


         auto size() const -> platform::size::type { return m_data.size();}
         auto empty() const { return m_data.empty();}

         auto begin() { return std::begin( m_data);}
         auto end() { return std::end( m_data);}
         auto begin() const { return std::begin( m_data);}
         auto end() const { return std::end( m_data);}

      private:
         bool valid_index( index_type key) const
         {
            return key.value() >= 0 && key.value() < std::ssize( m_data);
         }

         void validate_index( index_type key) const
         {
            if( ! valid_index( key))
               common::code::raise::error( common::code::casual::invalid_argument, "out of bounds: ", key);
         }

         std::vector< std::optional< value_type>> m_data;
      };

      template< typename T, typename index_type, typename lookup_type>
      struct lookup_index
      {
         using value_type = T;

         index_type insert( lookup_type key, T value)
         {
            if( auto id = lookup( key))
            {
               m_index[ id] = std::move( value);
               return id;
            }

            auto id = m_index.insert( std::move( value));
            m_lookup.emplace( std::move( key), id);
            return id;

         }

         template< typename... Ts>
         index_type emplace( lookup_type key, Ts&&... ts)
         {
            if( auto id = lookup( key))
            {
               m_index[ id] = T( std::forward< Ts>( ts)...);
               return id;
            }

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

         [[nodiscard]] bool contains( index_type key) const 
         {
            return m_index.contains( key);
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
            common::algorithm::container::erase_if( m_lookup, [ this, callback]( auto& pair)
            {
               if( ! callback( pair.second, pair.first, m_index[ pair.second]))
                  return false;
               
               m_index.erase( pair.second);
               return true;
            });
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

         //! @returns all 'occupied' indexes
         auto indexes() const
         {
            return m_index.indexes();
         }

         //! @returns all values
         auto values() const
         {
            return m_index.values();
         }

         auto size() const noexcept { return m_lookup.size();}
         auto empty() const noexcept { return m_lookup.empty();}

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