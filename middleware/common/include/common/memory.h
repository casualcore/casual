//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "common/range.h"
#include "common/traits.h"

#include <memory>

namespace casual
{
   namespace common
   {
      namespace memory
      {
         using size_type = platform::size::type;

         namespace detail
         {
            template< typename T>
            constexpr auto size() -> std::enable_if_t< ! std::is_array< T>::value && std::is_trivially_copyable< T>::value, size_type>
            {
               return sizeof( T);
            }

            template< typename T>
            constexpr auto size() -> std::enable_if_t< std::is_array< T>::value && std::is_trivially_copyable< T>::value, size_type>
            {
               return size< std::remove_extent_t< T>>() * std::extent< T>::value;
            }

         } // detail


         template< typename T>
         constexpr size_type size( T&&)
         {
            return detail::size< traits::remove_cvref_t<T>>();
         }


         template< typename T>
         auto append( T&& value, platform::binary::type& destination)
            -> std::enable_if_t< std::is_trivially_copyable_v< traits::remove_cvref_t< T>> 
               && ! traits::is::binary::like_v< traits::remove_cvref_t< T>>, size_type>
         {
            auto first = reinterpret_cast< const platform::character::type*>( &value);
            auto last = first + memory::size( value);

            destination.insert(
                  std::end( destination),
                  first,
                  last);

            return destination.size();
         }

         template< typename T>
         auto append( T&& value, platform::binary::type& destination) 
            -> std::enable_if_t< traits::is::binary::like_v< traits::remove_cvref_t< T>>, size_type>
         {
            destination.insert(
                  std::end( destination),
                  std::begin( value),
                  std::end( value));

            return destination.size();
         }

         //!
         //! Copy from @p std::begin( source) + offset into @value
         //!
         //! @param source memory
         //! @param offset where to begin from in the source
         //! @param value value to be 'assigned'
         //! @return the new offset ( @p offset + memory::size( value) )
         //!
         template< typename S, typename T>
         auto copy( S&& source, size_type offset, T& value)
            -> std::enable_if_t< std::is_trivially_copyable_v< T>, size_type>
         {
            auto size = memory::size( value);
            auto first = std::begin( source) + offset;
            auto last = std::end( source);

            assert( std::distance( first, last) >=  size);

            std::memcpy( &value, &(*first), size);

            return offset + size;
         }

         template< typename S, typename T>
         auto copy( S&& source, size_type offset, T&& value)
            -> std::enable_if_t< traits::is::binary::like_v< traits::remove_cvref_t< T>>, size_type>
         {
            auto size = std::distance( std::begin( value), std::end( value));
            auto first = std::begin( source) + offset;
            auto last = first + size;

            assert( std::end( source) >= last);

            std::copy( first, last, std::begin( value));

            return offset + size;
         }



         //!
         //! Create a 'deallocator' that uses @p deleter when it goes
         //! out of scope.
         //!
         //! Aims to improve safety when fiddeling with c-api:s
         //!
         //! @param memory
         //! @param deleter
         //! @return a guard that will apply deleter in dtor
         //!
         template< typename T, typename D>
         auto guard( T* memory, D&& deleter) noexcept
         {
            return std::unique_ptr< T, D>( memory, std::forward< D>( deleter));
         }


      } // memory


   } // common
} // casual


