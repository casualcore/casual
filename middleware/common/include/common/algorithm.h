//!
//! casual
//!

#ifndef CASUAL_COMMON_ALGORITHM_H_
#define CASUAL_COMMON_ALGORITHM_H_


#include "common/traits.h"
#include "common/error.h"
#include "common/platform.h"
#include "common/move.h"
#include "common/cast.h"
#include "common/optional.h"

#include <algorithm>
#include <numeric>
#include <iterator>
#include <type_traits>
#include <ostream>
#include <sstream>

#include <functional>
#include <memory>

#include <cassert>
#include <cstring>


namespace casual
{
   namespace common
   {




      namespace scope
      {

         //!
         //! executes an action ones.
         //! If the action has not been executed the
         //! destructor will perform the execution
         //!
         template< typename E>
         struct basic_execute
         {
            using execute_type = E;

            basic_execute( execute_type&& execute) : m_execute( std::move( execute)) {}

            ~basic_execute()
            {
               try
               {
                  (*this)();
               }
               catch( ...)
               {
                  error::handler();
               }
            }

            basic_execute( basic_execute&&) noexcept = default;
            basic_execute& operator = ( basic_execute&&) noexcept = default;

            //!
            //! executes the actions ones.
            //! no-op if already executed
            //!
            void operator () ()
            {
               if( ! m_moved)
               {
                  m_execute();
                  release();
               }
            }

            void release() { m_moved.release();}

         private:
            execute_type m_execute;
            move::Moved m_moved;
         };

         //!
         //! returns an executer that will do an action ones.
         //! If the action has not been executed the
         //! destructor will perform the execution
         //!
         template< typename E>
         auto execute( E&& executor) -> decltype( basic_execute< E>{ std::forward< E>( executor)})
         {
            return basic_execute< E>{ std::forward< E>( executor)};
         }

      } // scope

      namespace execute
      {
         //!
         //! Execute @p executor once
         //!
         //! @note To be used with lambdas only.
         //! @note Every invocation of the same type will only execute once.
         //!
         template< typename E>
         void once( E&& executor)
         {
            static bool done = false;

            if( ! done)
            {
               done = true;
               executor();
            }
         }

      } // execute

      namespace chain
      {
         namespace link
         {

            template< typename L, typename T1, typename T2>
            struct basic_link
            {
               using link_type = L;

               basic_link( T1&& left, T2&& right) : left( std::forward< T1>( left)), right( std::forward< T2>( right)) {}

               template< typename ...Values>
               auto operator() ( Values&& ...values) const -> decltype( link_type::link( std::declval< T1>(),  std::declval< T2>(), std::forward< Values>( values)...))
               {
                  return link_type::link( left, right, std::forward< Values>( values)...);
               }

               template< typename ...Values>
               auto operator() ( Values&& ...values) -> decltype( link_type::link( std::declval< T1>(),  std::declval< T2>(), std::forward< Values>( values)...))
               {
                  return link_type::link( left, right, std::forward< Values>( values)...);
               }

            private:
               T1 left;
               T2 right;
            };

            template< typename Link, typename Arg>
            Arg make( Arg&& param) //-> decltype( std::forward< Arg>( param))
            {
               return param; //std::forward< Arg>( param);
            }

            template< typename L, typename Arg, typename... Args>
            auto make( Arg&& param, Args&&... params) -> basic_link< L, Arg, decltype( make< L>( std::forward< Args>( params)...))>
            {
               using nested_type = decltype( make< L>( std::forward< Args>( params)...));
               return basic_link< L, Arg, nested_type>( std::forward< Arg>( param), make< L>( std::forward< Args>( params)...));
            }

            struct Nested
            {
               template< typename T1, typename T2, typename T>
               static auto link( T1&& left, T2&& right, T&& value) -> decltype( left( right( value)))
               {
                  return left( right( value));
               }
            };

            struct And
            {
               template< typename T1, typename T2, typename T>
               static auto link( T1&& left, T2&& right, T&& value) -> decltype( left( value) && right( value))
               {
                  return left( value) && right( value);
               }
            };

            struct Or
            {
               template< typename T1, typename T2, typename T>
               static auto link( T1&& left, T2&& right, T&& value) -> decltype( left( value) || right( value))
               {
                  return left( value) || right( value);
               }
            };

            struct Order
            {
               template< typename T1, typename T2, typename V1, typename V2>
               static auto link( T1&& left, T2&& right, V1&& lhs, V2&& rhs) -> decltype( left( lhs, rhs))
               {
                  if( left( lhs, rhs))
                     return true;
                  if( left( rhs, lhs))
                     return false;
                  return right( lhs, rhs);
               }
            };

         } // link

         template< typename Link>
         struct basic_chain
         {
            template< typename... Args>
            static auto link( Args&&... params) -> decltype( link::make< Link>( std::forward< Args>( params)...))
            {
               return link::make< Link>( std::forward< Args>( params)...);
            }
         };


