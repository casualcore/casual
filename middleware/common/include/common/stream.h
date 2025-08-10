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
#include "common/message/type.h"
#include "common/transcode.h"

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
            auto write( std::ostream& out, const T& value, traits::priority::tag< 1>) 
               -> decltype( customization::detail::write( out, value))
            {
               return customization::detail::write( out, value);
            }

            // if not, we takes all that can be serialized
            template< typename T> 
            auto write( std::ostream& out, const T& value, traits::priority::tag< 0>) 
               -> decltype( void( std::declval< serialize::line::Writer&>() << value), out)
            {
               serialize::line::Writer archive;
               archive << value;
               archive.consume( out);
               return out;
            }
            
         } // dispatch


         template< typename T> 
         auto write( std::ostream& out, const T& value)
            -> decltype( dispatch::write( out, value, traits::priority::tag< 1>{}))
         {
            return dispatch::write( out, value, traits::priority::tag< 1>{});
         }
         
      } // detail

      //! write multiple values
      template< typename... Ts>
      auto write( std::ostream& out, const Ts&... ts) 
        -> decltype( ( detail::write( out, ts), ...))
      {
         return ( detail::write( out, ts), ...);
      }

      namespace customization
      {
         template< typename T>
         struct delay
         {
            template< typename... Ts>
            static auto write( std::ostream& out, const Ts&... ts) 
               -> decltype( stream::write( out, ts...))
            {
               return stream::write( out, ts...);
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
            requires std::is_error_code_enum_v < T>
            struct point< T>
            {
               static void stream( std::ostream& out, T value)
               {
                  supersede::point< std::error_code>::stream( out, std::error_code{ value});
               }
            };

            //! specialization for std::error_condition
            template< typename T> 
            requires std::is_error_condition_enum_v< T>
            struct point< T>
            {
               static void stream( std::ostream& out, T value)
               {
                  supersede::point< std::error_condition>::stream( out, std::error_condition{ value});
               }
            };
            
         } // supersede


         //! Specialization for iterables, to log ranges
         template< typename C>
         requires ( concepts::range< C> && ! concepts::string::like< C> && ! concepts::binary::like< C>)
         struct point< C>
         {
            template< typename R>
            static void stream( std::ostream& out, R&& range)
            {
               out << '[';

               algorithm::for_each_interleave( 
                  range,
                  [&out]( auto& v){ stream::detail::write( out, v);},
                  [&out](){ out << ", ";}
               );

               out << ']';
            }
         };

         //! Specialization for enum
         template< concepts::enumerator T>
         struct point< T>
         { 
            static void stream( std::ostream& out, T value) 
            {
               stream( out, value, traits::priority::tag< 1>{});
            }

         private:
            // chosen if the enum type has a declared `description` overload
            template< typename E>
            static auto stream( std::ostream& out, const E& value, traits::priority::tag< 1>)
               -> decltype( void( out << description( value)))
            {
               out << description( value);
            }
            
            static auto stream( std::ostream& out, T value, traits::priority::tag< 0>)
            {
               out << std::to_underlying( value);
            }
         };


         //! Specialization for named
         template< concepts::serialize::named::value T>
         struct point< T>
         {               
            template< typename C>
            static void stream( std::ostream& out, const C& value)
            {
               stream::write( out, value.name(), ": ", value.value());
            }
         };

         //! Specialization for std::exception
         template< std::derived_from< std::exception>  T>
         struct point< T>
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
         template< common::message::like T>
         struct point< T>
         {  
            static void stream( std::ostream& out, const T& value)
            {
               stream::write( out, "{ type: ", value.type(), ", correlation: ", value.correlation, ", body: { "); 
               serialize::line::Writer archive;
               value.serialize( archive);
               archive.consume( out);
               out << "}}";
            }
         };

         template< concepts::binary::like T>
         struct point< T>
         {  
            static void stream( std::ostream& out, const T& value)
            {
               transcode::hex::encode( out, value);
            };
         };

         template< concepts::string::like T>
         struct point< T>
         {  
            static void stream( std::ostream& out, const T& value)
            {
               if constexpr( std::same_as< std::decay_t< std::ranges::range_value_t< T>>, char>)
                  out.write( std::data( value), std::size( value));
               else
               {
                  static_assert( sizeof( std::ranges::range_value_t< T>) == 1);
                  auto data = reinterpret_cast< const char*>( std::data( value));
                  out.write( data, std::size( value));
               }
            };
         };

         template< typename... Ts>
         struct point< std::variant< Ts...>>
         {  
            static void stream( std::ostream& out, const std::variant< Ts...>& value)
            {
               std::visit( [ &out]( const auto& value) 
               {
                  stream::write( out, value);
               }, value);
               
            };
         };

      } // customization

   } // common::stream
} // casual



