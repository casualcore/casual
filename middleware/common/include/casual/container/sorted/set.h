//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/traits.h"

#include "common/algorithm/sorted.h"
#include "common/algorithm/container.h"
#include "common/compare.h"

#include "common/serialize/macro.h"

#include <vector>

namespace casual
{
   namespace common::traits::is::allocator
   {
      //! traits to help deduce if type is allocator like.
      //! if needed elsewhere, we move it to common/traits.h
      template< typename Allocator, typename = void, typename = void>
      struct like : std::false_type {};

      template< typename Allocator>
      struct like< Allocator,
         std::void_t< typename Allocator::value_type>,
         std::void_t< decltype( std::declval< Allocator&>().allocate( std::size_t( 0)))>>
         : std::true_type {};

      template< typename Allocator>
      constexpr bool like_v = like< Allocator>::value;
      
   } // common::traits::is::allocator

   namespace container::sorted
   {
      //! A sorted _unique_ set that uses contigious memory
      template< typename Value, typename Compare = std::less< Value>, typename Allocator = std::allocator< Value>>
      struct Set : common::Compare< Set< Value, Compare, Allocator>>
      {
         using value_type = Value;
         using container_type = std::vector< value_type, Allocator>;
         using range_type = common::range::type_t< container_type>;
         using size_type = platform::size::type;
         using compare_type = Compare;
         using const_iterator = typename std::vector< value_type, Allocator>::const_iterator;
         using iterator = const_iterator;

         Set() = default;

         template< typename Iter> 
         Set( Iter first, Iter last, const compare_type& compare = compare_type(), const Allocator& allocator = Allocator())
            : m_compare( compare), m_values( allocator) 
         { 
            insert( first, last);
         }

         template< typename Range, typename = std::enable_if_t< common::traits::is::iterable_v< std::decay_t< Range>>>>
         explicit Set( Range&& range, const compare_type& compare = compare_type(), const Allocator& allocator = Allocator())
            : Set( std::begin( range), std::end( range), compare, allocator)
         {}
            

         explicit Set( const Compare& compare, const Allocator& allocator = Allocator()) : m_compare{ compare}, m_values{ allocator} {}
         explicit Set( const Allocator& allocator ) : Set{ Compare(), allocator} {};

         Set& operator = ( container_type other) 
         {
            // we assume `other` has few, if any, duplicates
            m_values = common::algorithm::sort( std::move( other));
            common::algorithm::container::trim( m_values, common::algorithm::unique( m_values));

            return *this;
         }


         template< typename... Args >
         std::pair< iterator, bool> emplace( Args&&... args)
         {
            auto value = value_type{ std::forward< Args>( args)...};

            if( auto found = find( value))
            {  
               // check if value is equal to found ( not less then found )
               if( ! ( value < *found))
                  return { std::begin( found), false};
               
               return { m_values.insert( std::begin( found), std::move( value)), true};
            }

            m_values.push_back( std::move( value));
            return { std::prev( std::end( m_values)), true};
         }

         std::pair< iterator, bool> insert( const value_type& value) { return insert_implementation( value);}
         std::pair< iterator, bool> insert( value_type&& value) { return insert_implementation( std::move( value));}

         template< typename Iter >
         auto insert( Iter first, Iter last) -> decltype( void( first != last), void( insert( *first++)))
         {
            while( first != last)
               insert( *first++);
         }

         template< typename Range >
         auto insert( Range&& range) -> decltype( insert( std::begin( range), std::end( range)))
         {
            insert( std::begin( range), std::end( range));
         }


         constexpr const_iterator erase( const_iterator where) { return m_values.erase( where);}
         constexpr const_iterator erase( const_iterator first, const_iterator last) { return m_values.erase( first, last);}
         template< typename Range>
         constexpr auto erase( Range range) -> decltype( erase( std::begin( range), std::end( range)))
         { 
            return erase( std::begin( range), std::end( range));
         }

         void reserve( size_type capacity) { m_values.reserve( capacity);}
         

         //! @returns const-iterators since elements are immutable
         //! @{
         [[nodiscard]] auto cbegin() const noexcept { return std::cbegin( m_values);}
         [[nodiscard]] auto cend() const noexcept { return std::cend( m_values);}
         [[nodiscard]] auto begin() const noexcept { return std::cbegin( m_values);}
         [[nodiscard]] auto end() const noexcept { return std::cend( m_values);}
         //! @}

         [[nodiscard]] auto empty() const noexcept { return m_values.empty();}
         [[nodiscard]] size_type size() const noexcept { return m_values.size();}

         constexpr auto max_size() const noexcept { return m_values.max_size();}

         void clear() noexcept { m_values.clear();}

         value_type extract( const_iterator where) { return common::algorithm::container::extract( m_values, where);}

         

         CASUAL_FORWARD_SERIALIZE( m_values)

         auto tie() const noexcept { return std::tie( m_values);}

      private:

         range_type find( const value_type& value)
         {
            return std::get< 1>( common::algorithm::sorted::lower_bound( m_values, value, compare_type{}));
         }

         template< typename T>
         std::pair< iterator, bool> insert_implementation( T&& value) 
         {
            if( auto found = find( value))
            {
               if( *found == value)
                  return { std::begin( found), false};
               else
                  return { m_values.insert( std::begin( found), std::move( value)), true};
            }

            m_values.push_back( std::move( value));
            return { std::prev( std::end( m_values)), true};
         }

         compare_type m_compare;
         container_type m_values;
      };

      //! deduction guide for range like.
      template< typename Range,
         typename Compare = std::less<>,
         typename Allocator = std::allocator< common::traits::iterable::value_t< Range>>,
         typename = std::enable_if_t< common::traits::is::allocator::like_v< Allocator>>,
         typename = std::enable_if_t< ! common::traits::is::allocator::like_v< Compare>>
         >
      Set( Range&&, Compare = Compare(), Allocator = Allocator())
         -> Set< common::traits::iterable::value_t< Range>, Compare, Allocator>;

      
      
   } // container::sorted
} // casual