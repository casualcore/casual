//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/traits.h"
#include "common/algorithm.h"
#include "common/functional.h"


#include <ostream>

namespace casual
{
   namespace common
   {
      namespace stream
      {
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
                     [&out]( auto& v){ out << v;},
                     [&out](){ out << ", ";}
                  );

                  out << ']';
               }
            };
         };

         //! specialization for tuple
         template< typename C> 
         struct has_formatter< C, std::enable_if_t< 
            traits::is::tuple< C>::value>>
            : std::true_type
         {
            struct formatter
            {
               template< typename T>
               void operator () ( std::ostream& out, T&& tuple) const
               { 
                  out << '[';
                  auto dispatch = [&out]( auto&&... parameters){
                     formatter::log( out, parameters...);
                  };
                  common::apply( dispatch, std::forward< T>( tuple));
                  out << ']';
               }
            private:

               //! 3 overloads so we can deduce when to put a ', ' delimiter
               //! @{

               template< typename T, typename... Ts>
               static void log( std::ostream& out, T&& value, Ts&&... ts) { out << value << ", "; log( out, std::forward< Ts>( ts)...);}

               template< typename T>
               static void log( std::ostream& out, T&& value) { out << value;}

               static void log( std::ostream& out) { /* no op */}
               //! @}

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
                  out << std::error_code( value);
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

         namespace detail
         {
            template< typename S>
            void part( S& stream)
            {
            }

            template< typename S, typename T>
            void part( S& stream, T&& value)
            {
               stream << std::forward< T>( value);
            }

            template< typename S, typename Arg, typename... Args>
            void part( S& stream, Arg&& arg, Args&&... args)
            {
               detail::part( stream, std::forward< Arg>( arg));
               detail::part( stream, std::forward< Args>( args)...);
            }
         } // detail

         template< typename S, typename... Args>
         S& write( S& stream, Args&&... args)
         {
            detail::part( stream, std::forward< Args>( args)...);
            return stream;
         }
         
      } // stream
   } // common
} // casual

namespace std
{
   // extended stream operator for std... This is not legal if I understand it correct,
   // but I find it hard to see what damage it could do, since it is restricted to the 
   // customization point 'casual::common::log::has_formatter', so we roll with it...
   template< typename T> 
   enable_if_t< casual::common::stream::has_formatter< casual::common::traits::remove_cvref_t< T>>::value, ostream&>
   operator << ( ostream& out, T&& value)
   {
      using namespace casual::common;
      typename stream::has_formatter< traits::remove_cvref_t< T>>::formatter{}( out, std::forward< T>( value));
      return out;
   }
   
} // std