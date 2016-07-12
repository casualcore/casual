//!
//! casual
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MEMORY_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MEMORY_H_


#include "common/algorithm.h"


namespace casual
{
   namespace common
   {
      namespace memory
      {
         using size_type = decltype( sizeof( int));

         namespace detail
         {
            template< typename T>
            constexpr auto size() -> typename std::enable_if< ! std::is_array< T>::value, size_type>::type
            {
               return sizeof( T);
            }

            template< typename T>
            constexpr auto size() -> typename std::enable_if< std::is_array< T>::value, size_type>::type
            {
               return size< typename std::remove_extent< T>::type>() * std::extent< T>::value;
            }

            template< typename Iter>
            struct is_suitable_iterator : std::integral_constant<bool,
                  traits::iterator::is_random_access< Iter>::value
                  && traits::is_trivially_copyable< typename std::iterator_traits< Iter>::value_type>::value
                  >{};


         } // detail


         template< typename T>
         constexpr auto size( T&&) -> typename std::enable_if< traits::is_trivially_copyable< typename std::remove_reference<T>::type>::value, size_type>::type
         {
            return detail::size< typename std::remove_reference<T>::type>();
         }


         namespace range
         {
            using range_type = Range< platform::binary_type::iterator::pointer>;
            using const_range_type = Range< platform::binary_type::const_iterator::pointer>;

            namespace detail
            {
               template< typename T>
               struct traits { using range = range_type; using iterator = range::pointer; };

               template< typename T>
               struct traits< const T> { using range = const_range_type; using iterator = range::pointer; };

            } // detail

            template< typename T>
            typename std::enable_if<
                  ! std::is_pointer< typename std::remove_reference< T>::type>::value
                  && traits::is_trivially_copyable< typename std::remove_reference< T>::type>::value
                  ,
               typename detail::traits< T>::range>::type
            make( T& value)
            {
               auto first = reinterpret_cast< typename detail::traits< T>::iterator>( &value);
               return common::range::make( first, memory::size( value));
            }
         } // range

         template< typename Iter>
         typename std::enable_if< detail::is_suitable_iterator< Iter>::value>::type
         set( Range< Iter> destination, int c = 0)
         {
            std::memset( destination.data(), c, destination.size() * memory::size( typename std::iterator_traits< Iter>::value_type{}));
         }

         template< typename T>
         typename std::enable_if< traits::is_trivially_copyable< T>::value>::type
         set( T& value, int c = 0)
         {
            set( range::make( value), c);
         }




         template< typename T>
         auto append( const T& value, platform::binary_type& buffer) -> decltype( buffer.size())
         {
            auto first = reinterpret_cast< const unsigned char*>( &value);

            buffer.insert(
                  std::end( buffer),
                  first,
                  first + memory::size( value));

            return buffer.size();
         }

         template< typename InputIter, typename OutputIter>
         std::size_t copy( const Range< InputIter> source, Range< OutputIter> destination)
         {
            assert( destination.size() >= source.size());

            std::copy(
               std::begin( source),
               std::end( source),
               std::begin( destination));

            return source.size();
         }


         template< typename T, typename Iter>
         std::size_t copy( const T& source, Range< Iter> destination)
         {
            return copy( range::make( source), destination);
         }

         //!
         //! Copy from @p std::begin( buffer) + offset into @value
         //!
         //! @param buffer memory
         //! @param offset where to begin from in the buffer
         //! @param value value to be 'assigned'
         //! @return the new offset ( @p offset + memory::size( value) )
         //!
         template< typename T>
         std::size_t copy( const platform::binary_type& buffer, std::size_t offset, T& value)
         {
            assert( buffer.size() - offset >=  memory::size( value));

            auto source = common::range::make( std::begin( buffer) + offset, memory::size( value));
            auto destination = memory::range::make( value);

            return offset + copy( source, destination);
         }

      } // memory


   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MEMORY_H_
