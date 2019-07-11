//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/traits.h"
#include "common/algorithm.h"
#include "common/functional.h"

#include "common/serialize/line.h"

#include <ostream>

namespace casual
{
   namespace common
   {
      namespace stream
      {

         template< typename S, typename... Ts>
         S& write( S& stream, Ts&&... ts);

         template< typename T, typename Enable = void>
         struct has_formatter : std::false_type{};
         
         //! Specialization for iterables, to log ranges
         template< typename C> 
         struct has_formatter< C, std::enable_if_t< 
            traits::is::iterable< C>::value 
            && ! traits::is::string::like< C>::value>>
            : std::true_type
         {
            struct formatter
            {
               template< typename R>
               void operator () ( std::ostream& out, R&& range) const
               {
                  out << '[';

                  algorithm::for_each_interleave( 
                     range,
                     [&out]( auto& v){ stream::write( out, v);},
                     [&out](){ out << ", ";}
                  );

                  out << ']';
               }
            };
         };

         //! Specialization for enum
         template< typename C> 
         struct has_formatter< C, std::enable_if_t< 
            std::is_enum< C>::value && ! std::is_error_code_enum< C>::value && ! std::is_error_condition_enum< C>::value>>
            : std::true_type
         {
            struct formatter
            {
               void operator () ( std::ostream& out, C value) const
               {
                  out << cast::underlying( value);
               }
            };
         };

         //! Specialization for error code
         template< typename C> 
         struct has_formatter< C, std::enable_if_t< 
            std::is_error_code_enum< C>::value>>
            : std::true_type
         {
            struct formatter
            {
               void operator () ( std::ostream& out, C value) const
               {
                  auto code = std::error_code( value);
                  out << '[' << code << ' ' << code.message() << ']';
               }
            };
         };

         //! Specialization for error condition
         template< typename C> 
         struct has_formatter< C, std::enable_if_t< 
            std::is_error_condition_enum< C>::value>>
            : std::true_type
         {
            struct formatter
            {
               void operator () ( std::ostream& out, C value) const
               {
                  auto condition = std::error_condition( value);
                  out << condition.category().name() << ':' << condition.value() << " - " << condition.message();
               }
            };
         };

         //! Specialization for named
         template< typename T>
         struct has_formatter< T, std::enable_if_t< serialize::traits::is::named::value< T>::value>>
            : std::true_type
         {
            struct formatter
            {
               template< typename C>
               void operator () ( std::ostream& out, C&& value) const
               {
                  stream::write( out, value.name(), ": ", value.value());
               }
            };
         };



         //! Specialization for std::exception
         template< typename T>
         struct has_formatter< T, std::enable_if_t< std::is_base_of< std::exception, T>::value>>
            : std::true_type
         {
            struct formatter
            {

               template< typename C>
               void operator () ( std::ostream& out, C&& value) const
               {
                  indirection( out, value);
               }

               static void indirection( std::ostream& out, const std::exception& value)
               {
                  stream::write( out, value.what());
               }

               static void indirection( std::ostream& out, const std::system_error& value)
               {
                  stream::write( out, value.code(), " ", value.what());
               }
            };
         };

         //! Specialization for reference_wrapper
         template< typename T>
         struct has_formatter< std::reference_wrapper< T>>
            : std::true_type
         {
            struct formatter
            {
               template< typename C>
               void operator () ( std::ostream& out, C&& value) const
               {
                  stream::write( out, value.get());
               }
            };
         };

         namespace detail
         {

            // just a helper to get rid of syntax
            template< typename S, typename T> 
            auto formatter( S& out, T&& value) 
               -> decltype( typename stream::has_formatter< traits::remove_cvref_t< T>>::formatter{}( out, std::forward< T>( value)))
            {
               typename stream::has_formatter< traits::remove_cvref_t< T>>::formatter{}( out, std::forward< T>( value));
            }

            // lowest priority, take all that doesn't have a formatter, but can be serialized with serialize::line::Writer
            template< typename T> 
            auto indirection( std::ostream& out, T&& value, traits::priority::tag< 0>) 
               -> decltype( std::declval< serialize::line::Writer&>() << std::forward< T>( value), void())
            {
               casual::common::serialize::line::Writer archive{ out};
               archive << std::forward< T>( value);
            }

            // higher priority, takes all that have a defined formatter
            template< typename T> 
            auto indirection( std::ostream& out, T&& value, traits::priority::tag< 1>) 
               -> decltype( (void)formatter( out, std::forward< T>( value)), void())
            {
               formatter( out, std::forward< T>( value));
            }

            // highest priority, takes all that have a defined ostream operator
            template< typename T> 
            auto indirection( std::ostream& out, T&& value, traits::priority::tag< 2>) 
               -> decltype( out << std::forward< T>( value), void())
            {
               out << std::forward< T>( value);
            }


            template< typename S>
            void part( S& stream) {}

            template< typename S, typename T>
            void part( S& stream, T&& value)
            {
               indirection( stream, std::forward< T>( value), traits::priority::tag< 2>{});
            }

            template< typename S, typename T, typename... Ts>
            void part( S& stream, T&& value, Ts&&... ts) 
            {
               detail::part( stream, std::forward< T>( value));
               detail::part( stream, std::forward< Ts>( ts)...);
            }

         } // detail

         template< typename S, typename... Ts>
         S& write( S& stream, Ts&&... ts)
         {
            detail::part( stream, std::forward< Ts>( ts)...);
            return stream;
         }
      } // stream
   } // common
} // casual

namespace std
{
   // extended stream operator for std... as I understand it, but I find it 
   // hard to see what damage it could do, since it is restricted to the 
   // customization point 'casual::common::log::has_formatter', so we roll with it...
   template< typename T> 
   std::enable_if_t< 
      casual::common::stream::has_formatter< casual::common::traits::remove_cvref_t< T>>::value
      , 
      std::ostream&>
   operator << ( std::ostream& out, T&& value)
   {
      using namespace casual::common;
      typename stream::has_formatter< traits::remove_cvref_t< T>>::formatter{}( out, std::forward< T>( value));
      return out;
   }
} // std
 



