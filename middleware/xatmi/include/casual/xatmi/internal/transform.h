//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/xatmi/xa.h"
#include "common/server/start.h"

namespace casual
{
   namespace xatmi
   {
      namespace transform
      {
         inline auto resources( const casual_xa_switch_map* xa)
         {
            std::vector< common::server::argument::transaction::Resource> result;

            while( xa->xa_switch != nullptr)
            {
               if( xa->name)
                  result.emplace_back( xa->key, xa->name, xa->xa_switch);
               else
                  result.emplace_back( xa->key, xa->xa_switch);
               ++xa;
            }
            return result;
         }

         inline auto resources( const casual_xa_switch_mapping* xa)
         {
            std::vector< common::server::argument::transaction::Resource> result;

            while( xa->xa_switch != nullptr)
            {
               result.emplace_back( xa->key, xa->xa_switch);
               ++xa;
            }
            return result;
         }
      } // transform
   } // xatmi
} // casual