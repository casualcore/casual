//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/traits.h"

#include <utility>
#include <type_traits>
#include <tuple>

namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace named
         {
            struct reference 
            {
               struct lvalue{};
               struct rvalue{};
            };

            template< typename T, typename Reference>
            class Value;


            //! Holds non const lvalue types
            template <typename T>
            class Value< T, reference::lvalue> : public std::tuple< const char*, T*>
            {
            public:

               using serialize_named_value_type = void;

               explicit Value (const char* name, T& value)
                  : std::tuple< const char*, T*>( name, &value) {}

               const char* name() const { return std::get< 0>( *this);}
               T& value() const { return  *( std::get< 1>( *this));}
            };

            //! Holds const lvalue types
            template <typename T>
            class Value< const T, reference::lvalue> : public std::tuple< const char*, const T*>
            {
            public:
               
               using serialize_named_value_type = void;

               explicit Value (const char* name, const T& value)
                  : std::tuple< const char*, const T*>( name, &value) {}

               const char* name() const { return std::get< 0>( *this); }
               const T& value() const { return *( std::get< 1>( *this));}
            };


            //! Holds rvalue types
            template <typename T>
            class Value< T, reference::rvalue> : public std::tuple< const char*, T>
            {
            public:

               using serialize_named_value_type = void;
               
               explicit Value (const char* name, T&& value)
                  : std::tuple< const char*, T>( name, std::move( value)) {}

               const char* name() const { return std::get< 0>( *this);}
               const T& value() const { return std::get< 1>( *this);}
               //const T& value() const & { return std::get< 1>( *this);}
               //T value() && { return std::move( std::get< 1>( *this));} 
            };

            namespace value
            {
               namespace internal
               {
                  template< typename T>
                  using value_traits_t = Value< 
                     std::remove_reference_t< T>, 
                     std::conditional_t< std::is_lvalue_reference<T>::value, reference::lvalue, reference::rvalue>>;
               }

               template< typename T>
               auto make( T&& value, const char* name)
               {
                  return internal::value_traits_t< T>( name, std::forward< T>( value));
               }
            } // value

         } // named
      } // serialize

   } // common
} // casual
