//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/code/casual.h"
#include "common/code/log.h"
#include "common/code/category.h"
#include "common/code/serialize.h"

#include "common/log.h"
#include "common/string.h"
#include "common/communication/log.h"

#include <string>

namespace casual
{
   namespace common::code
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
                  return std::string{ code::description( static_cast< code::casual>( code))};
               }

               // defines the log condition equivalence, so we can compare for logging
               bool equivalent( int code, const std::error_condition& condition) const noexcept override
               {
                  if( ! is::category< code::log>( condition))
                     return false;

                  switch( static_cast< code::casual>( code))
                  {
                     case casual::shutdown: 
                        return condition == code::log::information;

                     case casual::interupted:
                     case casual::domain_instance_unavailable: 
                        return condition == code::log::user;

                     case casual::communication_unavailable:
                     case casual::communication_retry:
                     case casual::communication_refused:
                        return condition == code::log::internal;

                     // rest is error
                     default: 
                        return condition == code::log::error;
                  }
               }
            };

            const auto& category = code::serialize::registration< Category>( 0x416fa72d281c4459b18242b78a1b73b5_uuid);

         } // <unnamed>
      } // local

      std::string_view description( code::casual code) noexcept
      {
         switch( code)
         {
            case casual::shutdown: return "shutdown";
            case casual::interupted: return "interupted";
            case casual::invalid_configuration: return "invalid-configuration";
            case casual::invalid_document: return "invalid-document";
            case casual::invalid_node: return "invalid-node";
            case casual::invalid_version: return "invalid-version";
            case casual::invalid_path: return "invalid-path";
            case casual::invalid_argument: return "invalid_argument";
            case casual::invalid_semantics: return "invalid-semantics";
            case casual::failed_transcoding: return "failed-transcoding";
            case casual::communication_unavailable: return "communication-unavailable";
            case casual::communication_refused: return "communication-refused";
            case casual::communication_protocol: return "communication-protocol";
            case casual::communication_retry: return "communication-retry";
            case casual::communication_no_message: return "communication-no-message";
            case casual::communication_address_in_use: return "communication-address-in-use";
            case casual::communication_invalid_address: return "communication-invalid-address";

            case casual::domain_unavailable: return "domain-unavailable";
            case casual::domain_running: return "domain-running";
            case casual::domain_instance_unavailable: return "domain-instance-unavailable";
            case casual::domain_instance_assassinate : return "domain-instance-assassinate";
            
            case casual::buffer_type_duplicate: return "buffer-type-duplicate";
            case casual::internal_out_of_bounds: return "internal-out-of-bounds";
            case casual::internal_unexpected_value: return "internal-unexpected-value";
            case casual::internal_correlation: return "internal-correlation";

            case casual::fatal_terminate: return "fatal-terminate";
         }
         return "unknown";
      }

      std::error_code make_error_code( code::casual code)
      {
         return { static_cast< int>( code), local::category};
      }

   } // common::code
} // casual
