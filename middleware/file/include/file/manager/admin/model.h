//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/transaction/id.h"
#include "common/serialize/macro.h"
#include "common/domain.h"

#include <vector>
#include <filesystem>

namespace casual
{
   namespace file::manager::admin::model
   {
      inline namespace v1 
      {
         enum class Stage : int
         {
            working,
            pending,
         };

         inline constexpr std::string_view description( const Stage value) noexcept
         {
            switch( value)
            {
            case admin::model::Stage::working:
               return "working";
            case admin::model::Stage::pending:
               return "pending";
            default:
               return "unknown";
            }
         };


         struct Request
         {
            common::strong::process::id pid;
            common::transaction::global::ID gtrid;
            Stage stage;
            std::filesystem::path path;
            std::chrono::system_clock::time_point time;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( pid);
               CASUAL_SERIALIZE( gtrid);
               CASUAL_SERIALIZE( stage);
               CASUAL_SERIALIZE( path);
               CASUAL_SERIALIZE( time);
            )
         };

         struct State
         {
            std::vector< Request> requests;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( requests);
            )
         };

         namespace recovery
         {
            enum class Directive : int
            {
               commit,
               rollback,
            };

            inline constexpr std::string_view description( Directive value) noexcept
            {
               switch( value)
               {
                  case Directive::commit: 
                     return "commit";
                  case Directive::rollback: 
                     return "rollback";
                  default: 
                     return "unknown";
               }
            }
         } // recovery

      } // v1

   } // file::manager::admin::model
} // casual


