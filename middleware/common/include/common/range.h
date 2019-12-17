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

         //! conversion from Range with convertable iterators
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

         friend constexpr bool operator == ( const Range& lhs, const Range& rhs) 
         {
            return equal( lhs, rhs);
         };
         friend constexpr bool operator != ( const Range& lhs, const Range& rhs) { return !( lhs == rhs);}

         template< typename C, std::enable_if_t< ! traits::is::string::like< C>::value, int> = 0>
         friend constexpr bool operator == ( const Range& lhs, const C& rhs)
         {
            return equal( lhs, rhs);
         }

         template< typename C, std::enable_if_t< ! traits::is::string::like< C>::value, int> = 0>
         friend constexpr bool operator == ( C& lhs, const Range< Iter>& rhs)
         {
            return equal( lhs, rhs);
         }

      private:

         template< typename L, typename R> 
         constexpr static bool equal( L&& lhs, R&& rhs) { return std::equal( std::begin( lhs), std::end( lhs), std::begin( rhs), std::end( rhs));}

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
         namespace category
         {
            struct fixed {};
            struct output_iterator {};
            struct associative {};
            struct container {};

            template< typename T, class Enable = void>
            struct tag {};

            template< typename T>
            struct tag< T, std::enable_if_t< traits::is::container::sequence::like< T>::value>>
            {
               using type = category::container;
            };

            template< typename T>
            struct tag< T, std::enable_if_t< traits::is::container::associative::like< T>::value>>
            {
               using type = category::associative;
            };

            template< typename T>
            struct tag< T, std::enable_if_t< traits::is::output::iterator< T>::value>>
            {
               using type = category::output_iterator;
            };

            template< typename T>
            struct tag< T, std::enable_if_t< traits::is::container::array::like< T>::value>>
            {
               using type = category::fixed;
            };            

            template< typename T>
            using tag_t = typename tag< T>::type; 

         } // category

         template< typename Iter, std::enable_if_t< common::traits::is::iterator< Iter>::value, int> = 0>
         auto make( Iter first, Iter last)
         {
            return Range< Iter>( first, last);
         }

         template< typename Iter, typename Count, std::enable_if_t< 
            common::traits::is::iterator< Iter>::value 
            && std::is_integral< Count>::value, int> = 0>
         auto make( Iter first, Count count)
         {
            return Range< Iter>( first, first + count);
         }

         template< typename C, std::enable_if_t< std::is_lvalue_reference< C>::value && common::traits::is::iterable< C>::value, int> = 0>
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
            static_assert( ! traits::is::container::like< R>::value || std::is_lvalue_reference< R>::value, "");

            return make( 
               detail::make_reverse_iterator( std::end( range)),
               detail::make_reverse_iterator( std::begin( range)));
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

         template< typename R, std::enable_if_t< std::is_array< std::remove_reference_t< R>>::value, int> = 0>
         constexpr platform::size::type size( R&& range) { return sizeof( R) / sizeof( *range);}

         template< typename R, std::enable_if_t< common::traits::has::size< R>::value, int> = 0>
         constexpr platform::size::type size( R&& range) { return range.size();}

         template< typename R, std::enable_if_t< std::is_array< std::remove_reference_t< R>>::value, int> = 0>
         constexpr bool empty( R&& range) { return false;}

         template< typename R, std::enable_if_t< common::traits::has::empty< R>::value, int> = 0>
         constexpr bool empty( R&& range) { return range.empty();}

         namespace position
         {
            //! @return returns true if @lhs overlaps @rhs in some way.
            template< typename R1, typename R2>
            bool overlap( R1&& lhs, R2&& rhs)
            {
               return std::end( lhs) >= std::begin( rhs) && std::begin( lhs) <= std::end( rhs);
            }

            //! @return true if @lhs is adjacent to @rhs or @rhs is adjacent to @lhs
            template< typename R1, typename R2>
            bool adjacent( R1&& lhs, R2&& rhs)
            {
               return std::end( lhs) + 1 == std::begin( rhs) || std::end( lhs) + 1 == std::begin( rhs);
            }

            //! @return true if @rhs is a sub-range to @lhs
            template< typename R1, typename R2>
            bool includes( R1&& lhs, R2&& rhs)
            {
               return std::begin( lhs) <= std::begin( rhs) && std::end( lhs) >= std::end( rhs);
            }

            template< typename R>
            auto intersection( R&& lhs, R&& rhs) -> decltype( range::make( std::forward< R>( lhs)))
            {
               if( overlap( lhs, rhs))
               {
                  auto result = range::make( lhs);

                  if( std::begin( lhs) < std::begin( rhs)) result.first = std::begin( rhs);
                  if( std::end( lhs) > std::end( rhs)) result.last = std::end( rhs);

                  return result;
               }
               return {};
            }

            template< typename R1, typename R2>
            auto subtract( R1&& lhs, R2&& rhs)
            {
               using range_type = range::type_t< R1>;

               if( overlap( lhs, rhs))
               {
                  if( std::begin( lhs) < std::begin( rhs) && std::end( lhs) > std::end( rhs))
                  {
                     return std::make_tuple(
                           range::make( std::begin( lhs), std::begin( rhs)),
                           range::make( std::end( rhs), std::end( lhs)));
                  }
                  else if( std::begin( lhs) < std::begin( rhs))
                  {
                     return std::make_tuple(
                           range::make( std::begin( lhs), std::begin( rhs)),
                           range_type{});
                  }
                  else if( std::end( lhs) > std::end( rhs))
                  {
                     return std::make_tuple(
                           range::make( std::end( rhs), std::end( lhs)),
                           range_type{});
                  }

                  return std::make_tuple(
                        range_type{},
                        range_type{});

               }
               return std::make_tuple( range::make( lhs), range_type{});
            }

         } // position


         template< typename R>
         auto to_vector( R&& range)
         {
            std::vector< typename std::decay< decltype( *std::begin( range))>::type> result;
            result.reserve( size( range));

            std::copy( std::begin( range), std::end( range), std::back_inserter( result));

            return result;
         }

         template< typename R>
         auto to_reference( R&& range)
         {
            using result_typ = std::vector< std::reference_wrapper< std::remove_reference_t< decltype( *std::begin( range))>>>;
            return result_typ( std::begin( range), std::end( range));
         }


         template< typename R>
         std::string to_string( R&& range)
         {
            std::ostringstream out;
            out << make( range);
            return out.str();
         }

         //! Returns the first value in the range
         //!
         //! @param range
         //! @return first value
         //! @throws std::out_of_range if range is empty
         template< typename R>
         decltype( auto) front( R&& range)
         {
            assert( ! empty( range));

            return range.front();
         }

         template< typename R>
         decltype( auto) back( R&& range)
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
