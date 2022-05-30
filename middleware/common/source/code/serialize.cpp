//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/code/serialize.h"

#include "common/code/raise.h"
#include "common/code/casual.h"
#include "common/algorithm.h"
#include "common/uuid.h"
#include "common/log.h"

#include "casual/assert.h"

#include <vector>

namespace casual
{
   namespace common::code::serialize
   {
      namespace local
      {
         namespace
         {
            namespace lookup
            {
               using Entry = serialize::lookup::detail::Entry;

               struct State
               {
                  static State& instance() 
                  {
                     static State singleton;
                     return singleton;
                  }

                  // register std categories
                  std::vector< Entry> entries = {
                     { &std::system_category(), 0x4bf6a0b545bb4f25b89bf659569d9ad2_uuid},
                     { &std::generic_category(), 0x86d1981afcc1471e994edd1e13d1fc51_uuid}
                  };

               private:
                  State() = default;
               };

               State& state()
               {
                  return State::instance();
               }

            } // lookup
         } // <unnamed>
      } // local

      namespace lookup
      {
         namespace detail
         { 
            inline bool operator == ( const Entry& lhs, const Uuid& rhs) { return lhs.id == rhs;}
            inline bool operator == ( const Entry& lhs, const std::error_category& rhs) { return *lhs.category == rhs;}

            std::ostream& operator << ( std::ostream& out, const Entry& value)
            {
               return common::stream::write( out, "{ name: ", value.category->name(), ", id: ", value.id, '}');
            }

            void registration( Entry entry)
            {
               assertion( ! algorithm::find( local::lookup::state().entries, entry.id), "error code is already registered - entry: ", entry);
               local::lookup::state().entries.push_back( std::move( entry));
            }

            const std::vector< Entry>& state() noexcept
            {
               return local::lookup::state().entries;
            }
         } // detail

         const Uuid& id( const std::error_category& category)
         {
            if( auto found = algorithm::find( local::lookup::state().entries, category))
               return found->id;

            common::log::line( common::log::category::error, code::casual::internal_unexpected_value, 
               "failed to lookup id for error category: ", category.name(), " - action: use 'null uuid'");

            return uuid::empty();
         }

      } // lookup

      std::error_code create( const Uuid& id, int code)
      {
         if( auto found = algorithm::find( local::lookup::state().entries, id))
            return { code, *found->category};

         common::log::line( common::log::category::error, code::casual::internal_unexpected_value, 
            "failed to lookup error category for id: ", id, " - code: ", code, " - action: use: ", code::casual::internal_unexpected_value);

         return code::casual::internal_unexpected_value;
      }
      
   } // common::code::serialize
} // casual