//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "serviceframework/namevaluepair.h"
#include "serviceframework/platform.h"

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
               enum class Category
               {
                  unknown = 0,
                  container,
                  composite,

                  integer,
                  floatingpoint,
                  character,
                  boolean,

                  string,
                  binary,
               };


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
                  template<> struct traits< platform::binary::type> : base_traits< Category::binary>{};

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
               std::vector< Type> attribues;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( role);
                  archive & CASUAL_MAKE_NVP( category);
                  archive & CASUAL_MAKE_NVP( attribues);
               })
            };

            std::string service;

            struct arguments_t
            {
               std::vector< Type> input;
               std::vector< Type> output;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( input);
                  archive & CASUAL_MAKE_NVP( output);
               })

            } arguments;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( service);
               archive & CASUAL_MAKE_NVP( arguments);
            })

         };


      } // service
   } // serviceframework



} // casual


