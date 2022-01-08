//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/stream/customization.h"
#include "common/traits.h"
#include "common/algorithm.h"
#include "common/functional.h"

#include "common/serialize/line.h"

#include <ostream>


namespace casual
{
   namespace common::stream
   {
      namespace detail
      {
         namespace dispatch
         {
            //! dispatch to the customization priority, including ostream stream operator
            template< typename T> 
            auto write( std::ostream& out, T&& value, traits::priority::tag< 1>) 
               -> decltype( customization::detail::write( out, std::forward< T>( value)))
            {
               return customization::detail::write( out, std::forward< T>( value));
            }

            // if not, we takes all that can be serialized
            template< typename T> 
            auto write( std::ostream& out, T&& value, traits::priority::tag< 0>) 
               -> decltype( void( std::declval< serialize::line::Writer&>() << std::forward< T>( value)), out)
            {
               serialize::line::Writer archive;
               archive << std::forward< T>( value);
               archive.consume( out);
               return out;
            }
            
         } // dispatch


         template< typename T> 
         auto write( std::ostream& out, T&& value)
            -> decltype( dispatch::write( out, std::forward< T>( value), traits::priority::tag< 1>{}))
         {
            return dispatch::write( out, std::forward< T>( value), traits::priority::tag< 1>{});
         }
         
      } // detail

      //! write multible values
      template< typename... Ts>
      auto write( std::ostream& out, Ts&&... ts) 
         -> decltype( ( detail::write( out, std::forward< Ts>( ts)), ...) )
      {
         return ( detail::write( out, std::forward< Ts>( ts)), ...);
      }

      namespace customization
      {
         template< typename T>
         struct delay
         {
            template< typename... Ts>
            static auto write( std::ostream& out, Ts&&... ts) -> decltype( stream::write( out, std::forward< Ts>( ts)...))
            {
               return stream::write( out, std::forward< Ts>( ts)...);
            }
         };

         namespace supersede
         {
            //! Specialization for error_code
            // will take presens over error_code ostream stream operator
            template<>
            struct point< std::error_code>
            {
               static void stream( std::ostream& out, const std::error_code& value)
               {
                  out << '[' << value.category().name() << ':' << value.message() << ']';
               }
            };

            //! Specialization for error_condition
            // will take presens over error_condition ostream stream operator, which is non existent?
            template<>
            struct point< std::error_condition>
            {
               static void stream( std::ostream& out, const std::error_condition& value)
               {
                  out << '[' << value.category().name() << ':' << value.message() << ']';
               }
            };

            //! specialization for std::error_code
            template< typename T> 
            struct point< T, std::enable_if_t< 
               std::is_error_code_enum< T>::value>>
            {
               static void stream( std::ostream& out, T value)
               {
                  supersede::point< std::error_code>::stream( out, std::error_code{ value});
               }
            };

            //! specialization for std::error_condition
            template< typename T> 
            struct point< T, std::enable_if_t< 
               std::is_error_condition_enum< T>::value>>
            {
               static void stream( std::ostream& out, T value)
               {
                  supersede::point< std::error_condition>::stream( out, std::error_condition{ value});
               }
            };
            
         } // supersede


         //! Specialization for iterables, to log ranges
         template< typename C> 
         struct point< C, std::enable_if_t< 
            traits::is::iterable_v< C>
            && ! traits::is::string::like_v< C>
            && traits::has::empty_v< C>>>
         {
            template< typename R>
            static void stream( std::ostream& out, R&& range)
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


         //! Specialization for named
         template< typename T>
         struct point< T, std::enable_if_t< serialize::traits::is::named::value_v< T>>>
         {               
            template< typename C>
            static void stream( std::ostream& out, const C& value)
            {
               stream::write( out, value.name(), ": ", value.value());
            }
         };

         //! Specialization for std::exception
         template< typename T>
         struct point< T, std::enable_if_t< std::is_base_of< std::exception, T>::value>>
         {
            template< typename C>
            static void stream( std::ostream& out, const C& value)
            {
               indirection( out, value);
            }

            static void indirection( std::ostream& out, const std::exception& value)
            {
               out << value.what();
            }

            static void indirection( std::ostream& out, const std::system_error& value)
            {
               stream::write( out, value.code(), " ", value.what());
            }
         };

         //! Specialization for reference_wrapper
         template< typename T>
         struct point< std::reference_wrapper< T>>
         {  
            template< typename C>
            static void stream( std::ostream& out, const C& value)
            {
               stream::write( out, value.get());
            };
         };

         //! Specialization for _messages_
         template< typename T>
         struct point< T, std::enable_if_t< serialize::traits::is::message::like_v< T>>>
         {  
            static void stream( std::ostream& out, const T& value)
            {
               stream::write( out, "{ type: ", value.type(), ", correlation: ", value.correlation, ", payload: ");
               serialize::line::Writer archive;
               value.serialize( archive);
               archive.consume( out);
               out << '}';
            }
         };

      } // customization

   } // common::stream
} // casual


 



