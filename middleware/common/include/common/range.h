//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "casual/platform.h"
#include "common/traits.h"

#include <iterator>
#include <functional>
#include <vector>
#include <sstream>
#include <array>

#include <cassert>

namespace casual 
{
   namespace common 
   {      
      template< typename Iter>
      struct Range
      {
         using iterator = Iter;
         using value_type = typename std::iterator_traits< iterator>::value_type;
         using pointer = typename std::iterator_traits< iterator>::pointer;
         using reference = typename std::iterator_traits< iterator>::reference;
         using difference_type = typename std::iterator_traits< iterator>::difference_type;
         using reverse_iterator = decltype( std::make_reverse_iterator( iterator{}));

         constexpr Range() = default;

         template< typename convertible_iterator, std::enable_if_t< std::is_convertible< convertible_iterator, iterator>::value>* dummy = nullptr>
         explicit constexpr Range( convertible_iterator first, convertible_iterator last) :  m_first( first), m_last( last) {}


         template< typename convertible_iterator, typename Size, std::enable_if_t< 
            std::is_convertible< convertible_iterator, iterator>::value
            && std::is_integral< Size>::value >* dummy = nullptr>
         explicit constexpr Range( convertible_iterator first, Size size) : m_first( first), m_last( first + size) {}

         //! conversion from Range with convertible iterators
         template< typename convertible_iterator, std::enable_if_t< std::is_convertible< convertible_iterator, iterator>::value>* dummy = nullptr>
         constexpr Range( Range< convertible_iterator> range) :  m_first( std::begin( range)), m_last( std::end( range)) {}
         
         //! conversion from a range with reverse-iterator
         template< typename ReverseIter>
         constexpr auto operator = ( Range< ReverseIter> range) 
            -> std::enable_if_t< std::is_same< ReverseIter, reverse_iterator>::value, Range&>
         {
            m_first = range.end().base();
            m_last = range.begin().base();
            return *this;
         }


         constexpr platform::size::type size() const { return std::distance( m_first, m_last);}

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

         constexpr reference front() { return *m_first;}
         constexpr const reference front() const { return *m_first;}

         constexpr reference back() { return *( std::prev( m_last));}
         constexpr const reference back() const { return *( std::prev( m_last));}

         constexpr reference at( const difference_type index) { return at( m_first, m_last, index);}
         constexpr const reference at( const difference_type index) const { return at( m_first, m_last, index);}

         friend constexpr Range operator + ( Range range, difference_type value) { return Range( range.m_first + value, range.m_last);}

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

         template< typename Iter, std::enable_if_t< common::traits::is::iterator_v< Iter>, int> = 0>
         auto make( Iter first, Iter last)
         {
            return Range< Iter>( first, last);
         }

         template< typename Iter, typename Count, std::enable_if_t< 
            common::traits::is::iterator_v< Iter> 
            && std::is_integral_v< Count>, int> = 0>
         auto make( Iter first, Count count)
         {
            return Range< Iter>( first, first + count);
         }

         template< typename C, std::enable_if_t< std::is_lvalue_reference_v< C> && common::traits::is::iterable_v< C>, int> = 0>
         auto make( C&& container)
         {
            return make( std::begin( container), std::end( container));
         }

         template< typename Iter>
         constexpr Range< Iter> make( Range< Iter> range)
         {
            return range;
         }

         namespace detail
         {
            // make sure we go back to base, if the iterator is reverse_iterator already.
            template< typename T>
            auto make_reverse_iterator( std::reverse_iterator< T> iterator) { return iterator.base();}

            template< typename Iter>
            auto make_reverse_iterator( Iter iterator) { return std::make_reverse_iterator( iterator);}
            
         } // detail

         template< typename R>
         constexpr auto reverse( R&& range)
         {
            // R can't be an owning r-value container.
            static_assert( ! traits::is::container::like_v< R> || std::is_lvalue_reference_v< R>, "");

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
         constexpr auto empty( C&& container) noexcept { return std::empty( std::forward< C>( container));}


         //! Returns the first value in the range
         //! @param range
         //! @return first value
         //! @throws std::out_of_range if range is empty
         template< typename R>
         auto front( R&& range) -> decltype( range.front())
         {
            assert( ! empty( range));

            return range.front();
         }

         template< typename R>
         auto back( R&& range) -> decltype( range.back())
         {
            assert( ! empty( range));
            
            return range.back();
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
