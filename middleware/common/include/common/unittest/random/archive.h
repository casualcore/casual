//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/platform.h"

#include "common/serialize/value.h"

#include <tuple>
#include <random>

namespace casual
{
   namespace common
   {
      namespace unittest
      {
         namespace random
         {
            namespace detail
            {

               template< platform::size::type min_value,  platform::size::type max_value>
               struct Cardinality
               {
                  static_assert( min_value <= max_value, "");

                  constexpr static platform::size::type min() { return Value::min_;}
                  constexpr static platform::size::type max() { return Value::max_;}

                  constexpr static bool fixed() { return min == max;}

               private:
                  enum Value : platform::size::type 
                  {
                     min_ = min_value,
                     max_ = max_value,
                  };
               };

               namespace cardinality
               {
                  template< typename T>
                  auto limit()
                  {
                     return detail::Cardinality< std::numeric_limits< T>::min(), std::numeric_limits< T>::max()>{};
                  }

                  template<>
                  inline auto limit< std::byte> ()
                  {
                     return detail::Cardinality< std::numeric_limits< std::uint8_t>::min(), std::numeric_limits< std::uint8_t>::max()>{};
                  }
               } // cardinality

               inline std::mt19937& engine()
               {
                  static std::mt19937 engine{ std::random_device{}()};
                  return engine;
               }
               

            } // detail

            template< typename Policy>
            struct basic_archive
            {
               constexpr static auto archive_properties() { return common::serialize::archive::Property::order;}

               inline std::tuple< platform::size::type, bool> container_start( platform::size::type size, const char*) 
               {
                  return { basic_archive::size( Policy::size::container()) , true};
               }

               inline void container_end( const char*) {} // no-op

               inline std::tuple< platform::size::type, bool> tuple_start( platform::size::type size, const char*) 
               {
                  return { size , true};
               }

               inline void tuple_end( const char*) {} // no-op

               inline bool composite_start( const char*) { return true;}
               inline void composite_end(  const char* name) {} // no-op

               template< std::integral T>
               bool read( T& value, const char*)
               {
                  static auto distribution = std::uniform_int_distribution< T>{ std::numeric_limits< T>::min(), std::numeric_limits< T>::max()};
                  value = distribution( detail::engine());
                  return true;
               }

               template< std::floating_point T>
               bool read( T& value, const char*)
               {
                  static auto distribution = std::uniform_real_distribution< T>{ T{ -3.0}, T{ 3.0}};
                  value = distribution( detail::engine());
                  return true;
               }

               bool read( bool& value, const char*)
               {
                  static auto distribution = std::uniform_int_distribution<>{ 0, 1};
                  value = distribution( detail::engine()) == 1;
                  return true;
               }

               bool read( char& value, const char*)
               {
                  value = character( Policy::character());
                  return true;
               }


               bool read( std::string& value, const char*)
               {
                  value.resize( size( Policy::size::string()));
                  for( auto& c : value)
                     read( c, nullptr);
                  return true;
               }

               bool read( platform::binary::type& value, const char*)
               {
                  value.resize( size( Policy::size::binary()));
                  return read( binary::span::make( value));
               }

               bool read( binary::span::Fixed< std::byte> value, const char*)
               {
                  return read( value);
               }

               template< typename T>
               basic_archive& operator >> ( T&& value)
               {
                  serialize::value::read( *this, value, nullptr);
                  return *this;
               }

            private:

               bool read( std::span< std::byte> value)
               {
                  static auto distribution = std::uniform_int_distribution< std::uint8_t>( std::numeric_limits< std::uint8_t>::min());
                  
                  for( auto& byte : value)
                     byte = static_cast< std::byte>( distribution( detail::engine()));
                  
                  return true;
               }

               template< typename C>
               constexpr static platform::size::type size( C cardinality)
               {
                  if( cardinality.fixed())
                     return cardinality.min();

                  return std::uniform_int_distribution< platform::size::type>{ cardinality.min(), cardinality.max()}( detail::engine());
               }

               template< typename C>
               constexpr static char character( C cardinality)
               {
                  return std::uniform_int_distribution<>{ cardinality.min(), cardinality.max()}( detail::engine());
               }
               
            };

            namespace policy
            {
               struct Default
               {
                  struct size
                  {
                     constexpr static auto container() { return detail::Cardinality< 10, 20>{};}
                     constexpr static auto string() { return detail::Cardinality< 5, 42>{};}
                     constexpr static auto binary() { return detail::Cardinality< 10, 100>{};}
                  };



                  constexpr static auto character() { return detail::Cardinality< 32, 126>{};}
               };
            } // policy

            using Archive = basic_archive< policy::Default>;

            template< typename T, typename A = Archive>
            void fill( T& value)
            {
               A archive;
               archive >> value;
            } 

            template< typename T, typename A = Archive>
            T create()
            {
               T value;
               A archive;
               archive >> value;
               return value;
            }
            
         } // random 

      } // unittest
   } // common
} // casual
