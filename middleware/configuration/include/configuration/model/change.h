//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/serialize/macro.h"
#include "common/algorithm.h"

#include <vector>

namespace casual
{
   namespace configuration::model::change
   {
      template< typename Range>
      struct Result
      {
         Range added;
         Range modified;
         Range removed;

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( added);
            CASUAL_SERIALIZE( modified);
            CASUAL_SERIALIZE( removed);
         )
      };

      namespace predicate
      {
         inline auto alias()
         {
            return []( auto& lhs, auto& rhs)
            { 
               return lhs.alias == rhs.alias;
            };
         }
         
      } // predicate
      
      //! calculates the changes between `current` and `wanted`
      //! @returns `Result` with added, modified, removed.
      auto calculate( auto&& current, auto&& wanted, auto predicate)
      {
         Result< decltype( common::range::make( current))> result;

         auto differ = std::get< 1>( common::algorithm::intersection( wanted, current));

         std::tie( result.modified, result.added) = common::algorithm::intersection( differ, current, predicate);
         result.removed = std::get< 1>( common::algorithm::intersection( current, wanted, predicate));

         return result;
      }

      //! calculates the changes between `current` and `wanted`, uses equal-alias-predicate
      //! @returns `Result` with added, modified, removed.
      auto calculate( auto&& current, auto&& wanted)
      {
         return calculate( current, wanted, change::predicate::alias());
      }

      namespace concrete
      {
         //! calculates the changes between `current` and `wanted`
         //! @returns `Result` with added, modified, removed. Range type is a concrete vector with copied values
         template< typename R1, typename R2, typename Predicate>
         requires std::same_as< std::ranges::range_value_t< R1>, std::ranges::range_value_t< R2>>
         auto calculate( R1&& current, R2&& wanted, Predicate key)
         {
            using range_type = std::vector< std::ranges::range_value_t< std::remove_cvref_t< R1>>>;

            auto calculated = change::calculate( std::forward< R1>( current), std::forward< R1>( wanted), std::move( key));

            return Result< range_type>{
               { std::begin( calculated.added), std::end( calculated.added)},
               { std::begin( calculated.modified), std::end( calculated.modified)},
               { std::begin( calculated.removed), std::end( calculated.removed)},
            };
         }

         //! calculates the changes between `current` and `wanted`, uses equal-alias-predicate 
         //! @returns `Result` with added, modified, removed. Range type is a concrete vector with copied values
         auto calculate( auto&& current, auto&& wanted)
         {
            return calculate( current, wanted, change::predicate::alias());
         }
      } // concrete


   } // configuration::model::change
} // casual