//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/serialize/macro.h"
#include <optional>

#include <string>
#include <vector>
#include <functional>

namespace casual
{
   namespace http
   {
      namespace outbound
      {
         namespace configuration
         {
            struct Header
            {
               Header() = default;
               inline Header( std::function< void(Header&)> foreign) { foreign( *this);}

               std::string name;
               std::string value;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( name);
                  CASUAL_SERIALIZE( value);
               )
            };

            struct Service
            {
               Service() = default;
               inline Service( std::function< void(Service&)> foreign) { foreign( *this);}

               std::string name;
               std::string url;
               std::optional< bool> discard_transaction;

               std::vector< Header> headers;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( name);
                  CASUAL_SERIALIZE( url);
                  CASUAL_SERIALIZE( headers);
                  CASUAL_SERIALIZE( discard_transaction);
               )
            };

            struct Default
            {
               struct
               {
                  std::vector< Header> headers;
                  bool discard_transaction = false;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( headers);
                     CASUAL_SERIALIZE( discard_transaction);
                  )

               } service;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( service);
               )

               friend Default operator + ( Default lhs, Default rhs);
            };

            struct Model
            {
               Model() = default;
               inline Model( std::function< void(Model&)> foreign) { foreign( *this);}

               Default casual_default;
               std::vector< Service> services;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE_NAME( casual_default, "default");
                  CASUAL_SERIALIZE( services);
               )

               friend Model operator + ( Model lhs, Model rhs);
            };


            Model get( const std::string& pattern);
            Model get( const std::vector< std::string>& patterns);

         } // configuration

      } // outbound

   } // http
} // casual




