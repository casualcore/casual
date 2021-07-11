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
   namespace common::stream::detail
   {
      // using SFINAE expression to get a sutable specialization
      template< typename T> 
      auto formatter( std::ostream& out, const T& value, traits::priority::tag< 1>) 
         -> decltype( void( customization::supersede::point< traits::remove_cvref_t< T>>::stream( out, value)), out)
      {
         customization::supersede::point< traits::remove_cvref_t< T>>::stream( out, value);
         return out;
      }

      template< typename T> 
      auto formatter( std::ostream& out, const T& value, traits::priority::tag< 0>) 
         -> decltype( void( customization::point< traits::remove_cvref_t< T>>::stream( out, value)), out)
      {
         customization::point< traits::remove_cvref_t< T>>::stream( out, value);
         return out;
      }

      template< typename T>
      auto write( std::ostream& out, const T& value) 
         -> decltype( formatter( out, value, traits::priority::tag< 1>{}))
      {
         return formatter( out, value, traits::priority::tag< 1>{});
      }

   } // common::stream::detail
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
   namespace common::stream
   {
      namespace detail
      {
         // highest priority, takes all that have specialized customization::supersede::point
         template< typename T> 
         auto indirection( std::ostream& out, const T& value, traits::priority::tag< 2>) 
            -> decltype( void( customization::supersede::point< traits::remove_cvref_t< T>>::stream( out, value)), out)
         {
            customization::supersede::point< traits::remove_cvref_t< T>>::stream( out, value);
            return out;
         }

         // takes all that can use the ostream stream operator, including the one defined above.
         template< typename T> 
         auto indirection( std::ostream& out, const T& value, traits::priority::tag< 1>) 
            -> decltype( out << value)
         {
            return out << value;
         }

         // lower, takes all that can be serialized
         template< typename T> 
         auto indirection( std::ostream& out, const T& value, traits::priority::tag< 0>) 
            -> decltype( void( std::declval< serialize::line::Writer&>() << value), out)
         {
            serialize::line::Writer archive;
            archive << value;
            archive.consume( out);
            return out;
         }
         
      } // detail

      //! sentinel
      inline std::ostream& write( std::ostream& out) { return out;}

      //! write multible values
      template< typename T, typename... Ts>
      auto write( std::ostream& out, const T& t, const Ts&... ts) 
         -> decltype( detail::indirection( out, t, traits::priority::tag< 2>{})) 
      {
         detail::indirection( out, t, traits::priority::tag< 2>{});
         return write( out, ts...);
      }


      namespace customization
      {

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
            
         } // supersede


         //! Specialization for iterables, to log ranges
         template< typename C> 
         struct point< C, std::enable_if_t< 
            traits::is::iterable_v< C>
            && ! traits::is::string::like_v< C>
            && traits::has::empty_v< C>>>
         {
            template< typename R>
            static void stream( std::ostream& out, const R& range)
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


         //! specialization for std::error_code
         template< typename C> 
         struct point< C, std::enable_if_t< 
            std::is_error_code_enum< C>::value>>
         {
            template< typename T>
            static auto stream( std::ostream& out, T value)
               -> decltype( void( stream::write( out, std::error_code{ value})))
            {
               stream::write( out, std::error_code{ value});
            }
         };

         //! specialization for std::error_condition
         template< typename C> 
         struct point< C, std::enable_if_t< 
            std::is_error_condition_enum< C>::value>>
         {
            template< typename T>
            static auto stream( std::ostream& out, T value)
               -> decltype( void( stream::write( out, std::error_condition{ value})))
            {
               stream::write( out, std::error_condition{ value});
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


 



