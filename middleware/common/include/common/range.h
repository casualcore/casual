//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/platform.h"
#include "common/traits.h"

#include <iterator>
#include <functional>
#include <vector>
#include <sstream>

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
         using reverse_iterator = std::reverse_iterator< iterator>;

         constexpr Range() = default;
         explicit constexpr Range( iterator first, iterator last) :  m_first( first), m_last( last) {}


         template< typename Size, std::enable_if_t< std::is_integral< Size>::value>* dummy = nullptr>
         explicit constexpr Range( iterator first, Size size) : m_first( first), m_last( first + size) {}

         //!
         //! conversion from Range with convertable iterators
         //!
         template< typename convertible_iterator, std::enable_if_t< std::is_convertible< convertible_iterator, iterator>::value>* dummy = nullptr>
         constexpr Range( Range< convertible_iterator> range) :  m_first( range.begin()), m_last( range.end()) {}


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
         constexpr reverse_iterator rbegin() const noexcept { return reverse_iterator( end());}
         constexpr reverse_iterator rend() const noexcept { return reverse_iterator( begin());}


         constexpr Range& advance( difference_type value) { std::advance( m_first, value); return *this;}

         constexpr pointer data() const noexcept { return data( m_first, m_last);}

         constexpr reference front() { return *m_first;}
         constexpr const reference front() const { return *m_first;}

         constexpr reference back() { return *( m_last - 1);}
         constexpr const reference back() const { return *( m_last - 1);}

         constexpr reference at( const difference_type index) { return at( m_first, m_last, index);}
         constexpr const reference at( const difference_type index) const { return at( m_first, m_last, index);}

         friend constexpr Range operator + ( Range range, difference_type value) { return Range( range.m_first + value, range.m_last);}


      private:

         constexpr static pointer data( iterator first, iterator last) noexcept
         {
            if( first != last)
            {
               return &( *first);
            }
            return nullptr;
         }

         constexpr static reference at( iterator first, iterator last, const difference_type index)
         {
            if( std::distance( first, last) < index) { throw std::out_of_range{ std::to_string( index)};}

            return *( first + index);
         }


         iterator m_first = iterator{};
         iterator m_last = iterator{};
      };

      //!
      //! This is not intended to be a serious attempt at a range-library
      //! Rather an abstraction that helps our use-cases and to get a feel for
      //! what a real range-library could offer. It's a work in progress
      //!
      namespace range
      {
         namespace category
         {
            struct fixed {};
            struct output_iterator {};
            struct container {};

            template< typename T, class Enable = void>
            struct tag {};

            template< typename T>
            struct tag< T, std::enable_if_t< traits::container::is_sequence< T>::value>>
            {
               using type = category::container;
            };

            template< typename T>
            struct tag< T, std::enable_if_t< traits::iterator::is_output< T>::value>>
            {
               using type = category::output_iterator;
            };

            template< typename T>
            struct tag< T, std::enable_if_t< 
               traits::is::iterable< T>::value
               && ! traits::has::push_back< T>::value>>
            {
               using type = category::fixed;
            };            



            template< typename T>
            using tag_t = typename tag< T>::type; 


         }

         template< typename Iter, typename = std::enable_if_t< common::traits::is::iterator< Iter>::value>>
         Range< Iter> make( Iter first, Iter last)
         {
            return Range< Iter>( first, last);
         }

         template< typename Iter, typename Count, std::enable_if_t< 
            common::traits::is::iterator< Iter>::value 
            && std::is_integral< Count>::value>* dummy = nullptr>
         Range< Iter> make( Iter first, Count count)
         {
            return Range< Iter>( first, first + count);
         }

         template< typename C, typename = std::enable_if_t<std::is_lvalue_reference< C>::value && common::traits::is::iterable< C>::value>>
         auto make( C&& container)
         {
            return make( std::begin( container), std::end( container));
         }

         //!
         //! specialization for literal strings
         //!
         //! @attention omits the null terminator
         //!
         //! @param container
         //! @return
         template< std::size_t size>
         auto make( const char (&container)[ size])
         {
            return make( std::begin( container), size - 1);
         }


         template< typename C, typename = std::enable_if_t<std::is_lvalue_reference< C>::value && common::traits::is::reverse::iterable< C>::value>>
         auto make_reverse( C&& container)
         {
            return make( container.rbegin(), container.rend());
         }


         template< typename Iter>
         constexpr Range< Iter> make( Range< Iter> range)
         {
            return range;
         }

         template< typename C>
         struct traits
         {
            using type = decltype( make( C().begin(), C().end()));
         };

         template< typename C>
         struct type_traits
         {
            using type = decltype( make( std::begin( std::declval< C&>()), std::end( std::declval< C&>())));
         };


         template< typename C>
         using type_t = typename type_traits< C>::type;

         template< typename C>
         using const_type_t = typename type_traits< const C>::type;

         template< typename R, std::enable_if_t< std::is_array< std::remove_reference_t< R>>::value>* dummy = nullptr>
         constexpr platform::size::type size( R&& range) { return sizeof( R) / sizeof( *range);}

         template< typename R, std::enable_if_t< common::traits::has::size< R>::value>* dummy = nullptr>
         constexpr platform::size::type size( R&& range) { return range.size();}

         template< typename R, std::enable_if_t< std::is_array< std::remove_reference_t< R>>::value>* dummy = nullptr>
         constexpr bool empty( R&& range) { return false;}

         template< typename R, std::enable_if_t< common::traits::has::empty< R>::value>* dummy = nullptr>
         constexpr bool empty( R&& range) { return range.empty();}

         namespace position
         {
            //!
            //! @return returns true if @lhs overlaps @rhs in some way.
            //!
            template< typename R1, typename R2>
            bool overlap( R1&& lhs, R2&& rhs)
            {
               return std::end( lhs) >= std::begin( rhs) && std::begin( lhs) <= std::end( rhs);
            }


            //!
            //! @return true if @lhs is adjacent to @rhs or @rhs is adjacent to @lhs
            //!
            template< typename R1, typename R2>
            bool adjacent( R1&& lhs, R2&& rhs)
            {
               return std::end( lhs) + 1 == std::begin( rhs) || std::end( lhs) + 1 == std::begin( rhs);
            }

            //!
            //! @return true if @rhs is a sub-range to @lhs
            //!
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
            std::vector< std::reference_wrapper< std::remove_reference_t< decltype( *std::begin( range))>>> result;
            result.reserve( size( range));

            std::copy( std::begin( range), std::end( range), std::back_inserter( result));

            return result;
         }


         template< typename R>
         std::string to_string( R&& range)
         {
            std::ostringstream out;
            out << make( range);
            return out.str();
         }

         //!
         //! Returns the first value in the range
         //!
         //! @param range
         //! @return first value
         //! @throws std::out_of_range if range is empty
         //!
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

         //!
         //! If @p range has size > 1, shorten range to size 1.
         //!
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