         using Nested = basic_chain< link::Nested>;
         using And = basic_chain< link::And>;
         using Or = basic_chain< link::Or>;
         using Order = basic_chain< link::Order>;


      } // chain

      namespace extract
      {
         struct Get
         {
            template< typename T>
            auto operator () ( T&& value) const -> decltype( value.get())
            {
               return value.get();
            }

         };


         struct Second
         {
            template< typename T>
            auto operator () ( T&& value) const -> decltype( value.second)
            {
               return value.second;
            }
         };

      }

      namespace compare
      {
         template< typename T>
         auto inverse( T&& functor) -> decltype( std::bind( std::forward< T>( functor), std::placeholders::_2, std::placeholders::_1))
         {
            return std::bind( std::forward< T>( functor), std::placeholders::_2, std::placeholders::_1);
         }

         template< typename T>
         struct Inverse
         {
            template< typename ...Args>
            Inverse( Args&&... args) : m_functor( std::forward< Args>( args)...) {}

            template< typename V1, typename V2>
            auto operator () ( V1&& lhs, V2&& rhs) const -> decltype( std::declval< T>()( std::forward< V2>( rhs), std::forward< V1>( lhs)))
            {
               return m_functor( std::forward< V2>( rhs), std::forward< V1>( lhs));
            }

            template< typename V1, typename V2>
            auto operator () ( V1&& lhs, V2&& rhs) -> decltype( std::declval< T>()( std::forward< V2>( rhs), std::forward< V1>( lhs)))
            {
               return m_functor( std::forward< V2>( rhs), std::forward< V1>( lhs));
            }

         private:
            T m_functor;
         };

      } // compare

      namespace detail
      {
         namespace coalesce
         {
            template< typename T>
            bool empty( T& value) { return value.empty();}

            template< typename T>
            bool empty( T* value) { return value == nullptr;}

            template< typename T>
            bool empty( optional< T>& value) { return ! value.has_value();}

            template< typename R, typename T>
            R implementation( T&& value)
            {
               return std::forward< T>( value);
            }

            template< typename R, typename T, typename... Args>
            R implementation( T&& value, Args&&... args)
            {
               if( empty( value)) { return implementation< R>( std::forward< Args>( args)...);}
               return std::forward< T>( value);
            }

         } // coalesce

      } // detail


      //!
      //! Chooses the first argument that is not 'empty'
      //!
      //! @note if all parameters has exactly the same type the return type will be
      //!  exactly that. Otherwise it will be the common type of all types
      //!
      //! @return the first argument that is not 'empty'
      //!
      template< typename T, typename... Args>
      auto coalesce( T&& value,  Args&&... args)
         -> typename std::conditional<
               traits::is_same< T, Args...>::value,
               T, // only if T1 and T1 are exactly the same
               typename std::common_type< T, Args...>::type
            >::type
      {

         using return_type = typename std::conditional<
               traits::is_same< T, Args...>::value,
               T, // only if T1 and T1 are exactly the same
               typename std::common_type< T, Args...>::type
            >::type;

         return detail::coalesce::implementation< return_type>( std::forward< T>( value), std::forward< Args>( args)...);
      }


      template< typename Iter>
      struct Range
      {
         using iterator = Iter;
         using value_type = typename std::iterator_traits< iterator>::value_type;
         using pointer = typename std::iterator_traits< iterator>::pointer;
         using reference = typename std::iterator_traits< iterator>::reference;
         using difference_type = typename std::iterator_traits< iterator>::difference_type;

         Range() : m_size{ 0} {}
         Range( iterator first, std::size_t size) : m_first( first), m_size( size) {}
         Range( iterator first, iterator last) : Range( first, std::distance( first, last)) {}


         std::size_t size() const
         {
            return m_size;
         }

         bool empty() const
         {
            return m_size == 0;
         }

         operator bool () const
         {
            return ! empty();
         }

         template<typename T>
         operator T() const = delete;


         reference operator * () const { return *m_first;}
         iterator operator -> () const { return m_first;}


         Range operator ++ ()
         {
            Range other{ *this};
            advance( 1);
            return other;
         }

         Range& operator ++ ( int)
         {
            advance( 1);
            return *this;
         }

         iterator begin() const { return m_first;}
         iterator end() const { return std::next( m_first,  m_size);}

         void advance( std::size_t size)
         {
            //assert( size <= m_size);
            m_first += size;
            m_size -= size;
         }


         pointer data()
         {
            if( ! empty())
            {
               return &( *m_first);
            }
            return nullptr;
         }

         const pointer data() const
         {
            if( ! empty())
            {
               return &( *m_first);
            }
            return nullptr;
         }

         reference front() { return *m_first;}

         reference at( const difference_type index)
         {
            if( m_size <= static_cast< decltype( m_size)>( index)) { throw std::out_of_range{ std::to_string( index)};}

            return *( m_first + index);
         }

         const reference at( const difference_type index) const
         {
            if( m_size <= static_cast< decltype( m_size)>( index)){ throw std::out_of_range{ std::to_string( index)};}

            return *( m_first + index);
         }


