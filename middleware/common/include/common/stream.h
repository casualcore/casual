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

         template< typename T, typename Enable = void>
         struct has_formatter : std::false_type{};
         
         namespace detail
         {
            // just a helper to get rid of syntax
            template< typename T> 
            auto formatter( std::ostream& out, const T& value) 
               -> decltype( typename stream::has_formatter< traits::remove_cvref_t< T>>::formatter{}( out, value), out)
            {
               typename stream::has_formatter< traits::remove_cvref_t< T>>::formatter{}( out, value);
               return out;
            }

            template< typename T>
            auto write( std::ostream& out, const T& value) 
               -> decltype( formatter( out, value))
            {
               return formatter( out, value);
            }

         } // detail

      } // stream
   } // common
} // casual


// std stream operator is a better match then this, hence this is a fallback
// this also need to be declared before used by formatters below
template< typename S, typename T>
auto operator << ( S& out, const T& value) -> decltype( casual::common::stream::detail::write( out, value)) 
{
   return casual::common::stream::detail::write( out, value);
}




namespace casual
{
   namespace common
   {
      namespace stream
      {

         namespace detail
         {
            // lower, takes all that have a defined formatter
            template< typename T> 
            auto indirection( std::ostream& out, T&& value, traits::priority::tag< 0>) 
               -> decltype( void( std::declval< serialize::line::Writer&>() << std::forward< T>( value)), out)
            {
               serialize::line::Writer archive{ out};
               archive << std::forward< T>( value);
               return out;
            }

            // higher priority, takes all that can use the ostream stream operator
            template< typename T> 
            auto indirection( std::ostream& out, T&& value, traits::priority::tag< 1>) 
               -> decltype( out << std::forward< T>( value))
            {
               return out << std::forward< T>( value);
            }
            
         } // detail

         //! sentinel
         inline std::ostream& write( std::ostream& out) { return out;}

         //! write multible values
         template< typename T, typename... Ts>
         auto write( std::ostream& out, T&& t, Ts&&... ts) 
            -> decltype( detail::indirection( out, std::forward< T>( t), traits::priority::tag< 1>{})) 
         {
            detail::indirection( out, std::forward< T>( t), traits::priority::tag< 1>{});
            return write( out, std::forward< Ts>( ts)...);
         }

         //! Specialization for iterables, to log ranges
         template< typename C> 
         struct has_formatter< C, std::enable_if_t< 
            traits::is::iterable< C>::value 
            && ! traits::is::string::like< C>::value
            >>
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


         //! Specialization for error code
         template< typename C> 
         struct has_formatter< C, std::enable_if_t< 
            std::is_error_code_enum< C>::value>>
            : std::true_type
         {
            struct formatter
            {
               template< typename S>
               void operator () ( std::ostream& out, const C& value) const
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
               void operator () ( std::ostream& out, const C& value) const
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
               void operator () ( std::ostream& out, const C& value) const
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
               void operator () ( std::ostream& out, const C& value) const
               {
                  indirection( out, value);
               }

               static void indirection( std::ostream& out, const std::exception& value)
               {
                  out << value.what();
               }

               static void indirection( std::ostream& out, const std::system_error& value)
               {
                  out << value.code() << " " << value.what();
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
               void operator () ( std::ostream& out, const C& value) const
               {
                  stream::write( out, value.get());
               }
            };
         };

         //! Specialization for _messages_
         template< typename T>
         struct has_formatter< T, std::enable_if_t< serialize::traits::is::message::like< T>::value>>
            : std::true_type
         {
            struct formatter
            {
               void operator () ( std::ostream& out, const T& value) const
               {
                  stream::write( out, "{ type: ", value.type(), ", correlation: ", value.correlation, ", payload: ");
                  serialize::line::Writer archive{ out};
                  value.serialize( archive);
                  out << '}';
               }
            };
         };

      } // stream
   } // common
} // casual


 



