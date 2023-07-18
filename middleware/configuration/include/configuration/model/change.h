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
      
      //! calculates the changes between `current` and `wanted`
      //! @returns `Result` with added, modified, removed.
      template< typename Range, typename Predicate>
      auto calculate( Range&& current, Range&& wanted, Predicate key)
      {
         Result< decltype( common::range::make( current))> result;

         auto differ = std::get< 1>( common::algorithm::intersection( wanted, current));

         std::tie( result.modified, result.added) = common::algorithm::intersection( differ, current, key);
         result.removed = std::get< 1>( common::algorithm::intersection( current, wanted, key));

         return result;
      }

      namespace concrete
      {
         //! calculates the changes between `current` and `wanted`
         //! @returns `Result` with added, modified, removed. Range type is a concrete vector with copied values
         template< typename Range, typename Predicate>
         auto calculate( Range&& current, Range&& wanted, Predicate key)
         {
            using range_type = std::vector< common::traits::iterable::value_t< Range>>;
            Result< range_type> result;

            auto calculated = change::calculate( std::forward< Range>( current), std::forward< Range>( wanted), std::move( key));

            return Result< range_type>{
               { std::begin( calculated.added), std::end( calculated.added)},
               { std::begin( calculated.modified), std::end( calculated.modified)},
               { std::begin( calculated.removed), std::end( calculated.removed)},
            };
         }
      } // concrete


   } // configuration::model::change
} // casual