      private:
         iterator m_first;
         std::size_t m_size;
      };







      namespace detail
      {
         template< typename T>
         struct negate
         {

            negate( T&& functor) : m_functor{ std::move( functor)}
            {

            }

            template< typename... Args>
            bool operator () ( Args&& ...args) const
            {
               return ! m_functor( std::forward< Args>( args)...);
            }

            template< typename... Args>
            bool operator () ( Args&& ...args)
            {
               return ! m_functor( std::forward< Args>( args)...);
            }

         private:
            T m_functor;
         };

         template< typename R>
         struct size_traits
         {
            static std::size_t size( const R& range) { return range.size();}
         };

         template< typename T, std::size_t s>
         struct size_traits< T[ s]>
         {
            constexpr static std::size_t size( const T(&)[ s]) { return s;}
         };

      } // detail



      template< typename T>
      detail::negate< T> negate( T&& functor)
      {
         return detail::negate< T>{ std::forward< T>( functor)};
      }


      namespace make
      {
         template< typename T, typename... Args>
         std::unique_ptr< T> unique( Args&&... args)
         {
            return std::unique_ptr< T>( new T( std::forward< Args>( args)...));
         }

         template< typename P, typename D>
         auto deleter( P* pointer, D&& deleter) -> std::unique_ptr< P, D>
         {
            return std::unique_ptr< P, D>( pointer, std::forward< D>( deleter));
         }
      } // make


      //!
      //! This is not intended to be a serious attempt at a range-library
      //! Rather an abstraction that helps our use-cases and to get a feel for
      //! what a real range-library could offer. It's a work in progress
      //!
      namespace range
      {

         template< typename Iter>
         Range< Iter> make( Iter first, Iter last)
         {
            return Range< Iter>( first, last);
         }

         template< typename Iter>
         Range< Iter> make( Iter first, std::size_t count)
         {
            return Range< Iter>( first, first + count);
         }

         template< typename C, class = typename std::enable_if<std::is_lvalue_reference< C>::value>::type >
         auto make( C&& container) -> decltype( make( std::begin( container), std::end( container)))
         {
            return make( std::begin( container), std::end( container));
         }


         template< typename C, class = typename std::enable_if<std::is_lvalue_reference< C>::value>::type >
         auto make_reverse( C&& container) -> decltype( make( container.rbegin(), container.rend()))
         {
            return make( container.rbegin(), container.rend());
         }

         template< typename Iter>
         auto make_reverse( Range< Iter> range) -> decltype( make( std::reverse_iterator< Iter>( range.end()), std::reverse_iterator< Iter>( range.begin())))
         {
            return make( std::reverse_iterator< Iter>( range.end()), std::reverse_iterator< Iter>( range.begin()));
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
            using type = decltype( make( std::declval< C>().begin(), std::size_t{}));
         };


         template< typename C>
         using type_t = typename type_traits< C>::type;

         template< typename C>
         using const_type_t = typename type_traits< const C>::type;

         template< typename R>
         typename std::enable_if< std::is_array< typename std::remove_reference< R>::type>::value, std::size_t>::type
         size( R&& range) { return sizeof( R);}

         template< typename R>
         typename std::enable_if< ! std::is_array< typename std::remove_reference< R>::type>::value, std::size_t>::type
         size( R&& range) { return range.size();}


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
               -> std::tuple< decltype( range::make( std::forward< R1>( lhs))), decltype( range::make( std::forward< R1>( lhs)))>
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
         std::size_t size( const R& range)
         {
            return detail::size_traits< typename std::remove_reference< R>::type>::size( range);
         }

         template< typename R>
         auto to_vector( R&& range) -> std::vector< typename std::decay< decltype( *std::begin( range))>::type>
         {
            std::vector< typename std::decay< decltype( *std::begin( range))>::type> result;
            result.reserve( size( range));

            std::copy( std::begin( range), std::end( range), std::back_inserter( result));

            return result;
         }

