//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "casual/platform.h"
#include "casual/concepts.h"
#include "common/traits.h"

#include <iterator>
#include <concepts>
#include <span>
#include <cassert>

namespace casual 
{
   namespace common 
   {
   
      template< std::input_or_output_iterator Iter>
      struct Range
      {
         using iterator = Iter;
         using value_type = typename std::iterator_traits< iterator>::value_type;
         using pointer = typename std::iterator_traits< iterator>::pointer;
         using reference = typename std::iterator_traits< iterator>::reference;
         using difference_type = typename std::iterator_traits< iterator>::difference_type;
         using reverse_iterator = decltype( std::make_reverse_iterator( iterator{}));

         constexpr Range() = default;

         template< std::convertible_to< iterator> convertible_iterator>
         explicit constexpr Range( convertible_iterator first, convertible_iterator last) : m_first( first), m_last( last) {}

         //! conversion from Range with convertible iterators
         template< std::convertible_to< iterator> convertible_iterator>
         constexpr Range( Range< convertible_iterator> range) : m_first( std::begin( range)), m_last( std::end( range)) {}
         
         //! conversion from a range with reverse-iterator
         template< std::same_as< reverse_iterator> RI>
         constexpr Range& operator = ( Range< RI> range) 
         {
            m_first = range.end().base();
            m_last = range.begin().base();
            return *this;
         }


         constexpr platform::size::type size() const noexcept { return std::distance( m_first, m_last);}

         constexpr bool empty() const noexcept { return m_first == m_last;}
         constexpr explicit operator bool () const noexcept { return ! empty();}


         constexpr reference operator * () const noexcept { return *m_first;}
         constexpr iterator operator -> () const noexcept { return m_first;}

         constexpr Range& operator ++ () noexcept
         {
            ++m_first;
            return *this;
         }

         constexpr Range operator ++ ( int) noexcept
         {
            Range other{ *this};
            ++m_first;
            return other;
         }

         constexpr iterator begin() const noexcept { return m_first;}
         constexpr iterator end() const noexcept { return m_last;}

         constexpr Range& advance( difference_type value) { std::advance( m_first, value); return *this;}

         constexpr pointer data() const noexcept { return data( m_first, m_last);}

         constexpr reference at( const difference_type index) { return at( m_first, m_last, index);}
         constexpr const reference at( const difference_type index) const { return at( m_first, m_last, index);}

         operator std::span< value_type> () const noexcept { return std::span< value_type>( data(), size());}

      private:

         constexpr static pointer data( iterator first, iterator last) noexcept
         {
            if( first != last)
               return &( *first);

            return nullptr;
         }

         constexpr static reference at( iterator first, iterator last, const difference_type index)
         {
            assert( index < std::distance( first, last));
            return *( std::next( first, index));
         }

         iterator m_first = iterator{};
         iterator m_last = iterator{};
      };

      static_assert( ! std::is_trivial< Range< int*>>::value, "trivially copyable");




      //! This is not intended to be a serious attempt at a range-library
      //! Rather an abstraction that helps our use-cases and to get a feel for
      //! what a real range-library could offer. It's a work in progress
      namespace range
      {

         template< std::input_or_output_iterator Iter>
         constexpr auto make( Iter first, Iter last) noexcept
         {
            return Range< Iter>( first, last);
         }

         template< std::input_or_output_iterator Iter>
         constexpr auto make( Iter first, platform::size::type size) noexcept -> decltype( range::make( first, first + size))
         {
            return range::make( first, first + size);
         }

         template< typename C>
         auto make( C& container) -> decltype( range::make( std::begin( container), std::end( container)))
         {
            return range::make( std::begin( container), std::end( container));
         }

         template< typename T>
         constexpr Range< T> make( Range< T> range)
         {
            return range;
         }

         template< concepts::range R>
         constexpr auto next( R&& range, platform::size::type n = 1)
         {
            return range::make( std::next( std::begin( range), n), std::end( range));
         }

         namespace detail
         {
            // make sure we go back to base, if the iterator is reverse_iterator already.
            template< typename T>
            constexpr auto make_reverse_iterator( std::reverse_iterator< T> iterator) { return iterator.base();}

            template< typename Iter>
            constexpr auto make_reverse_iterator( Iter iterator) { return std::make_reverse_iterator( iterator);}
            
         } // detail

         template< typename R>
         constexpr auto reverse( R&& range)
         {
            // R can't be an owning r-value container.
            static_assert( ! concepts::container::like< R> || std::is_lvalue_reference_v< R>, "");

            return make( 
               detail::make_reverse_iterator( std::end( range)),
               detail::make_reverse_iterator( std::begin( range)));
         }

         //! @returns a _move range_ with move_iterator, that will 'move' (return rvalue reference) 
         //!    elements during subscripts
         template< typename R>
         constexpr auto move( R&& range)
         {
            return make( std::move_iterator{ std::begin( range)}, std::move_iterator{ std::end( range)});
         }
         

         template< typename C>
         struct type_traits
         {
            using type = decltype( make( std::begin( std::declval< C&>()), std::end( std::declval< C&>())));
         };


         template< typename C>
         using type_t = typename type_traits< C>::type;

         template< typename C>
         using const_type_t = typename type_traits< const C>::type;

         template< typename C>
         constexpr platform::size::type size( const C& container) noexcept { return std::ssize( container);}

         template< typename C>
         constexpr auto empty( C&& container) noexcept { return std::ranges::empty( std::forward< C>( container));}


         //! Returns the first value in the range
         //! @param range
         //! @return first value
         //! @throws std::out_of_range if range is empty
         template< typename R>
         auto front( R&& range) -> decltype( *std::begin( range))
         {
            assert( ! empty( range));
            return *std::begin( range);
         }

         template< typename R>
         auto back( R&& range) -> decltype( *std::prev( std::end( range)))
         {
            assert( ! empty( range));
            return *std::prev( std::end( range));
         }

         //! If @p range has size > 1, shorten range to size 1.
         template< typename R>
         auto zero_one( R&& range)
         {
            if( range)
               return range::make( std::begin( range), 1);

            return range;
         }

      } // range
   } // common 
} // casual 
