//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/instance.h"

#include "common/environment.h"
#include "common/serialize/json.h"


namespace casual
{
   namespace common
   {
      namespace instance
      {
         namespace local
         {
            namespace
            {
               namespace variable
               {
                  constexpr auto name = "CASUAL_INSTANCE_INFORMATION";

                  // allways consume the information so it doesn't propabate to children
                  // (parent has to set this explicitly)
                  std::string value = common::environment::variable::consume( name, {});

               } // variable

               optional< Information> information()
               {
                  auto value = std::move( variable::value);

                  if( value.empty())
                     return {};

                  Information instance;
                  {
                     auto archive = serialize::json::relaxed::reader( value); 
                     archive >> CASUAL_NAMED_VALUE( instance);
                  }

                  return optional< Information>{ std::move( instance)};
               }
            } // <unnamed>
         } // local

         environment::Variable variable( const Information& instance)
         {
            std::ostringstream out;
            out << local::variable::name << '=';
            {
               auto archive = serialize::json::writer( out);
               archive << CASUAL_NAMED_VALUE( instance);
            }

            return { std::move( out).str()};
         };

         const optional< Information>& information()
         {
            static const auto singleton = local::information();
            return singleton;
         }

      } // instance
   } // common
} // casual