         template< typename R>
         auto to_reference( R&& range) -> std::vector< std::reference_wrapper< common::traits::remove_reference_t< decltype( *std::begin( range))>>>
         {
            std::vector< std::reference_wrapper< common::traits::remove_reference_t< decltype( *std::begin( range))>>> result;
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
         auto front( R&& range) -> decltype( make( std::forward< R>( range)).front())
         {
            auto result = make( std::forward< R>( range));
            if( result)
            {
               return result.front();
            }
            throw std::out_of_range{ "range::front - range is empty"};
         }

         template< typename R>
         auto back( R&& range) -> decltype( make( std::forward< R>( range)).back())
         {
            auto result = make( std::forward< R>( range));
            if( result)
            {
               return result.back();
            }
            throw std::out_of_range{ "range::back - range is empty"};
         }




         template< typename R>
         auto reverse( R&& range) -> decltype( std::forward< R>( range))
         {
            std::reverse( std::begin( range), std::end( range));
            return std::forward< R>( range);
         }


         template< typename R, typename C>
         auto sort( R&& range, C compare) -> decltype( std::forward< R>( range))
         {
            std::sort( std::begin( range), std::end( range), compare);
            return std::forward< R>( range);
         }

         template< typename R>
         auto sort( R&& range) -> decltype( std::forward< R>( range))
         {
            std::sort( std::begin( range), std::end( range));
            return std::forward< R>( range);
         }

         template< typename R, typename C>
         auto stable_sort( R&& range, C compare) -> decltype( std::forward< R>( range))
         {
            std::stable_sort( std::begin( range), std::end( range), compare);
            return std::forward< R>( range);
         }

         template< typename R>
         auto stable_sort( R&& range) -> decltype( std::forward< R>( range))
         {
            std::stable_sort( std::begin( range), std::end( range));
            return std::forward< R>( range);
         }

         template< typename R, typename P>
         auto partition( R&& range, P predicate) -> decltype( std::make_tuple( make( std::forward< R>( range)), make( std::forward< R>( range))))
         {
            auto middle = std::partition( std::begin( range), std::end( range), predicate);
            return std::make_tuple( make( std::begin( range), middle), make( middle, std::end( range)));
         }


         template< typename R, typename P>
         auto stable_partition( R&& range, P predicate) -> decltype( std::make_tuple( make( std::forward< R>( range)), make( std::forward< R>( range))))
         {
            auto middle = std::stable_partition( std::begin( range), std::end( range), predicate);
            return std::make_tuple( make( std::begin( range), middle), make( middle, std::end( range)));
         }


         template< typename R, typename OutIter>
         typename std::enable_if< common::traits::iterator::is_output< OutIter>::value, OutIter>::type
         copy( R&& range, OutIter output)
         {
            return std::copy( std::begin( range), std::end( range), output);
         }

         template< typename R, typename OutIter, typename P>
         typename std::enable_if< common::traits::iterator::is_output< OutIter>::value, OutIter>::type
         copy_if( R&& range, OutIter output, P predicate)
         {
            return std::copy_if( std::begin( range), std::end( range), output, predicate);
         }

         template< typename Range, typename Container>
         Container& append( Range&& source, Container& destination)
         {
            destination.insert( std::end( destination), std::begin( source), std::end( source));
            return destination;
         }




         //!
         //! Copies from @p source to @p destination
         //!   size of destination dictates the maximum that will be
         //!   copied
         //!
         //! @param source
         //! @param destination sets the maximum what will be copied
         //!
         template< typename Range1, typename Range2>
         void copy_max( Range1&& source, Range2&& destination)
         {
            auto max = range::size( destination);
            auto wanted = range::size( source);

            if( wanted <= max)
            {
               copy( source, destination);
            }
            else
            {
               std::copy_n( std::begin( source), max, std::begin( destination));
            }
         }

         template< typename R, typename Size, typename Iter>
         void copy_max( R&& range, Size size, Iter output)
         {
            auto inputRange = make( range);

            if( inputRange.size() <= static_cast< decltype( inputRange.size())>( size))
            {
               copy( inputRange, output);
            }
            else
            {
               std::copy_n( std::begin( inputRange), size, output);
            }
         }

         template< typename R, typename C>
         auto move( R&& range, C& container) -> decltype( make( container))
         {
            std::move( std::begin( range), std::end( range), std::back_inserter( container));
            return make( container);
         }

         template< typename R, typename C, typename P>
         C& move_if( R&& range, C& container, P predicate)
         {
            auto first = std::begin( range);
            while (first != std::end( range))
            {
               if( predicate( *first))
               {
                  container.push_back( std::move(*first));
               }
              ++first;
            }
            return container;
         }


         //!
         //! Transform @p range to @p container, using @p transform
         //!
         //! @param range source range/container
         //! @param container output/holder container
         //!
         //! @return range containing the inserted transformed values, previous values in @p container is excluded.
         //!
         template< typename R, typename C, typename T>
         auto transform( R&& range, C& container, T transform) -> decltype( make( container))
         {
            std::transform( std::begin( range), std::end( range), std::back_inserter( container), transform);
            return make( std::end( container) - range.size(), std::end( container));
         }

         //!
         //! Transform @p range, using @p transform
         //!
         //! @param range source range/container
         //!
         //! @return std::vector with the transformed values
         //!
         template< typename R, typename T>
         auto transform( R&& range, T transformer) -> std::vector< typename std::remove_reference< decltype( transformer( *std::begin( range)))>::type>
         {
            std::vector< typename std::remove_reference< decltype( transformer( *std::begin( range)))>::type> result;
            result.reserve( range.size());
            std::transform( std::begin( range), std::end( range), std::back_inserter( result), transformer);
            return result;
         }

         template< typename InputIter1, typename InputIter2, typename outputIter, typename T>
         outputIter transform( Range< InputIter1> range1, Range< InputIter2> range2, outputIter output, T transform)
         {
            assert( range1.size() == range2.size());
            return std::transform(
               std::begin( range1), std::end( range1),
               std::begin( range2),
               output, transform);
         }

         //!
         //! Transform @p range, using @p transform
         //!
         //! @param range source range/container
         //!
         //! @return std::vector with the transformed values
         //!
         /*
         template< typename R, typename P, typename T>
         auto transform_if( R&& range, P predicate, T transformer) -> std::vector< typename std::remove_reference< decltype( transformer( *std::begin( range)))>::type>
         {
            std::vector< typename std::remove_reference< decltype( transformer( *std::begin( range)))>::type> result;

            for( auto&& value : range)
            {
               if( predicate( value))
               {
                  result.push_back( transformer( value));
               }
            }
            return result;
         }
         */

         //!
         //! Applies std::unique on [std::begin( range), std::end( range) )
         //!
         //! @return the unique range
         //!
         template< typename R>
         auto unique( R&& range) -> decltype( make( range))
         {
            return make( std::begin( range), std::unique( std::begin( range), std::end( range)));
         }

         //!
         //! Trims @p container so it matches @p range
         //!
         //! @return range that matches the trimmed @p container
         //!
         template< typename C, typename R>
         C& trim( C& container, R&& range)
         {
            auto index = std::begin( range) - std::begin( container);
            container.erase( std::end( range), std::end( container));
            container.erase( std::begin( container), std::begin( container) + index);
            return container;
         }


         template< typename C, typename Iter>
         C& erase( C& container, Range< Iter> range)
         {
            container.erase( std::begin( range), std::end( range));
            return container;
         }

         //!
         //! Erases occurrences from an associative container that
         //! fulfill the predicate
         //!
         //! @param container associative
         //! @param predicate that takes C::mapped_type as parameter and returns bool
         //! @return the container
         //!
         template< typename C, typename P>
         C& erase_if( C& container, P&& predicate)
         {
            for( auto current = std::begin( container); current != std::end( container);)
            {
               if( predicate( current->second))
               {
                  current = container.erase( current);
               }
               else
               {
                  ++current;
               }
            }
            return container;
         }

         template< typename R, typename T>
         auto remove( R&& range, const T& value) -> decltype( make( std::forward< R>( range)))
         {
            return make( std::begin( range), std::remove( std::begin( range), std::end( range), value));
         }


         template< typename R, typename P>
         auto remove_if( R&& range, P predicate) -> decltype( make( std::forward< R>( range)))
         {
            return make( std::begin( range), std::remove_if( std::begin( range), std::end( range), predicate));
         }


         template< typename R1, typename R2, typename P>
         bool equal( R1&& lhs, R2&& rhs, P predicate)
         {
            if( size( lhs) != size( rhs)) { return false;}
            return std::equal( std::begin( lhs), std::end( lhs), std::begin( rhs), predicate);
         }


         template< typename R1, typename R2>
         bool equal( R1&& lhs, R2&& rhs)
         {
            if( size( lhs) != size( rhs)) { return false;}
            return std::equal( std::begin( lhs), std::end( lhs), std::begin( rhs));
         }


         template< typename R, typename T>
         auto accumulate( R&& range, T&& value) -> decltype( *std::begin( range) + value)
         {
            return std::accumulate( std::begin( range), std::end( range), std::forward< T>( value));
         }

         template< typename R, typename T, typename F>
         auto accumulate( R&& range, T&& value, F&& functor) -> decltype( functor( std::forward< T>( value), *std::begin( range)))
         {
            return std::accumulate(
                  std::begin( range),
                  std::end( range),
                  std::forward< T>( value),
                  std::forward< F>( functor));
         }



         template< typename R, typename P>
         bool all_of( R&& range, P predicate)
         {
            return std::all_of( std::begin( range), std::end( range), predicate);
         }

         template< typename R, typename P>
         bool any_of( R&& range, P predicate)
         {
            return std::any_of( std::begin( range), std::end( range), predicate);
         }

         template< typename R, typename P>
         bool none_of( R&& range, P predicate)
         {
            return std::none_of( std::begin( range), std::end( range), predicate);
         }


         template< typename R, typename F>
         auto for_each( R&& range, F functor) -> decltype( std::forward< R>( range))
         {
            std::for_each( std::begin( range), std::end( range), functor);
            return std::forward< R>( range);
         }


         //!
         //! associate container specialization
         //!
         template< typename R, typename T>
         auto find( R&& range, T&& value)
            -> typename std::enable_if<
               common::traits::container::is_associative< common::traits::decay_t< R>>::value,
               decltype( make( std::forward< R>( range)))>::type
         {
            return make( range.find( value), std::end( range));
         }

         //!
         //! non associate container specialization
         //!
         template< typename R, typename T>
         auto find( R&& range, T&& value)
            -> typename std::enable_if<
               ! common::traits::container::is_associative< common::traits::decay_t< R>>::value,
               decltype( make( std::forward< R>( range)))>::type
         {
            return make( std::find( std::begin( range), std::end( range), std::forward< T>( value)), std::end( range));
         }



         template< typename R, typename P>
         auto find_if( R&& range, P predicate) -> decltype( make( std::forward< R>( range)))
         {
            return make( std::find_if( std::begin( range), std::end( range), predicate), std::end( range));
         }


         template< typename R>
         auto adjacent_find( R&& range) -> decltype( make( std::forward< R>( range)))
         {
            return make( std::adjacent_find( std::begin( range), std::end( range)), std::end( range));
         }


         template< typename R, typename P>
         auto adjacent_find( R&& range, P predicate) -> decltype( make( std::forward< R>( range)))
         {
            return make( std::adjacent_find( std::begin( range), std::end( range), predicate), std::end( range));
         }


         //!
         //! Divide @p range in two parts [range-first, divider), [divider, range-last).
         //! where divider is the first occurrence that is equal to @p value
         //!
         //! @return a tuple with the two ranges
         //!
         template< typename R1, typename T>
         auto divide( R1&& range, T&& value) ->  decltype( std::make_tuple( make( std::forward< R1>( range)), make( std::forward< R1>( range))))
         {
            auto divider = std::find(
                  std::begin( range), std::end( range),
                  std::forward< T>( value));

            return std::make_tuple( make( std::begin( range), divider), make( divider, std::end( range)));
         }

         //!
         //! Split @p range in two parts [range-first, divider), [divider + 1, range-last).
         //! where divider is the first occurrence of @p value
         //!
         //! That is, exactly as divide but the divider is omitted in the resulting ranges
         //!
         //! @return a tuple with the two ranges
         //!
         template< typename R, typename T>
         auto split( R&& range, T&& value) ->  decltype( divide( std::forward< R>( range), value))
         {
            auto result = divide( std::forward< R>( range), std::forward< T>( value));
            if( ! std::get< 1>( result).empty())
            {
               ++std::get< 1>( result);
            }
            return result;
         }

         //!
         //! Divide @p range in two parts [range-first, divider), [divider, range-last).
         //! where divider is the first occurrence where @p predicate is true
         //!
         //! @return a tuple with the two ranges
         //!
         template< typename R1, typename P>
         auto divide_if( R1&& range, P predicate) ->  decltype( std::make_tuple( make( std::forward< R1>( range)), make( std::forward< R1>( range))))
         {
            auto divider = std::find_if( std::begin( range), std::end( range), predicate);

            return std::make_tuple( make( std::begin( range), divider), make( divider, std::end( range)));
         }

         template< typename R1, typename R2>
         auto search( R1&& range, R2&& to_find) -> decltype( make( range))
         {
            auto first = std::search( std::begin( range), std::end( range), std::begin( to_find), std::end( to_find));
            return { first, std::end( range)};
         }


         template< typename R1, typename R2, typename F>
         auto find_first_of( R1&& target, R2&& source, F functor) -> decltype( make( std::forward< R1>( target)))
         {
            auto resultRange = make( std::forward< R1>( target));

            resultRange.first = std::find_first_of(
                  std::begin( resultRange), std::end( resultRange),
                  std::begin( source), std::end( source), functor);

            return resultRange;
         }


         template< typename R1, typename R2>
         auto find_first_of( R1&& target, R2&& source) -> decltype( make( std::forward< R1>( target)))
         {
            auto resultRange = make( std::forward< R1>( target));

            resultRange.first = std::find_first_of(
                  std::begin( resultRange), std::end( resultRange),
                  std::begin( source), std::end( source));

            return resultRange;
         }

         //!
         //! Divide @p range in two parts [range-first, divider), [divider, range-last).
         //! where divider is the first occurrence found in @p lookup
         //!
         //! @return a tuple with the two ranges
         //!
         template< typename R1, typename R2>
         auto divide_first( R1&& range, R2&& lookup) ->  decltype( std::make_tuple( make( std::forward< R1>( range)), make( std::forward< R1>( range))))
         {
            auto divider = std::find_first_of(
                  std::begin( range), std::end( range),
                  std::begin( lookup), std::end( lookup));

            return std::make_tuple( make( std::begin( range), divider), make( divider, std::end( range)));
         }



         //!
         //! Divide @p range in two parts [range-first, divider), [divider, range-last).
         //! where divider is the first occurrence found in @p lookup
         //!
         //! @return a tuple with the two ranges
         //!
         template< typename R1, typename R2, typename F>
         auto divide_first( R1&& range, R2&& lookup, F functor) ->  decltype( std::make_tuple( make( std::forward< R1>( range)), make( std::forward< R1>( range))))
         {
            auto divider = std::find_first_of(
                  std::begin( range), std::end( range),
                  std::begin( lookup), std::end( lookup), functor);

            return std::make_tuple( make( std::begin( range), divider), make( divider, std::end( range)));
         }


         //!
         //! Divide @p range in two parts [range-first, intersection-end), [intersection-end, range-last).
         //! where the first range is the intersection of the @p range and @p lookup
         //! and the second range is the complement of @range with regards to @p lookup
         //!
         //! @return a tuple with the two ranges
         //!
         template< typename R1, typename R2>
         auto intersection( R1&& range, R2&& lookup) -> decltype( std::make_tuple( make( std::forward< R1>( range)), make( std::forward< R1>( range))))
         {
            using range_type = decltype( make( std::forward< R1>( range)));
            using value_type = typename range_type::value_type;

            auto lambda = [&]( const value_type& value){ return find( lookup, value);};
            return stable_partition( std::forward< R1>( range), lambda);
         }

         //!
         //! Divide @p range in two parts [range-first, intersection-end), [intersection-end, range-last).
         //! where the first range is the intersection of the @p range and @p lookup
         //! and the second range is the complement of @range with regards to @p lookup
         //!
         //! @return a tuple with the two ranges
         //!
         template< typename R1, typename R2, typename F>
         auto intersection( R1&& range, R2&& lookup, F functor) -> decltype( std::make_tuple( make( std::forward< R1>( range)), make( std::forward< R1>( range))))
         {
            //using range_type = decltype( make( std::forward< R1>( range)));
            using range_value_type = decltype( *std::begin( range));
            using lookup_value_type = decltype( *std::begin( lookup));

            auto lambda = [&]( range_value_type v){
               return find_if( std::forward< R2>( lookup), [&]( lookup_value_type& l){
                  return functor( v, l);
               }); // std::bind( functor, value, std::placeholders::_1));
            };
            return stable_partition( std::forward< R1>( range), lambda);

         }

         //!
         //! @returns a range from @p source with values not found in @p other
         //!
         //! @deprecated use intersection instead...
         template< typename R1, typename R2>
         auto difference( R1&& source, R2&& other) -> decltype( make( source))
         {
            return std::get< 1>( intersection( std::forward< R1>( source), std::forward< R2>( other)));
         }

         //!
         //! @returns a range from @p source with values not found in @p other
         //!
         template< typename R1, typename R2, typename F>
         auto difference( R1&& source, R2&& other, F functor) -> decltype( make( source))
         {
            return std::get< 1>( intersection( std::forward< R1>( source), std::forward< R2>( other), functor));
         }


         template< typename R, typename F>
         auto max( R&& range, F functor) -> decltype( make( range))
         {
            //
            // Just to make sure range is not an rvalue container. we could use enable_if instead
            //
            auto result = make( std::forward< R>( range));

            return make( std::max_element( std::begin( result), std::end( result), functor), std::end( result));
         }

         template< typename R, typename F>
         auto min( R&& range, F functor) -> decltype( make( range))
         {
            //
            // Just to make sure range is not an rvalue container. we could use enable_if instead.
            //
            auto result = make( std::forward< R>( range));

            return make( std::min_element( std::begin( result), std::end( result), functor), std::end( result));
         }

         template< typename R>
         auto min( R&& range) -> decltype( make( range))
         {
            //
            // Just to make sure range is not an rvalue container. we could use enable_if instead.
            //
            auto result = make( std::forward< R>( range));

            return make( std::min_element( std::begin( result), std::end( result)), std::end( result));
         }


         //!
         //! @return true if all elements in @p other is found in @p source
         //!
         template< typename R1, typename R2>
         bool includes( R1&& source, R2&& other)
         {
            using range_type = decltype( make( std::forward< R2>( other)));
            using value_type = typename range_type::value_type;

            auto lambda = [&]( const value_type& value){ return find( std::forward< R1>( source), value);};
            return all_of( other, lambda);
         }

         //!
         //! Uses @p compare to compare for equality
         //!
         //! @return true if all elements in @p other is found in @p source
         //!
         template< typename R1, typename R2, typename Compare>
         bool includes( R1&& source, R2&& other, Compare compare)
         {
            using range_type = decltype( make( std::forward< R2>( other)));
            using value_type = typename range_type::value_type;

            auto lambda = [&]( const value_type& value){ return find_if( source, std::bind( compare, std::placeholders::_1, value));};
            return all_of( other, lambda);

         }

         //!
         //! @return true if all elements in the range compare equal
         //!
         template< typename R>
         bool uniform( R&& range)
         {
            auto first = std::begin( range);

            for( auto&& value : range)
            {
               if( value != *first)
               {
                  return false;
               }
            }
            return true;
         }

         //!
         //! @return true if @p range1 includes @p range2, AND @p range2 includes @p range1
         //!
         template< typename R1, typename R2, typename Compare>
         bool uniform( R1&& range1, R2&& range2, Compare comp)
         {
            return includes( std::forward< R1>( range1), std::forward< R2>( range2), comp)
                 && includes( std::forward< R2>( range2), std::forward< R1>( range1), compare::inverse( comp));
         }


         template< typename Range, typename Predicate>
         auto count_if( Range&& range, Predicate predicate) -> decltype( std::count_if( std::begin( range), std::end( range), predicate))
         {
            return std::count_if( std::begin( range), std::end( range), predicate);
         }


         template< typename Stream, typename Range, typename D>
         Stream& print( Stream& out, Range&& range, D&& delimeter)
         {
            for( auto current = std::begin( range); current != std::end( range); ++current)
            {
               out << *current;

               if( current + 1 != std::end( range))
               {
                  out << delimeter;
               }
            }
            return out;
         }

         template< typename Stream, typename Range, typename D, typename F>
         Stream& print( Stream& out, Range&& range,  D&& delimeter, F&& functor)
         {
            for( auto current = std::begin( range); current != std::end( range); ++current)
            {
               functor( out, *current);

               if( current + 1 != std::end( range))
               {
                  out << delimeter;
               }
            }
            return out;
         }

         namespace numeric
         {
            template< typename R, typename T>
            void iota( R&& range, T value)
            {
               std::iota( std::begin( range), std::end( range), value);
            }

         } // numeric


         namespace sorted
         {

            template< typename R, typename T>
            bool search( R&& range, T&& value)
            {
               return std::binary_search( std::begin( range), std::end( range), std::forward< T>( value));
            }

            template< typename R1, typename R2, typename Output>
            auto intersection( R1&& source, R2&& other, Output& result) -> decltype( make( result))
            {
               std::set_intersection(
                     std::begin( source), std::end( source),
                     std::begin( other), std::end( other),
                     std::back_inserter( result));

               return make( result);
            }

            template< typename R1, typename R2, typename Output, typename Compare>
            auto intersection( R1&& source, R2&& other, Output& result, Compare compare) -> decltype( make( result))
            {
               std::set_intersection(
                     std::begin( source), std::end( source),
                     std::begin( other), std::end( other),
                     std::back_inserter( result),
                     compare);

               return make( result);
            }

            template< typename R1, typename R2, typename Output>
            auto difference( R1&& source, R2&& other, Output& result) -> decltype( make( result))
            {
               std::set_difference(
                     std::begin( source), std::end( source),
                     std::begin( other), std::end( other),
                     std::back_inserter( result));

               return make( result);
            }

            template< typename R1, typename R2, typename Output, typename Compare>
            auto difference( R1&& source, R2&& other, Output& result, Compare compare) -> decltype( make( result))
            {
               std::set_difference(
                     std::begin( source), std::end( source),
                     std::begin( other), std::end( other),
                     std::back_inserter( result),
                     compare);

               return make( result);
            }


            template< typename R, typename T, typename C>
            auto bound( R&& range, const T& value, C compare) -> decltype( make( std::forward< R>( range)))
            {
               auto first = std::lower_bound( std::begin( range), std::end( range), value, compare);
               auto last = std::upper_bound( first, std::end( range), value, compare);

               return make( first, last);
            }


            template< typename R, typename T>
            auto bound( R&& range, const T& value) -> decltype( make( range))
            {
               return bound( std::forward< R>( range), value, std::less< T>{});
            }


            template< typename R, typename C>
            auto group( R&& range, C compare) -> std::vector< decltype( make( std::forward< R>( range)))>
            {
               std::vector< range::type_t< R>> result;

               auto current = std::begin( range);

               while( current != std::end( range))
               {
                  result.push_back( bound( make( current, std::end( range)), *current, compare));
                  current = std::end( result.back());
               }

               return result;
            }

            template< typename R, typename T>
            auto group( R&& range) -> std::vector< decltype( make( std::forward< R>( range)))>
            {
               return group( std::forward< R>( range), std::less< typename R::value_type>{});
            }

         } // sorted
      } // range

