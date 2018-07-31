//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "common/range.h"
#include "common/traits.h"

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
            constexpr auto size() -> std::enable_if_t< std::is_array< T>::value, size_type>
            {
               return size< typename std::remove_extent< T>::type>() * std::extent< T>::value;
            }

            using range_type = Range< platform::binary::type::iterator::pointer>;
            using const_range_type = Range< platform::binary::type::const_iterator::pointer>;

            namespace range
            {
               template< typename T>
               auto cast( T&& value) -> std::enable_if_t< std::is_const< std::remove_reference_t< T>>::value, const_range_type>
               {
                  using pointer = platform::binary::type::const_iterator::pointer;
                  return const_range_type{ 
                     reinterpret_cast< pointer>( &value),
                     detail::size< std::remove_reference_t<T>>()
                  };
               }

               template< typename T>
               auto cast( T&& value) -> std::enable_if_t< ! std::is_const< std::remove_reference_t< T>>::value, range_type>
               {
                  using pointer = platform::binary::type::iterator::pointer;
                  return range_type{ 
                     reinterpret_cast< pointer>( &value),
                     detail::size< std::remove_reference_t< T>>()
                  };
               }
            } // range
         } // detail

         namespace range
         {
            template< typename T, typename = std::enable_if_t< 
               std::is_trivially_copyable< traits::remove_cvref_t< T>>::value>>
            auto cast( T&& value)
            {
               return detail::range::cast( std::forward< T>( value));
            }  
         } // range


         template< typename T>
         constexpr auto size( T&&) -> std::enable_if_t< ! traits::has::size< T>::value, size_type>
         {
            return detail::size< typename std::remove_reference<T>::type>();
         }

         template< typename T>
         constexpr auto size( T&& value) -> std::enable_if_t< traits::has::size< T>::value, size_type>
         {
            return value.size();
         }

         template< typename D>
         std::enable_if_t< traits::is::binary::like< D>::value>
         set( D&& destination, int c = 0)
         {
            std::memset( &(*std::begin( destination)), c, size( destination));
         }

         template< typename T>
         std::enable_if_t< 
            traits::is_trivially_copyable< T>::value
            && ! traits::is::binary::like< T>::value>
         set( T& value, int c = 0)
         {
            set( range::cast( value), c);
         }


         template< typename T>
         auto append( T&& value, platform::binary::type& destination)
         {
            auto source = range::cast( value);

            destination.insert(
                  std::end( destination),
                  std::begin( source),
                  std::end( source));

            return destination.size();
         }

         template< typename S, typename D> 
         std::enable_if_t< 
            traits::is::binary::like< S>::value 
            && traits::is::binary::like< D>::value, size_type>
         copy( S&& source, D&& destination)
         {
            assert( size( destination) >= size( source));

            std::copy(
               std::begin( source),
               std::end( source),
               std::begin( destination));

            return size( source);
         }

         template< typename S, typename D> 
         std::enable_if_t< 
            ! traits::is::binary::like< S>::value
            && std::is_trivially_copyable< S>::value
            && traits::is::binary::like< D>::value, size_type>
         copy( S&& source, D&& destination)
         {
            return copy( range::cast( source), std::forward< D>( destination));
         }

         //!
         //! Copy from @p std::begin( source) + offset into @value
         //!
         //! @param buffer memory
         //! @param offset where to begin from in the source
         //! @param value value to be 'assigned'
         //! @return the new offset ( @p offset + memory::size( value) )
         //!
         template< typename S, typename T>
         size_type copy( S&& source, size_type offset, T& value)
         {
            auto destination = range::cast( value);
            assert( common::range::size( source) - offset >=  destination.size());
            auto range = common::range::make( std::begin( source) + offset, destination.size());

            return offset + copy( range, destination);
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
         auto guard( T* memory, D&& deleter)
         {
            return std::unique_ptr< T, D>( memory, std::forward< D>( deleter));
         }


      } // memory


   } // common
} // casual


