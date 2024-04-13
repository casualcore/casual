//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/serialize/macro.h"
#include "casual/platform.h"
#include "common/view/binary.h"

#include <string>
#include <vector>


namespace casual
{
   namespace serviceframework
   {
      namespace service
      {

         namespace model
         {

            namespace type
            {
               enum class Category : int
               {
                  unknown = 0,
                  container,
                  composite,

                  integer,
                  floatingpoint,
                  character,
                  boolean,

                  string,
                  //unicode,
                  binary,
                  fixed_binary,
               };
               inline std::string_view description( Category value) noexcept
               {
                  switch( value)
                  {
                     case Category::unknown: return "unknown";
                     case Category::container: return "container";
                     case Category::composite: return "composite";
                     case Category::integer: return "integer";
                     case Category::floatingpoint: return "floatingpoint";
                     case Category::character: return "character";
                     case Category::boolean: return "boolean";
                     case Category::string: return "string";
                     case Category::binary: return "binary";
                     case Category::fixed_binary: return "fixed_binary";
                  }
                  return "<unknown>";
               }


               namespace details
               {
                  template< Category value>
                  struct base_traits
                  {
                     static constexpr Category category() { return value;}
                  };

                  template< typename T>
                  using decay_t = typename std::decay< T>::type;

                  template< typename T>
                  struct traits;

                  template<> struct traits< std::string> : base_traits< Category::string>{};
                  template<> struct traits< std::u8string> : base_traits< Category::string>{};
                  template<> struct traits< platform::binary::type> : base_traits< Category::binary>{};
                  template<> struct traits< common::view::Binary> : base_traits< Category::fixed_binary>{};
                  template<> struct traits< common::view::immutable::Binary> : base_traits< Category::fixed_binary>{};

                  template<> struct traits< char> : base_traits< Category::character>{};
                  template<> struct traits< bool> : base_traits< Category::boolean>{};

                  template<> struct traits< short> : base_traits< Category::integer>{};
                  template<> struct traits< int> : base_traits< Category::integer>{};
                  template<> struct traits< long> : base_traits< Category::integer>{};
                  template<> struct traits< long long> : base_traits< Category::integer>{};
                  template<> struct traits< unsigned int> : base_traits< Category::integer>{};
                  template<> struct traits< unsigned long> : base_traits< Category::integer>{};
                  template<> struct traits< unsigned long long> : base_traits< Category::integer>{};

                  template<> struct traits< float> : base_traits< Category::floatingpoint>{};
                  template<> struct traits< double> : base_traits< Category::floatingpoint>{};
               } // details


               template< typename T>
               struct traits : details::traits< details::decay_t< T>>{};

            } // type



         } // model


         struct Model
         {

            struct Type
            {

               Type() = default;
               Type( const char* role, model::type::Category category) : role( role == nullptr ? "" : role), category( category) {}

               std::string role;
               model::type::Category category = model::type::Category::unknown;
               std::vector< Type> attributes;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  CASUAL_SERIALIZE( role);
                  CASUAL_SERIALIZE( category);
                  CASUAL_SERIALIZE( attributes);
               })
            };

            std::string service;

            struct arguments_t
            {
               std::vector< Type> input;
               std::vector< Type> output;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  CASUAL_SERIALIZE( input);
                  CASUAL_SERIALIZE( output);
               })

            } arguments;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               CASUAL_SERIALIZE( service);
               CASUAL_SERIALIZE( arguments);
            })

         };


      } // service
   } // serviceframework



} // casual


