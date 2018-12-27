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
         std::enable_if_t< 
            std::is_trivially_copyable< std::remove_reference_t< T>>::value
            && ! traits::is::binary::like< T>::value
         >
         clear( T& value)
         {
            std::memset( &value, 0, size( value));
         }

         template< typename R>
         std::enable_if_t< traits::is::binary::like< R>::value>
         clear( R& value)
         {
            std::memset( &( *std::begin( value)), 0, range::size( value));
         }



         template< typename T>
         std::enable_if_t< traits::is_trivially_copyable< traits::remove_cvref_t< T>>::value, size_type>
         append( T&& value, platform::binary::type& destination)
         {
            auto first = reinterpret_cast< const platform::character::type*>( &value);
            auto last = first + memory::size( value);

            destination.insert(
                  std::end( destination),
                  first,
                  last);

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
         std::enable_if_t< traits::is_trivially_copyable< T>::value, size_type>
         copy( S&& source, size_type offset, T& value)
         {
            auto size = memory::size( value);
            auto first = std::begin( source) + offset;
            auto last = std::end( source);

            assert( std::distance( first, last) >=  size);

            std::memcpy( &value, &(*first), size);

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