      template< typename Iter>
      std::ostream& operator << ( std::ostream& out, const Range< Iter>& range)
      {
         if( out)
         {
            out << "[";
            range::print( out, range, ',') << ']';
         }
         return out;
      }

      template< typename Iter1, typename Iter2>
      bool operator == ( const Range< Iter1>& lhs, const Range< Iter2>& rhs)
      {
         return range::equal( lhs, rhs);
      }

      template< typename Iter, typename C>
      bool operator == ( const Range< Iter>& lhs, const C& rhs)
      {
         return range::equal( lhs, rhs);
      }

      template< typename C, typename Iter>
      bool operator == ( C& lhs, const Range< Iter>& rhs)
      {
         return range::equal( lhs, rhs);
      }

      template< typename Iter>
      bool operator == ( const Range< Iter>& lhs, bool rhs)
      {
         return static_cast< bool>( lhs) ==  rhs;
      }


      namespace compare
      {
         template< typename T, typename R>
         bool any( T&& value, R&& range)
         {
            return ! range::find( range, value).empty();
         }

         template< typename T, typename V>
         bool any( T&& value, std::initializer_list< V> range)
         {
            return ! range::find( range, value).empty();
         }
      } // compare

   } // common
} // casual

namespace std
{
   template< typename Enum>
   typename enable_if< is_enum< Enum>::value, ostream&>::type
   operator << ( ostream& out, Enum value)
   {
     return out << casual::common::cast::underlying( value);
   }

} // std




#endif // CASUAL_COMMON_ALGORITHM_H_
