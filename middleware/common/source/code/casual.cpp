//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/code/casual.h"

#include "common/log.h"

#include <string>

namespace casual
{
   namespace common
   {
      namespace code
      {
         namespace local
         {
            namespace
            {
               struct Category : std::error_category
               {
                  const char* name() const noexcept override
                  {
                     return "casual";
                  }

                  std::string message( int code) const override
                  {
                     switch( static_cast< code::casual>( code))
                     {
                        case casual::shutdown: return "shutdown";
                        case casual::invalid_configuration: return "invalid configuration";
                        case casual::invalid_document: return "invalid document";
                        case casual::invalid_node: return "invalid node";
                        case casual::validation: return "validation";
                        case casual::invalid_version: return "invalid version";
                     }
                     return "unknown";
                  }
               };

               const Category category{};

            } // <unnamed>
         } // local

         std::error_code make_error_code( code::casual code)
         {
            return { static_cast< int>( code), local::category};
         }

         common::log::Stream& stream( code::casual code)
         {
            switch( code)
            {
               case casual::shutdown: return common::log::category::information;
               // debug
               //case casual::ok: return common::log::debug;

               // rest is errors
               default: return common::log::category::error;
            }
         }

      } // code
   } // common
} // casual
